Here is OpenVR api headers https://github.com/ValveSoftware/openvr/blob/master/headers/openvr.h I need a long running c++ program named openvr_battery_monitor that gets all tracked device with battery and send its serial, battery percentage to InfluxDB 2.x HTTP API server. Get the measurement name (openvr), server ip (localhost), port (8086) and other necessary parameter from the openvr_battery_monitor.conf located in the same directory.

```
measurement="openvr"
influx_host="localhost"
influx_port=8086
influx_org="your_org"
influx_bucket="your_bucket"
influx_token="your_influxdb_token"
interval_seconds=5
```

Example line protocol: `openvr,serial=LHR-ABCDEF01,type=controller battery=75.00  1735473371000000000`

 If `--install` or `--uninstall` arguments are passed the program will (un)register itself as auto launch OpenVR overlay and exits.
https://github.com/pushrax/OpenVR-SpaceCalibrator/blob/1cc0583a5ec5f18dc56c95716884529c05526d25/OpenVR-SpaceCalibrator/OpenVR-SpaceCalibrator.cpp#L413
Example manifest is here https://github.com/pushrax/OpenVR-SpaceCalibrator/blob/1cc0583a5ec5f18dc56c95716884529c05526d25/OpenVR-SpaceCalibrator/manifest.vrmanifest
Set OPENVR_APPLICATION_KEY to "mutr.openvr_battery_monitor"`

Check `bool isInstalled = applications->IsApplicationInstalled(OPENVR_APPLICATION_KEY);` and unregister old manifest before install new one.

Dont forgent to do vr::VR_Init and check result before start vr::VRApplications.
Write all messages to openvr_battery_monitor.log in the same directory.
Do not explain, just code.

I have install instruction in README.md I need github actions file to build it and make release automatically.