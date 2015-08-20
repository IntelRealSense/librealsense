#include "UVCCamera.h"

////////////////
// UVC Camera //
////////////////

namespace rs
{
    
UVCCamera::UVCCamera(uvc_context_t * c, uvc_device_t * h) : hardware(h), internalContext(c)
{
    uvc_device_descriptor_t * desc;
    uvc_error_t status = uvc_get_device_descriptor(h, &desc);
    if(status < 0) {} // Handle error

    cameraName = desc->product;
    uvc_free_device_descriptor(desc);
}

UVCCamera::~UVCCamera()
{
    for (auto & stream : streams)
    {
        if(stream && stream->uvcHandle)
        {
            uvc_close(stream->uvcHandle);
        }
    }
    uvc_unref_device(hardware);
}

void UVCCamera::EnableStream(int stream, int width, int height, int fps, int format)
{
    int cameraNumber;
    switch(stream)
    {
    case RS_DEPTH: cameraNumber = GetDepthCameraNumber(); break;
    case RS_COLOR: cameraNumber = GetColorCameraNumber(); break;
    default: throw std::invalid_argument("unsupported stream");
    }

    streams[stream].reset(new StreamInterface());
    CheckUVC("uvc_open2", uvc_open2(hardware, &streams[stream]->uvcHandle, cameraNumber));

    RetrieveCalibration();

    // Choose a resolution mode based on the user's request
    ResolutionMode mode = [=]()
    {
        for(const auto & mode : modes)
        {
            if(mode.stream == stream && mode.width == width && mode.height == height && mode.fps == fps && mode.format == format)
            {
                return mode;
            }
        }
        throw std::runtime_error("invalid mode");
    }();

    streams[stream]->set_mode(mode);

    if(stream == RS_DEPTH) vertices.resize(mode.width * mode.height * 3);
}

void UVCCamera::StartStreaming()
{
    GetUSBInfo(hardware, usbInfo);
    // std::cout << "Serial Number: " << usbInfo.serial << std::endl;
    // std::cout << "USB VID: " << usbInfo.vid << std::endl;
    // std::cout << "USB PID: " << usbInfo.pid << std::endl;

    SetStreamIntent(!!streams[RS_DEPTH], !!streams[RS_COLOR]);

    for(auto & stream : streams)
    {
        if (stream && stream->uvcHandle) CheckUVC("uvc_start_streaming", uvc_start_streaming(stream->uvcHandle, &stream->ctrl,
            [](uvc_frame_t * frame, void * ptr) { static_cast<StreamInterface*>(ptr)->on_frame(frame); }, stream.get(), 0));
    }
}

void UVCCamera::StopStreaming()
{

}
    
void UVCCamera::WaitAllStreams()
{
    if(streams[RS_COLOR])
    {
        streams[RS_COLOR]->update_image();
    }

    if(streams[RS_DEPTH])
    {
        if(streams[RS_DEPTH]->update_image())
        {
            ComputeVertexImage();
        }
    }
}

rs_intrinsics UVCCamera::GetStreamIntrinsics(int stream) const
{
    if(!streams[stream]) throw std::runtime_error("stream not enabled");
    return streams[stream]->mode.intrinsics;
}

void UVCCamera::StreamInterface::set_mode(const ResolutionMode & mode)
{
    this->mode = mode;
    CheckUVC("uvc_get_stream_ctrl_format_size", uvc_get_stream_ctrl_format_size(uvcHandle, &ctrl, mode.uvcFormat, mode.uvcWidth, mode.uvcHeight, mode.uvcFps));

    switch(mode.format)
    {
    case RS_Z16: front.resize(mode.width * mode.height * sizeof(uint16_t)); break;
    case RS_RGB: front.resize(mode.width * mode.height * 3); break;
    default: throw std::runtime_error("invalid format");
    }
    back = middle = front;
    updated = false;
}

bool UVCCamera::StreamInterface::update_image()
{
    if(!updated) return false;
    std::lock_guard<std::mutex> guard(mutex);
    front.swap(middle);
    updated = false;
    return true;
}

void UVCCamera::StreamInterface::on_frame(uvc_frame_t * frame)
{
    assert(frame->width == mode.uvcWidth && frame->height == mode.uvcHeight && frame->frame_format == mode.uvcFormat);

    // Copy or convert the image into the back buffer
    if(mode.format == RS_Z16)
    {
        auto in = reinterpret_cast<const uint8_t *>(frame->data);
        auto out = back.data();
        for(int y=0; y<mode.height; ++y)
        {
            memcpy(out, in, mode.width * sizeof(uint16_t));
            in += mode.uvcWidth * sizeof(uint16_t);
            out += mode.width * sizeof(uint16_t);
        }
    }
    else if(mode.format == RS_RGB)
    {
        assert(mode.uvcFormat == UVC_FRAME_FORMAT_YUYV);
        convert_yuyv_rgb((uint8_t *)frame->data, frame->width, frame->height, back.data());
    }
    else throw std::runtime_error("bad format");

    // Swap the backbuffer to the middle buffer and indicate that we have updated
    std::lock_guard<std::mutex> guard(mutex);
    back.swap(middle);
    updated = true;
}

} // end namespace rs
