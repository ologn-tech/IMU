#include "sensor_manager.h"
#include <android/log.h>
#include <cstring>
#include <algorithm>
#include <map>

#define LOG_TAG "NativeSensorManager"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static const char* SUFFIX_NON_WAKEUP = " Non-wakeup";
static const char* SUFFIX_WAKEUP = " Wakeup";

static bool isSensorTypeWanted(int type) {
    return type == ASENSOR_TYPE_ACCELEROMETER ||
           type == ASENSOR_TYPE_GYROSCOPE ||
           type == ASENSOR_TYPE_MAGNETIC_FIELD;
}

// For same type + same base name (e.g. "lsm6dst Accelerometer"), prefer Non-wakeup over Wakeup.
// Sets baseName (name without " Non-wakeup" / " Wakeup") and isNonWakeup.
static void getBaseNameAndVariant(const std::string& name, std::string& baseName, bool& isNonWakeup) {
    baseName = name;
    isNonWakeup = false;
    size_t len = name.length();
    size_t nwLen = strlen(SUFFIX_NON_WAKEUP);
    size_t wLen = strlen(SUFFIX_WAKEUP);
    if (len >= nwLen && name.compare(len - nwLen, nwLen, SUFFIX_NON_WAKEUP) == 0) {
        baseName = name.substr(0, len - nwLen);
        isNonWakeup = true;
    } else if (len >= wLen && name.compare(len - wLen, wLen, SUFFIX_WAKEUP) == 0) {
        baseName = name.substr(0, len - wLen);
        isNonWakeup = false;
    }
}

NativeSensorManager::NativeSensorManager()
    : sensorManager(nullptr)
    , looper(nullptr)
    , isInitialized(false)
    , isListening(false) {
}

NativeSensorManager::~NativeSensorManager() {
    stopListening();
    for (auto& entry : sensors) {
        if (entry.queue) {
            ASensorEventQueue_disableSensor(entry.queue, entry.sensor);
            ASensorManager_destroyEventQueue(sensorManager, entry.queue);
            entry.queue = nullptr;
        }
    }
    sensors.clear();
}

bool NativeSensorManager::initialize() {
    if (isInitialized) {
        return true;
    }
    
    sensorManager = ASensorManager_getInstance();
    if (!sensorManager) {
        LOGE("Failed to get sensor manager");
        return false;
    }
    
    ASensorList sensorList = nullptr;
    int count = ASensorManager_getSensorList(sensorManager, &sensorList);
    if (count <= 0 || !sensorList) {
        LOGE("Failed to get sensor list or empty");
        return false;
    }
    
    looper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
    if (!looper) {
        LOGE("Failed to create looper");
        return false;
    }
    
    // Candidate: same type + same base name -> prefer Non-wakeup over Wakeup
    struct Candidate {
        const ASensor* sensor;
        std::string name;
        bool isNonWakeup;
    };
    std::map<std::pair<int, std::string>, std::vector<Candidate>> byTypeAndBase;
    
    for (int i = 0; i < count; i++) {
        const ASensor* sensor = sensorList[i];
        int type = ASensor_getType(sensor);
        if (!isSensorTypeWanted(type)) continue;
        
        const char* namePtr = ASensor_getName(sensor);
        std::string name = namePtr ? namePtr : "Unknown";
        std::string baseName;
        bool isNonWakeup = false;
        getBaseNameAndVariant(name, baseName, isNonWakeup);
        
        byTypeAndBase[{type, baseName}].push_back({sensor, name, isNonWakeup});
    }
    
    // For each (type, baseName) pick one: prefer Non-wakeup if present
    for (auto& kv : byTypeAndBase) {
        std::vector<Candidate>& candidates = kv.second;
        const Candidate* chosen = nullptr;
        for (const auto& c : candidates) {
            if (c.isNonWakeup) {
                chosen = &c;
                break;
            }
        }
        if (!chosen) chosen = &candidates[0];
        
        SensorEntry entry;
        entry.sensor = chosen->sensor;
        entry.queue = nullptr;  // set below
        entry.type = kv.first.first;
        entry.name = chosen->name;
        entry.x = 0.0f;
        entry.y = 0.0f;
        entry.z = 0.0f;
        entry.manager = this;
        sensors.push_back(entry);
    }
    
    // Create event queue for each chosen sensor
    for (size_t i = 0; i < sensors.size(); i++) {
        ASensorEventQueue* queue = ASensorManager_createEventQueue(
            sensorManager, looper, (int)i, sensorEventCallback, &sensors[i]);
        if (!queue) {
            LOGE("Failed to create event queue for %s", sensors[i].name.c_str());
            sensors[i].queue = nullptr;
            continue;
        }
        sensors[i].queue = queue;
    }
    
    sensors.erase(
        std::remove_if(sensors.begin(), sensors.end(),
            [](const SensorEntry& e) { return e.queue == nullptr; }),
        sensors.end());
    
    if (sensors.empty()) {
        LOGE("No ACCELEROMETER, GYROSCOPE, or MAGNETIC_FIELD sensors found");
        return false;
    }
    
    isInitialized = true;
    LOGI("Sensor manager initialized with %zu sensors", sensors.size());
    return true;
}

