cmake_minimum_required(VERSION 3.22.1)

project("imu")

# Find required packages
find_library(log-lib log)
find_library(android-lib android)

# Create the shared library
add_library(
    imu
    SHARED
    sensor_manager.cpp
    sensor_jni.cpp
)

# Include directories
target_include_directories(
    imu
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link libraries
target_link_libraries(
    imu
    ${log-lib}
    ${android-lib}
) 
