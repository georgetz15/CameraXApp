#include <string>
#include <jni.h>
#include <android/bitmap.h>
#include <omp.h>
#include <vector>
#include "ImageTypes.h"
#include "Resampling.h"


template<typename T>
void gray(Image<rgba<T>> img) {
#pragma omp parallel for collapse(2)
    for (int y = 0; y < img.height; ++y) {
        for (int x = 0; x < img.width; ++x) {
            // retrieve color of all channels
            auto &px = img(x, y);
            float gray = 0.299f * px.r + 0.587f * px.g + 0.114f * px.b;
            px.r = px.g = px.b = gray;
        }
    }
}

extern "C" {

JNIEXPORT void JNICALL
Java_com_android_example_cameraxapp_ImageProcessing_toGrayscale(JNIEnv *env,
                                                                jobject,
                                                                jobject bitmapIn) {

    auto bmp = JNIBitmap(&bitmapIn, env);
    auto img = bmp.getImageView();
    gray(img);
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
    auto width = inImg.width;
    auto height = inImg.height;
    int _kernelSize = static_cast<int>(kernelSize);

    // scan through every single pixel
#pragma omp parallel for collapse(2)
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            rgba16u out;
            out = 0;
            for (int j = -_kernelSize / 2; j < -_kernelSize / 2 + _kernelSize; ++j) {
                for (int i = -_kernelSize / 2; i < -_kernelSize / 2 + _kernelSize; ++i) {
                    auto xi = x + i;
                    auto yj = y + j;
                    if (xi < 0 || width <= xi || yj < 0 || height <= yj) {
                        continue;
                    }

                    // retrieve color of all channels
                    auto &px = inImg(xi, yj);
                    out.r += px.r;
                    out.g += px.g;
                    out.b += px.b;
                }
            }
            outImg(x, y) = out / (_kernelSize * _kernelSize);
            outImg(x, y).a = inImg(x, y).a;
        }
    }
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