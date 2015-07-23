#pragma once

#ifndef LIBREALSENSE_UVC_CAMERA_H
#define LIBREALSENSE_UVC_CAMERA_H

#include "libuvc/libuvc.h"

#include <stdint.h>
#include <string>
#include <vector>
#include <iostream>
#include <exception>
#include <mutex>
#include <map>

#include "Common.h"
#include "R200_XU.h"
#include "R200_SPI.h"
#include "R200_CameraHeader.h"

#define STREAM_DEPTH 1
#define STREAM_LR 2
#define STREAM_RGB 4

struct StreamInfo
{
    int width;
    int height;
    int fps;
    uvc_frame_format format;
};

struct DeviceInfo
{
    std::string serial;
    uint16_t vid;
    uint16_t pid;
};

class UVCCamera;
struct DeviceHandle
{
    UVCCamera * camera = nullptr;
    uvc_device_handle_t * uvcHandle = nullptr;
    uvc_frame_format fmt = UVC_FRAME_FORMAT_UNKNOWN;
    uvc_stream_ctrl_t ctrl = {0};
};

////////////////
// UVC Camera //
////////////////

class UVCCamera
{
    NO_MOVE(UVCCamera);
    
    uvc_device_t * hardware = nullptr;

    bool isInitialized = false;
    bool isStreaming = false; // reduntant for StreamingModeBitfield
    
    int streamingModeBitfield = 0;
    
    int cameraNum;
    
    bool firstTime = true;
    
    StreamInfo sInfo = {};
    DeviceInfo dInfo = {};
    
    std::mutex frameMutex;
    std::unique_ptr<TripleBufferedFrame> depthFrame;
    std::unique_ptr<TripleBufferedFrame> colorFrame;
    
    std::unique_ptr<SPI_Interface> spiInterface;
    
    std::map<int, DeviceHandle *> deviceHandles;
    
    static void cb(uvc_frame_t * frame, void * ptr)
    {
        DeviceHandle * handle = static_cast<DeviceHandle*>(ptr);
        handle->camera->frameCallback(frame, handle);
    }
    
    void frameCallback(uvc_frame_t * frame, DeviceHandle * handle);
    
public:
    
    UVCCamera(uvc_device_t * device, int num);
    
    ~UVCCamera();
    
    bool ConfigureStreams();
    
    void StartStream(int streamNum, const StreamInfo & info);
    
    void StopStream(int streamNum);
    
    // @tofix get rid of this function
    void DumpInfo();
    
    uint16_t * GetDepthImage();
    
    uint8_t * GetColorImage();
    
    int GetCameraIndex() { return cameraNum; }
    
    bool IsStreaming() { return isStreaming; }
    
    void EnableDepthStream() { streamingModeBitfield |= STREAM_DEPTH; }
    void EnableColorStream() { streamingModeBitfield |= STREAM_RGB; }
    
    RectifiedIntrinsics GetCalibrationDataRectZ()
    {
        return spiInterface->GetZIntrinsics();
    }
    
};

#endif