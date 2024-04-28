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


std::string getHello() {
    return "Hello";
}

extern "C" {
JNIEXPORT jstring JNICALL Java_com_android_example_cameraxapp_MainActivity_getHello(JNIEnv
                                                                                    *env, jobject) {
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
Java_com_android_example_cameraxapp_MainActivity_toGrayscaleCpp(JNIEnv *env,
                                                                jobject,
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
#pragma omp parallel for
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

}
