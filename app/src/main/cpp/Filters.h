//
// Created by George Tzoupis on 12/5/2024.
//

#ifndef CAMERAXAPP_FILTERS_H
#define CAMERAXAPP_FILTERS_H


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

#endif //CAMERAXAPP_FILTERS_H
