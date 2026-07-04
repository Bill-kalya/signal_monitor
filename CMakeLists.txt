cmake_minimum_required(VERSION 3.10)
project(SuperSignalMonitor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Create build directory
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/build)

# Find SQLite3
find_package(sqlite3 QUIET)
if(NOT sqlite3_FOUND)
    find_library(SQLITE_LIBRARY sqlite3)
endif()

if(WIN32)
    add_executable(signal_monitor src/main.cpp)
    target_link_libraries(signal_monitor wlanapi ole32)
    if(SQLITE_LIBRARY)
        target_link_libraries(signal_monitor ${SQLITE_LIBRARY})
    endif()
else()
    add_executable(signal_monitor src/main.cpp)
    if(sqlite3_FOUND)
        target_link_libraries(signal_monitor sqlite3)
    elseif(SQLITE_LIBRARY)
        target_link_libraries(signal_monitor ${SQLITE_LIBRARY})
    endif()
endif()

# Include directories
target_include_directories(signal_monitor PRIVATE src)

# Set output directory
set_target_properties(signal_monitor PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build
)