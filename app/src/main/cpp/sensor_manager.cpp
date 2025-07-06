#include "sensor_manager.h"
#include <android/log.h>

#define LOG_TAG "NativeSensorManager"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

NativeSensorManager::NativeSensorManager()
    : sensorManager(nullptr)
    , accelerometer(nullptr)
    , sensorEventQueue(nullptr)
    , looper(nullptr)
    , isInitialized(false)
    , isListening(false) {
    currentData = {0.0f, 0.0f, 0.0f};
}

NativeSensorManager::~NativeSensorManager() {
    stopListening();
    if (sensorEventQueue) {
        ASensorEventQueue_disableSensor(sensorEventQueue, accelerometer);
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
    
    // Set sensor delay to normal (200ms)
    result = ASensorEventQueue_setEventRate(sensorEventQueue, accelerometer, 200000);
    if (result < 0) {
        LOGE("Failed to set sensor event rate");
    }
    
    isListening = true;
    LOGI("Started listening to accelerometer");
}

void NativeSensorManager::stopListening() {
    if (!isInitialized || !isListening) {
        return;
    }
    
    ASensorEventQueue_disableSensor(sensorEventQueue, accelerometer);
    isListening = false;
    LOGI("Stopped listening to accelerometer");
}

void NativeSensorManager::setDataCallback(std::function<void(const AccelerometerData&)> callback) {
    dataCallback = callback;
}

AccelerometerData NativeSensorManager::getCurrentData() const {
    return currentData;
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
    if (event->type == ASENSOR_TYPE_ACCELEROMETER) {
        currentData.x = event->acceleration.x;
        currentData.y = event->acceleration.y;
        currentData.z = event->acceleration.z;
        
        if (dataCallback) {
            dataCallback(currentData);
        }
    }
} 
