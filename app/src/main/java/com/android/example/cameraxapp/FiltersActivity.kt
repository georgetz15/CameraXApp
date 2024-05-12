package com.android.example.cameraxapp

import android.Manifest
import android.content.pm.PackageManager
import android.graphics.Bitmap
import android.graphics.Matrix
import android.os.Build
import android.os.Bundle
import android.util.Log
import android.view.View
import android.widget.AdapterView
import android.widget.ArrayAdapter
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import androidx.camera.core.CameraSelector
import androidx.camera.core.ImageAnalysis
import androidx.camera.core.ImageCapture
import androidx.camera.lifecycle.ProcessCameraProvider
import androidx.camera.video.Quality
import androidx.camera.video.QualitySelector
import androidx.camera.video.Recorder
import androidx.camera.video.Recording
import androidx.camera.video.VideoCapture
import androidx.core.content.ContextCompat
import com.android.example.cameraxapp.ImageProcessing.boxBlur
import com.android.example.cameraxapp.ImageProcessing.gaussianBlur
import com.android.example.cameraxapp.ImageProcessing.gray
import com.android.example.cameraxapp.ImageProcessing.sepia
import com.android.example.cameraxapp.databinding.BasicDisplayBinding
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors


enum class FilterMethod(val value: Int) {
    GRAY(0),
    BLUR(1),
    GAUSS_BLUR(2),
    SEPIA(3), ;

    companion object {
        fun fromInt(value: Int) = entries.first { it.value == value }
    }
}

class FiltersActivity : AppCompatActivity() {

    private lateinit var viewBinding: BasicDisplayBinding

    private var imageCapture: ImageCapture? = null

    private var videoCapture: VideoCapture<Recorder>? = null
    private var recording: Recording? = null

    private lateinit var cameraExecutor: ExecutorService

    private lateinit var tempBitmap: Bitmap
    private lateinit var filterMethod: FilterMethod

    private val activityResultLauncher = registerForActivityResult(
        ActivityResultContracts.RequestMultiplePermissions()
    ) { permissions ->
        // Handle Permission granted/rejected
        var permissionGranted = true
        permissions.entries.forEach {
            if (it.key in REQUIRED_PERMISSIONS && !it.value) permissionGranted = false
        }
        if (!permissionGranted) {
            Toast.makeText(
                baseContext, "Permission request denied", Toast.LENGTH_SHORT
            ).show()
        } else {
            startCamera()
        }
    }


    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        viewBinding = BasicDisplayBinding.inflate(layoutInflater)
        setContentView(viewBinding.root)

        // Request camera permissions
        if (allPermissionsGranted()) {
            startCamera()
        } else {
            requestPermissions()
        }

        cameraExecutor = Executors.newSingleThreadExecutor()

        // Spinner
        val filterMethods = resources.getStringArray(R.array.FilterMethods)
        val spinner = viewBinding.dropdownSpinner
        if (spinner != null) {
            val adapter = ArrayAdapter(
                this,
                R.layout.general_list_item, filterMethods
            )
            spinner.adapter = adapter
            spinner.onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
                override fun onItemSelected(
                    parent: AdapterView<*>?,
                    view: View?,
                    position: Int,
                    id: Long
                ) {
                    Toast.makeText(
                        this@FiltersActivity,
                        "Selected " + filterMethods[position], Toast.LENGTH_SHORT
                    ).show()

                    filterMethod = FilterMethod.fromInt(position)
                }

                override fun onNothingSelected(parent: AdapterView<*>?) {
                    TODO("Not yet implemented")
                }
            }
        }
    }

    private fun startCamera() {
        val cameraProviderFuture = ProcessCameraProvider.getInstance(this)

        cameraProviderFuture.addListener({
            // Used to bind the lifecycle of cameras to the lifecycle owner
            val cameraProvider: ProcessCameraProvider = cameraProviderFuture.get()

            // Image capture
            imageCapture = ImageCapture.Builder().build()
            val imageViewer = ImageAnalysis.Builder()
                .setOutputImageFormat(ImageAnalysis.OUTPUT_IMAGE_FORMAT_RGBA_8888)
                .build()
                .also {
                    it.setAnalyzer(cameraExecutor) { image ->
                        Log.d("image view", "image.format = ${image.format}")
                        // Get the image bitmap
                        var bitmap = image.toBitmap()
                        image.close()

                        // Rotate the image if needed
                        val matrix =
                            Matrix().apply { postRotate(image.imageInfo.rotationDegrees.toFloat()) }
                        bitmap =
                            Bitmap.createBitmap(
                                bitmap,
                                0,
                                0,
                                bitmap.width,
                                bitmap.height,
                                matrix,
                                true
                            )
                        // Create resized
//                        val conf = Bitmap.Config.ARGB_8888 // see other conf types
//                        val (h, w) = resizeShape(256, bitmap.height, bitmap.width)
//                        if (!this::tempBitmap.isInitialized) {
//                            tempBitmap = Bitmap.createBitmap(w, h, conf)
//                        }
//                        downsampleArea(bitmap, tempBitmap)
//                        bitmap = tempBitmap.copy(tempBitmap.config, true)
                        tempBitmap = bitmap.copy(bitmap.config, true)

                        when (filterMethod) {
                            FilterMethod.GRAY -> {
                                gray(bitmap)
                            }

                            FilterMethod.BLUR -> {
                                boxBlur(tempBitmap, bitmap, 5)
                            }

                            FilterMethod.GAUSS_BLUR -> {
                                gaussianBlur(tempBitmap, bitmap, 5)
                            }

                            FilterMethod.SEPIA -> {
                                sepia(bitmap)
                            }
                        }

                        // Render from UI thread
                        runOnUiThread {
                            viewBinding.viewFinder.setImageBitmap(bitmap)
                        }
                    }
                }

            // Recorder
            val recorder = Recorder.Builder()
                .setQualitySelector(QualitySelector.from(Quality.HIGHEST))
                .build()
            videoCapture = VideoCapture.withOutput(recorder)


            // Select back camera as a default
            val cameraSelector = CameraSelector.DEFAULT_BACK_CAMERA

            try {
                // Unbind use cases before rebinding
                cameraProvider.unbindAll()

                // Bind use cases to camera
                cameraProvider.bindToLifecycle(
                    this,
                    cameraSelector,
                    imageViewer,
                    imageCapture,
                    videoCapture
                )

            } catch (exc: Exception) {
                Log.e(TAG, "Use case binding failed", exc)
            }

        }, ContextCompat.getMainExecutor(this))
    }

    private fun requestPermissions() {
        activityResultLauncher.launch(REQUIRED_PERMISSIONS)
    }

    private fun allPermissionsGranted() = REQUIRED_PERMISSIONS.all {
        ContextCompat.checkSelfPermission(
            baseContext, it
        ) == PackageManager.PERMISSION_GRANTED
    }

    override fun onDestroy() {
        super.onDestroy()
        cameraExecutor.shutdown()
    }

    companion object {
        private const val TAG = "CameraXApp"
        private const val FILENAME_FORMAT = "yyyy-MM-dd-HH-mm-ss-SSS"
        private val REQUIRED_PERMISSIONS = mutableListOf(
            Manifest.permission.CAMERA, Manifest.permission.RECORD_AUDIO
        ).apply {
            if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.P) {
                add(Manifest.permission.WRITE_EXTERNAL_STORAGE)
            }
        }.toTypedArray()
    }
}
