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

std::string getHello() {
    return "Hello";
}

extern "C" {
JNIEXPORT jstring JNICALL
Java_com_android_example_cameraxapp_GrayscaleActivity_getHello(JNIEnv *env, jobject) {
    const auto hello = getHello();
    jstring result = env->NewStringUTF(hello.c_str());
    return result;
}

struct RGBA8 {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

JNIEXPORT jobject JNICALL
Java_com_android_example_cameraxapp_GrayscaleActivity_toGrayscale(JNIEnv *env, jobject,
                                                                  jobject bitmapIn) {

    AndroidBitmapInfo infoIn;
    RGBA8 *pixels;

    // Get image info
    if (AndroidBitmap_getInfo(env, bitmapIn, &infoIn) != ANDROID_BITMAP_RESULT_SUCCESS) {
        return env->NewStringUTF("failed");
    }

    // Check image
    if (infoIn.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
        return env->NewStringUTF(
                "Only support ANDROID_BITMAP_FORMAT_RGBA_8888");
    }

    // Lock all images
    if (AndroidBitmap_lockPixels(env, bitmapIn, (void **) &pixels) !=
        ANDROID_BITMAP_RESULT_SUCCESS) {
        return env->NewStringUTF("AndroidBitmap_lockPixels failed!");
    }

    auto GS_RED = 0.299;
    auto GS_GREEN = 0.587;
    auto GS_BLUE = 0.114;

    // get image size
    // get image size
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

    return bitmapIn;
}

struct ImageView {
    RGBA8 *pixels;
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
};

JNIEXPORT jobject JNICALL
Java_com_android_example_cameraxapp_GrayscaleActivity_blur(JNIEnv *env,
                                                           jobject,
                                                           jobject bitmapIn,
                                                           jobject tempBitmap,
                                                           jint kernelSize) {

    auto GS_RED = 0.299;
    auto GS_GREEN = 0.587;
    auto GS_BLUE = 0.114;

    auto inputImg = ImageView(&bitmapIn, env);
    auto outImg = ImageView(&tempBitmap, env);
    auto inputPixels = inputImg.pixels;
    auto outputPixels = outImg.pixels;
    auto width = inputImg.width;
    auto height = inputImg.height;

    int _kernelSize = static_cast<int>(kernelSize);

    // scan through every single pixel
#pragma omp parallel for collapse(2)
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            double sum = 0;
            for (int j = -_kernelSize / 2; j < -_kernelSize / 2 + _kernelSize; ++j) {
                for (int i = -_kernelSize / 2; i < -_kernelSize / 2 + _kernelSize; ++i) {
                    auto xi = x + i;
                    auto yj = y + j;
                    if (xi < 0 || width <= xi || yj < 0 || height <= yj) {
                        continue;
                    }

                    // retrieve color of all channels
                    auto px = inputPixels[yj * width + xi];
                    auto r = px.r;
                    auto g = px.g;
                    auto b = px.b;
                    double gray = (GS_RED * r + GS_GREEN * g + GS_BLUE * b);
                    sum += gray;
                }
            }
            auto px = &outputPixels[y * width + x];
            px->r = px->g = px->b = sum / (_kernelSize * _kernelSize);
        }
    }

    return bitmapIn;
}

}  // extern "C"
