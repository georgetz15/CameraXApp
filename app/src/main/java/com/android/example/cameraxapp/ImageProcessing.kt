package com.android.example.cameraxapp

import android.graphics.Bitmap

object ImageProcessing {
    init {
        System.loadLibrary("cameraxapp")
    }

    external fun toGrayscale(bitmap: Bitmap)
    external fun blur(bitmapIn: Bitmap, bitmapOut: Bitmap, kernelSize: Int)
    external fun bilinearResize(bitmapIn: Bitmap, bitmapOut: Bitmap)
    external fun areaResize(bitmapIn: Bitmap, bitmapOut: Bitmap)
}
