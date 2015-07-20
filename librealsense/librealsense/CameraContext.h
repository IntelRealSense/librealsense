#pragma once

#ifndef CAMERA_CONTEXT_H
#define CAMERA_CONTEXT_H

#include "libuvc/libuvc.h"

#include <stdint.h>
#include <string>
#include <vector>
#include <iostream>
#include <exception>
#include <mutex>

#include "Common.h"
#include "R200_XU.h"
#include "R200_SPI.h"
#include "R200_CameraHeader.h"

enum StreamType
{
    DEPTH,
    COLOR,
    IR
};

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
struct StreamHandle
{
    UVCCamera * camera;
    StreamType t;
};

////////////////
// UVC Camera //
////////////////

class UVCCamera
{
    NO_MOVE(UVCCamera);
    
    uvc_device_t * device = nullptr;
    uvc_device_handle_t * deviceHandle = nullptr;
    
    uvc_stream_ctrl_t ctrl = {0};
    
    bool isInitialized = false;
    bool isStreaming = false;
    
    uint64_t frameCount = 0;
    int cameraNum;
    
    StreamInfo sInfo = {};
    DeviceInfo dInfo = {};
    
    std::mutex frameMutex;
    std::unique_ptr<TripleBufferedFrame> depthFrame;
    std::unique_ptr<TripleBufferedFrame> colorFrame;
    
    std::unique_ptr<SPI_Interface> spiInterface;
    
    static void cb(uvc_frame_t *frame, void *ptr)
    {
        UVCCamera * camera = static_cast<UVCCamera*>(ptr);
        camera->frameCallback(frame);
    }
    
    void frameCallback(uvc_frame_t * frame);
    
public:
    
    // ivcam = 0x0a66
    // UVC_FRAME_FORMAT_INVI, 640, 480, 60
    
    UVCCamera(uvc_device_t * device, int num);
    
    ~UVCCamera();
    
    void Open(int streamNum);
    
    void Start(const StreamInfo & info);
    
    void Stop();
    
    void DumpInfo();
    
    uint16_t * GetDepthImage();
    
    uint8_t * GetColorImage();
    
    int GetCameraIndex() { return cameraNum; }
    
    DSCalibIntrinsicsRectified GetCalibrationDataRectZ()
    {
        return spiInterface->GetZIntrinsics();
    }

};

////////////////////
// Camera Context //
////////////////////

class CameraContext
{
    NO_MOVE(CameraContext);
    uvc_context_t * privateContext;
    void QueryDeviceList();
public:
    CameraContext();
    ~CameraContext();
    std::vector<std::shared_ptr<UVCCamera>> cameras;
};

#endif
