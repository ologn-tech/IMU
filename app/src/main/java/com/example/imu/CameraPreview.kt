package com.example.imu

import android.Manifest
import android.content.Context
import android.content.pm.PackageManager
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.camera.core.CameraSelector
import androidx.camera.core.Preview
import androidx.camera.lifecycle.ProcessCameraProvider
import androidx.camera.view.PreviewView
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.Text
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalLifecycleOwner
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.viewinterop.AndroidView
import androidx.core.content.ContextCompat
import androidx.lifecycle.LifecycleOwner
import com.google.accompanist.permissions.ExperimentalPermissionsApi
import com.google.accompanist.permissions.isGranted
import com.google.accompanist.permissions.rememberPermissionState
import com.google.accompanist.permissions.shouldShowRationale

@OptIn(ExperimentalPermissionsApi::class)
@Composable
fun CameraPreview() {
    val context = LocalContext.current
    val lifecycleOwner = LocalLifecycleOwner.current
    
    val cameraPermissionState = rememberPermissionState(
        Manifest.permission.CAMERA
    ) { isGranted ->
        // Handle permission result
    }
    
    LaunchedEffect(Unit) {
        if (!cameraPermissionState.status.isGranted) {
            cameraPermissionState.launchPermissionRequest()
        }
    }
    
    when {
        cameraPermissionState.status.isGranted -> {
            CameraContent(context, lifecycleOwner)
        }
        cameraPermissionState.status.shouldShowRationale -> {
            Box(
                modifier = Modifier.fillMaxSize(),
                contentAlignment = Alignment.Center
            ) {
                Text("Camera permission is required to show camera preview")
            }
        }
        else -> {
            Box(
                modifier = Modifier.fillMaxSize(),
                contentAlignment = Alignment.Center
            ) {
                Text("Camera permission denied")
            }
        }
    }
}

@Composable
fun CameraContent(context: Context, lifecycleOwner: LifecycleOwner) {
    val cameraProviderFuture = remember { ProcessCameraProvider.getInstance(context) }
    
    // Create and observe sensor manager
    val sensorManager = remember { SensorManager(context) }
    val accelerometerData by sensorManager.accelerometerData.collectAsState()
    val gyroscopeData by sensorManager.gyroscopeData.collectAsState()
    val magnetometerData by sensorManager.magnetometerData.collectAsState()
    
    // Observe lifecycle events
    DisposableEffect(lifecycleOwner) {
        lifecycleOwner.lifecycle.addObserver(sensorManager)
        onDispose {
            lifecycleOwner.lifecycle.removeObserver(sensorManager)
        }
    }
    
    Box(modifier = Modifier.fillMaxSize()) {
        // Camera preview
        AndroidView(
            factory = { ctx ->
                PreviewView(ctx).apply {
                    this.scaleType = PreviewView.ScaleType.FILL_CENTER
                }
            },
            modifier = Modifier.fillMaxSize(),
            update = { previewView ->
                cameraProviderFuture.addListener({
                    val cameraProvider = cameraProviderFuture.get()
                    
                    val preview = Preview.Builder().build().also {
                        it.setSurfaceProvider(previewView.surfaceProvider)
                    }
                    
                    val cameraSelector = CameraSelector.DEFAULT_BACK_CAMERA
                    
                    try {
                        cameraProvider.unbindAll()
                        cameraProvider.bindToLifecycle(
                            lifecycleOwner,
                            cameraSelector,
                            preview
                        )
                    } catch (e: Exception) {
                        e.printStackTrace()
                    }
                }, ContextCompat.getMainExecutor(context))
            }
        )
        
        // Sensors data overlay (Accelerometer, Gyroscope, Magnetometer - vertical)
        SensorsOverlay(
            accelerometerData = accelerometerData,
            gyroscopeData = gyroscopeData,
            magnetometerData = magnetometerData,
            modifier = Modifier.align(Alignment.TopStart)
        )
    }
}

@Composable
fun SensorsOverlay(
    accelerometerData: AccelerometerData,
    gyroscopeData: GyroscopeData,
    magnetometerData: MagnetometerData,
    modifier: Modifier = Modifier
) {
    Column(
        modifier = modifier.padding(16.dp),
        verticalArrangement = Arrangement.spacedBy(8.dp)
    ) {
        AccelerometerOverlay(accelerometerData = accelerometerData)
        GyroscopeOverlay(gyroscopeData = gyroscopeData)
        MagnetometerOverlay(magnetometerData = magnetometerData)
    }
}

@Composable
fun AccelerometerOverlay(
    accelerometerData: AccelerometerData,
    modifier: Modifier = Modifier
) {
    SensorCard(
        title = "Accelerometer",
        x = accelerometerData.x,
        y = accelerometerData.y,
        z = accelerometerData.z,
        unit = "m/s²",
        modifier = modifier
    )
}

@Composable
fun GyroscopeOverlay(
    gyroscopeData: GyroscopeData,
    modifier: Modifier = Modifier
) {
    SensorCard(
        title = "Gyroscope",
        x = gyroscopeData.x,
        y = gyroscopeData.y,
        z = gyroscopeData.z,
        unit = "rad/s",
        modifier = modifier
    )
}

@Composable
fun MagnetometerOverlay(
    magnetometerData: MagnetometerData,
    modifier: Modifier = Modifier
) {
    SensorCard(
        title = "Magnetometer",
        x = magnetometerData.x,
        y = magnetometerData.y,
        z = magnetometerData.z,
        unit = "µT",
        modifier = modifier
    )
}

@Composable
private fun SensorCard(
    title: String,
    x: Float,
    y: Float,
    z: Float,
    unit: String,
    modifier: Modifier = Modifier
) {
    Card(
        modifier = modifier,
        colors = CardDefaults.cardColors(
            containerColor = Color.Black.copy(alpha = 0.7f)
        ),
        shape = RoundedCornerShape(8.dp)
    ) {
        Column(
            modifier = Modifier.padding(16.dp),
            verticalArrangement = Arrangement.spacedBy(2.dp)
        ) {
            Text(
                text = title,
                color = Color.White,
                fontSize = 16.sp,
                fontWeight = FontWeight.Bold
            )
            Text(
                text = "X: ${String.format("%.2f", x)} $unit",
                color = Color.White,
                fontSize = 14.sp
            )
            Text(
                text = "Y: ${String.format("%.2f", y)} $unit",
                color = Color.White,
                fontSize = 14.sp
            )
            Text(
                text = "Z: ${String.format("%.2f", z)} $unit",
                color = Color.White,
                fontSize = 14.sp
            )
        }
    }
} 
