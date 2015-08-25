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
    subdevices.clear();
    uvc_unref_device(device);
}

void UVCCamera::EnableStream(int stream, int width, int height, int fps, int format)
{
    int sdnum = [stream]() { switch(stream)
        {
        case RS_INFRARED: return 0;
        case RS_DEPTH: return 1;
        case RS_COLOR: return 2;
        } }();

    // Open interface to stream, and optionally retrieve calibration info
    subdevices[stream].reset(new Subdevice(device, sdnum));
    if(!first_handle) first_handle = subdevices[stream]->get_handle();
    if(calib.modes.empty()) calib = RetrieveCalibration();

    // Choose a resolution mode based on the user's request
    SubdeviceMode mode = [=]()
    {
        for(const auto & mode : calib.modes)
        {
            for(const auto & smode : mode.streams)
            {
                if(smode.stream == stream && smode.intrinsics.image_size[0] == width && smode.intrinsics.image_size[1] == height && smode.fps == fps && smode.format == format)
                {
                    return mode;
                }
            }
        }
        throw std::runtime_error("invalid mode");
    }();

    std::vector<std::shared_ptr<Stream>> mode_streams;

    streams[stream].reset(new Stream());
    mode_streams.push_back(streams[stream]);

    subdevices[stream]->set_mode(mode, mode_streams);
}

void UVCCamera::StartStreaming()
{
    SetStreamIntent();
    for(auto & subdevice : subdevices) subdevice->start_streaming();
}

void UVCCamera::StopStreaming()
{
    for(auto & subdevice : subdevices) subdevice->stop_streaming();
}
    
void UVCCamera::WaitAllStreams()
{
    int maxFps = 0;
    for(auto & stream : streams) maxFps = stream ? std::max(maxFps, stream->get_mode().fps) : maxFps;

    for(auto & stream : streams)
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
    if(!streams[stream]) throw std::runtime_error("stream not enabled");
    return streams[stream]->get_mode().intrinsics;
}

rs_extrinsics UVCCamera::GetStreamExtrinsics(int from, int to) const
{
    auto transform = inverse(calib.stream_poses[from]) * calib.stream_poses[to]; // TODO: Make sure this is the right order
    rs_extrinsics extrin;
    (float3x3 &)extrin.rotation = transform.orientation;
    (float3 &)extrin.translation = transform.position;
    return extrin;
}

void UVCCamera::Stream::set_mode(const StreamMode & mode)
{
    auto pixels = mode.intrinsics.image_size[0] * mode.intrinsics.image_size[1];
    this->mode = mode;
    switch(mode.format)
    {
    case RS_Z16: front.resize(pixels * sizeof(uint16_t)); break;
    case RS_RGB8: front.resize(pixels * 3); break;
    case RS_Y8: front.resize(pixels); break;
    default: throw std::runtime_error("invalid format");
    }
    back = middle = front;
    updated = false;
}

bool UVCCamera::Stream::update_image()
{
    if(!updated) return false;
    std::lock_guard<std::mutex> guard(mutex);
    front.swap(middle);
    updated = false;
    return true;
}

void UVCCamera::Subdevice::set_mode(const SubdeviceMode & mode, std::vector<std::shared_ptr<Stream>> streams)
{
    assert(mode.streams.size() == streams.size());
    CheckUVC("uvc_get_stream_ctrl_format_size", uvc_get_stream_ctrl_format_size(uvcHandle, &ctrl, mode.format, mode.width, mode.height, mode.fps));

    this->mode = mode;
    this->streams = streams;
    for(size_t i=0; i<mode.streams.size(); ++i) streams[i]->set_mode(mode.streams[i]);
}

static size_t get_pixel_size(int format)
{
    switch(format)
    {
    case RS_Z16: return sizeof(uint16_t);
    case RS_Y8: return sizeof(uint8_t);
    case RS_RGB8: return sizeof(uint8_t) * 3;
    default: assert(false);
    }
}

void unpack_strided_image(void * dest[], const SubdeviceMode & mode, const uint8_t * source)
{
    assert(mode.streams.size() == 1);
    copy_strided_image(dest[0], mode.streams[0].intrinsics.image_size[0] * get_pixel_size(mode.streams[0].format),
        source, mode.width * get_pixel_size(mode.streams[0].format), mode.streams[0].intrinsics.image_size[1]);
}

void unpack_yuyv_to_rgb(void * dest[], const SubdeviceMode & mode, const uint8_t * source)
{
    assert(mode.format == UVC_FRAME_FORMAT_YUYV && mode.streams.size() == 1 && mode.streams[0].format == RS_RGB8);
    convert_yuyv_to_rgb(dest[0], mode.width, mode.height, source);
}

void UVCCamera::Subdevice::start_streaming()
{
    CheckUVC("uvc_start_streaming", uvc_start_streaming(uvcHandle, &ctrl, [](uvc_frame_t * frame, void * ptr)
    {
        // Validate that this frame matches the mode information we've set
        auto self = reinterpret_cast<Subdevice *>(ptr);
        assert(frame->width == self->mode.width && frame->height == self->mode.height && frame->frame_format == self->mode.format);

        // Unpack the image into the user stream interface back buffer
        std::vector<void *> dest;
        for(auto & stream : self->streams) dest.push_back(stream->back.data());
        self->mode.unpacker(dest.data(), self->mode, (const uint8_t *)frame->data);

        // Swap the backbuffer to the middle buffer and indicate that we have updated
        for(auto & stream : self->streams)
        {
            std::lock_guard<std::mutex> guard(stream->mutex);
            stream->back.swap(stream->middle);
            stream->updated = true;
        }
    }, this, 0));
}
    
void UVCCamera::Subdevice::stop_streaming()
{
    CheckUVC("uvc_stream_stop", uvc_stream_stop(ctrl.handle));
}

} // end namespace rs
