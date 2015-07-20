#include "CameraContext.h"

#include <stdio.h>
#include <stdint.h>
#include <iostream>
#include <thread>
#include <chrono>

////////////////
// UVC Camera //
////////////////

UVCCamera::UVCCamera(uvc_device_t * device, int num) : device(device), cameraNum(num)
{
    printf("%s", __PRETTY_FUNCTION__);
}

UVCCamera::~UVCCamera()
{
    printf("%s", __PRETTY_FUNCTION__);
    
    
    if (deviceHandle != nullptr)
    {
        uvc_close(deviceHandle);
    }
    
    uvc_unref_device(device);
}

void UVCCamera::Open(int streamNum)
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
            dInfo.serial = desc->serialNumber;
        
        if (desc->idVendor)
            dInfo.vid = desc->idVendor;
        
        if (desc->idProduct)
            dInfo.pid = desc->idProduct;
        
        uvc_free_device_descriptor(desc);
    }
    
    std::cout << "Serial Number: " << dInfo.serial << std::endl;
    std::cout << "USB VID: " << dInfo.vid << std::endl;
    std::cout << "USB PID: " << dInfo.pid << std::endl;
    
    std::cout << "Initializing SPI... " << std::endl;
    
    spiInterface.reset(new SPI_Interface(deviceHandle));
    spiInterface->Initialize();
    auto calibParams = spiInterface->GetRectifiedParameters();
    
    // Debugging --
    std::cout << "Calib Version Number: " << calibParams.versionNumber << std::endl;
    
    isInitialized = true;
}

void UVCCamera::Start(const StreamInfo & info)
{
    printf("%s", __PRETTY_FUNCTION__);
    
    if (isStreaming) throw std::runtime_error("Camera is already streaming");
    
    // Fix for multiple streams:
    sInfo = info;
    
    uvc_error_t status = uvc_get_stream_ctrl_format_size(deviceHandle, &ctrl, info.format, info.width, info.height, info.fps);
    
    if (status < 0)
    {
        uvc_perror(status, "uvc_get_stream_ctrl_format_size");
        throw std::runtime_error("Open camera_handle Failed");
    }
    
    // Allocate stream memory
    // @todo - check modes and only allocate if necessary
    depthFrame.reset(new TripleBufferedFrame(info.width, info.height, 2)); // uint8_t * 2 (uint16_t)
    colorFrame.reset(new TripleBufferedFrame(info.width, info.height, 3)); // uint8_t * 3
    
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
    
    // Spam camera info for debugging
    DumpInfo();
    
    isStreaming = true;
}

void UVCCamera::Stop()
{
    printf("%s", __PRETTY_FUNCTION__);
    if (!isStreaming) throw std::runtime_error("Camera is not already streaming...");
    uvc_stop_streaming(deviceHandle);
    isStreaming = false;
}

void UVCCamera::DumpInfo()
{
    printf("%s", __PRETTY_FUNCTION__);
    
    if (!isInitialized) throw std::runtime_error("Camera not initialized. Must call Start() first");
    
    uvc_print_stream_ctrl(&ctrl, stderr);
    uvc_print_diag(deviceHandle, stderr);
}

void UVCCamera::frameCallback(uvc_frame_t * frame)
{
    //@tofix check invalid frame width, height
    
    frameCount++;

    memcpy(depthFrame->back.data(), frame->data, (frame->width * frame->height - 1) * 2);
    
    {
        std::lock_guard<std::mutex> lock(frameMutex);
        depthFrame->swap_back();
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

uint16_t * UVCCamera::GetDepthImage()
{
    if (depthFrame->updated)
    {
        std::lock_guard<std::mutex> guard(frameMutex);
        depthFrame->swap_front();
    }
    uint16_t * framePtr = reinterpret_cast<uint16_t *>(depthFrame->front.data());
    return framePtr;
}

uint8_t * UVCCamera::GetColorImage()
{
    if (colorFrame->updated)
    {
        std::lock_guard<std::mutex> guard(frameMutex);
        colorFrame->swap_front();
    }
    uint8_t * framePtr = reinterpret_cast<uint8_t *>(colorFrame->front.data());
    return framePtr;
}

////////////////////
// Camera Context //
////////////////////

CameraContext::CameraContext()
{
    uvc_error_t initStatus;

    initStatus = uvc_init(&privateContext, NULL);
    
    if (initStatus < 0)
    {
        uvc_perror(initStatus, "uvc_init");
        throw std::runtime_error("Could not initialize a UVC context");
    }
    
    QueryDeviceList();
}

CameraContext::~CameraContext()
{
    cameras.clear(); // tear down cameras before context
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    if (privateContext)
    {
        uvc_exit(privateContext);
    }
}

void CameraContext::QueryDeviceList()
{
    uvc_device_t **list;
    uvc_error_t status;
    
    status = uvc_get_device_list(privateContext, &list);
    if (status != UVC_SUCCESS)
    {
        uvc_perror(status, "uvc_get_device_list");
        return;
    }
    
    size_t index = 0;
    while (list[index] != nullptr)
    {
        uvc_ref_device(list[index]);
        cameras.push_back(std::make_shared<UVCCamera>(list[index], index));
        index++;
    }
    
    uvc_free_device_list(list, 1);
}