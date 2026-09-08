#pragma once

#include <opencv2/opencv.hpp>

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>

#include <string>

class NetworkSender
{
public:

    NetworkSender(
        const std::string& ip,
        int port,
        int width,
        int height,
        int fps
    );

    ~NetworkSender();

    bool sendFrame(
        const cv::Mat& frame
    );

private:

    GstElement* pipeline;

    GstElement* appsrc;

    uint64_t frameCount;

    int width;
    int height;
    int fps;
};