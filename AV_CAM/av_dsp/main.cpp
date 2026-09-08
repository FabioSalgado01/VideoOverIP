#include "NetworkSender.h"

#include <opencv2/opencv.hpp>
#include <iostream>
#include <chrono>


using namespace cv;
using namespace std;


int main()
{
    int width = 1920;
    int height = 1080;
    int fps = 30;


    VideoCapture cap(
        "libcamerasrc ! "
        "video/x-raw,width=1920,height=1080,framerate=30/1 ! "
        "videoconvert ! "
        "video/x-raw,format=BGR ! "
        "appsink drop=true max-buffers=1",
        CAP_GSTREAMER
    );


    if(!cap.isOpened())
    {
        cout << "Failed to open camera\n";
        return -1;
    }


    cout << "Camera opened\n";


    NetworkSender sender(
        "192.168.45.178",   // Pi 5 IP
        5004,
        width,
        height,
        fps
    );


    Mat frame;


    while(true)
    {
        cap >> frame;


        if(frame.empty())
        {
            cout << "Empty frame\n";
            continue;
        }


        if(!sender.sendFrame(frame))
        {
            cout << "Send failed\n";
        }
    }


    return 0;
}