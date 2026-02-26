package com.example.imu

import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow

// Android sensor type constants (ASENSOR_TYPE_*)
const val SENSOR_TYPE_ACCELEROMETER = 1
const val SENSOR_TYPE_MAGNETIC_FIELD = 2
const val SENSOR_TYPE_GYROSCOPE = 4

data class SensorReading(
    val name: String,
    val type: Int,
    val x: Float = 0f,
    val y: Float = 0f,
    val z: Float = 0f
) {
    fun unit(): String = when (type) {
        SENSOR_TYPE_ACCELEROMETER -> "m/s²"
        SENSOR_TYPE_GYROSCOPE -> "rad/s"
        SENSOR_TYPE_MAGNETIC_FIELD -> "µT"
        else -> ""
    }
}

interface SensorDataCallback {
    fun onSensorEvent(sensorName: String, type: Int, x: Float, y: Float, z: Float)
}

class NativeSensorManager : SensorDataCallback {

    companion object {
        init {
            System.loadLibrary("imu")
        }
    }

    private val _sensorReadings = MutableStateFlow<List<SensorReading>>(emptyList())
    val sensorReadings: StateFlow<List<SensorReading>> = _sensorReadings.asStateFlow()

    private var isInitialized = false
    private val readingsByKey = mutableMapOf<String, SensorReading>()
    private val sensorOrder = mutableListOf<String>()  // stable order for UI

    fun initialize(): Boolean {
        if (!isInitialized) {
            isInitialized = nativeInitialize()
            if (isInitialized) {
                nativeSetCallback(this)
                sensorOrder.clear()
                val count = nativeGetSensorCount()
                for (i in 0 until count) {
                    val name = nativeGetSensorName(i) ?: "Sensor $i"
                    val type = nativeGetSensorType(i)
                    val data = nativeGetSensorData(i)
                    val (x, y, z) = if (data != null && data.size >= 3) {
                        Triple(data[0], data[1], data[2])
                    } else Triple(0f, 0f, 0f)
                    sensorOrder.add(name)
                    readingsByKey[name] = SensorReading(name, type, x, y, z)
                }
                _sensorReadings.value = sensorOrder.mapNotNull { readingsByKey[it] }
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

    fun getSensorCount(): Int = if (isInitialized) nativeGetSensorCount() else 0

    fun getSensorReading(index: Int): SensorReading? {
        if (!isInitialized || index < 0 || index >= nativeGetSensorCount()) return null
        val name = nativeGetSensorName(index) ?: return null
        val type = nativeGetSensorType(index)
        val data = nativeGetSensorData(index)
        val (x, y, z) = if (data != null && data.size >= 3) {
            Triple(data[0], data[1], data[2])
        } else Triple(0f, 0f, 0f)
        return SensorReading(name, type, x, y, z)
    }

    fun cleanup() {
        if (isInitialized) {
            nativeCleanup()
            isInitialized = false
            readingsByKey.clear()
            sensorOrder.clear()
            _sensorReadings.value = emptyList()
        }
    }

    override fun onSensorEvent(sensorName: String, type: Int, x: Float, y: Float, z: Float) {
        readingsByKey[sensorName] = SensorReading(sensorName, type, x, y, z)
        if (sensorName !in sensorOrder) sensorOrder.add(sensorName)
        _sensorReadings.value = sensorOrder.mapNotNull { readingsByKey[it] }
    }

    private external fun nativeInitialize(): Boolean
    private external fun nativeStartListening()
    private external fun nativeStopListening()
    private external fun nativeSetCallback(callback: SensorDataCallback)
    private external fun nativeGetSensorCount(): Int
    private external fun nativeGetSensorName(index: Int): String?
    private external fun nativeGetSensorType(index: Int): Int
    private external fun nativeGetSensorData(index: Int): FloatArray?
    private external fun nativeCleanup()
}
