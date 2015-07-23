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

//@tofix protect against calling this multiple times
bool UVCCamera::ConfigureStreams()
{
    if (streamingModeBitfield == 0)
        throw std::invalid_argument("No streams have been configured...");
    
    uvc_ref_device(device);
    
    //@tofix better error handling...
    auto openDevice = [&](int idx)
    {
        // YES: Need different device handles!!
        uvc_error_t status = uvc_open2(device, &deviceHandle, idx);
        
        if (status < 0)
        {
            uvc_perror(status, "uvc_open2");
            return false;
        }
        return true;
    };
    
    //@tofix: Test for successful open
    if (streamingModeBitfield & STREAM_DEPTH)
    {
        std::cout << "\tStream_Depth\n";
        openDevice(1);
        //DumpInfo();
    }
    
    if (streamingModeBitfield & STREAM_RGB)
    {
        std::cout << "\tStream_RGB\n";
        openDevice(2);
        //DumpInfo();
    }
    
    
    DumpInfo();
    
    /*
    if (streamingModeBitfield & STREAM_LR)
    {
        openDevice(0);
    }
    */
    
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
    
    auto streamIntent = XUControl::SetStreamIntent(deviceHandle, streamingModeBitfield); //@tofix - proper streaming mode, assume color and depth
    if (!streamIntent)
    {
        throw std::runtime_error("Could not set stream intent");
    }
    
    isInitialized = true;
    
    return true;
}

//@tofix: cap number of devices <= 2
void UVCCamera::StartStream(int streamNum, const StreamInfo & info)
{
  //  if (isStreaming) throw std::runtime_error("Camera is already streaming");
    
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
    
    // ...
    
    {
        // @todo Add LR streaming
    }
    
    uvc_error_t startStreamResult = uvc_start_streaming(deviceHandle, &handle->ctrl, &UVCCamera::cb, handle, 0);
    
    if (startStreamResult < 0)
    {
        uvc_perror(startStreamResult, "start_stream");
        throw std::runtime_error("Could not start stream");
    }
    
    isStreaming = true;
}

void UVCCamera::StopStream(int streamNum)
{
    //@tofix - uvc_stream_stop with a real stream handle -> index with map that we have 
    if (!isStreaming) throw std::runtime_error("Camera is not already streaming...");
    uvc_stop_streaming(deviceHandle);
    isStreaming = false;
}

void UVCCamera::DumpInfo()
{
    //if (!isInitialized) throw std::runtime_error("Camera not initialized. Must call Start() first");
    
    //uvc_print_stream_ctrl(&ctrl, stderr);
    uvc_print_diag(deviceHandle, stderr);
}

void UVCCamera::frameCallback(uvc_frame_t * frame, StreamHandle * handle)
{
    if (handle->fmt == UVC_FRAME_FORMAT_Z16)
    {
        memcpy(depthFrame->back.data(), frame->data, (frame->width * frame->height - 1) * 2);
        {
            std::lock_guard<std::mutex> lock(frameMutex);
            depthFrame->swap_back();
            depthFrame->frameCount += 1;
        }
    }
    
    else if (handle->fmt == UVC_FRAME_FORMAT_YUYV)
    {
        //@tofix - this is a bit silly to overallocate. Blame Leo.
        static uint8_t color_cvt[1920 * 1080 * 3]; // YUYV = 16 bits in in -> 24 out
        convert_yuyv_rgb((uint8_t *)frame->data, frame->width, frame->height, color_cvt);
        memcpy(colorFrame->back.data(), color_cvt, (frame->width * frame->height) * 3);
        
         {
            std::lock_guard<std::mutex> lock(frameMutex);
            colorFrame->swap_back();
            colorFrame->frameCount += 1;
         }
    }
    
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
