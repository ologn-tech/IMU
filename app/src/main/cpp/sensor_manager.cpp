#include "sensor_manager.h"
#include <android/log.h>

#define LOG_TAG "NativeSensorManager"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

NativeSensorManager::NativeSensorManager()
    : sensorManager(nullptr)
    , accelerometer(nullptr)
    , gyroscope(nullptr)
    , magnetometer(nullptr)
    , sensorEventQueue(nullptr)
    , looper(nullptr)
    , isInitialized(false)
    , isListening(false) {
    currentData = {0.0f, 0.0f, 0.0f};
    currentGyroscopeData = {0.0f, 0.0f, 0.0f};
    currentMagnetometerData = {0.0f, 0.0f, 0.0f};
}

NativeSensorManager::~NativeSensorManager() {
    stopListening();
    if (sensorEventQueue) {
        if (accelerometer) ASensorEventQueue_disableSensor(sensorEventQueue, accelerometer);
        if (gyroscope) ASensorEventQueue_disableSensor(sensorEventQueue, gyroscope);
        if (magnetometer) ASensorEventQueue_disableSensor(sensorEventQueue, magnetometer);
        ASensorManager_destroyEventQueue(sensorManager, sensorEventQueue);
    }
}

bool NativeSensorManager::initialize() {
    if (isInitialized) {
        return true;
    }
    
    // Get the sensor manager
    sensorManager = ASensorManager_getInstance();
    if (!sensorManager) {
        LOGE("Failed to get sensor manager");
        return false;
    }
    
    // Get the accelerometer sensor
    accelerometer = ASensorManager_getDefaultSensor(sensorManager, ASENSOR_TYPE_ACCELEROMETER);
    if (!accelerometer) {
        LOGE("Failed to get accelerometer sensor");
        return false;
    }
    
    // Get the gyroscope sensor
    gyroscope = ASensorManager_getDefaultSensor(sensorManager, ASENSOR_TYPE_GYROSCOPE);
    if (!gyroscope) {
        LOGE("Failed to get gyroscope sensor");
    }
    
    // Get the magnetometer sensor
    magnetometer = ASensorManager_getDefaultSensor(sensorManager, ASENSOR_TYPE_MAGNETIC_FIELD);
    if (!magnetometer) {
        LOGE("Failed to get magnetometer sensor");
    }
    
    // Create looper
    looper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
    if (!looper) {
        LOGE("Failed to create looper");
        return false;
    }
    
    // Create sensor event queue
    sensorEventQueue = ASensorManager_createEventQueue(sensorManager, looper, 3, sensorEventCallback, this);
    if (!sensorEventQueue) {
        LOGE("Failed to create sensor event queue");
        return false;
    }
    
    isInitialized = true;
    LOGI("Sensor manager initialized successfully");
    return true;
}

void NativeSensorManager::startListening() {
    if (!isInitialized || isListening) {
        return;
    }
    
    int result = ASensorEventQueue_enableSensor(sensorEventQueue, accelerometer);
    if (result < 0) {
        LOGE("Failed to enable accelerometer sensor");
        return;
    }
    ASensorEventQueue_setEventRate(sensorEventQueue, accelerometer, 200000);
    
    if (gyroscope) {
        result = ASensorEventQueue_enableSensor(sensorEventQueue, gyroscope);
        if (result >= 0) {
            ASensorEventQueue_setEventRate(sensorEventQueue, gyroscope, 200000);
        }
    }
    if (magnetometer) {
        result = ASensorEventQueue_enableSensor(sensorEventQueue, magnetometer);
        if (result >= 0) {
            ASensorEventQueue_setEventRate(sensorEventQueue, magnetometer, 200000);
        }
    }
    
    isListening = true;
    LOGI("Started listening to accelerometer, gyroscope, magnetometer");
}

void NativeSensorManager::stopListening() {
    if (!isInitialized || !isListening) {
        return;
    }
    
    ASensorEventQueue_disableSensor(sensorEventQueue, accelerometer);
    if (gyroscope) ASensorEventQueue_disableSensor(sensorEventQueue, gyroscope);
    if (magnetometer) ASensorEventQueue_disableSensor(sensorEventQueue, magnetometer);
    isListening = false;
    LOGI("Stopped listening to sensors");
}

void NativeSensorManager::setDataCallback(std::function<void(const AccelerometerData&)> callback) {
    dataCallback = callback;
}

void NativeSensorManager::setGyroscopeCallback(std::function<void(const GyroscopeData&)> callback) {
    gyroscopeCallback = callback;
}

void NativeSensorManager::setMagnetometerCallback(std::function<void(const MagnetometerData&)> callback) {
    magnetometerCallback = callback;
}

AccelerometerData NativeSensorManager::getCurrentData() const {
    return currentData;
}

GyroscopeData NativeSensorManager::getCurrentGyroscopeData() const {
    return currentGyroscopeData;
}

MagnetometerData NativeSensorManager::getCurrentMagnetometerData() const {
    return currentMagnetometerData;
}

int NativeSensorManager::sensorEventCallback(int fd, int events, void* data) {
    NativeSensorManager* manager = static_cast<NativeSensorManager*>(data);
    if (!manager) {
        return 0;
    }
    
    ASensorEvent event;
    while (ASensorEventQueue_getEvents(manager->sensorEventQueue, &event, 1) > 0) {
        manager->processSensorEvent(&event);
    }
    
    return 1;
}

void NativeSensorManager::processSensorEvent(const ASensorEvent* event) {
    switch (event->type) {
        case ASENSOR_TYPE_ACCELEROMETER:
            currentData.x = event->acceleration.x;
            currentData.y = event->acceleration.y;
            currentData.z = event->acceleration.z;
            if (dataCallback) {
                dataCallback(currentData);
            }
            break;
        case ASENSOR_TYPE_GYROSCOPE:
            currentGyroscopeData.x = event->vector.x;
            currentGyroscopeData.y = event->vector.y;
            currentGyroscopeData.z = event->vector.z;
            if (gyroscopeCallback) {
                gyroscopeCallback(currentGyroscopeData);
            }
            break;
        case ASENSOR_TYPE_MAGNETIC_FIELD:
            currentMagnetometerData.x = event->magnetic.x;
            currentMagnetometerData.y = event->magnetic.y;
            currentMagnetometerData.z = event->magnetic.z;
            if (magnetometerCallback) {
                magnetometerCallback(currentMagnetometerData);
            }
            break;
        default:
            break;
    }
} 
