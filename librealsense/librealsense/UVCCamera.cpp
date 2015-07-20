#include "UVCCamera.h"

////////////////
// UVC Camera //
////////////////

UVCCamera::UVCCamera(uvc_device_t * device, int num) : device(device), cameraNum(num)
{
    
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

bool UVCCamera::ProbeDevice(int deviceNum)
{
    uvc_ref_device(device);
    uvc_error_t status = uvc_open2(device, &deviceHandle, deviceNum);
    
    if (status < 0)
    {
        uvc_perror(status, "uvc_open2");
        return false;
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
    
    spiInterface.reset(new SPI_Interface(deviceHandle));
    spiInterface->Initialize();
    auto calibParams = spiInterface->GetRectifiedParameters();
    
    // Debugging Camera Firmware:
    std::cout << "Calib Version Number: " << calibParams.versionNumber << std::endl;
    
    isInitialized = true;
    
    return true;
}

//@tofix: cap number of devices <= 2
void UVCCamera::StartStream(int streamNum, const StreamInfo & info)
{
    if (isStreaming) throw std::runtime_error("Camera is already streaming");
    
    StreamHandle * handle = new StreamHandle();
    handle->camera = this;
    handle->fmt = info.format;
    
    uvc_error_t status = uvc_get_stream_ctrl_format_size(deviceHandle, &handle->ctrl, info.format, info.width, info.height, info.fps);
    
    if (status < 0)
    {
        uvc_perror(status, "uvc_get_stream_ctrl_format_size");
        throw std::runtime_error("Open camera_handle Failed");
    }
    
    streamHandles.insert(std::pair<int, StreamHandle *>(streamNum, handle));
    
    if (info.format == UVC_FRAME_FORMAT_Z16)
    {
        depthFrame.reset(new TripleBufferedFrame(info.width, info.height, 2)); // uint8_t * 2 (uint16_t)
    }
    
    else if (info.format == UVC_FRAME_FORMAT_YUYV)
    {
        colorFrame.reset(new TripleBufferedFrame(info.width, info.height, 3)); // uint8_t * 3
    }
    
    // @tofix other stream types here
    
    auto streamIntent = XUControl::SetStreamIntent(deviceHandle, 0); //@tofix - proper streaming mode, assume color and depth
    if (!streamIntent)
    {
        throw std::runtime_error("Could not set stream intent");
    }
    
    uvc_error_t startStreamResult = uvc_start_streaming(deviceHandle, &handle->ctrl, &UVCCamera::cb, handle, 0);
    
    if (startStreamResult < 0)
    {
        uvc_perror(startStreamResult, "start_stream");
        throw std::runtime_error("Could not start stream");
    }
    
    // Spam camera info for debugging
    DumpInfo();
    
    isStreaming = true;
}

void UVCCamera::StopStream(int streamNum)
{
    //@tofix - uvc_stream_stop with a real stream handle.
    if (!isStreaming) throw std::runtime_error("Camera is not already streaming...");
    uvc_stop_streaming(deviceHandle);
    isStreaming = false;
}

void UVCCamera::DumpInfo()
{
    if (!isInitialized) throw std::runtime_error("Camera not initialized. Must call Start() first");
    
    //uvc_print_stream_ctrl(&ctrl, stderr);
    uvc_print_diag(deviceHandle, stderr);
}

void UVCCamera::frameCallback(uvc_frame_t * frame, StreamHandle * handle)
{
    frameCount++;
    
    //@tofix -- assumes depth here. check frame->format
    
    if (handle->fmt == UVC_FRAME_FORMAT_Z16)
    {
        memcpy(depthFrame->back.data(), frame->data, (frame->width * frame->height - 1) * 2);
        {
            std::lock_guard<std::mutex> lock(frameMutex);
            depthFrame->swap_back();
        }
    }
    
    else if (handle->fmt == UVC_FRAME_FORMAT_YUYV)
    {
        /*
         memcpy(depthFrame->back.data(), frame->data, (frame->width * frame->height - 1) * 2);
         {
         std::lock_guard<std::mutex> lock(frameMutex);
         depthFrame->swap_back();
         }
         */
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
