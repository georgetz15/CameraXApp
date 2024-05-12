//
// Created by George Tzoupis on 12/5/2024.
//

#ifndef CAMERAXAPP_FILTERS_H
#define CAMERAXAPP_FILTERS_H

#define _USE_MATH_DEFINES

#include <math.h>
#include <algorithm>
#include "ImageTypes.h"

template<typename T, typename O, typename K>
void sepfilt(const Image<rgba<T>> in,
             Image<rgba<T>> out,
             const std::vector<O> &filterHorizontal,
             const std::vector<O> &filterVertical,
             float norm) {
    Image<rgba<K>> temp(in.width, in.height);

#pragma omp parallel for collapse(2)
    for (int y = 0; y < in.height; ++y) {
        for (int x = 0; x < in.width; ++x) {
            temp(x, y) = 0;
            for (int k = 0; k < filterHorizontal.size(); ++k) {
                const int xk = x + k - filterHorizontal.size() / 2;
                if (xk < 0 || in.width <= xk) continue;
                temp(x, y).r += in(xk, y).r * filterHorizontal[k];
                temp(x, y).g += in(xk, y).g * filterHorizontal[k];
                temp(x, y).b += in(xk, y).b * filterHorizontal[k];
            }
        }
    }

#pragma omp parallel for collapse(2)
    for (int y = 0; y < in.height; ++y) {
        for (int x = 0; x < in.width; ++x) {
            rgba16u pxOut;
            pxOut = 0;
            for (int k = 0; k < filterVertical.size(); ++k) {
                const int yk = y + k - filterVertical.size() / 2;
                if (yk < 0 || in.height <= yk) continue;
                pxOut.r += temp(x, yk).r * filterVertical[k];
                pxOut.g += temp(x, yk).g * filterVertical[k];
                pxOut.b += temp(x, yk).b * filterVertical[k];
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
    sepfilt<T, T, uint16_t>(inImg, outImg, filter, filter, 1.0f / (kernelSize * kernelSize));
}

template<typename T>
void gaussianBlur(const Image<rgba<T>> inImg,
                  Image<rgba<T>> outImg,
                  float sigma) {
//    std::fill(filter.begin(), filter.end(), 1);
    // WIP: see
//    https://www.rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/
//    https://computergraphics.stackexchange.com/questions/39/how-is-gaussian-blur-implemented

    const int ks = ceil(sigma) * 2 + 1;
    std::vector<float> filter(ks);

    float norm = 0.0f;
    for (int i = 0; i < ks; ++i) {
        auto xi = (i - ks / 2);
        filter[i] = std::expf(-(xi * xi) / (2.0f * sigma * sigma)) /
                    (2.0f * M_PI * sigma * sigma);
        norm += filter[i];
    }
    for (int i = 0; i < ks; ++i) {
        filter[i] /= norm;
    }
    sepfilt<T, float, float>(inImg, outImg, filter, filter, 1.0f);
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
