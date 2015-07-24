#include <librealsense/UVCCamera.h>

////////////////
// UVC Camera //
////////////////

UVCCamera::UVCCamera(uvc_device_t * h, int idx) : hardware(h), cameraIdx(idx)
{
    
}

UVCCamera::~UVCCamera()
{
    for (auto stream : streamInterfaces)
    {
        if (stream.second->uvcHandle != nullptr)
        {
            uvc_close(stream.second->uvcHandle);
        }
    }
    uvc_unref_device(hardware);
}

void UVCCamera::frameCallback(uvc_frame_t * frame, StreamInterface * stream)
{
    // @tofix - this is still R200 specific
    if (stream->fmt == UVC_FRAME_FORMAT_Z16)
    {
        memcpy(depthFrame->back.data(), frame->data, (frame->width * frame->height - 1) * 2);
        {
            std::lock_guard<std::mutex> lock(frameMutex);
            depthFrame->swap_back();
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
         }
    }
    
    frameCount++;
    
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

bool UVCCamera::IsStreaming()
{
    if (streamingModeBitfield & STREAM_DEPTH)
    {
        return true;
    }
    else if (streamingModeBitfield & STREAM_RGB)
    {
        return true;
    }
    return false;
}
