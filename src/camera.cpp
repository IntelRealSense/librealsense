#include "rs-internal.h"
#include "image.h"

using namespace rs;

////////////////
// UVC Camera //
////////////////

rs_camera::rs_camera(uvc_context_t * context, uvc_device_t * device) : context(context), device(device), first_handle()
{
    uvc_device_descriptor_t * desc;
    CheckUVC("uvc_get_device_descriptor", uvc_get_device_descriptor(device, &desc));

    for(auto & req : requests) req = {};

    cameraName = desc->product;
    // Other interesting properties
    // desc->serialNumber
    // desc->idVendor
    // desc->idProduct

    uvc_free_device_descriptor(desc);  

    calib = {};
}

rs_camera::~rs_camera()
{
    subdevices.clear();
    uvc_unref_device(device);
}

void rs_camera::EnableStream(int stream, int width, int height, int fps, int format)
{
    requests[stream] = {true, width, height, format, fps};
}

bool choose_mode(std::vector<SubdeviceMode> & dest, std::array<StreamRequest, MAX_STREAMS> requests, const std::vector<SubdeviceMode> & modes, int subdevice)
{
    if(dest.size() == subdevice)
    {
        for(auto & req : requests) if(req.enabled) return false; // This request was not satisfied
        return true; // Otherwise all requests are satisfied and we have chosen a mode for all subdevices. Win!
    }

    for(auto & mode : modes)
    {
        if(mode.subdevice != subdevice) continue;

        bool valid = true;
        auto reqs = requests;
        for(auto & stream_mode : mode.streams)
        {
            auto & req = reqs[stream_mode.stream];
            if(req.enabled)
            {
                if(req.width != stream_mode.width || req.height != stream_mode.height || req.format != stream_mode.format || req.fps != stream_mode.fps)
                {
                    valid = false;
                    break;
                }
                req.enabled = false;
            }
        }
        if(valid)
        {
            dest[subdevice] = mode;
            if(choose_mode(dest, reqs, modes, subdevice + 1)) return true;
        }
    }

    return false;
}

void rs_camera::StartStreaming()
{
    if (first_handle == nullptr)
    {
        // Open subdevice handles and retrieve calibration
        for(int i=0; i<subdevices.size(); ++i) subdevices[i].reset(new Subdevice(device, i));
        first_handle = subdevices[0]->get_handle();
        calib = RetrieveCalibration();
    }

    // Choose suitable modes for all subdevices
    std::vector<SubdeviceMode> modes(subdevices.size());
    if(!choose_mode(modes, requests, calib.modes, 0)) throw std::runtime_error("bad stream combination");

    // Set chosen modes and open streams
    for(size_t i=0; i<subdevices.size(); ++i)
    {
        std::vector<std::shared_ptr<Stream>> mode_streams;
        for(auto & stream_mode : modes[i].streams)
        {
            streams[stream_mode.stream].reset(new Stream());
            mode_streams.push_back(streams[stream_mode.stream]);
        }
        subdevices[i]->set_mode(modes[i], mode_streams);
    }

    // Shut off user access to streams that were not requested
    for(int i=0; i<MAX_STREAMS; ++i) if(!requests[i].enabled) streams[i].reset();

    // Start streaming
    SetStreamIntent();
    
    for(auto & subdevice : subdevices) subdevice->start_streaming();
    isCapturing = true;
}

void rs_camera::StopStreaming()
{
    for(auto & subdevice : subdevices) subdevice->stop_streaming();
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

rs_intrinsics rs_camera::GetStreamIntrinsics(int stream) const
{
    if(!streams[stream]) throw std::runtime_error("stream not enabled");
    return calib.intrinsics[streams[stream]->get_mode().intrinsics_index];
}

rs_extrinsics rs_camera::GetStreamExtrinsics(int from, int to) const
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
    case RS_Z16: front.resize(mode.width * mode.height * sizeof(uint16_t)); break;
    case RS_RGB8: front.resize(mode.width * mode.height * 3); break;
    case RS_Y8: front.resize(mode.width * mode.height); break;
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

namespace rs
{
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

    void unpack_strided_image(void * dest[], const SubdeviceMode & mode, const void * source)
    {
        assert(mode.streams.size() == 1);
        copy_strided_image(dest[0], mode.streams[0].width * get_pixel_size(mode.streams[0].format), source, mode.width * get_pixel_size(mode.streams[0].format), mode.streams[0].height);
    }

    void unpack_rly12_to_y8(void * dest[], const SubdeviceMode & mode, const void * frame)
    {
        assert(mode.format == UVC_FRAME_FORMAT_Y12I && mode.streams.size() == 2 && mode.streams[0].format == RS_Y8 && mode.streams[1].format == RS_Y8);
        convert_rly12_to_y8_y8(dest[0], dest[1], mode.streams[0].width, mode.streams[0].height, frame, 3*mode.width);
    }

    void unpack_yuyv_to_rgb(void * dest[], const SubdeviceMode & mode, const void * source)
    {
        assert(mode.format == UVC_FRAME_FORMAT_YUYV && mode.streams.size() == 1 && mode.streams[0].format == RS_RGB8);
        convert_yuyv_to_rgb((uint8_t *) dest[0], mode.width, mode.height, source);
    }
} // end namespace rs
