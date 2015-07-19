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
    uint32_t vid;
    uint32_t pid;
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
    
    std::vector<uint8_t> frameData;
    
    std::mutex cameraMutex;
    
    static void cb(uvc_frame_t *frame, void *ptr)
    {
        UVCCamera * camera = static_cast<UVCCamera*>(ptr);
        camera->frameCallback(frame);
    }
    
    void frameCallback(uvc_frame_t * frame)
    {
        //@tofix check invalid frame width, height
        
        frameCount++;
        
        if (sInfo.format == UVC_FRAME_FORMAT_Z16)
        {
            // memcpy then .swapBack
            memcpy(frameData.data(), frame->data, (frame->width * frame->height - 1) * 2);
            
            // { std::lock_guard<std::mutex> lock(cameraMutex); swapBack() }
        }
        else
        {
            //@tofix other pixel formats
        }
        
        /*
         void rgbCallback(uvc_frame_t *frame, void *ptr)
         {
         static int frameCount;
         
         std::cout << "rgb_frame_count: " << frameCount << std::endl;
         
         std::vector<uint8_t> colorImg(frame->width * frame->height * 3); // YUY2 = 16 in -> 24 out
         convert_yuyv_rgb((const uint8_t *)frame->data, frame->width, frame->height, colorImg.data());
         memcpy(g_rgbImage, colorImg.data(), 640 * 480 * 3);
         
         frameCount++;
         }
         */

        
    }
    
public:
    
    // ivcam = 0x0a66
    // UVC_FRAME_FORMAT_INVI, 640, 480, 60
    
    UVCCamera(uvc_device_t * device, int num) : device(device), cameraNum(num)
    {
        printf("%s", __PRETTY_FUNCTION__);
    }
    
    ~UVCCamera()
    {
        printf("%s", __PRETTY_FUNCTION__);
        if (deviceHandle != nullptr)
        {
            uvc_close(deviceHandle);
        }
        uvc_unref_device(device);
    }
    
    void Open(int streamNum)
    {
        printf("%s", __PRETTY_FUNCTION__);
        
        uvc_ref_device(device);
        uvc_error_t status = uvc_open2(device, &deviceHandle, streamNum);
        
        if (status < 0)
        {
            uvc_perror(status, "uvc_open2");
            return;
            //throw std::runtime_error("Open camera_handle Failed");
        }
        
        // Get Device Info!
        uvc_device_descriptor_t * desc;
        if(uvc_get_device_descriptor(device, &desc) == UVC_SUCCESS)
        {
            if (desc->serialNumber)
                dInfo.usbSerial = desc->serialNumber;
            
            if (desc->idVendor)
                dInfo.vid = desc->idVendor;
            
            if (desc->idProduct)
                dInfo.pid = desc->idProduct;
            
            uvc_free_device_descriptor(desc);
        }
        
        std::cout << "Serial Number: " << dInfo.usbSerial << std::endl;
        std::cout << "USB VID: " << dInfo.vid << std::endl;
        std::cout << "USB PID: " << dInfo.pid << std::endl;
        
        isInitialized = true;
    }
    
    void Start(const StreamInfo & info)
    {
        if (isStreaming) throw std::runtime_error("Camera is already streaming");
        
        printf("%s", __PRETTY_FUNCTION__);
        
        sInfo = info;
        
        uvc_error_t status = uvc_get_stream_ctrl_format_size(deviceHandle, &ctrl, info.format, info.width, info.height, info.fps);
        
        if (status < 0)
        {
            uvc_perror(status, "uvc_get_stream_ctrl_format_size");
            //throw std::runtime_error("Open camera_handle Failed");
        }
        
        auto streamIntent = XUControl::SetStreamIntent(deviceHandle, 5); //@tofix - proper streaming mode
        if (!streamIntent)
        {
            throw std::runtime_error("Could not set stream intent");
        }
        
        uvc_error_t startStreamResult = uvc_start_streaming(deviceHandle, &ctrl, &UVCCamera::cb, this, 0);
        
        if (startStreamResult < 0)
        {
            uvc_perror(startStreamResult, "start_stream");
            throw std::runtime_error("Could not start stream");
        }
        
        // @tofix: proper treatment of strides and frame data types
        frameData.resize(info.width * info.height * 2);
        
        // Debugging
        DumpInfo();
        
        isStreaming = true;
    }
    
    void Stop()
    {
        printf("%s", __PRETTY_FUNCTION__);
        if (!isStreaming) throw std::runtime_error("Camera is not already streaming...");
        uvc_stop_streaming(deviceHandle);
        isStreaming = false;
    }
    
    void DumpInfo()
    {
        printf("%s", __PRETTY_FUNCTION__);
        
        if (!isInitialized) throw std::runtime_error("Camera not initialized. Must call Start() first");
        
        uvc_print_stream_ctrl(&ctrl, stderr);
        uvc_print_diag(deviceHandle, stderr);
    }
    
    std::vector<uint8_t> getDepthImage()
    {
        return frameData;
    }
    
    //@todo Calibration info!
    // ------------------
    
};

////////////////////
// Camera Context //
////////////////////

class CameraContext
{
    
    NO_MOVE(CameraContext);
    
    uvc_context_t * privateContext;
    
    // Probes currently connected USB3 cameras and returns an array of device ids
    void QueryDeviceList();
    
public:
    
    // RAII - Initialize libusb context
    CameraContext();
    
    // Stops open streams, closes devices, closes libusb
    ~CameraContext();
    
    std::vector<std::shared_ptr<UVCCamera>> cameras;
};

#endif
