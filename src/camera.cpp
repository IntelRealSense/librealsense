#include "camera.h"
#include "image.h"

#include <iostream>

using namespace rsimpl;

////////////////
// UVC Camera //
////////////////

rs_camera::rs_camera(uvc_context_t * context, uvc_device_t * device, const static_camera_info & camera_info)
    : context(context), device(device), camera_info(camera_info), first_handle(), is_capturing()
{
    {
        uvc_device_descriptor_t * desc;
        CheckUVC("uvc_get_device_descriptor", uvc_get_device_descriptor(device, &desc));
        camera_name = desc->product;
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

void rs_camera::enable_stream(rs_stream stream, int width, int height, rs_format format, int fps)
{
    if(first_handle) throw std::runtime_error("streams cannot be reconfigured after having called start_capture()");
    if(camera_info.stream_subdevices[stream] == -1) throw std::runtime_error("unsupported stream");
    requests[stream] = {true, width, height, format, fps};
}

void rs_camera::enable_stream_preset(rs_stream stream, rs_preset preset)
{
    if(first_handle) throw std::runtime_error("streams cannot be reconfigured after having called start_capture()");
    if(!camera_info.presets[stream][preset].enabled) throw std::runtime_error("unsupported stream");
    requests[stream] = camera_info.presets[stream][preset];
}

void rs_camera::configure_enabled_streams()
{
    for(auto & s : subdevices) s.reset();
    for(auto & s : streams) s.reset();
    
    // First create the subdevices and assign first_handle as appropriate
    // Creating a subdevice_handle will reach out to the hardware and open the handle
    if (!first_handle)
    {
        // Satisfy stream_requests as necessary for each subdevice, calling set_mode and
        // dispatching the uvc configuration for a requested stream to the hardware
        for(int i = 0; i < subdevices.size(); ++i)
        {
            if(const subdevice_mode * mode = camera_info.select_mode(requests, i))
            {
                subdevices[i].reset(new subdevice_handle(device, i));
                if (!first_handle) first_handle = subdevices[i]->get_handle();

                // For each stream provided by this mode
                std::vector<std::shared_ptr<stream_buffer>> stream_list;
                for(auto & stream_mode : mode->streams)
                {
                    // Create a buffer to receive the images from this stream
                    auto stream = std::make_shared<stream_buffer>();
                    stream_list.push_back(stream);
                    
                    // If this is one of the streams requested by the user, store the buffer so they can access it
                    if(requests[stream_mode.stream].enabled) streams[stream_mode.stream] = stream;
                }
                
                // Initialize the subdevice and set it to the selected mode, then exit the loop early
                subdevices[i]->set_mode(*mode, stream_list);
            }
        }
    }
    
    // If we have not yet retrieved calibration info and at least one subdevice is open, do so
    if(calib.intrinsics.empty() && first_handle)
    {
        calib = retrieve_calibration();
        uvc_print_diag(first_handle, stdout);
    }
}

void rs_camera::start_capture()
{
    if (!first_handle) configure_enabled_streams();
    
    set_stream_intent();
    for(auto & subdevice : subdevices) if (subdevice) subdevice->start_streaming();
    is_capturing = true;
}

void rs_camera::stop_capture()
{
    for(auto & subdevice : subdevices) if (subdevice) subdevice->stop_streaming();
    is_capturing = false;
}

void rs_camera::wait_all_streams()
{
    if (!is_capturing) return;
    
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

rs_intrinsics rs_camera::get_stream_intrinsics(rs_stream stream) const
{
    if(!streams[stream]) throw std::runtime_error("stream not enabled");
    return calib.intrinsics[streams[stream]->get_mode().intrinsics_index];
}

rs_extrinsics rs_camera::get_stream_extrinsics(rs_stream from, rs_stream to) const
{
    auto transform = inverse(calib.stream_poses[from]) * calib.stream_poses[to]; // TODO: Make sure this is the right order
    rs_extrinsics extrin;
    (float3x3 &)extrin.rotation = transform.orientation;
    (float3 &)extrin.translation = transform.position;
    return extrin;
}

///////////////////
// stream_buffer //
///////////////////

void rs_camera::stream_buffer::set_mode(const stream_mode & mode)
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

bool rs_camera::stream_buffer::update_image()
{
    if(!updated) return false;
    std::lock_guard<std::mutex> guard(mutex);
    front.swap(middle);
    updated = false;
    return true;
}

//////////////////////
// subdevice_handle //
//////////////////////

rs_camera::subdevice_handle::subdevice_handle(uvc_device_t * device, int subdevice_index)
{
    CheckUVC("uvc_open2", uvc_open2(device, &handle, subdevice_index));
}

rs_camera::subdevice_handle::~subdevice_handle()
{
    // uvc_stop_streaming(handle);
    uvc_close(handle);
}

void rs_camera::subdevice_handle::set_mode(const subdevice_mode & mode, std::vector<std::shared_ptr<stream_buffer>> streams)
{
    assert(mode.streams.size() == streams.size());
    CheckUVC("uvc_get_stream_ctrl_format_size", uvc_get_stream_ctrl_format_size(handle, &ctrl, mode.format, mode.width, mode.height, mode.fps));

    uvc_print_stream_ctrl(&ctrl, stdout);
    
    this->mode = mode;
    this->streams = streams;
    for(size_t i=0; i<mode.streams.size(); ++i) streams[i]->set_mode(mode.streams[i]);
}

void rs_camera::subdevice_handle::start_streaming()
{
    CheckUVC("uvc_start_streaming", uvc_start_streaming(handle, &ctrl, [](uvc_frame_t * frame, void * ptr)
    {
        // Validate that this frame matches the mode information we've set
        auto self = reinterpret_cast<subdevice_handle *>(ptr);
        assert((int)frame->width == self->mode.width && (int)frame->height == self->mode.height && frame->frame_format == self->mode.format);

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
    
void rs_camera::subdevice_handle::stop_streaming()
{
    uvc_stop_streaming(handle);
}
