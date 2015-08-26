#include "rs-internal.h"
#include "image.h"

using namespace rsimpl;

////////////////
// UVC Camera //
////////////////

rs_camera::rs_camera(uvc_context_t * context, uvc_device_t * device, const StaticCameraInfo & camera_info)
    : context(context), device(device), camera_info(camera_info), first_handle()
{
    {
        uvc_device_descriptor_t * desc;
        CheckUVC("uvc_get_device_descriptor", uvc_get_device_descriptor(device, &desc));
        cameraName = desc->product;
        uvc_free_device_descriptor(desc);
    }

    for(auto & req : requests) req = {};
    calib = {};

    int max_subdevice = 0;
    for(auto & mode : camera_info.subdevice_modes) max_subdevice = std::max(max_subdevice, mode.subdevice);
    subdevices.resize(max_subdevice+1);
}

rs_camera::~rs_camera()
{
    subdevices.clear();
    //uvc_unref_device(device); // we never ref
}

void rs_camera::EnableStream(rs_stream stream, int width, int height, rs_format format, int fps)
{
    if(camera_info.stream_subdevices[stream] == -1) throw std::runtime_error("unsupported stream");
    requests[stream] = {true, width, height, format, fps};
}

void rs_camera::StartStreaming()
{
    for(auto & s : subdevices) s.reset();
    for(auto & s : streams) s.reset();
    first_handle = nullptr;

    // For each subdevice
    for(int i = 0; i < subdevices.size(); ++i)
    {
        if(const SubdeviceMode * mode = camera_info.select_mode(requests, i))
        {
            // For each stream provided by this mode
            std::vector<std::shared_ptr<Stream>> stream_list;
            for(auto & stream_mode : mode->streams)
            {
                // Create a buffer to receive the images from this stream
                auto stream = std::make_shared<Stream>();
                stream_list.push_back(stream);

                // If this is one of the streams requested by the user, store the buffer so they can access it
                if(requests[stream_mode.stream].enabled) streams[stream_mode.stream] = stream;
            }

            // Initialize the subdevice and set it to the selected mode, then exit the loop early
            subdevices[i].reset(new Subdevice(device, i));
            subdevices[i]->set_mode(*mode, stream_list);
            if(!first_handle) first_handle = subdevices[i]->get_handle();
        }
    }

    // If we have not yet retrieved calibration info and at least one subdevice is open, do so
    if(calib.intrinsics.empty() && first_handle)
    {
        calib = RetrieveCalibration();
    }

    SetStreamIntent();
    for(auto & subdevice : subdevices) if(subdevice) subdevice->start_streaming();
    isCapturing = true;
}

void rs_camera::StopStreaming()
{
    for(auto & subdevice : subdevices) if(subdevice) subdevice->stop_streaming();
    isCapturing = false;
}
    
void rs_camera::WaitAllStreams()
{
    if (!isCapturing) return;
    
    int maxFps = 0;
    for(auto & stream : streams) maxFps = stream ? std::max(maxFps, stream->get_mode().fps) : maxFps;

    for(auto & stream : streams)
    {
        if(stream)
        {
            // If this is the fastest stream, wait until a new frame arrives
            if(stream->get_mode().fps == maxFps)
            {
                while(true) if(stream->update_image()) break; // ok, this just blocks the entire loop on stop
            }
            else // Otherwise simply check for a new frame
            {
                stream->update_image();
            }
        }
    }
}

rs_intrinsics rs_camera::GetStreamIntrinsics(rs_stream stream) const
{
    if(!streams[stream]) throw std::runtime_error("stream not enabled");
    return calib.intrinsics[streams[stream]->get_mode().intrinsics_index];
}

rs_extrinsics rs_camera::GetStreamExtrinsics(rs_stream from, rs_stream to) const
{
    auto transform = inverse(calib.stream_poses[from]) * calib.stream_poses[to]; // TODO: Make sure this is the right order
    rs_extrinsics extrin;
    (float3x3 &)extrin.rotation = transform.orientation;
    (float3 &)extrin.translation = transform.position;
    return extrin;
}

void rs_camera::Stream::set_mode(const StreamMode & mode)
{
    this->mode = mode;
    switch(mode.format)
    {
    case RS_FORMAT_Z16: front.resize(mode.width * mode.height * sizeof(uint16_t)); break;
    case RS_FORMAT_RGB8: front.resize(mode.width * mode.height * 3); break;
    case RS_FORMAT_Y8: front.resize(mode.width * mode.height); break;
    default: throw std::runtime_error("invalid format");
    }
    back = middle = front;
    updated = false;
}

bool rs_camera::Stream::update_image()
{
    if(!updated) return false;
    std::lock_guard<std::mutex> guard(mutex);
    front.swap(middle);
    updated = false;
    return true;
}

void rs_camera::Subdevice::set_mode(const SubdeviceMode & mode, std::vector<std::shared_ptr<Stream>> streams)
{
    assert(mode.streams.size() == streams.size());
    CheckUVC("uvc_get_stream_ctrl_format_size", uvc_get_stream_ctrl_format_size(uvcHandle, &ctrl, mode.format, mode.width, mode.height, mode.fps));

    this->mode = mode;
    this->streams = streams;
    for(size_t i=0; i<mode.streams.size(); ++i) streams[i]->set_mode(mode.streams[i]);
}

void rs_camera::Subdevice::start_streaming()
{
    CheckUVC("uvc_start_streaming", uvc_start_streaming(uvcHandle, &ctrl, [](uvc_frame_t * frame, void * ptr)
    {
        // Validate that this frame matches the mode information we've set
        auto self = reinterpret_cast<Subdevice *>(ptr);
        assert(frame->width == self->mode.width && frame->height == self->mode.height && frame->frame_format == self->mode.format);

        // Unpack the image into the user stream interface back buffer
        std::vector<void *> dest;
        for(auto & stream : self->streams) dest.push_back(stream->back.data());
        self->mode.unpacker(dest.data(), self->mode, frame->data);

        // Swap the backbuffer to the middle buffer and indicate that we have updated
        for(auto & stream : self->streams)
        {
            std::lock_guard<std::mutex> guard(stream->mutex);
            stream->back.swap(stream->middle);
            stream->updated = true;
        }
    }, this, 0));
}
    
void rs_camera::Subdevice::stop_streaming()
{
    CheckUVC("uvc_stream_stop", uvc_stream_stop(ctrl.handle));
}
