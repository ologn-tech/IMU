#include "sensor_manager.h"
#include <jni.h>
#include <android/log.h>
#include <memory>

#define LOG_TAG "SensorJNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Global variables for JNI
static JavaVM* g_vm = nullptr;
static jobject g_callback_object = nullptr;
static jmethodID g_callback_method = nullptr;

// Global sensor manager instance
static std::unique_ptr<NativeSensorManager> g_sensor_manager;

// Callback function to be called from native code
void onSensorDataChanged(const AccelerometerData& data) {
    if (g_vm && g_callback_object && g_callback_method) {
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
        
        // Call the Java callback method
        env->CallVoidMethod(g_callback_object, g_callback_method, data.x, data.y, data.z);
        
        if (need_detach) {
            g_vm->DetachCurrentThread();
        }
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
        g_sensor_manager->setDataCallback(onSensorDataChanged);
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

JNIEXPORT jfloatArray JNICALL
Java_com_example_imu_NativeSensorManager_nativeGetCurrentData(JNIEnv* env, jobject thiz) {
    if (!g_sensor_manager) {
        return nullptr;
    }
    
    AccelerometerData data = g_sensor_manager->getCurrentData();
    
    jfloatArray result = env->NewFloatArray(3);
    if (result) {
        jfloat values[3] = {data.x, data.y, data.z};
        env->SetFloatArrayRegion(result, 0, 3, values);
    }
    
    return result;
}

JNIEXPORT void JNICALL
Java_com_example_imu_NativeSensorManager_nativeSetCallback(JNIEnv* env, jobject thiz, jobject callback_object) {
    // Store the JavaVM reference
    env->GetJavaVM(&g_vm);
    
    // Store the callback object as a global reference
    if (g_callback_object) {
        env->DeleteGlobalRef(g_callback_object);
    }
    g_callback_object = env->NewGlobalRef(callback_object);
    
    // Get the method ID for the callback
    jclass callback_class = env->GetObjectClass(callback_object);
    g_callback_method = env->GetMethodID(callback_class, "onSensorDataChanged", "(FFF)V");
    
    if (!g_callback_method) {
        LOGE("Failed to find onSensorDataChanged method");
    }
}

JNIEXPORT void JNICALL
Java_com_example_imu_NativeSensorManager_nativeCleanup(JNIEnv* env, jobject thiz) {
    if (g_callback_object) {
        env->DeleteGlobalRef(g_callback_object);
        g_callback_object = nullptr;
    }
    
    g_sensor_manager.reset();
    g_vm = nullptr;
    g_callback_method = nullptr;
}

} // extern "C" 
