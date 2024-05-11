// Write C++ code here.
//
// Do not forget to dynamically load the C++ library into your application.
//
// For instance,
//
// In MainActivity.java:
//    static {
//       System.loadLibrary("cameraxapp");
//    }
//
// Or, in MainActivity.kt:
//    companion object {
//      init {
//         System.loadLibrary("cameraxapp")
//      }
//    }

#include <string>
#include <jni.h>
#include <android/bitmap.h>
#include <omp.h>
#include <vector>
#include "ImageTypes.h"


template<typename T>
void bilinearInterpolation(const rgba<T> *input, const int inputWidth, const int inputHeight,
                           rgba<T> *output, const int outputWidth, const int outputHeight) {
    float xRatio, yRatio;
    if (outputWidth > 1) {
        xRatio = ((float) inputWidth - 1.0) / ((float) outputWidth - 1.0);
    } else {
        xRatio = 0;
    }

    if (outputHeight > 1) {
        yRatio = ((float) inputHeight - 1.0) / ((float) outputHeight - 1.0);
    } else {
        yRatio = 0;
    }

#pragma omp parallel for collapse(2)
    for (int y = 0; y < outputHeight; y++) {
        for (int x = 0; x < outputWidth; x++) {
            int xLow = floor(xRatio * (float) x);
            int yLow = floor(yRatio * (float) y);
            int xHigh = ceil(xRatio * (float) x);
            int yHigh = ceil(yRatio * (float) y);

            float x_weight = (xRatio * (float) x) - xLow;
            float y_weight = (yRatio * (float) y) - yLow;

            auto a = input[yLow * inputWidth + xLow];
            auto b = input[yLow * inputWidth + xHigh];
            auto c = input[yHigh * inputWidth + xLow];
            auto d = input[yHigh * inputWidth + xHigh];

            auto pixel = a * (1.0f - x_weight) * (1.0f - y_weight) +
                         b * x_weight * (1.0f - y_weight) +
                         c * y_weight * (1.0f - x_weight) +
                         d * x_weight * y_weight;

            output[y * outputWidth + x] = pixel;
        }
    }
}

template<typename T>
void areaResize(const rgba<T> *input,
                const int inputWidth,
                const int inputHeight,
                rgba<T> *output,
                const int outputWidth,
                const int outputHeight) {
    const float xStep = (float) inputWidth / outputWidth;
    const float yStep = (float) inputHeight / outputHeight;
    const int kernelWidth = ceil(xStep);
    const int kernelHeight = ceil(yStep);
    const float area = (kernelWidth * kernelHeight);

#pragma omp parallel for collapse(2)
    for (int y = 0; y < outputHeight; y++) {
        for (int x = 0; x < outputWidth; x++) {
            rgba16u sum = {0, 0, 0, 0};
            for (int j = 0; j < kernelHeight; ++j) {
                for (int i = 0; i < kernelWidth; ++i) {
                    const int yi = (y * yStep + j);
                    const int xi = (x * xStep + i);
                    if (xi < 0 || inputWidth <= xi || yi < 0 || inputHeight <= yi) {
                        continue;
                    }
                    auto px = input[yi * inputWidth + xi];
                    sum.r += px.r;
                    sum.g += px.g;
                    sum.b += px.b;
                    sum.a += px.a;
                }
            }

            output[y * outputWidth + x] = sum / area;
        }
    }
}

