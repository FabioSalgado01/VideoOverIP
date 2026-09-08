#ifndef FILTER_H
#define FILTER_H

#include <opencv2/opencv.hpp>
#pragma once

using namespace cv;


// Available kernels
extern int8_t sharpenKernel[3][3];
extern int8_t blurKernel[3][3];
extern int8_t edgeKernel[3][3];
extern int8_t embossKernel[3][3];


// Apply a 3x3 FIR filter
void firFilter(
    const Mat& input,
    Mat& output,
    int8_t kernel[3][3],
    int divisor
);

void brightnessContrast(
    const cv::Mat& input,
    cv::Mat& output,
    float contrast,
    int brightness
);


#endif