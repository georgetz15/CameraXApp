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

RGBA8 operator*(const RGBA8 &px, const float &mul) {
    return {
            static_cast<uint8_t>(px.r * mul),
            static_cast<uint8_t>(px.g * mul),
            static_cast<uint8_t>(px.b * mul),
            static_cast<uint8_t>(px.a * mul),
    };
}

RGBA8 operator+(const RGBA8 &px1, const RGBA8 &px2) {
    return {
            static_cast <uint8_t>(px1.r + px2.r),
            static_cast <uint8_t>(px1.g + px2.g),
            static_cast <uint8_t>(px1.b + px2.b),
            static_cast <uint8_t>(px1.a + px2.a),
    };
}

JNIEXPORT void JNICALL
Java_com_android_example_cameraxapp_GrayscaleActivity_toGrayscale(JNIEnv *env, jobject,
                                                                  jobject bitmapIn) {

    AndroidBitmapInfo infoIn;
    RGBA8 *pixels;

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

JNIEXPORT void JNICALL
Java_com_android_example_cameraxapp_GrayscaleActivity_blur(JNIEnv *env,
                                                           jobject,
                                                           jobject bitmapIn,
                                                           jobject tempBitmap,
                                                           jint kernelSize) {
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
            double rs, gs, bs, as;
            rs = gs = bs = as = 0;
            for (int j = -_kernelSize / 2; j < -_kernelSize / 2 + _kernelSize; ++j) {
                for (int i = -_kernelSize / 2; i < -_kernelSize / 2 + _kernelSize; ++i) {
                    auto xi = x + i;
                    auto yj = y + j;
                    if (xi < 0 || width <= xi || yj < 0 || height <= yj) {
                        continue;
                    }

                    // retrieve color of all channels
                    auto px = inputPixels[yj * width + xi];
                    rs += px.r;
                    gs += px.g;
                    bs += px.b;
                    as += px.a;
                }
            }
            auto px = &outputPixels[y * width + x];
            px->r = rs / (_kernelSize * _kernelSize);
            px->g = gs / (_kernelSize * _kernelSize);
            px->b = bs / (_kernelSize * _kernelSize);
            px->a = as / (_kernelSize * _kernelSize);
        }
    }
}

JNIEXPORT void JNICALL
Java_com_android_example_cameraxapp_GrayscaleActivity_bilinearResize(JNIEnv *env,
                                                                     jobject,
                                                                     jobject bitmapIn,
                                                                     jobject bitmapOut) {
    auto inputImg = ImageView(&bitmapIn, env);
    auto outImg = ImageView(&bitmapOut, env);
    auto inputPixels = inputImg.pixels;
    auto outputPixels = outImg.pixels;

    float xRatio, yRatio;
    if (outImg.width > 1) {
        xRatio = ((float) inputImg.width - 1.0) / ((float) outImg.width - 1.0);
    } else {
        xRatio = 0;
    }

    if (outImg.height > 1) {
        yRatio = ((float) inputImg.height - 1.0) / ((float) outImg.height - 1.0);
    } else {
        yRatio = 0;
    }

#pragma omp parallel for collapse(2)
    for (int y = 0; y < outImg.height; y++) {
        for (int x = 0; x < outImg.width; x++) {
            int xLow = floor(xRatio * (float) x);
            int yLow = floor(yRatio * (float) y);
            int xHigh = ceil(xRatio * (float) x);
            int yHigh = ceil(yRatio * (float) y);

            float x_weight = (xRatio * (float) x) - xLow;
            float y_weight = (yRatio * (float) y) - yLow;

            auto a = inputPixels[yLow * inputImg.width + xLow];
            auto b = inputPixels[yLow * inputImg.width + xHigh];
            auto c = inputPixels[yHigh * inputImg.width + xLow];
            auto d = inputPixels[yHigh * inputImg.width + xHigh];

            auto pixel = a * (1.0f - x_weight) * (1.0f - y_weight) +
                         b * x_weight * (1.0f - y_weight) +
                         c * y_weight * (1.0f - x_weight) +
                         d * x_weight * y_weight;

            outputPixels[y * outImg.width + x] = pixel;
        }
    }
}

}

// extern "C"
