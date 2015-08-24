#include "UVCCamera.h"
#include "image.h"

////////////////
// UVC Camera //
////////////////

namespace rs
{
    
UVCCamera::UVCCamera(uvc_context_t * context, uvc_device_t * device) : context(context), device(device), first_handle()
{
    uvc_device_descriptor_t * desc;
    CheckUVC("uvc_get_device_descriptor", uvc_get_device_descriptor(device, &desc));

    cameraName = desc->product;
    // Other interesting properties
    // desc->serialNumber
    // desc->idVendor
    // desc->idProduct

    uvc_free_device_descriptor(desc);  

    calib = {};
}

UVCCamera::~UVCCamera()
{
    for(auto & s : streams) s = nullptr;
    uvc_unref_device(device);
}

void UVCCamera::EnableStream(int stream, int width, int height, int fps, int format)
{
    // Open interface to stream, and optionally retrieve calibration info
    streams[stream].reset(new StreamInterface(device, GetStreamSubdeviceNumber(stream)));
    if(!first_handle) first_handle = streams[stream]->get_handle();
    if(calib.modes.empty()) calib = RetrieveCalibration(streams[stream]->get_handle());

    // Choose a resolution mode based on the user's request
    ResolutionMode mode = [=]()
    {
        for(const auto & mode : calib.modes)
        {
            if(mode.stream == stream && mode.width == width && mode.height == height && mode.fps == fps && mode.format == format)
            {
                return mode;
            }
        }
        throw std::runtime_error("invalid mode");
    }();
    streams[stream]->set_mode(mode);
    user_streams[stream].reset(new UserStreamInterface());
    user_streams[stream]->set_mode(mode);
}

void UVCCamera::StartStreaming()
{
    SetStreamIntent();
    for(int i=0; i<3; ++i)
    {
        if(streams[i] && user_streams[i])
        {
            streams[i]->start_streaming(user_streams[i].get());
        }
    }
}

void UVCCamera::StopStreaming()
{
    for(auto & stream : streams) if(stream) stream->stop_streaming();
}
    
void UVCCamera::WaitAllStreams()
{
    int maxFps = 0;
    for(auto & stream : user_streams) maxFps = stream ? std::max(maxFps, stream->get_mode().fps) : maxFps;

    for(auto & stream : user_streams)
    {
        if(stream)
        {
            // If this is the fastest stream, wait until a new frame arrives
            if(stream->get_mode().fps == maxFps)
            {
                while(true) if(stream->update_image()) break;
            }
            else // Otherwise simply check for a new frame
            {
                stream->update_image();
            }
        }
    }
}

rs_intrinsics UVCCamera::GetStreamIntrinsics(int stream) const
{
    if(!user_streams[stream]) throw std::runtime_error("stream not enabled");
    return user_streams[stream]->get_mode().intrinsics;
}

rs_extrinsics UVCCamera::GetStreamExtrinsics(int from, int to) const
{
    auto transform = inverse(calib.stream_poses[from]) * calib.stream_poses[to]; // TODO: Make sure this is the right order
    rs_extrinsics extrin;
    (float3x3 &)extrin.rotation = transform.orientation;
    (float3 &)extrin.translation = transform.position;
    return extrin;
}

void UVCCamera::UserStreamInterface::set_mode(const ResolutionMode & mode)
{
    this->mode = mode;
    switch(mode.format)
    {
    case RS_Z16: front.resize(mode.width * mode.height * sizeof(uint16_t)); break;
    case RS_RGB8: front.resize(mode.width * mode.height * 3); break;
    case RS_Y8: front.resize(mode.width * mode.height); break;
    default: throw std::runtime_error("invalid format");
    }
    back = middle = front;
    updated = false;
}

bool UVCCamera::UserStreamInterface::update_image()
{
    if(!updated) return false;
    std::lock_guard<std::mutex> guard(mutex);
    front.swap(middle);
    updated = false;
    return true;
}

void UVCCamera::StreamInterface::set_mode(const ResolutionMode & mode)
{
    this->mode = mode;
    CheckUVC("uvc_get_stream_ctrl_format_size", uvc_get_stream_ctrl_format_size(uvcHandle, &ctrl, mode.uvcFormat, mode.uvcWidth, mode.uvcHeight, mode.uvcFps));
}

void UVCCamera::StreamInterface::start_streaming(UserStreamInterface * user_interface)
{
    this->user_interface = user_interface;
    CheckUVC("uvc_start_streaming", uvc_start_streaming(uvcHandle, &ctrl, [](uvc_frame_t * frame, void * ptr)
        {
            auto self = static_cast<StreamInterface *>(ptr);
            const auto & mode = self->mode;
            assert(frame->width == mode.uvcWidth && frame->height == mode.uvcHeight && frame->frame_format == mode.uvcFormat);

            // Copy or convert the image into the back buffer
            switch(mode.format)
            {
            case RS_Z16:
                copy_strided_image(self->user_interface->back.data(), mode.width * sizeof(uint16_t), frame->data, mode.uvcWidth * sizeof(uint16_t), mode.height);
                break;
            case RS_Y8:
                copy_strided_image(self->user_interface->back.data(), mode.width * sizeof(uint8_t), frame->data, mode.uvcWidth * sizeof(uint8_t), mode.height);
                break;
            case RS_RGB8:
                assert(mode.uvcFormat == UVC_FRAME_FORMAT_YUYV);
                convert_yuyv_to_rgb(self->user_interface->back.data(), frame->width, frame->height, (uint8_t *)frame->data);
                break;
            default: throw std::runtime_error("bad format");
            }

            // Swap the backbuffer to the middle buffer and indicate that we have updated
            std::lock_guard<std::mutex> guard(self->user_interface->mutex);
            self->user_interface->back.swap(self->user_interface->middle);
            self->user_interface->updated = true;
        }, this, 0));
}
    
void UVCCamera::StreamInterface::stop_streaming()
{
    CheckUVC("uvc_stream_stop", uvc_stream_stop(ctrl.handle));
}

} // end namespace rs
