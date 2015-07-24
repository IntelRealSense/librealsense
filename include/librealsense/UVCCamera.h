#pragma once

#ifndef LIBREALSENSE_UVC_CAMERA_H
#define LIBREALSENSE_UVC_CAMERA_H

#include "libuvc/libuvc.h"

#include <librealsense/Common.h>

#define STREAM_DEPTH 1
#define STREAM_LR 2
#define STREAM_RGB 4

struct StreamConfiguration
{
    int width;
    int height;
    int fps;
    uvc_frame_format format;
};

struct USBDeviceInfo
{
    std::string serial;
    uint16_t vid;
    uint16_t pid;
};

////////////////
// UVC Camera //
////////////////

class UVCCamera
{
    NO_MOVE(UVCCamera);
 
protected:
    
    struct StreamInterface
    {
        UVCCamera * camera = nullptr;
        uvc_device_handle_t * uvcHandle = nullptr;
        uvc_frame_format fmt = UVC_FRAME_FORMAT_UNKNOWN;
        uvc_stream_ctrl_t ctrl = {0};
    };
    
    int cameraIdx;
    
    uvc_device_t * hardware = nullptr;
    USBDeviceInfo usbInfo = {};
    
    bool hardwareInitialized = false;
    
    uint32_t streamingModeBitfield = 0;
    uint64_t frameCount = 0;
    
    std::mutex frameMutex;
    std::unique_ptr<TripleBufferedFrame> depthFrame;
    std::unique_ptr<TripleBufferedFrame> colorFrame;
    
    std::map<int, StreamInterface *> streamInterfaces;
    
    static void cb(uvc_frame_t * frame, void * ptr)
    {
        StreamInterface * stream = static_cast<StreamInterface*>(ptr);
        stream->camera->frameCallback(frame, stream);
    }
    
    void frameCallback(uvc_frame_t * frame, StreamInterface * stream);
    
public:
    
    UVCCamera(uvc_device_t * device, int num);
    
    virtual ~UVCCamera();
    
    void EnableStream(int whichStream) { streamingModeBitfield |= whichStream; }
    
    virtual bool ConfigureStreams() = 0;
    
    virtual void StartStream(int streamIdentifier, const StreamConfiguration & config) = 0;
    
    virtual void StopStream(int streamIdentifier) = 0;

    uint16_t * GetDepthImage();
    
    uint8_t * GetColorImage();
    
    bool IsStreaming();
    
    int GetCameraIndex() { return cameraIdx; }

    uint64_t GetFrameCount() { return frameCount; }
};

#endif