//
// Created by George Tzoupis on 11/5/2024.
//

#ifndef CAMERAXAPP_IMAGETYPES_H
#define CAMERAXAPP_IMAGETYPES_H

#include <type_traits>
#include <jni.h>
#include <android/bitmap.h>
#include <sys/types.h>

template<typename T>
struct rgba {
    T r;
    T g;
    T b;
    T a;

    template<typename O>
    operator rgba<O>() {
        return {
                static_cast<O>(r),
                static_cast<O>(g),
                static_cast<O>(b),
                static_cast<O>(a),
        };
    }

    template<typename O,
            typename = typename std::enable_if<std::is_arithmetic<O>::value>::type>
    rgba<T> &operator=(const O &other) {
        r = g = b = a = static_cast<O>(other);
        return *this;
    }
};

typedef rgba<uint8_t> rgba8u;
typedef rgba<uint16_t> rgba16u;
typedef rgba<float> rgba32f;

template<typename T>
inline rgba<T> operator/(const rgba<T> &px, const float &div) {
    return {
            static_cast<T>(px.r / div),
            static_cast<T>(px.g / div),
            static_cast<T>(px.b / div),
            static_cast<T>(px.a / div),
    };
}

template<typename T>
inline rgba<T> operator*(const rgba<T> &px, const float &mul) {
    return {
            static_cast<T>(px.r * mul),
            static_cast<T>(px.g * mul),
            static_cast<T>(px.b * mul),
            static_cast<T>(px.a * mul),
    };
}

template<typename T, typename O>
inline rgba<T> operator+(const rgba<T> &px1, const rgba<O> &px2) {
    return {
            static_cast<T>(px1.r + px2.r),
            static_cast<T>(px1.g + px2.g),
            static_cast<T>(px1.b + px2.b),
            static_cast<T>(px1.a + px2.a),
    };
}

template<typename T, typename O>
inline rgba<T> &operator+=(rgba<T> &px1, const rgba<O> &px2) {
    px1.r += px2.r;
    px1.g += px2.g;
    px1.b += px2.b;
    px1.a += px2.a;
    return px1;
}

template<typename T,
        typename O,
        typename = typename std::enable_if<std::is_arithmetic<O>::value>::type>
inline rgba<T> &operator/=(rgba<T> &px1, const O &num) {
    px1 = px1 / static_cast<T>(num);
    return px1;
}


template<typename T>
class Image {
public:
    Image(T *pixels, const int width, const int height)
            : width(width),
              height(height) {
        this->pixels = pixels;
    }

    rgba8u &operator()(const int &x, const int &y) const {
        return pixels[y * width + x];
    }

    T *getPixels() {
        return pixels;
    }

    const int width;
    const int height;

private:
    T *pixels;
};


struct JNIBitmap {
    rgba8u *pixels;
    uint width;
    uint height;
    jobject *bitmap{nullptr};
    JNIEnv *env{nullptr};

    JNIBitmap(jobject *bitmap, JNIEnv *env) {
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

    ~JNIBitmap() {
        AndroidBitmap_unlockPixels(this->env, *this->bitmap);
    }

    rgba8u &operator()(const int &x, const int &y) {
        return pixels[y * width + x];
    }

    /*
     * Returns a view to the image data.
     * The buffer ownership is held by the JNIBitmap instance!
     */
    Image<rgba8u> getImageView() {
        auto img = Image<rgba8u>(pixels, width, height);
        return img;
    }
};

#endif //CAMERAXAPP_IMAGETYPES_H
