# Building

1. Install MSYS2
2. Start UCRT64
3. `pacman -S --needed base-devel mingw-w64-ucrt-x86_64-toolchain mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-curl zip`
4. `mkdir build && cd build`
5. `cmake .. -DCMAKE_BUILD_TYPE=Release`
6. `cmake --build .`
7. `cp ../openvr_battery_monitor.conf .`
8. `cp /ucrt64/bin/libcurl-4.dll /ucrt64/bin/libopenvr_api.dll .`
9. `zip openvr_battery_monitor libcurl-4.dll libopenvr_api.dll openvr_battery_monitor.exe openvr_battery_monitor.conf`