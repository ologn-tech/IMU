#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <jni.h>
#include <android/sensor.h>
#include <android/looper.h>
#include <functional>

struct AccelerometerData {
    float x;
    float y;
    float z;
};

struct GyroscopeData {
    float x;
    float y;
    float z;
};

struct MagnetometerData {
    float x;
    float y;
    float z;
};

class NativeSensorManager {
public:
    NativeSensorManager();
    ~NativeSensorManager();
    
    bool initialize();
    void startListening();
    void stopListening();
    void setDataCallback(std::function<void(const AccelerometerData&)> callback);
    void setGyroscopeCallback(std::function<void(const GyroscopeData&)> callback);
    void setMagnetometerCallback(std::function<void(const MagnetometerData&)> callback);
    AccelerometerData getCurrentData() const;
    GyroscopeData getCurrentGyroscopeData() const;
    MagnetometerData getCurrentMagnetometerData() const;
    
private:
    static int sensorEventCallback(int fd, int events, void* data);
    void processSensorEvent(const ASensorEvent* event);
    
    ASensorManager* sensorManager;
    const ASensor* accelerometer;
    const ASensor* gyroscope;
    const ASensor* magnetometer;
    ASensorEventQueue* sensorEventQueue;
    ALooper* looper;
    
    std::function<void(const AccelerometerData&)> dataCallback;
    std::function<void(const GyroscopeData&)> gyroscopeCallback;
    std::function<void(const MagnetometerData&)> magnetometerCallback;
    AccelerometerData currentData;
    GyroscopeData currentGyroscopeData;
    MagnetometerData currentMagnetometerData;
    
    bool isInitialized;
    bool isListening;
};

#endif // SENSOR_MANAGER_H 
