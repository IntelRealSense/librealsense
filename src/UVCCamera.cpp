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
    for(auto & s : streams) s = nullptr;
    uvc_unref_device(hardware);
}

void UVCCamera::EnableStream(int stream, int width, int height, int fps, int format)
{
    // Open interface to stream
    streams[stream].reset(new StreamInterface(hardware, GetStreamSubdeviceNumber(stream)));

    // If this was the first interface to open, give subclass a change to retrieve calibration information
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
}

void UVCCamera::StartStreaming()
{
    GetUSBInfo(hardware, usbInfo);
    // std::cout << "Serial Number: " << usbInfo.serial << std::endl;
    // std::cout << "USB VID: " << usbInfo.vid << std::endl;
    // std::cout << "USB PID: " << usbInfo.pid << std::endl;

    SetStreamIntent(!!streams[RS_DEPTH], !!streams[RS_COLOR]);
    for(auto & stream : streams) if(stream) stream->start_streaming();
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
        streams[RS_DEPTH]->update_image();
    }
}

rs_intrinsics UVCCamera::GetStreamIntrinsics(int stream) const
{
    if(!streams[stream]) throw std::runtime_error("stream not enabled");
    return streams[stream]->get_mode().intrinsics;
}

uvc_device_handle_t * UVCCamera::GetHandleToAnyStream()
{
    for(auto & s : streams) if(s) return s->get_handle();
    return nullptr;
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

void UVCCamera::StreamInterface::start_streaming()
{
    CheckUVC("uvc_start_streaming", uvc_start_streaming(uvcHandle, &ctrl, [](uvc_frame_t * frame, void * ptr)
        {
            auto self = static_cast<StreamInterface *>(ptr);
            const auto & mode = self->mode;
            assert(frame->width == mode.uvcWidth && frame->height == mode.uvcHeight && frame->frame_format == mode.uvcFormat);

            // Copy or convert the image into the back buffer
            if(mode.format == RS_Z16)
            {
                auto in = reinterpret_cast<const uint8_t *>(frame->data);
                auto out = self->back.data();
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
                convert_yuyv_rgb((uint8_t *)frame->data, frame->width, frame->height, self->back.data());
            }
            else throw std::runtime_error("bad format");

            // Swap the backbuffer to the middle buffer and indicate that we have updated
            std::lock_guard<std::mutex> guard(self->mutex);
            self->back.swap(self->middle);
            self->updated = true;
        }, this, 0));
}

bool UVCCamera::StreamInterface::update_image()
{
    if(!updated) return false;
    std::lock_guard<std::mutex> guard(mutex);
    front.swap(middle);
    updated = false;
    return true;
}

} // end namespace rs
