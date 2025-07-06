package com.example.imu

import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow

data class AccelerometerData(
    val x: Float = 0f,
    val y: Float = 0f,
    val z: Float = 0f
)

interface SensorDataCallback {
    fun onSensorDataChanged(x: Float, y: Float, z: Float)
}

class NativeSensorManager : SensorDataCallback {
    
    companion object {
        init {
            System.loadLibrary("imu")
        }
    }
    
    private val _accelerometerData = MutableStateFlow(AccelerometerData())
    val accelerometerData: StateFlow<AccelerometerData> = _accelerometerData.asStateFlow()
    
    private var isInitialized = false
    
    fun initialize(): Boolean {
        if (!isInitialized) {
            isInitialized = nativeInitialize()
            if (isInitialized) {
                nativeSetCallback(this)
            }
        }
        return isInitialized
    }
    
    fun startListening() {
        if (isInitialized) {
            nativeStartListening()
        }
    }
    
    fun stopListening() {
        if (isInitialized) {
            nativeStopListening()
        }
    }
    
    fun getCurrentData(): AccelerometerData {
        return if (isInitialized) {
            val data = nativeGetCurrentData()
            if (data != null) {
                AccelerometerData(data[0], data[1], data[2])
            } else {
                AccelerometerData()
            }
        } else {
            AccelerometerData()
        }
    }
    
    fun cleanup() {
        if (isInitialized) {
            nativeCleanup()
            isInitialized = false
        }
    }
    
    override fun onSensorDataChanged(x: Float, y: Float, z: Float) {
        _accelerometerData.value = AccelerometerData(x, y, z)
    }
    
    // Native method declarations
    private external fun nativeInitialize(): Boolean
    private external fun nativeStartListening()
    private external fun nativeStopListening()
    private external fun nativeGetCurrentData(): FloatArray?
    private external fun nativeSetCallback(callback: SensorDataCallback)
    private external fun nativeCleanup()
} 
