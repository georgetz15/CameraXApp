//
// Created by George Tzoupis on 12/5/2024.
//

#ifndef CAMERAXAPP_FILTERS_H
#define CAMERAXAPP_FILTERS_H


#include <algorithm>
#include "ImageTypes.h"

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

template<typename T>
void boxBlur(const Image<rgba<T>> inImg, Image<rgba<T>> outImg, const int kernelSize) {
    auto width = inImg.width;
    auto height = inImg.height;

// scan through every single pixel
#pragma omp parallel for collapse(2)
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            rgba16u out;
            out = 0;
            for (int j = -kernelSize / 2; j < -kernelSize / 2 + kernelSize; ++j) {
                for (int i = -kernelSize / 2; i < -kernelSize / 2 + kernelSize; ++i) {
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
            outImg(x, y) = out / (kernelSize * kernelSize);
            outImg(x, y).a = inImg(x, y).a;
        }
    }
}

template<typename T>
void sepia(Image<rgba<T>> img) {
#pragma omp parallel for collapse(2)
    for (int y = 0; y < img.height; ++y) {
        for (int x = 0; x < img.width; ++x) {
            auto& px = img(x, y);
            rgba16u val;
            val.r = px.r * .393f + px.g * .769f + px.b * .189f;
            val.r = std::clamp(val.r, (uint16_t) 0, (uint16_t) 255);

            val.g = px.r * .349f + px.g * .686f + px.b * .168f;
            val.g = std::clamp(val.g, (uint16_t) 0, (uint16_t) 255);

            val.b = px.r * .272f + px.g * .534f + px.b * .131f;
            val.b = std::clamp(val.b, (uint16_t) 0, (uint16_t) 255);

            val.a = px.a;

            img(x, y) = val;
        }
    }
}

#endif //CAMERAXAPP_FILTERS_H
