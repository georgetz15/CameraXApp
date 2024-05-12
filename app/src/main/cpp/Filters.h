//
// Created by George Tzoupis on 12/5/2024.
//

#ifndef CAMERAXAPP_FILTERS_H
#define CAMERAXAPP_FILTERS_H


#include <algorithm>
#include "ImageTypes.h"

template<typename T>
void sepfilt(const Image<rgba<T>> in, Image<rgba<T>> out, const std::vector<T>& filter, float norm) {
    Image<rgba16u> temp(in.width, in.height);

    const int kernelSize = filter.size();

#pragma omp parallel for collapse(2)
    for (int y = 0; y < in.height; ++y) {
        for (int x = 0; x < in.width; ++x) {
            temp(x, y) = 0;
            for (int k = 0; k < kernelSize; ++k) {
                const int xk = x + k - kernelSize / 2;
                if (xk < 0 || in.width <= xk) continue;
                temp(x, y).r += in(xk, y).r * filter[k];
                temp(x, y).g += in(xk, y).g * filter[k];
                temp(x, y).b += in(xk, y).b * filter[k];
            }
        }
    }

#pragma omp parallel for collapse(2)
    for (int y = 0; y < in.height; ++y) {
        for (int x = 0; x < in.width; ++x) {
            rgba16u pxOut;
            pxOut = 0;
            for (int k = 0; k < kernelSize; ++k) {
                const int yk = y + k - kernelSize / 2;
                if (yk < 0 || in.height <= yk) continue;
                pxOut.r += temp(x, yk).r * filter[k];
                pxOut.g += temp(x, yk).g * filter[k];
                pxOut.b += temp(x, yk).b * filter[k];
            }
            out(x, y) = norm * pxOut;
            out(x, y).a = in(x, y).a;
        }
    }
}

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
    std::vector<T> filter(kernelSize);
    std::fill(filter.begin(), filter.end(), 1);
    sepfilt(inImg, outImg, filter, 1.0f / (kernelSize * kernelSize));
}

template<typename T>
void sepia(Image<rgba<T>> img) {
#pragma omp parallel for collapse(2)
    for (int y = 0; y < img.height; ++y) {
        for (int x = 0; x < img.width; ++x) {
            auto &px = img(x, y);
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
