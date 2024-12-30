#include <openvr.h>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>

const char* OPENVR_APPLICATION_KEY = "mutr.openvr_battery_monitor";
std::ofstream logFile;

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

void log(const std::string& message) {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::string timestamp = std::ctime(&time);
    timestamp = timestamp.substr(0, timestamp.length()-1); // Remove newline
    
    logFile << "[" << timestamp << "] " << message << std::endl;
    logFile.flush();
    std::cout << message << std::endl;
}

struct Config {
    std::string measurement;
    std::string influx_host;
    int influx_port;
    std::string influx_org;
    std::string influx_bucket;
    std::string influx_token;
    int interval_seconds;

    static Config load(const std::string& filename) {
        Config cfg;
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open config file: " + filename);
        }

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;
            
            auto pos = line.find('=');
            if (pos == std::string::npos) continue;
            
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            
            if (value.size() >= 2 && value[0] == '"' && value[value.size()-1] == '"') {
                value = value.substr(1, value.size() - 2);
            }

            if (key == "measurement") cfg.measurement = value;
            else if (key == "influx_host") cfg.influx_host = value;
            else if (key == "influx_port") cfg.influx_port = std::stoi(value);
            else if (key == "influx_org") cfg.influx_org = value;
            else if (key == "influx_bucket") cfg.influx_bucket = value;
            else if (key == "influx_token") cfg.influx_token = value;
            else if (key == "interval_seconds") cfg.interval_seconds = std::stoi(value);
        }
        return cfg;
    }
};

class InfluxDBWriter {
private:
    net::io_context ioc;
    tcp::resolver resolver;
    beast::tcp_stream stream;
    std::string host;
    std::string port;
    std::string target;
    std::string auth_token;

public:
    InfluxDBWriter(const Config& cfg) 
        : resolver(ioc)
        , stream(ioc)
        , host(cfg.influx_host)
        , port(std::to_string(cfg.influx_port))
    {
        target = "/api/v2/write?org=" + cfg.influx_org + 
                "&bucket=" + cfg.influx_bucket + 
                "&precision=ns";
        auth_token = cfg.influx_token;
        
        log("InfluxDB writer initialized: " + host + ":" + port);
    }

    ~InfluxDBWriter() {
        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);
    }

    bool write(const std::string& line_protocol) {
        try {
            // Look up the domain name
            auto const results = resolver.resolve(host, port);

            // Make the connection on the IP address we get from a lookup
            stream.connect(results);

            // Set up an HTTP POST request message
            http::request<http::string_body> req{http::verb::post, target, 11};
            req.set(http::field::host, host);
            req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
            req.set(http::field::content_type, "text/plain; charset=utf-8");
            req.set("Authorization", "Token " + auth_token);
            req.body() = line_protocol;
            req.prepare_payload();

            // Send the HTTP request to the remote host
            http::write(stream, req);

            // This buffer is used for reading and must be persisted
            beast::flat_buffer buffer;

            // Declare a container to hold the response
            http::response<http::dynamic_body> res;

            // Receive the HTTP response
            http::read(stream, buffer, res);

            // Gracefully close the socket
            beast::error_code ec;
            stream.socket().shutdown(tcp::socket::shutdown_both, ec);

            if (res.result() < http::status::ok || res.result() >= http::status::multiple_choices) {
                log("HTTP error: " + std::to_string(static_cast<int>(res.result())));
                return false;
            }

            return true;
        }
        catch (const std::exception& e) {
            log("Failed to send data: " + std::string(e.what()));
            return false;
        }
    }
};

class VRSystem {
private:
    vr::IVRSystem* m_pSystem;

public:
    VRSystem() : m_pSystem(nullptr) {
        vr::EVRInitError error = vr::VRInitError_None;
        m_pSystem = vr::VR_Init(&error, vr::VRApplication_Background);
        
        if (error != vr::VRInitError_None) {
            throw std::runtime_error("Failed to initialize OpenVR: " + 
                std::string(vr::VR_GetVRInitErrorAsEnglishDescription(error)));
        }
        log("VR system initialized");
    }

    ~VRSystem() {
        if (m_pSystem) {
            vr::VR_Shutdown();
            m_pSystem = nullptr;
            log("VR system shut down");
        }
    }

    std::string GetDeviceSerial(vr::TrackedDeviceIndex_t deviceIndex) {
        char buffer[1024];
        vr::ETrackedPropertyError propError;
        m_pSystem->GetStringTrackedDeviceProperty(deviceIndex, 
            vr::Prop_SerialNumber_String,
            buffer, sizeof(buffer), &propError);
        
        if (propError != vr::TrackedProp_Success) {
            return "unknown";
        }
        return std::string(buffer);
    }

    float GetDeviceBattery(vr::TrackedDeviceIndex_t deviceIndex) {
        vr::ETrackedPropertyError propError;
        float battery = m_pSystem->GetFloatTrackedDeviceProperty(deviceIndex,
            vr::Prop_DeviceBatteryPercentage_Float,
            &propError);
        
        if (propError != vr::TrackedProp_Success) {
            return -1.0f;
        }
        return battery * 100.0f;
    }

    std::string GetDeviceClass(vr::TrackedDeviceIndex_t deviceIndex) {
        vr::ETrackedDeviceClass deviceClass = m_pSystem->GetTrackedDeviceClass(deviceIndex);
        switch (deviceClass) {
            case vr::TrackedDeviceClass_Controller:
                return "controller";
            case vr::TrackedDeviceClass_HMD:
                return "hmd";
            case vr::TrackedDeviceClass_GenericTracker:
                return "tracker";
            case vr::TrackedDeviceClass_TrackingReference:
                return "base_station";
            default:
                return "unknown";
        }
    }

