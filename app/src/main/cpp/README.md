# Native Sensor Implementation

This directory contains the native C++ implementation of the sensor functionality for the IMU app.

## Files

- `CMakeLists.txt` - CMake build configuration
- `sensor_manager.h` - Header file for the native sensor manager class
- `sensor_manager.cpp` - Implementation of the native sensor manager
- `sensor_jni.cpp` - JNI bindings to connect native code with Kotlin

## Architecture

### NativeSensorManager (C++)
- Uses Android NDK sensor APIs (`android/sensor.h`)
- Manages accelerometer sensor events through `ASensorManager`
- Provides callback mechanism for sensor data updates
- Handles sensor lifecycle (initialize, start, stop, cleanup)

### JNI Bindings
- `sensor_jni.cpp` provides the bridge between C++ and Kotlin
- Implements JNI functions that can be called from Kotlin
- Handles callback mechanism to notify Kotlin code of sensor updates
- Manages memory and thread safety

### Kotlin Wrapper
- `NativeSensorManager.kt` provides a clean Kotlin interface
- Loads the native library and exposes native methods
- Implements `SensorDataCallback` to receive sensor updates
- Provides `StateFlow` for reactive sensor data

## Usage

The native implementation is used transparently through the existing `SensorManager` class:

```kotlin
val sensorManager = SensorManager(context)
val accelerometerData by sensorManager.accelerometerData.collectAsState()

// Lifecycle management is handled automatically
lifecycleOwner.lifecycle.addObserver(sensorManager)
```

## Benefits of Native Implementation

1. **Performance**: Direct access to sensor hardware through NDK
2. **Lower Latency**: Reduced overhead compared to Java/Kotlin sensor APIs
3. **Real-time Processing**: Better suited for high-frequency sensor data
4. **Memory Efficiency**: Reduced garbage collection pressure
5. **Cross-platform**: C++ code can be shared with other platforms

## Building

The native code is built automatically by Android Studio using CMake. The build configuration is defined in `app/build.gradle.kts`:

```kotlin
externalNativeBuild {
    cmake {
        path = file("src/main/cpp/CMakeLists.txt")
        version = "3.22.1"
    }
}
```

## Debugging

Native code logs can be viewed using:
```bash
adb logcat | grep -E "(NativeSensorManager|SensorJNI)"
```

## Thread Safety

The implementation handles thread safety by:
- Using global references for JNI callbacks
- Properly attaching/detaching threads when calling Java from native code
- Using Android Looper for sensor event processing 
