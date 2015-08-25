#include "rs-internal.h"
#include "image.h"

using namespace rs;

////////////////
// UVC Camera //
////////////////

rs_camera::rs_camera(uvc_context_t * context, uvc_device_t * device, const StaticCameraInfo & camera_info)
    : context(context), device(device), camera_info(camera_info), first_handle()
{
    uvc_device_descriptor_t * desc;
    CheckUVC("uvc_get_device_descriptor", uvc_get_device_descriptor(device, &desc));
    cameraName = desc->product;
    // Other interesting properties
    // desc->serialNumber
    // desc->idVendor
    // desc->idProduct
    uvc_free_device_descriptor(desc);  

    int max_subdevice = 0;
    for(auto & mode : camera_info.subdevice_modes) max_subdevice = std::max(max_subdevice, mode.subdevice);
    subdevices.resize(max_subdevice+1);

    for(auto & req : requests) req = {};

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

#include <algorithm>

void rs_camera::StartStreaming()
{
    for(auto & s : subdevices) s.reset();
    for(auto & s : streams) s.reset();
    first_handle = nullptr;

    // For each subdevice
    for(int i = 0; i < subdevices.size(); ++i)
    {
        // Determine if the user has requested any streams which are supplied by this subdevice
        bool any_stream_requested = false;
        std::array<bool, MAX_STREAMS> stream_requested = {};
        for(int j = 0; j < MAX_STREAMS; ++j)
        {
            if(requests[j].enabled && camera_info.stream_subdevices[j] == i)
            {
                stream_requested[j] = true;
                any_stream_requested = true;
            }
        }

        // If no streams were requested, skip to the next subdevice
        if(!any_stream_requested) continue;

        // Look for an appropriate mode
        for(auto & subdevice_mode : camera_info.subdevice_modes)
        {
            // Determine if this mode satisfies the requirements on our requested streams
            auto stream_unsatisfied = stream_requested;
            for(auto & stream_mode : subdevice_mode.streams)
            {
                const auto & req = requests[stream_mode.stream];
                if(req.enabled && req.width == stream_mode.width && req.height == stream_mode.height && req.format == stream_mode.format && req.fps == stream_mode.fps)
                {
                    stream_unsatisfied[stream_mode.stream] = false;
                }
            }

            // If any requested streams are still unsatisfied, skip to the next mode
            if(std::any_of(begin(stream_unsatisfied), end(stream_unsatisfied), [](bool b) { return b; })) continue;

            // For each stream provided by this mode
            std::vector<std::shared_ptr<Stream>> stream_list;
            for(auto & stream_mode : subdevice_mode.streams)
            {
                // Create a buffer to receive the images from this stream
                auto stream = std::make_shared<Stream>();
                stream_list.push_back(stream);

                // If this is one of the streams requested by the user, store the buffer so they can access it
                if(stream_requested[stream_mode.stream])
                {
                    streams[stream_mode.stream] = stream;
                }
            }

            // Initialize the subdevice and set it to the selected mode, then exit the loop early
            subdevices[i].reset(new Subdevice(device, i));
            subdevices[i]->set_mode(subdevice_mode, stream_list);
            if(!first_handle) first_handle = subdevices[i]->get_handle();
            break;
        }

        // If we did not find an appropriate mode, report an error
        if(!subdevices[i])
        {
            std::ostringstream ss;
            ss << "uvc subdevice " << i << " cannot provide";
            bool first = true;
            for(int j = 0; j < MAX_STREAMS; ++j)
            {
                if(!stream_requested[j]) continue;
                ss << (first ? " " : " and ");

                ss << requests[j].width << 'x' << requests[j].height << ':';
                switch(requests[j].format)
                {
                case RS_Z16: ss << "Z16"; break;
                case RS_RGB8: ss << "RGB8"; break;
                case RS_Y8: ss << "Y8"; break;
                }
                ss << '@' << requests[j].fps << "Hz ";
                switch(j)
                {
                case RS_DEPTH: ss << "DEPTH"; break;
                case RS_COLOR: ss << "COLOR"; break;
                case RS_INFRARED: ss << "INFRARED"; break;
                case RS_INFRARED_2: ss << "INFRARED_2"; break;
                }
                first = false;
            }
            throw std::runtime_error(ss.str());
        }
    }

    // If we have not yet retrieved calibration info and at least one subdevice is open, do so
    if(calib.intrinsics.empty() && first_handle)
    {
        calib = RetrieveCalibration();
    }

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
