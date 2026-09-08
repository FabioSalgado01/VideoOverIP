#include "NetworkSender.h"

#include <opencv2/opencv.hpp>
#include <cstdlib>
#include <iostream>
#include <chrono>
#include <thread>
#include "filter.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

using namespace cv;
using namespace std;

#include <atomic>

std::atomic<bool> enableEdge(false);
std::atomic<bool> enableEmboss(false);

std::atomic<int> sharpenStrength(0);
std::atomic<int> blurStrength(0);

std::atomic<float> contrastValue(1.0);
std::atomic<int> brightnessValue(0);

std::atomic<int> videoPort(5004);

VideoCapture openStream(int port)
{
    string pipeline =
    "udpsrc port=" + to_string(port) +
    " ! application/x-rtp,media=video,encoding-name=H264,payload=96,clock-rate=90000 "
    "! rtpjitterbuffer latency=0 "
    "! rtph264depay "
    "! h264parse "
    "! avdec_h264 "
    "! videoconvert "
    "! appsink sync=false drop=true max-buffers=1";

    return VideoCapture(pipeline, CAP_GSTREAMER);
}

void controlServer()
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(6000);

    bind(server_fd, (sockaddr*)&address, sizeof(address));

    listen(server_fd, 3);

    cout << "Control server listening...\n";


    while(true)
    {
        int client = accept(server_fd, nullptr, nullptr);

        char buffer[128] = {};
        read(client, buffer, sizeof(buffer));


        string cmd(buffer);

        cmd.erase(
        cmd.find_last_not_of(" \n\r\0") + 1
        );

cout << "Received: [" << cmd << "]" << endl;

    if(cmd.rfind("sharpen_", 0) == 0)
    {
        sharpenStrength = stoi(cmd.substr(8));

        blurStrength = 0;

        enableEdge = false;
        enableEmboss = false;
    }

    else if(cmd.rfind("blur_", 0) == 0)
    {
        blurStrength = stoi(cmd.substr(5));

        sharpenStrength = 0;

        enableEdge = false;
        enableEmboss = false;

    }

        else if(cmd == "edge_on")
        {
            blurStrength = 0;
            sharpenStrength = 0;
            enableEdge = true;
            enableEmboss = false;
        }

        else if(cmd == "emboss_on")
        {
            blurStrength = 0;
            sharpenStrength = 0;
            enableEdge = false;
            enableEmboss = true;
        }

        else if(cmd == "off")
        {
        sharpenStrength = 0;
        blurStrength = 0;

        enableEdge = false;
        enableEmboss = false;
        }

        else if(cmd.rfind("brightness_", 0) == 0)
        {

        int value = stoi(cmd.substr(11));
            brightnessValue = value;

            cout << "Brightness set to: "
                << value << endl;
        }

        else if(cmd.rfind("contrast_", 0) == 0)
        {
            float value = stof(cmd.substr(9));
            contrastValue = value;

            cout << "Contrast set to: "
                << value << endl;
                
        }

        else if(cmd == "source_camera")
        {
            videoPort = 5004;
            cout << "Switched to camera\n";
        }

        else if(cmd == "source_byod")
        {
            videoPort = 5005;
            cout << "Switched to BYOD\n";
        }

        close(client);
    }
}



int main()
{
    // Video settings
    int width = 1920;
    int height = 1080;
    int fps = 30;
    auto frameInterval = std::chrono::milliseconds(1000 / fps);


VideoCapture cap = openStream(5004);

    if(!cap.isOpened())
    {
        cout << "Failed to open camera\n";
        return -1;
    }


    // Set camera resolution
    cap.set(
        CAP_PROP_FRAME_WIDTH,
        width
    );

    cap.set(
        CAP_PROP_FRAME_HEIGHT,
        height
    );

    cap.set(
        CAP_PROP_FPS,
        fps
    );


    // Change this to the Pi 4 decoder IP address
    string decoderIP = "192.168.45.199";


    NetworkSender sender(
        decoderIP,
        5000,
        width,
        height,
        fps
    );


    Mat frame;
    Mat filtered;

    cout << "Starting video stream...\n";

    int frameNum = 0;

    system("cd web && python3 app.py &");

    thread control(controlServer);
    control.detach();

    int currentPort = videoPort.load();

    while(true)
    {
   auto start = std::chrono::steady_clock::now();

   if(videoPort.load() != currentPort)
    {
        currentPort = videoPort.load();

        cap.release();

        cout << "Changing stream to port "
             << currentPort << endl;

        cap = openStream(currentPort);
    }

    cap >> frame;

    if(frame.empty()) { cout << "Empty frame at #" << frameNum << "\n"; break; }

    Mat adjusted;

    resize(frame, frame, Size(width, height));

    brightnessContrast(
    frame,
    adjusted,
    contrastValue.load(),
    brightnessValue.load()
    );


    auto filterStart = chrono::high_resolution_clock::now();

    filtered = adjusted.clone();


    // BLUR AND SHARPEN CAN BE APPLIED SEVERAL TIMES TO INCREASE THEIR EFFECT. 
    
    if(sharpenStrength.load() > 0)
    {
             Mat temp = adjusted.clone();

    for(int i = 0; i < sharpenStrength.load(); i++)
    {
        firFilter(temp, filtered, sharpenKernel, 1);
        temp = filtered.clone();
    }

    }
    else if(blurStrength.load() > 0)
    {
     Mat temp = adjusted.clone();

    for(int i = 0; i < blurStrength.load(); i++)
    {
        firFilter(temp, filtered, blurKernel, 16);
        temp = filtered.clone();
    }


    }
    else if(enableEdge)
    {
        firFilter(adjusted, filtered, edgeKernel, 1);
    }
    else if(enableEmboss)
    {
        firFilter(adjusted, filtered, embossKernel, 1);
    }
    else
    {
        filtered = adjusted.clone();
    }

    auto filterEnd = chrono::high_resolution_clock::now();

    cout << chrono::duration<double,milli>(filterEnd-filterStart).count()
     << " ms\n";

    if(!sender.sendFrame(filtered))
        cout << "Failed sending frame #" << frameNum << "\n";

    frameNum++;

    auto elapsed = std::chrono::steady_clock::now() - start;
    if(elapsed < frameInterval)
        std::this_thread::sleep_for(frameInterval - elapsed);
    }


    return 0;
}