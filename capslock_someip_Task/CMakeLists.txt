cmake_minimum_required(VERSION 3.10)
project(capslock_someip)

set(CMAKE_CXX_STANDARD 14)

find_package(vsomeip3 REQUIRED)
find_package(Boost REQUIRED COMPONENTS system thread)

include_directories(common ${VSOMEIP3_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})

# Example 01: Control
add_executable(control_server example_01_control/server.cpp)
target_link_libraries(control_server vsomeip3 ${Boost_LIBRARIES} pthread)

add_executable(control_client example_01_control/client.cpp)
target_link_libraries(control_client vsomeip3 ${Boost_LIBRARIES} pthread)

# Example 02: Monitor
add_executable(monitor_server example_02_monitor/server.cpp)
target_link_libraries(monitor_server vsomeip3 ${Boost_LIBRARIES} pthread)

add_executable(monitor_client example_02_monitor/client.cpp)
target_link_libraries(monitor_client vsomeip3 ${Boost_LIBRARIES} pthread)