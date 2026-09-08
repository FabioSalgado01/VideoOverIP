#include "NetworkSender.h"

#include <iostream>
#include <sstream>
#include <cstring>

using namespace std;


NetworkSender::NetworkSender(
    const string& ip,
    int port,
    int width,
    int height,
    int fps
)
{
    this->width = width;
    this->height = height;
    this->fps = fps;
    frameCount = 0;

    gst_init(nullptr, nullptr);


    stringstream pipelineStr;
    pipelineStr
    << "appsrc name=mysource is-live=true do-timestamp=true format=time "
    << "! videoconvert "
    << "! video/x-raw,format=I420 "
    << "! queue max-size-buffers=2 leaky=downstream "
    << "! x264enc tune=zerolatency speed-preset=ultrafast bitrate=16000 "
    << "key-int-max=30 threads=4 "
    << "! h264parse config-interval=1 "
    << "! rtph264pay config-interval=1 pt=96 mtu=1200 "
    << "! udpsink host=" << ip << " port=" << port << " sync=false";


    GError* error = nullptr;


    pipeline = gst_parse_launch(
        pipelineStr.str().c_str(),
        &error
    );


    if(error)
    {
        cout << "Pipeline Error: "
             << error->message
             << endl;

        g_error_free(error);

        pipeline = nullptr;

        return;
    }


    appsrc = gst_bin_get_by_name(
        GST_BIN(pipeline),
        "mysource"
    );


    if(!appsrc)
    {
        cout << "Could not find appsrc."
             << endl;

        return;
    }


    GstCaps* caps = gst_caps_new_simple(
        "video/x-raw",
        "format",
        G_TYPE_STRING,
        "BGR",
        "width",
        G_TYPE_INT,
        width,
        "height",
        G_TYPE_INT,
        height,
        "framerate",
        GST_TYPE_FRACTION,
        fps,
        1,
        NULL
    );


    gst_app_src_set_caps(
        GST_APP_SRC(appsrc),
        caps
    );


    gst_caps_unref(caps);


    gst_app_src_set_stream_type(
        GST_APP_SRC(appsrc),
        GST_APP_STREAM_TYPE_STREAM
    );

    gst_app_src_set_latency(
    GST_APP_SRC(appsrc),
    0,
    0
    );

    g_object_set(
    G_OBJECT(appsrc),
    "format",
    GST_FORMAT_TIME,
    NULL
    );


    gst_element_set_state(
        pipeline,
        GST_STATE_PLAYING
    );


    cout << "Network sender started."
         << endl;
}



NetworkSender::~NetworkSender()
{
    if(pipeline)
    {
        gst_element_set_state(
            pipeline,
            GST_STATE_NULL
        );

        gst_object_unref(pipeline);
    }
}



bool NetworkSender::sendFrame(const cv::Mat& frame)
{
    if(!pipeline || !appsrc)
        return false;


    if(frame.empty())
        return false;


    if(frame.cols != width ||
       frame.rows != height)
    {
        cerr << "Frame size mismatch\n";
        return false;
    }


    cv::Mat input;

    if(!frame.isContinuous())
        input = frame.clone();
    else
        input = frame;


    int bytesPerRow = width * 3;   // BGR

    int dataSize = bytesPerRow * height;


    GstBuffer* buffer =
        gst_buffer_new_allocate(
            NULL,
            dataSize,
            NULL
        );


    if(!buffer)
        return false;


    GstMapInfo map;


    if(!gst_buffer_map(
        buffer,
        &map,
        GST_MAP_WRITE))
    {
        gst_buffer_unref(buffer);
        return false;
    }


    for(int y = 0; y < height; y++)
    {
        memcpy(
            map.data + y * bytesPerRow,
            input.ptr(y),
            bytesPerRow
        );
    }


    gst_buffer_unmap(
        buffer,
        &map
    );


    GstFlowReturn ret =
        gst_app_src_push_buffer(
            GST_APP_SRC(appsrc),
            buffer
        );


    return ret == GST_FLOW_OK;
}