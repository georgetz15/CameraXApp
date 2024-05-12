//
// Created by George Tzoupis on 12/5/2024.
//

#ifndef CAMERAXAPP_RESAMPLING_H
#define CAMERAXAPP_RESAMPLING_H

#include <math.h>
#include "ImageTypes.h"

template<typename T>
void downsampleBilinear(const Image<rgba<T>> in, Image<rgba<T>> out) {
    float xRatio, yRatio;
    if (out.width > 1) {
        xRatio = ((float) in.width - 1.0f) / ((float) out.width - 1.0f);
    } else {
        xRatio = 0;
    }

    if (out.height > 1) {
        yRatio = ((float) in.height - 1.0f) / ((float) out.height - 1.0f);
    } else {
        yRatio = 0;
    }

#pragma omp parallel for collapse(2)
    for (int y = 0; y < out.height; y++) {
        for (int x = 0; x < out.width; x++) {
            int xLow = floor(xRatio * (float) x);
            int yLow = floor(yRatio * (float) y);
            int xHigh = ceil(xRatio * (float) x);
            int yHigh = ceil(yRatio * (float) y);

            float wx = (xRatio * (float) x) - xLow;
            float wy = (yRatio * (float) y) - yLow;

            out(x, y) = in(xLow, yLow) * (1.0f - wx) * (1.0f - wy);
            out(x, y) += in(xHigh, yLow) * wx * (1.0f - wy);
            out(x, y) += in(xLow, yHigh) * wy * (1.0f - wx);
            out(x, y) += in(xHigh, yHigh) * wx * wy;
        }
    }
}

template<typename T>
void downsampleArea(const Image<rgba<T>> in, Image<rgba<T>> out) {
    const float xStep = (float) in.width / out.width;
    const float yStep = (float) in.height / out.height;
    const int kernelWidth = ceil(xStep);
    const int kernelHeight = ceil(yStep);
    const float area = (kernelWidth * kernelHeight);

#pragma omp parallel for collapse(2)
    for (int y = 0; y < out.height; y++) {
        for (int x = 0; x < out.width; x++) {
            rgba16u sum = {0, 0, 0, 0};
            for (int j = 0; j < kernelHeight; ++j) {
                for (int i = 0; i < kernelWidth; ++i) {
                    const int yi = (y * yStep + j);
                    const int xi = (x * xStep + i);
                    if (xi < 0 || in.width <= xi || yi < 0 || in.height <= yi) {
                        continue;
                    }
                    auto &px = in(xi, yi);
                    sum += px;
                }
            }

            out(x, y) = sum / area;
        }
    }
}


#endif //CAMERAXAPP_RESAMPLING_H
