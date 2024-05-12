package com.android.example.cameraxapp

import android.graphics.Bitmap

object ImageProcessing {
    init {
        System.loadLibrary("cameraxapp")
    }

    external fun toGrayscale(bitmap: Bitmap)
    external fun boxBlur(bitmapIn: Bitmap, bitmapOut: Bitmap, kernelSize: Int)
    external fun downsampleBilinear(bitmapIn: Bitmap, bitmapOut: Bitmap)
    external fun downsampleArea(bitmapIn: Bitmap, bitmapOut: Bitmap)

    fun resizeShape(size: Int, oldHeight: Int, oldWidth: Int): Pair<Int, Int> {
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
}
