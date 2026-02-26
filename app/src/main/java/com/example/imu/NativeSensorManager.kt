package com.example.imu

import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow

data class AccelerometerData(
    val x: Float = 0f,
    val y: Float = 0f,
    val z: Float = 0f
)

data class GyroscopeData(
    val x: Float = 0f,
    val y: Float = 0f,
    val z: Float = 0f
)

data class MagnetometerData(
    val x: Float = 0f,
    val y: Float = 0f,
    val z: Float = 0f
)

interface SensorDataCallback {
    fun onSensorDataChanged(x: Float, y: Float, z: Float)
    fun onGyroscopeDataChanged(x: Float, y: Float, z: Float) {}
    fun onMagnetometerDataChanged(x: Float, y: Float, z: Float) {}
}

class NativeSensorManager : SensorDataCallback {
    
    companion object {
        init {
            System.loadLibrary("imu")
        }
    }
    
    private val _accelerometerData = MutableStateFlow(AccelerometerData())
    val accelerometerData: StateFlow<AccelerometerData> = _accelerometerData.asStateFlow()
    
    private val _gyroscopeData = MutableStateFlow(GyroscopeData())
    val gyroscopeData: StateFlow<GyroscopeData> = _gyroscopeData.asStateFlow()
    
    private val _magnetometerData = MutableStateFlow(MagnetometerData())
    val magnetometerData: StateFlow<MagnetometerData> = _magnetometerData.asStateFlow()
    
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
    
    fun getCurrentGyroscopeData(): GyroscopeData {
        return if (isInitialized) {
            val data = nativeGetCurrentGyroscopeData()
            if (data != null) {
                GyroscopeData(data[0], data[1], data[2])
            } else {
                GyroscopeData()
            }
        } else {
            GyroscopeData()
        }
    }
    
    fun getCurrentMagnetometerData(): MagnetometerData {
        return if (isInitialized) {
            val data = nativeGetCurrentMagnetometerData()
            if (data != null) {
                MagnetometerData(data[0], data[1], data[2])
            } else {
                MagnetometerData()
            }
        } else {
            MagnetometerData()
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
    
    override fun onGyroscopeDataChanged(x: Float, y: Float, z: Float) {
        _gyroscopeData.value = GyroscopeData(x, y, z)
    }
    
    override fun onMagnetometerDataChanged(x: Float, y: Float, z: Float) {
        _magnetometerData.value = MagnetometerData(x, y, z)
    }
    
    // Native method declarations
    private external fun nativeInitialize(): Boolean
    private external fun nativeStartListening()
    private external fun nativeStopListening()
    private external fun nativeGetCurrentData(): FloatArray?
    private external fun nativeGetCurrentGyroscopeData(): FloatArray?
    private external fun nativeGetCurrentMagnetometerData(): FloatArray?
    private external fun nativeSetCallback(callback: SensorDataCallback)
    private external fun nativeCleanup()
} 
