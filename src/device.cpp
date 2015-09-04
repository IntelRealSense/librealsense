#include "device.h"
#include "image.h"

#include <iostream>
#include <algorithm>

using namespace rsimpl;

static static_device_info add_standard_unpackers(const static_device_info & device_info)
{
    static_device_info info = device_info;
    for(auto & mode : device_info.subdevice_modes)
    {
        // Unstrided YUYV modes can be unpacked into RGB and BGR
        if(mode.format == uvc::frame_format::YUYV && mode.unpacker == &unpack_strided_image && mode.width == mode.streams[0].width && mode.height == mode.streams[0].height)
        {
            auto m = mode;
            m.streams[0].format = RS_FORMAT_RGB8;
            m.unpacker = &unpack_yuyv_to_rgb;
            info.subdevice_modes.push_back(m);

            m.streams[0].format = RS_FORMAT_BGR8;
            m.unpacker = &unpack_yuyv_to_bgr;
            info.subdevice_modes.push_back(m);

            m.streams[0].format = RS_FORMAT_RGBA8;
            m.unpacker = &unpack_yuyv_to_rgba;
            info.subdevice_modes.push_back(m);

            m.streams[0].format = RS_FORMAT_BGRA8;
            m.unpacker = &unpack_yuyv_to_bgra;
            info.subdevice_modes.push_back(m);
        }
    }
    return info;
}

rs_device::rs_device(rsimpl::uvc::device device, const static_device_info & device_info) : device(device), device_info(add_standard_unpackers(device_info)), first_handle(), capturing()
{
    for(auto & req : requests) req = rsimpl::stream_request();
    calib = {};

    int max_subdevice = 0;
    for(auto & mode : device_info.subdevice_modes) max_subdevice = std::max(max_subdevice, mode.subdevice);
    subdevices.resize(max_subdevice+1);
}

rs_device::~rs_device()
{
    subdevices.clear();
}

void rs_device::enable_stream(rs_stream stream, int width, int height, rs_format format, int fps)
{
    if(first_handle) throw std::runtime_error("streams cannot be reconfigured after having called start_capture()");
    if(device_info.stream_subdevices[stream] == -1) throw std::runtime_error("unsupported stream");
    requests[stream] = {true, width, height, format, fps};
}

void rs_device::enable_stream_preset(rs_stream stream, rs_preset preset)
{
    if(first_handle) throw std::runtime_error("streams cannot be reconfigured after having called start_capture()");
    if(!device_info.presets[stream][preset].enabled) throw std::runtime_error("unsupported stream");
    requests[stream] = device_info.presets[stream][preset];
}

void rs_device::configure_enabled_streams()
{
    for(auto & s : subdevices) s.reset();
    for(auto & s : streams) s.reset();
    
    // First create the subdevices and assign first_handle as appropriate
    // Creating a subdevice_handle will reach out to the hardware and open the handle
    if (!first_handle)
    {
        // Check and modify our requests to enforce all interstream constraints
        for(auto & rule : device_info.interstream_rules)
        {
            auto & a = requests[rule.a], & b = requests[rule.b]; auto f = rule.field;
            if(a.enabled && b.enabled)
            {
                // Check for incompatibility if both values specified
                if(a.*f != 0 && b.*f != 0 && a.*f + rule.delta != b.*f)
                {
                    throw std::runtime_error(to_string() << "requested " << rule.a << " and " << rule.b << " settings are incompatible");
                }

                // If only one value is specified, modify the other request to match
                if(a.*f != 0 && b.*f == 0) b.*f = a.*f + rule.delta;
                if(a.*f == 0 && b.*f != 0) a.*f = b.*f - rule.delta;
            }
        }

        // Satisfy stream_requests as necessary for each subdevice, calling set_mode and
        // dispatching the uvc configuration for a requested stream to the hardware
        for(int i = 0; i < subdevices.size(); ++i)
        {
            if(const subdevice_mode * mode = device_info.select_mode(requests, i))
            {
                subdevices[i].reset(new subdevice_handle(device, i));
                if (!first_handle) first_handle = subdevices[i]->get_handle();
                
                #if defined(ENABLE_DEBUG_SPAM)
                uvc_print_diag(subdevices[i]->get_handle(), stdout);
                #endif
                
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
    }
}

void rs_device::start_capture()
{
    if (!first_handle) configure_enabled_streams();
    
    set_stream_intent();
    for(auto & subdevice : subdevices) if (subdevice) subdevice->start_streaming();
    capturing = true;
}

void rs_device::stop_capture()
{
    for(auto & subdevice : subdevices) if (subdevice) subdevice->stop_streaming();
    capturing = false;
}

void rs_device::wait_all_streams()
{
    if (!capturing) return;
    
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

rs_intrinsics rs_device::get_stream_intrinsics(rs_stream stream) const
{
    if(!streams[stream]) throw std::runtime_error("stream not enabled");
    return calib.intrinsics[streams[stream]->get_mode().intrinsics_index];
}

rs_extrinsics rs_device::get_stream_extrinsics(rs_stream from, rs_stream to) const
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

void rs_device::stream_buffer::set_mode(const stream_mode & mode)
{
    this->mode = mode;
    front.pixels.resize(get_image_size(mode.width, mode.height, mode.format));
    front.number = 0;
    back = middle = front;
    updated = false;
}

bool rs_device::stream_buffer::update_image()
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

rs_device::subdevice_handle::subdevice_handle(uvc::device device, int subdevice_index) : handle(device.claim_subdevice(subdevice_index))
{

}

rs_device::subdevice_handle::~subdevice_handle()
{
    stop_streaming();
}

void rs_device::subdevice_handle::set_mode(const subdevice_mode & mode, std::vector<std::shared_ptr<stream_buffer>> streams)
{
    assert(mode.streams.size() == streams.size());
    handle.set_mode(mode.width, mode.height, mode.format, mode.fps);
    
    if(!state) state = std::make_shared<capture_state>();
    state->mode = mode;
    state->streams = streams;
    for(size_t i=0; i<mode.streams.size(); ++i) streams[i]->set_mode(mode.streams[i]);
}

void rs_device::subdevice_handle::start_streaming()
{    
    // The callback may actually outlive the subdevice_handle
    // Make sure to capture our state by shared_ptr, not by the this ptr
    auto s = state;
    handle.start_streaming([s](const void * frame)
    {
        // Unpack the image into the user stream interface back buffer
        std::vector<void *> dest;
        for(auto & stream : s->streams) dest.push_back(stream->back.pixels.data());
        s->mode.unpacker(dest.data(), s->mode, frame);

        // If a frame number decoder is available, make use of it
        if(s->mode.frame_number_decoder)
        {
            const int frame_number = s->mode.frame_number_decoder(s->mode, frame);
            for(auto & stream : s->streams) stream->back.number = frame_number;
        }

        // Swap the backbuffer to the middle buffer and indicate that we have updated
        for(auto & stream : s->streams)
        {
            std::lock_guard<std::mutex> guard(stream->mutex);
            stream->back.swap(stream->middle);
            stream->updated = true;
        }
    });
}
    
void rs_device::subdevice_handle::stop_streaming()
{
    handle.stop_streaming();
}
