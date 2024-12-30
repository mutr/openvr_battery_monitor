# Openvr Battery Monitor

Program to collect battery discharge statistics of VIVE trackers, controllers and HMD (and possible other OpenVR API devices) to InfluxDB v2 server.

# Installing

1. Unpack openvr_battery_monitor.zip
2. Edit `openvr_battery_monitor.conf` set your InfluxDB host, org, port, bucket, token and interval.
3. Start cmd.exe and cd to dir.
4. Execute the command: `openvr_battery_monitor.exe --install`

The program will now run every time you launch SteamVR. You can disable autostart in the SteamVR settings or uninstall as follow  `openvr_battery_monitor.exe --uninstall`

# Building

1. Install MSYS2
2. Start UCRT64
3. `pacman -S --needed base-devel mingw-w64-ucrt-x86_64-toolchain mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-boost mingw-w64-ucrt-x86_64-openvr zip`
4. `mkdir build && cd build`
5. `cmake .. -DCMAKE_BUILD_TYPE=Release`
6. `cmake --build .`
7. `cp ../openvr_battery_monitor.conf .`
8. `for i in libgcc_s_seh-1.dll libwinpthread-1.dll libopenvr_api.dll libstdc++-6.dll; do cp /ucrt64/bin/$i .; done`
9. `zip openvr_battery_monitor *.dll openvr_battery_monitor.exe openvr_battery_monitor.conf`
