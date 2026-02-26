package com.example.imu

import android.content.Context
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleEventObserver
import androidx.lifecycle.LifecycleOwner
import kotlinx.coroutines.flow.StateFlow

class SensorManager(private val context: Context) : LifecycleEventObserver {

    private val nativeSensorManager = NativeSensorManager()

    val sensorReadings: StateFlow<List<SensorReading>> = nativeSensorManager.sensorReadings

    fun initialize(): Boolean {
        return nativeSensorManager.initialize()
    }

    fun startListening() {
        nativeSensorManager.startListening()
    }

    fun stopListening() {
        nativeSensorManager.stopListening()
    }

    fun getSensorCount(): Int = nativeSensorManager.getSensorCount()

    fun cleanup() {
        nativeSensorManager.cleanup()
    }

    override fun onStateChanged(source: LifecycleOwner, event: Lifecycle.Event) {
        when (event) {
            Lifecycle.Event.ON_RESUME -> {
                if (initialize()) {
                    startListening()
                }
            }
            Lifecycle.Event.ON_PAUSE -> stopListening()
            Lifecycle.Event.ON_DESTROY -> cleanup()
            else -> {}
        }
    }
}
