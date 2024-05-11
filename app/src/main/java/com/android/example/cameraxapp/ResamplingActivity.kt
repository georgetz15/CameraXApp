package com.android.example.cameraxapp

import android.Manifest
import android.content.ContentValues
import android.content.pm.PackageManager
import android.graphics.Bitmap
import android.graphics.Matrix
import android.os.Build
import android.os.Bundle
import android.provider.MediaStore
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
import androidx.camera.core.ImageCaptureException
import androidx.camera.lifecycle.ProcessCameraProvider
import androidx.camera.video.MediaStoreOutputOptions
import androidx.camera.video.Quality
import androidx.camera.video.QualitySelector
import androidx.camera.video.Recorder
import androidx.camera.video.Recording
import androidx.camera.video.VideoCapture
import androidx.camera.video.VideoRecordEvent
import androidx.core.content.ContextCompat
import androidx.core.content.PermissionChecker
import com.android.example.cameraxapp.databinding.ActivityResampleBinding
import java.text.SimpleDateFormat
import java.util.Locale
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors


enum class ResamplingMethod(val value: Int) {
    BILINEAR(0), AREA(1);

    companion object {
        fun fromInt(value: Int) = entries.first { it.value == value }
    }
}

class ResamplingActivity : AppCompatActivity() {

    private lateinit var viewBinding: ActivityResampleBinding

    private var imageCapture: ImageCapture? = null

    private var videoCapture: VideoCapture<Recorder>? = null
    private var recording: Recording? = null

    private lateinit var cameraExecutor: ExecutorService

    private lateinit var tempBitmap: Bitmap
    private lateinit var resamplingMethod: ResamplingMethod

