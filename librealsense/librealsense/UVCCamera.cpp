#include "UVCCamera.h"

////////////////
// UVC Camera //
////////////////

UVCCamera::UVCCamera(uvc_device_t * h, int num) : hardware(h), cameraNum(num)
{
    printf("%s", __PRETTY_FUNCTION__);
}

UVCCamera::~UVCCamera()
{
    printf("%s", __PRETTY_FUNCTION__);
    for (auto stream : streamInterfaces)
    {
        if (stream.second->uvcHandle != nullptr)
        {
            uvc_close(stream.second->uvcHandle);
        }
    }
    uvc_unref_device(hardware);
}

//@tofix protect against calling this multiple times
bool UVCCamera::ConfigureStreams()
{
    if (streamingModeBitfield == 0)
        throw std::invalid_argument("No streams have been configured...");
    
    uvc_ref_device(hardware);
    
    //@tofix better error handling...
    auto openStreamWithHardwareIndex = [&](int idx, uvc_device_handle_t *& uvc_handle)
    {
        // YES: Need different device handles!!
        uvc_error_t status = uvc_open2(hardware, &uvc_handle, idx);
        
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
        StreamInterface * stream = new StreamInterface();
        stream->camera = this;
        
        openStreamWithHardwareIndex(1, stream->uvcHandle);
        streamInterfaces.insert(std::pair<int, StreamInterface *>(STREAM_DEPTH, stream));
        //DumpInfo();
    }
    
    if (streamingModeBitfield & STREAM_RGB)
    {
        StreamInterface * stream = new StreamInterface();
        stream->camera = this;
        
        openStreamWithHardwareIndex(2, stream->uvcHandle);
        streamInterfaces.insert(std::pair<int, StreamInterface *>(STREAM_RGB, stream));
        //DumpInfo();
    }
    
    //DumpInfo();
    
    /*
    if (streamingModeBitfield & STREAM_LR)
    {    
        openDevice(0);
    }
    */
    
    uvc_device_descriptor_t * desc;
    if(uvc_get_device_descriptor(hardware, &desc) == UVC_SUCCESS)
    {
        if (desc->serialNumber) usbInfo.serial = desc->serialNumber;
        if (desc->idVendor) usbInfo.vid = desc->idVendor;
        if (desc->idProduct) usbInfo.pid = desc->idProduct;
        uvc_free_device_descriptor(desc);
    }
    
    std::cout << "Serial Number: " << usbInfo.serial << std::endl;
    std::cout << "USB VID: " << usbInfo.vid << std::endl;
    std::cout << "USB PID: " << usbInfo.pid << std::endl;
    
    auto oneTimeInitialize = [&](uvc_device_handle_t * uvc_handle)
    {
        //@tofix if first time:
        spiInterface.reset(new SPI_Interface(uvc_handle));
        spiInterface->Initialize();
        auto calibParams = spiInterface->GetRectifiedParameters();
        
        // Debugging Camera Firmware:
        std::cout << "Calib Version Number: " << calibParams.versionNumber << std::endl;
        
        auto streamIntent = XUControl::SetStreamIntent(uvc_handle, streamingModeBitfield); //@tofix - proper streaming mode, assume color and depth
        if (!streamIntent)
        {
            throw std::runtime_error("Could not set stream intent");
        }
    };
    
    // We only need to do this once, so check if any stream has been configured
    if (streamInterfaces[STREAM_DEPTH]->uvcHandle)
    {
        oneTimeInitialize(streamInterfaces[STREAM_DEPTH]->uvcHandle);
    }
    
    else if (streamInterfaces[STREAM_RGB]->uvcHandle)
    {
        oneTimeInitialize(streamInterfaces[STREAM_RGB]->uvcHandle);
    }

    isInitialized = true;
    
    return true;
}

//@tofix: cap number of devices <= 2
void UVCCamera::StartStream(int streamIdentifier, const StreamConfiguration & config)
{
  //  if (isStreaming) throw std::runtime_error("Camera is already streaming");
    
    auto stream = streamInterfaces[streamIdentifier];
    
    if (stream->uvcHandle)
    {
        stream->fmt = config.format;
        
        uvc_error_t status = uvc_get_stream_ctrl_format_size(
                                                             stream->uvcHandle,
                                                             &stream->ctrl,
                                                             config.format,
                                                             config.width,
                                                             config.height,
                                                             config.fps);
        
        if (status < 0)
        {
            uvc_perror(status, "uvc_get_stream_ctrl_format_size");
            throw std::runtime_error("Open camera_handle Failed");
        }
        
        //@todo and check streaming mode
        if (config.format == UVC_FRAME_FORMAT_Z16)
        {
            depthFrame.reset(new TripleBufferedFrame(config.width, config.height, 2)); // uint8_t * 2 (uint16_t)
        }
        
        else if (config.format == UVC_FRAME_FORMAT_YUYV)
        {
            
            colorFrame.reset(new TripleBufferedFrame(config.width, config.height, 3)); // uint8_t * 3
        }
        
        {
            // @todo Add LR streaming
        }
        
        uvc_error_t startStreamResult = uvc_start_streaming(stream->uvcHandle, &stream->ctrl, &UVCCamera::cb, stream, 0);
        
        if (startStreamResult < 0)
        {
            uvc_perror(startStreamResult, "start_stream");
            throw std::runtime_error("Could not start stream");
        }
        
        isStreaming = true;
    }
    
    // Else what?
    
}

void UVCCamera::StopStream(int streamNum)
{
    //@tofix - uvc_stream_stop with a real stream handle -> index with map that we have 
    if (!isStreaming) throw std::runtime_error("Camera is not already streaming...");
    //uvc_stop_streaming(deviceHandle);
    isStreaming = false;
}

void UVCCamera::DumpInfo()
{
    //if (!isInitialized) throw std::runtime_error("Camera not initialized. Must call Start() first");
    
    //uvc_print_stream_ctrl(&ctrl, stderr);
    //uvc_print_diag(deviceHandle, stderr);
}

void UVCCamera::frameCallback(uvc_frame_t * frame, StreamInterface * stream)
{
    if (stream->fmt == UVC_FRAME_FORMAT_Z16)
    {
        memcpy(depthFrame->back.data(), frame->data, (frame->width * frame->height - 1) * 2);
        {
            std::lock_guard<std::mutex> lock(frameMutex);
            depthFrame->swap_back();
            depthFrame->frameCount += 1;
        }
    }
    
    else if (stream->fmt == UVC_FRAME_FORMAT_YUYV)
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
