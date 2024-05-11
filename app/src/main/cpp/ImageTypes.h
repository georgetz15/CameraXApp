//
// Created by George Tzoupis on 11/5/2024.
//

#ifndef CAMERAXAPP_IMAGETYPES_H
#define CAMERAXAPP_IMAGETYPES_H

#include <type_traits>

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

template<typename T>
inline rgba<T> operator+(const rgba<T> &px1, const rgba<T> &px2) {
    return {
            static_cast<T>(px1.r + px2.r),
            static_cast<T>(px1.g + px2.g),
            static_cast<T>(px1.b + px2.b),
            static_cast<T>(px1.a + px2.a),
    };
}

template<typename T, typename O>
inline rgba<T> &operator+=(rgba<T> &px1, rgba<O> &px2) {
    px1 = px1 + px2;
    return px1;
}

template<typename T,
        typename O,
        typename = typename std::enable_if<std::is_arithmetic<O>::value>::type>
inline rgba<T> &operator/=(rgba<T> &px1, const O &num) {
    px1 = px1 / static_cast<T>(num);
    return px1;
}


//template<typename T>
//class Image {
//    public:
//    private:
//
//};

#endif //CAMERAXAPP_IMAGETYPES_H
