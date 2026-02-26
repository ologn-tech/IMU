#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <jni.h>
#include <android/sensor.h>
#include <android/looper.h>
#include <functional>
#include <vector>
#include <string>

// Sensor type constants matching Android ASENSOR_TYPE_*
#define SENSOR_TYPE_ACCELEROMETER  1
#define SENSOR_TYPE_MAGNETIC_FIELD 2
#define SENSOR_TYPE_GYROSCOPE      4

struct SensorEntry {
    const ASensor* sensor;
    ASensorEventQueue* queue;
    int type;
    std::string name;
    float x;
    float y;
    float z;
    class NativeSensorManager* manager;  // back-pointer for callback
};

// Callback: sensorName, type (SENSOR_TYPE_*), x, y, z
typedef std::function<void(const char* name, int type, float x, float y, float z)> SensorEventCallback;

class NativeSensorManager {
public:
    NativeSensorManager();
    ~NativeSensorManager();
    
    bool initialize();
    void startListening();
    void stopListening();
    void setSensorEventCallback(SensorEventCallback callback);
    
    int getSensorCount() const;
    void getSensorInfo(int index, std::string& outName, int& outType) const;
    void getSensorData(int index, float& x, float& y, float& z) const;
    
private:
    static int sensorEventCallback(int fd, int events, void* data);
    
    ASensorManager* sensorManager;
    ALooper* looper;
    std::vector<SensorEntry> sensors;
    SensorEventCallback eventCallback;
    
    bool isInitialized;
    bool isListening;
};

#endif // SENSOR_MANAGER_H
