cmake_minimum_required(VERSION 3.5.2)
project(openvr_battery_monitor)
set(TARGET_NAME openvr_battery_monitor)

find_library(OPENVR_LIBRARIES
  NAMES
    openvr_api
)

set(Boost_USE_STATIC_LIBS ON) 
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME ON) 
find_package(Boost REQUIRED CONFIG COMPONENTS system filesystem beast asio)
link_directories( ${Boost_LIBRARIES})

include_directories(
  ${OPENVR_INCLUDE_DIR}
)

add_executable(${TARGET_NAME}
  openvr_battery_monitor.cpp
)
if(MINGW)
  target_link_libraries(${TARGET_NAME} PRIVATE ${OPENVR_LIBRARIES} Boost::beast Boost::asio ws2_32 )
else()
  target_link_libraries(${TARGET_NAME} PRIVATE ${OPENVR_LIBRARIES} Boost::beast Boost::asio )
endif()

target_compile_features(${TARGET_NAME} PUBLIC cxx_std_20)