void NativeSensorManager::startListening() {
    if (!isInitialized || isListening) {
        return;
    }
    
    for (auto& entry : sensors) {
        int result = ASensorEventQueue_enableSensor(entry.queue, entry.sensor);
        if (result < 0) {
            LOGE("Failed to enable sensor %s", entry.name.c_str());
            continue;
        }
        ASensorEventQueue_setEventRate(entry.queue, entry.sensor, 200000);
    }
    
    isListening = true;
    LOGI("Started listening to %zu sensors", sensors.size());
}

void NativeSensorManager::stopListening() {
    if (!isInitialized || !isListening) {
        return;
    }
    
    for (auto& entry : sensors) {
        ASensorEventQueue_disableSensor(entry.queue, entry.sensor);
    }
    isListening = false;
    LOGI("Stopped listening to sensors");
}

void NativeSensorManager::setSensorEventCallback(SensorEventCallback callback) {
    eventCallback = callback;
}

int NativeSensorManager::getSensorCount() const {
    return (int)sensors.size();
}

void NativeSensorManager::getSensorInfo(int index, std::string& outName, int& outType) const {
    if (index < 0 || index >= (int)sensors.size()) {
        outName = "";
        outType = 0;
        return;
    }
    const SensorEntry& e = sensors[index];
    outName = e.name;
    outType = e.type;
}

void NativeSensorManager::getSensorData(int index, float& x, float& y, float& z) const {
    if (index < 0 || index >= (int)sensors.size()) {
        x = y = z = 0.0f;
        return;
    }
    const SensorEntry& e = sensors[index];
    x = e.x;
    y = e.y;
    z = e.z;
}

int NativeSensorManager::sensorEventCallback(int fd, int events, void* data) {
    SensorEntry* entry = static_cast<SensorEntry*>(data);
    if (!entry || !entry->queue) {
        return 0;
    }
    
    ASensorEvent event;
    while (ASensorEventQueue_getEvents(entry->queue, &event, 1) > 0) {
        switch (event.type) {
            case ASENSOR_TYPE_ACCELEROMETER:
                entry->x = event.acceleration.x;
                entry->y = event.acceleration.y;
                entry->z = event.acceleration.z;
                break;
            case ASENSOR_TYPE_GYROSCOPE:
                entry->x = event.vector.x;
                entry->y = event.vector.y;
                entry->z = event.vector.z;
                break;
            case ASENSOR_TYPE_MAGNETIC_FIELD:
                entry->x = event.magnetic.x;
                entry->y = event.magnetic.y;
                entry->z = event.magnetic.z;
                break;
            default:
                break;
        }
        
        if (entry->manager && entry->manager->eventCallback) {
            entry->manager->eventCallback(entry->name.c_str(), entry->type, entry->x, entry->y, entry->z);
        }
    }
    
    return 1;
}