    std::string CollectMetrics(const std::string& measurement) {
        std::stringstream metrics;
        auto now = std::chrono::system_clock::now();
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            now.time_since_epoch()).count();

        log("=== " + std::format("{:%Y-%m-%d %T}",now));
        for (vr::TrackedDeviceIndex_t device = 0; 
             device < vr::k_unMaxTrackedDeviceCount; device++) {
            
            if (!m_pSystem->IsTrackedDeviceConnected(device)) {
                continue;
            }

            float battery = GetDeviceBattery(device);
            if (battery >= 0.0f) {
                std::string serial = GetDeviceSerial(device);
                std::string deviceClass = GetDeviceClass(device);

                metrics << measurement
                       << ",serial=" << serial
                       << ",type=" << deviceClass
                       << " battery=" << std::fixed << std::setprecision(2) << battery
                       << " " << ns << "\n";
                
                log("Device " + serial + " (" + deviceClass + ") battery: " + 
                    std::to_string(battery) + "%");
            }
        }
        return metrics.str();
    }
};

bool InstallManifest(bool install, std::filesystem::path exePathWithoutExt) {
    vr::EVRInitError error = vr::VRInitError_None;
    vr::VR_Init(&error, vr::VRApplication_Utility);
    if (error != vr::VRInitError_None) {
        log("Failed to initialize OpenVR: " + 
            std::string(vr::VR_GetVRInitErrorAsEnglishDescription(error)));
        return false;
    }

    vr::IVRApplications* applications = vr::VRApplications();
    if (!applications) {
        log("Failed to get IVRApplications interface");
        vr::VR_Shutdown();
        return false;
    }

    std::filesystem::path manifestPath = exePathWithoutExt.string() + ".vrmanifest";

    bool isInstalled = applications->IsApplicationInstalled(OPENVR_APPLICATION_KEY);
    if (isInstalled) {
        log("Removing existing installation");
        char oldWd[MAX_PATH] = { 0 };
		auto vrAppErr = vr::VRApplicationError_None;
		vr::VRApplications()->GetApplicationPropertyString(OPENVR_APPLICATION_KEY, vr::VRApplicationProperty_WorkingDirectory_String, oldWd, MAX_PATH, &vrAppErr);
		if (vrAppErr != vr::VRApplicationError_None)
		{
			log("Failed to get old working dir, skipping removal: " + std::string(vr::VRApplications()->GetApplicationsErrorNameFromEnum(vrAppErr)));
		}
		else
		{
			std::string manifestPath = oldWd;
			manifestPath += "\\openvr_battery_monitor.vrmanifest";
			std::cout << "Removing old manifest path: " << manifestPath << std::endl;
			vrAppErr = vr::VRApplications()->RemoveApplicationManifest(manifestPath.c_str());
            if (vrAppErr != vr::VRApplicationError_None)
		    {
			    log("Failed to remove old manifest: " + std::string(vr::VRApplications()->GetApplicationsErrorNameFromEnum(vrAppErr)));
		    }
		}
    }
    
    if (install) {
        log("Installing manifest: " + manifestPath.string());
        std::ofstream manifest(manifestPath);
        manifest << R"({
    "source": "builtin",
    "applications": [
        {
            "app_key": ")" << OPENVR_APPLICATION_KEY << R"(",
            "launch_type": "binary",
            "binary_path_windows": ")" << "openvr_battery_monitor.exe" << R"(",
            "is_dashboard_overlay": true,
            "strings": {
                "en_us": {
                    "name": "OpenVR Battery Monitor",
                    "description": "Battery monitoring utility"
                }
            }
        }
    ]
})";
        manifest.close();

        vr::EVRApplicationError appError = applications->AddApplicationManifest(
            manifestPath.string().c_str(), false);
        if (appError != vr::VRApplicationError_None) {
            log("Failed to add application manifest");
            vr::VR_Shutdown();
            return false;
        }

        appError = applications->SetApplicationAutoLaunch(OPENVR_APPLICATION_KEY, true);
        if (appError != vr::VRApplicationError_None) {
            log("Failed to set auto launch");
            vr::VR_Shutdown();
            return false;
        }
        log("Installation successful");
    } else {
        log("Uninstallation successful");
    }

    vr::VR_Shutdown();
    return true;
}

int main(int argc, char* argv[]) {
    std::filesystem::path exePathWithoutExt = std::filesystem::canonical(argv[0]).replace_extension();
    std::filesystem::path logPath = exePathWithoutExt.string() + ".log";
    logFile.open(logPath, std::ios::app);

    if (!logFile.is_open()) {
        std::cerr << "Failed to open log file" << std::endl;
        return 1;
    }

    log("OpenVR Battery Monitor starting");

    if (argc > 1) {
        std::string arg = argv[1];
        if (arg == "--install" || arg == "--uninstall") {
            bool success = InstallManifest(arg == "--install", exePathWithoutExt);
            logFile.close();
            return success ? 0 : 1;
        }
    }

    try {
        std::filesystem::path confPath = exePathWithoutExt.string() + ".conf";
        log("Loading config from: " + confPath.string());
        Config config = Config::load(confPath.string());
        
        VRSystem vr;
        InfluxDBWriter influx(config);

        log("Starting monitoring loop with " + 
            std::to_string(config.interval_seconds) + "s interval");

        while (true) {
            try {
                std::string metrics = vr.CollectMetrics(config.measurement);
                if (!metrics.empty()) {
                    influx.write(metrics);
                }
            } catch (const std::exception& e) {
                log("Error collecting metrics: " + std::string(e.what()));
            }

            std::this_thread::sleep_for(
                std::chrono::seconds(config.interval_seconds));
        }

        return 0;
    } catch (const std::exception& e) {
        log("Fatal error: " + std::string(e.what()));
        return 1;
    }
}