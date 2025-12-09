cmake_minimum_required(VERSION 3.10)
project(SuperSignalMonitor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Create build directory
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/build)

if(WIN32)
    add_executable(signal_monitor src/main.cpp)
    target_link_libraries(signal_monitor wlanapi ole32)
else()
    add_executable(signal_monitor src/main.cpp)
endif()

# Set output directory
set_target_properties(signal_monitor PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build
)