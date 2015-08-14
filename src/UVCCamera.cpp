#include "UVCCamera.h"

////////////////
// UVC Camera //
////////////////

namespace rs
{
    
UVCCamera::UVCCamera(uvc_context_t * c, uvc_device_t * h, int idx) : rs_camera(idx), hardware(h), internalContext(c)
{
    uvc_device_descriptor_t * desc;
    uvc_error_t status = uvc_get_device_descriptor(h, &desc);
    if(status < 0) {} // Handle error

    cameraName = desc->product;
    uvc_free_device_descriptor(desc);
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

bool UVCCamera::ConfigureStreams()
{
    if (streamingModeBitfield == 0)
        throw std::invalid_argument("No streams have been configured...");

    if (streamingModeBitfield & RS_STREAM_DEPTH)
    {
        StreamInterface * stream = new StreamInterface();
        stream->camera = this;

        CheckUVC("uvc_open2", uvc_open2(hardware, &stream->uvcHandle, GetDepthCameraNumber()));
        uvc_print_stream_ctrl(&stream->ctrl, stdout); // Debugging

        streamInterfaces.insert(std::pair<int, StreamInterface *>(RS_STREAM_DEPTH, stream));
    }

    if (streamingModeBitfield & RS_STREAM_RGB)
    {
        StreamInterface * stream = new StreamInterface();
        stream->camera = this;

        CheckUVC("uvc_open2", uvc_open2(hardware, &stream->uvcHandle, GetColorCameraNumber()));
        uvc_print_stream_ctrl(&stream->ctrl, stdout); // Debugging

        streamInterfaces.insert(std::pair<int, StreamInterface *>(RS_STREAM_RGB, stream));
    }

    GetUSBInfo(hardware, usbInfo);
    std::cout << "Serial Number: " << usbInfo.serial << std::endl;
    std::cout << "USB VID: " << usbInfo.vid << std::endl;
    std::cout << "USB PID: " << usbInfo.pid << std::endl;

    RetrieveCalibration();

    return true;
}

void UVCCamera::frameCallback(uvc_frame_t * frame, StreamInterface * stream)
{
    if (stream->fmt == UVC_FRAME_FORMAT_Z16 || stream->fmt == UVC_FRAME_FORMAT_INVR || stream->fmt == UVC_FRAME_FORMAT_INVZ)
    {
        memcpy(depthFrame.back.data(), frame->data, (frame->width * frame->height - 1) * 2);
        {
            std::lock_guard<std::mutex> lock(frameMutex);
            depthFrame.swap_back();
        }
    }
    
    else if (stream->fmt == UVC_FRAME_FORMAT_YUYV)
    {
        //@tofix - this is a bit silly to overallocate. Blame Leo.
        static uint8_t color_cvt[1920 * 1080 * 3]; // YUYV = 16 bits in in -> 24 out
        convert_yuyv_rgb((uint8_t *)frame->data, frame->width, frame->height, color_cvt);
        memcpy(colorFrame.back.data(), color_cvt, (frame->width * frame->height) * 3);
        {
            std::lock_guard<std::mutex> lock(frameMutex);
            colorFrame.swap_back();
        }
    }
    
    frameCount++;
}
    
} // end namespace rs