extern "C" {

JNIEXPORT void JNICALL
Java_com_android_example_cameraxapp_ImageProcessing_toGrayscale(JNIEnv *env,
                                                                jobject,
                                                                jobject bitmapIn) {

    AndroidBitmapInfo infoIn;
    rgba8u *pixels;

    // Get image info
    if (AndroidBitmap_getInfo(env, bitmapIn, &infoIn) != ANDROID_BITMAP_RESULT_SUCCESS) {
//        return env->NewStringUTF("failed");
    }

    // Check image
    if (infoIn.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
//        return env->NewStringUTF(
//                "Only support ANDROID_BITMAP_FORMAT_RGBA_8888");
    }

    // Lock all images
    if (AndroidBitmap_lockPixels(env, bitmapIn, (void **) &pixels) !=
        ANDROID_BITMAP_RESULT_SUCCESS) {
//        return env->NewStringUTF("AndroidBitmap_lockPixels failed!");
    }

    auto GS_RED = 0.299;
    auto GS_GREEN = 0.587;
    auto GS_BLUE = 0.114;

    int width = infoIn.width;
    int height = infoIn.height;

    // scan through every single pixel
#pragma omp parallel for collapse(2)
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // retrieve color of all channels
            auto px = &pixels[y * width + x];
            auto r = px->r;
            auto g = px->g;
            auto b = px->b;
            px->r = px->g = px->b = (GS_RED * r + GS_GREEN * g + GS_BLUE * b);
        }
    }

    // Unlocks everything
    AndroidBitmap_unlockPixels(env, bitmapIn);
}

struct ImageView {
    rgba8u *pixels;
    uint width;
    uint height;
    jobject *bitmap{nullptr};
    JNIEnv *env{nullptr};

    ImageView(jobject *bitmap, JNIEnv *env) {
        this->bitmap = bitmap;
        this->env = env;

        AndroidBitmapInfo infoIn;

        // Get image info
        if (AndroidBitmap_getInfo(this->env, *this->bitmap, &infoIn) !=
            ANDROID_BITMAP_RESULT_SUCCESS) {
//            return this->env->NewStringUTF("failed");
        }

        // Check image
        if (infoIn.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
//            return env->NewStringUTF(
//                    "Only support ANDROID_BITMAP_FORMAT_RGBA_8888");
        }

        // Lock all images
        if (AndroidBitmap_lockPixels(this->env, *this->bitmap, (void **) &pixels) !=
            ANDROID_BITMAP_RESULT_SUCCESS) {
//            return env->NewStringUTF("AndroidBitmap_lockPixels failed!");
        }

        width = infoIn.width;
        height = infoIn.height;
    }

    ~ImageView() {
        AndroidBitmap_unlockPixels(this->env, *this->bitmap);
    }

    rgba8u &operator()(const int &x, const int &y) {
        return pixels[y * width + x];
    }
};

JNIEXPORT void JNICALL
Java_com_android_example_cameraxapp_ImageProcessing_boxBlur(JNIEnv *env,
                                                            jobject,
                                                            jobject bitmapIn,
                                                            jobject tempBitmap,
                                                            jint kernelSize) {
    auto inputImg = ImageView(&bitmapIn, env);
    auto outImg = ImageView(&tempBitmap, env);
    auto width = inputImg.width;
    auto height = inputImg.height;

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
                    auto &px = inputImg(xi, yj);
                    out.r += px.r;
                    out.g += px.g;
                    out.b += px.b;
                }
            }
            outImg(x, y) = out / (_kernelSize * _kernelSize);
            outImg(x, y).a = inputImg(x, y).a;
        }
    }
}

JNIEXPORT void JNICALL
Java_com_android_example_cameraxapp_ImageProcessing_bilinearResize(JNIEnv *env,
                                                                   jobject,
                                                                   jobject bitmapIn,
                                                                   jobject bitmapOut) {
    auto inputImg = ImageView(&bitmapIn, env);
    auto outImg = ImageView(&bitmapOut, env);

    bilinearInterpolation(inputImg.pixels,
                          inputImg.width,
                          inputImg.height,
                          outImg.pixels,
                          outImg.width,
                          outImg.height);
}

JNIEXPORT void JNICALL
Java_com_android_example_cameraxapp_ImageProcessing_areaResize(JNIEnv *env,
                                                               jobject,
                                                               jobject bitmapIn,
                                                               jobject bitmapOut) {
    auto inputImg = ImageView(&bitmapIn, env);
    auto outImg = ImageView(&bitmapOut, env);

    areaResize(inputImg.pixels,
               inputImg.width,
               inputImg.height,
               outImg.pixels,
               outImg.width,
               outImg.height);
}


} // extern "C"