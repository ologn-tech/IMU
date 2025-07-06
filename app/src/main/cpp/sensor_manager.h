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

class NativeSensorManager {
public:
    NativeSensorManager();
    ~NativeSensorManager();
    
    bool initialize();
    void startListening();
    void stopListening();
    void setDataCallback(std::function<void(const AccelerometerData&)> callback);
    AccelerometerData getCurrentData() const;
    
private:
    static int sensorEventCallback(int fd, int events, void* data);
    void processSensorEvent(const ASensorEvent* event);
    
    ASensorManager* sensorManager;
    const ASensor* accelerometer;
    ASensorEventQueue* sensorEventQueue;
    ALooper* looper;
    
    std::function<void(const AccelerometerData&)> dataCallback;
    AccelerometerData currentData;
    
    bool isInitialized;
    bool isListening;
};

#endif // SENSOR_MANAGER_H 
