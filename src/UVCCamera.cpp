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

void UVCCamera::EnableStream(int stream, int width, int height, int fps, FrameFormat format)
{
    int cameraNumber;
    switch(stream)
    {
    case RS_STREAM_DEPTH: cameraNumber = GetDepthCameraNumber(); break;
    case RS_STREAM_RGB: cameraNumber = GetColorCameraNumber(); break;
    default: throw std::invalid_argument("unsupported stream");
    }

    StreamInterface * streamInterface = new StreamInterface();
    streamInterface->camera = this;
    CheckUVC("uvc_open2", uvc_open2(hardware, &streamInterface->uvcHandle, cameraNumber));
    uvc_print_stream_ctrl(&streamInterface->ctrl, stdout); // Debugging

    streamInterfaces.insert(std::pair<int, StreamInterface *>(stream, streamInterface));

    RetrieveCalibration();

    // TODO: Check formats and the like
    streamInterface->fmt = static_cast<uvc_frame_format>(format);
    CheckUVC("uvc_get_stream_ctrl_format_size", uvc_get_stream_ctrl_format_size(streamInterface->uvcHandle, &streamInterface->ctrl, streamInterface->fmt, width, height, fps));

    switch(stream)
    {
    case RS_STREAM_DEPTH:
        switch(format)
        {
        case FrameFormat::Z16:
        case FrameFormat::INVR:
        case FrameFormat::INVZ:
            depthFrame.resize(width, height, 1);
            vertices.resize(width * height * 3);
            break;
        default: throw std::runtime_error("invalid frame format");
        }
        zConfig = {width, height, fps, format};
        break;
    case RS_STREAM_RGB:
        switch(format)
        {
        case FrameFormat::YUYV:
            colorFrame.resize(width, height, 3); break;
        default: throw std::runtime_error("invalid frame format");
        }
        rgbConfig = {width, height, fps, format};
        break;
    }
}

void UVCCamera::StartStreaming()
{
    GetUSBInfo(hardware, usbInfo);
    std::cout << "Serial Number: " << usbInfo.serial << std::endl;
    std::cout << "USB VID: " << usbInfo.vid << std::endl;
    std::cout << "USB PID: " << usbInfo.pid << std::endl;

    SetStreamIntent(!!streamInterfaces[RS_STREAM_DEPTH], !!streamInterfaces[RS_STREAM_RGB]);

    for(auto & p : streamInterfaces)
    {
        if (p.second->uvcHandle) CheckUVC("uvc_start_streaming", uvc_start_streaming(p.second->uvcHandle, &p.second->ctrl, &UVCCamera::cb, p.second, 0));
    }
}

void UVCCamera::StopStreaming()
{

}

void UVCCamera::frameCallback(uvc_frame_t * frame, StreamInterface * stream)
{
    if (stream->fmt == UVC_FRAME_FORMAT_Z16 || stream->fmt == UVC_FRAME_FORMAT_INVR || stream->fmt == UVC_FRAME_FORMAT_INVZ)
    {
        memcpy(depthFrame.back_data(), frame->data, (frame->width * frame->height - 1) * sizeof(uint16_t));
        depthFrame.swap_back();
    }
    
    else if (stream->fmt == UVC_FRAME_FORMAT_YUYV)
    {
        //@tofix - this is a bit silly to overallocate. Blame Leo.
        static uint8_t color_cvt[1920 * 1080 * 3]; // YUYV = 16 bits in in -> 24 out
        convert_yuyv_rgb((uint8_t *)frame->data, frame->width, frame->height, color_cvt);
        memcpy(colorFrame.back_data(), color_cvt, (frame->width * frame->height) * 3);
        colorFrame.swap_back();
    }
    
    frameCount++;
}
    
} // end namespace rs
