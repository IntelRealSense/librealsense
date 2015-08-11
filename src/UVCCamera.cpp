#include "UVCCamera.h"

////////////////
// UVC Camera //
////////////////

namespace rs
{

UVCCamera::UVCCamera(uvc_device_t * h, int idx) : rs_camera(idx), hardware(h)
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

bool UVCCamera::OpenStreamOnSubdevice(uvc_device_t * dev,  uvc_device_handle_t *& h, int idx)
{
    uvc_error_t status = uvc_open2(dev, &h, idx);
    if (status < 0)
    {
        uvc_perror(status, "uvc_open2");
        return false;
    }
    return true;
};
    
} // end namespace rs