    private val activityResultLauncher = registerForActivityResult(
        ActivityResultContracts.RequestMultiplePermissions()
    ) { permissions ->
        // Handle Permission granted/rejected
        var permissionGranted = true
        permissions.entries.forEach {
            if (it.key in REQUIRED_PERMISSIONS && it.value == false) permissionGranted = false
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
        viewBinding = ActivityResampleBinding.inflate(layoutInflater)
        setContentView(viewBinding.root)

        // Request camera permissions
        if (allPermissionsGranted()) {
            startCamera()
        } else {
            requestPermissions()
        }

        // Set up the listeners for take photo and video capture buttons
        viewBinding.imageCaptureButton.setOnClickListener { takePhoto() }
        viewBinding.videoCaptureButton.setOnClickListener { captureVideo() }

        cameraExecutor = Executors.newSingleThreadExecutor()

        // Spinner
        val resamplingMethods = resources.getStringArray(R.array.ResamplingMethods)
        val spinner = viewBinding.resamplingMethodSpinner;
        if (spinner != null) {
            val adapter = ArrayAdapter(
                this,
                android.R.layout.simple_spinner_item, resamplingMethods
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
                        this@ResamplingActivity,
                        "Selected " + resamplingMethods[position], Toast.LENGTH_SHORT
                    ).show()

                    resamplingMethod = ResamplingMethod.fromInt(position)
                }

                override fun onNothingSelected(parent: AdapterView<*>?) {
                    TODO("Not yet implemented")
                }
            }
        }
    }

    private fun takePhoto() {// Get a stable reference of the modifiable image capture use case
        val imageCapture = imageCapture ?: return

        // Create time stamped name and MediaStore entry.
        val name = SimpleDateFormat(FILENAME_FORMAT, Locale.US).format(System.currentTimeMillis())
        val contentValues = ContentValues().apply {
            put(MediaStore.MediaColumns.DISPLAY_NAME, name)
            put(MediaStore.MediaColumns.MIME_TYPE, "image/jpeg")
            if (Build.VERSION.SDK_INT > Build.VERSION_CODES.P) {
                put(MediaStore.Images.Media.RELATIVE_PATH, "Pictures/CameraX-Image")
            }
        }

        // Create output options object which contains file + metadata
        val outputOptions = ImageCapture.OutputFileOptions.Builder(
            contentResolver, MediaStore.Images.Media.EXTERNAL_CONTENT_URI, contentValues
        ).build()

        // Set up image capture listener, which is triggered after photo has
        // been taken
        imageCapture.takePicture(outputOptions,
            ContextCompat.getMainExecutor(this),
            object : ImageCapture.OnImageSavedCallback {
                override fun onError(exc: ImageCaptureException) {
                    Log.e(TAG, "Photo capture failed: ${exc.message}", exc)
                }

                override fun onImageSaved(output: ImageCapture.OutputFileResults) {
                    val msg = "Photo capture succeeded: ${output.savedUri}"
                    Toast.makeText(baseContext, msg, Toast.LENGTH_SHORT).show()
                    Log.d(TAG, msg)
                }
            })
    }

    // Implements VideoCapture use case, including start and stop capturing.
    private fun captureVideo() {
        val videoCapture = this.videoCapture ?: return

        viewBinding.videoCaptureButton.isEnabled = false

        val curRecording = recording
        if (curRecording != null) {
            // Stop the current recording session.
            curRecording.stop()
            recording = null
            return
        }

        // create and start a new recording session
        val name = SimpleDateFormat(FILENAME_FORMAT, Locale.US)
            .format(System.currentTimeMillis())
        val contentValues = ContentValues().apply {
            put(MediaStore.MediaColumns.DISPLAY_NAME, name)
            put(MediaStore.MediaColumns.MIME_TYPE, "video/mp4")
            if (Build.VERSION.SDK_INT > Build.VERSION_CODES.P) {
                put(MediaStore.Video.Media.RELATIVE_PATH, "Movies/CameraX-Video")
            }
        }

        val mediaStoreOutputOptions = MediaStoreOutputOptions
            .Builder(contentResolver, MediaStore.Video.Media.EXTERNAL_CONTENT_URI)
            .setContentValues(contentValues)
            .build()
        recording = videoCapture.output
            .prepareRecording(this, mediaStoreOutputOptions)
            .apply {
                if (PermissionChecker.checkSelfPermission(
                        this@ResamplingActivity,
                        Manifest.permission.RECORD_AUDIO
                    ) ==
                    PermissionChecker.PERMISSION_GRANTED
                ) {
                    withAudioEnabled()
                }
            }
            .start(ContextCompat.getMainExecutor(this)) { recordEvent ->
                when (recordEvent) {
                    is VideoRecordEvent.Start -> {
                        viewBinding.videoCaptureButton.apply {
                            text = getString(R.string.stop_capture)
                            isEnabled = true
                        }
                    }

                    is VideoRecordEvent.Finalize -> {
                        if (!recordEvent.hasError()) {
                            val msg = "Video capture succeeded: " +
                                    "${recordEvent.outputResults.outputUri}"
                            Toast.makeText(baseContext, msg, Toast.LENGTH_SHORT)
                                .show()
                            Log.d(TAG, msg)
                        } else {
                            recording?.close()
                            recording = null
                            Log.e(
                                TAG, "Video capture ends with error: " +
                                        "${recordEvent.error}"
                            )
                        }
                        viewBinding.videoCaptureButton.apply {
                            text = getString(R.string.start_capture)
                            isEnabled = true
                        }
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
                        if (!this::tempBitmap.isInitialized) {
                            val conf = Bitmap.Config.ARGB_8888 // see other conf types
                            val (h, w) = resizeShape(256, bitmap.height, bitmap.width)
                            tempBitmap = Bitmap.createBitmap(w, h, conf)
                        }

                        // Processing logic
                        when (resamplingMethod) {
                            ResamplingMethod.BILINEAR -> bilinearResize(bitmap, tempBitmap)
                            ResamplingMethod.AREA -> areaResize(bitmap, tempBitmap)
                        }

                        bitmap = tempBitmap.copy(tempBitmap.config, true)

                        // Render from UI thread
                        runOnUiThread {
//                            viewBinding.viewFinder.layoutParams.height = bitmap.height
//                            viewBinding.viewFinder.layoutParams.width = bitmap.width
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

    // Image processing
    init {
        System.loadLibrary("cameraxapp")
    }

    private external fun getHello(): String
    private external fun toGrayscale(bitmap: Bitmap)
    private external fun blur(bitmapIn: Bitmap, bitmapOut: Bitmap, kernelSize: Int)
    private external fun bilinearResize(bitmapIn: Bitmap, bitmapOut: Bitmap)
    private external fun areaResize(bitmapIn: Bitmap, bitmapOut: Bitmap)
    private fun resize(bitmap: Bitmap, size: Int = 256): Bitmap {
        // Resize the image to 256 smaller dim

        val (newHeight, newWidth) = resizeShape(size, bitmap.height, bitmap.width)
        return Bitmap.createScaledBitmap(bitmap, newWidth, newHeight, false)
    }

    private fun resizeShape(size: Int, oldHeight: Int, oldWidth: Int): Pair<Int, Int> {
        val newHeight: Int
        val newWidth: Int
        if (oldHeight > oldWidth) {
            newWidth = size
            newHeight =
                (oldHeight.toFloat() / oldWidth.toFloat() * size).toInt()
        } else {
            newHeight = size
            newWidth =
                (oldWidth.toFloat() / oldHeight.toFloat() * size).toInt()
        }

        return Pair(newHeight, newWidth)
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