cmake_minimum_required(VERSION 3.10)
project(commonapi_hello)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages
find_package(CommonAPI REQUIRED)
find_package(CommonAPI-SomeIP REQUIRED)
find_package(vsomeip3 REQUIRED)

# Include directories
include_directories(
    src
    src-gen
    ${COMMONAPI_INCLUDE_DIRS}
    ${COMMONAPI_SOMEIP_INCLUDE_DIRS}
    ${VSOMEIP3_INCLUDE_DIRS}
)

# Generated source files
file(GLOB GENERATED_SOURCES "src-gen/v1/firmware/*.cpp")

# Server executable
add_executable(hello_server
    src/server.cpp
    ${GENERATED_SOURCES}
)

target_link_libraries(hello_server
    CommonAPI
    CommonAPI-SomeIP
    vsomeip3
)

# Client executable
add_executable(hello_client
    src/client.cpp
    ${GENERATED_SOURCES}
)

target_link_libraries(hello_client
    CommonAPI
    CommonAPI-SomeIP
    vsomeip3
)