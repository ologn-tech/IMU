#include "sensor_manager.h"
#include <jni.h>
#include <android/log.h>
#include <memory>

#define LOG_TAG "SensorJNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static JavaVM* g_vm = nullptr;
static jobject g_callback_object = nullptr;
static jmethodID g_onSensorEvent_method = nullptr;

static std::unique_ptr<NativeSensorManager> g_sensor_manager;

static void forwardSensorEvent(const char* name, int type, float x, float y, float z) {
    if (!g_vm || !g_callback_object || !g_onSensorEvent_method) return;
    
    JNIEnv* env;
    bool need_detach = false;
    int get_env_stat = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (get_env_stat == JNI_EDETACHED) {
        if (g_vm->AttachCurrentThread(&env, nullptr) != 0) {
            LOGE("Failed to attach current thread");
            return;
        }
        need_detach = true;
    }
    
    jstring jname = env->NewStringUTF(name ? name : "");
    if (jname) {
        env->CallVoidMethod(g_callback_object, g_onSensorEvent_method, jname, (jint)type, (jfloat)x, (jfloat)y, (jfloat)z);
        env->DeleteLocalRef(jname);
    }
    
    if (need_detach) {
        g_vm->DetachCurrentThread();
    }
}

extern "C" {

JNIEXPORT jboolean JNICALL
Java_com_example_imu_NativeSensorManager_nativeInitialize(JNIEnv* env, jobject thiz) {
    if (!g_sensor_manager) {
        g_sensor_manager = std::make_unique<NativeSensorManager>();
    }
    
    bool result = g_sensor_manager->initialize();
    if (result) {
        g_sensor_manager->setSensorEventCallback(forwardSensorEvent);
    }
    return result;
}

JNIEXPORT void JNICALL
Java_com_example_imu_NativeSensorManager_nativeStartListening(JNIEnv* env, jobject thiz) {
    if (g_sensor_manager) {
        g_sensor_manager->startListening();
    }
}

JNIEXPORT void JNICALL
Java_com_example_imu_NativeSensorManager_nativeStopListening(JNIEnv* env, jobject thiz) {
    if (g_sensor_manager) {
        g_sensor_manager->stopListening();
    }
}

JNIEXPORT void JNICALL
Java_com_example_imu_NativeSensorManager_nativeSetCallback(JNIEnv* env, jobject thiz, jobject callback_object) {
    env->GetJavaVM(&g_vm);
    
    if (g_callback_object) {
        env->DeleteGlobalRef(g_callback_object);
    }
    g_callback_object = env->NewGlobalRef(callback_object);
    
    jclass callback_class = env->GetObjectClass(callback_object);
    g_onSensorEvent_method = env->GetMethodID(callback_class, "onSensorEvent", "(Ljava/lang/String;IFFF)V");
    if (!g_onSensorEvent_method) {
        LOGE("Failed to find onSensorEvent(String,int,float,float,float) method");
    }
}

JNIEXPORT jint JNICALL
Java_com_example_imu_NativeSensorManager_nativeGetSensorCount(JNIEnv* env, jobject thiz) {
    if (!g_sensor_manager) return 0;
    return (jint)g_sensor_manager->getSensorCount();
}

JNIEXPORT jstring JNICALL
Java_com_example_imu_NativeSensorManager_nativeGetSensorName(JNIEnv* env, jobject thiz, jint index) {
    if (!g_sensor_manager) return nullptr;
    std::string name;
    int type;
    g_sensor_manager->getSensorInfo((int)index, name, type);
    return env->NewStringUTF(name.c_str());
}

JNIEXPORT jint JNICALL
Java_com_example_imu_NativeSensorManager_nativeGetSensorType(JNIEnv* env, jobject thiz, jint index) {
    if (!g_sensor_manager) return 0;
    std::string name;
    int type;
    g_sensor_manager->getSensorInfo((int)index, name, type);
    return (jint)type;
}

JNIEXPORT jfloatArray JNICALL
Java_com_example_imu_NativeSensorManager_nativeGetSensorData(JNIEnv* env, jobject thiz, jint index) {
    if (!g_sensor_manager) return nullptr;
    float x, y, z;
    g_sensor_manager->getSensorData((int)index, x, y, z);
    jfloatArray result = env->NewFloatArray(3);
    if (result) {
        jfloat values[3] = {x, y, z};
        env->SetFloatArrayRegion(result, 0, 3, values);
    }
    return result;
}

JNIEXPORT void JNICALL
Java_com_example_imu_NativeSensorManager_nativeCleanup(JNIEnv* env, jobject thiz) {
    if (g_callback_object) {
        env->DeleteGlobalRef(g_callback_object);
        g_callback_object = nullptr;
    }
    g_sensor_manager.reset();
    g_vm = nullptr;
    g_onSensorEvent_method = nullptr;
}

} // extern "C"
