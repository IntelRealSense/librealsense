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

struct StreamInfo
{
    int width;
    int height;
    int fps;
    uvc_frame_format format;
};

struct DeviceInfo
{
    std::string usbSerial;
    std::string cameraSerial;
    uint16_t vid;
    uint16_t pid;
};

//////////////////
// Math Helpers //
//////////////////

// Compute field of view angles in degrees from rectified intrinsics
// Get intrinsics via DSAPI getCalibIntrinsicsZ, getCalibIntrinsicsRectLeftRight or getCalibIntrinsicsRectOther
inline void GetFieldOfView(const DSCalibIntrinsicsRectified & intrinsics, float & horizontalFOV, float & verticalFOV)
{
    horizontalFOV = atan2(intrinsics.rpx + 0.5f, intrinsics.rfx) + atan2(intrinsics.rw - intrinsics.rpx - 0.5f, intrinsics.rfx);
    verticalFOV = atan2(intrinsics.rpy + 0.5f, intrinsics.rfy) + atan2(intrinsics.rh - intrinsics.rpy - 0.5f, intrinsics.rfy);
    
    // Convert to degrees
    const float pi = 3.14159265358979323846f;
    horizontalFOV = horizontalFOV * 180.0f / pi;
    verticalFOV = verticalFOV * 180.0f / pi;
}

// From z image to z camera (right-handed coordinate system).
// zImage is assumed to contain [z row, z column, z depth].
// Get zIntrinsics via DSAPI getCalibIntrinsicsZ.
inline void TransformFromZImageToZCamera(const DSCalibIntrinsicsRectified & zIntrinsics, const float zImage[3], float zCamera[3])
{
    zCamera[0] = zImage[2] * (zImage[0] - zIntrinsics.rpx) / zIntrinsics.rfx;
    zCamera[1] = zImage[2] * (zImage[1] - zIntrinsics.rpy) / zIntrinsics.rfy;
    zCamera[2] = zImage[2];
}

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
    
    std::vector<uint8_t> frameData;
    
    std::mutex cameraMutex;
    
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
    
    std::vector<uint8_t> getDepthImage() { return frameData; }
    
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
