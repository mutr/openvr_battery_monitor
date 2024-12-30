cmake_minimum_required(VERSION 3.5.2)
project(openvr_battery_monitor)
set(TARGET_NAME openvr_battery_monitor)

find_library(OPENVR_LIBRARIES
  NAMES
    openvr_api
)

find_package(CURL REQUIRED)

include_directories(
  ${OPENVR_INCLUDE_DIR}
)

add_executable(${TARGET_NAME}
  openvr_battery_monitor.cpp
)

target_link_libraries(${TARGET_NAME} PRIVATE -static ${OPENVR_LIBRARIES} ${CURL_LIBRARIES})
