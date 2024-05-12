#include <string>
#include <jni.h>
#include <android/bitmap.h>
#include <omp.h>
#include <vector>
#include "ImageTypes.h"
#include "Resampling.h"
#include "Filters.h"


extern "C" {

JNIEXPORT void JNICALL
Java_com_android_example_cameraxapp_ImageProcessing_gray(JNIEnv *env,
                                                         jobject,
                                                         jobject bitmapIn) {

    auto bmp = JNIBitmap(&bitmapIn, env);
    auto img = bmp.getImageView();
    gray(img);
}

JNIEXPORT void JNICALL
Java_com_android_example_cameraxapp_ImageProcessing_sepia(JNIEnv *env,
                                                                jobject,
                                                                jobject bitmapIn) {

    auto bmp = JNIBitmap(&bitmapIn, env);
    auto img = bmp.getImageView();
    sepia(img);
}

JNIEXPORT void JNICALL
Java_com_android_example_cameraxapp_ImageProcessing_boxBlur(JNIEnv *env,
                                                            jobject,
                                                            jobject bitmapIn,
                                                            jobject tempBitmap,
                                                            jint kernelSize) {
    auto inBmp = JNIBitmap(&bitmapIn, env);
    auto outBmp = JNIBitmap(&tempBitmap, env);
    auto inImg = inBmp.getImageView();
    auto outImg = outBmp.getImageView();

    boxBlur(inImg, outImg, kernelSize);
}

JNIEXPORT void JNICALL
Java_com_android_example_cameraxapp_ImageProcessing_gaussianBlur(JNIEnv *env,
                                                            jobject,
                                                            jobject bitmapIn,
                                                            jobject tempBitmap,
                                                            jfloat sigma) {
    auto inBmp = JNIBitmap(&bitmapIn, env);
    auto outBmp = JNIBitmap(&tempBitmap, env);
    auto inImg = inBmp.getImageView();
    auto outImg = outBmp.getImageView();

    gaussianBlur(inImg, outImg, sigma);
}

JNIEXPORT void JNICALL
Java_com_android_example_cameraxapp_ImageProcessing_downsampleBilinear(JNIEnv *env,
                                                                       jobject,
                                                                       jobject bitmapIn,
                                                                       jobject bitmapOut) {
    auto inBmp = JNIBitmap(&bitmapIn, env);
    auto outBmp = JNIBitmap(&bitmapOut, env);
    auto inImg = inBmp.getImageView();
    auto outImg = outBmp.getImageView();

    downsampleBilinear(inImg, outImg);
}

JNIEXPORT void JNICALL
Java_com_android_example_cameraxapp_ImageProcessing_downsampleArea(JNIEnv *env,
                                                                   jobject,
                                                                   jobject bitmapIn,
                                                                   jobject bitmapOut) {
    auto inBmp = JNIBitmap(&bitmapIn, env);
    auto outBmp = JNIBitmap(&bitmapOut, env);
    auto inImg = inBmp.getImageView();
    auto outImg = outBmp.getImageView();

    downsampleArea(inImg, outImg);
}


} // extern "C"