#include "device.h"
#include "image.h"

#include <thread>
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

rs_device::rs_device(rsimpl::uvc::device device, const static_device_info & device_info) : device(device), device_info(add_standard_unpackers(device_info)), capturing()
{
    for(auto & req : requests) req = rsimpl::stream_request();
}

rs_device::~rs_device()
{

}

void rs_device::enable_stream(rs_stream stream, int width, int height, rs_format format, int fps)
{
    if(capturing) throw std::runtime_error("streams cannot be reconfigured after having called start_capture()");
    if(device_info.stream_subdevices[stream] == -1) throw std::runtime_error("unsupported stream");

    requests[stream] = {true, width, height, format, fps};
    for(auto & s : streams) s.reset(); // Changing stream configuration invalidates the current stream info
}

void rs_device::enable_stream_preset(rs_stream stream, rs_preset preset)
{
    if(capturing) throw std::runtime_error("streams cannot be reconfigured after having called start_capture()");
    if(!device_info.presets[stream][preset].enabled) throw std::runtime_error("unsupported stream");

    requests[stream] = device_info.presets[stream][preset];
    for(auto & s : streams) s.reset(); // Changing stream configuration invalidates the current stream info
} 

void rs_device::start()
{
    if(capturing) throw std::runtime_error("cannot restart device without first stopping device");
        
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

    for(auto & s : streams) s.reset(); // Starting capture invalidates the current stream info, if any exists from previous capture

    int max_subdevice = 0;
    for(auto & mode : device_info.subdevice_modes) max_subdevice = std::max(max_subdevice, mode.subdevice);

    // Satisfy stream_requests as necessary for each subdevice, calling set_mode and
    // dispatching the uvc configuration for a requested stream to the hardware
    for(int i = 0; i <= max_subdevice; ++i)
    {
        if(const subdevice_mode * m = device_info.select_mode(requests, i))
        {               
            // Create a stream buffer for each stream served by this subdevice mode
            const subdevice_mode mode = *m;
            std::vector<std::shared_ptr<stream_buffer>> stream_list;
            for(auto & stream_mode : mode.streams)
            {
                // Create a buffer to receive the images from this stream
                auto stream = std::make_shared<stream_buffer>(stream_mode);
                stream_list.push_back(stream);
                    
                // If this is one of the streams requested by the user, store the buffer so they can access it
                if(requests[stream_mode.stream].enabled) streams[stream_mode.stream] = stream;
            }
                
            // Initialize the subdevice and set it to the selected mode
            device.set_subdevice_mode(i, mode.width, mode.height, mode.format, mode.fps, [mode, stream_list](const void * frame) mutable
            {
                // Unpack the image into the user stream interface back buffer
                std::vector<void *> dest;
                for(auto & stream : stream_list) dest.push_back(stream->get_back_data());
                mode.unpacker(dest.data(), mode, frame);
                const int frame_number = mode.frame_number_decoder(mode, frame);
                
                // Swap the backbuffer to the middle buffer and indicate that we have updated
                for(auto & stream : stream_list) stream->swap_back(frame_number);
            });
        }
    }
    
    set_stream_intent();
    device.start_streaming();
    capture_started = std::chrono::high_resolution_clock::now();
    capturing = true;
}

void rs_device::stop()
{
    if(!capturing) throw std::runtime_error("cannot stop device without first starting device");
    device.stop_streaming();
    capturing = false;
}

void rs_device::wait_all_streams()
{
    if (!capturing) return;
    
    // Determine timeout time
    const auto timeout = std::max(std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(250), capture_started + std::chrono::seconds(2));

    // Check for new data on all streams, make sure that at least one stream provides an update
    bool updated = false;
    while(true)
    {
        for(int i=0; i<RS_STREAM_COUNT; ++i)
        {
            if(streams[i] && streams[i]->swap_front())
            {
                updated = true;
            }
        }
        if(updated) break;

        std::this_thread::sleep_for(std::chrono::microseconds(100)); // TODO: Use a condition variable or something to avoid this
        if(std::chrono::high_resolution_clock::now() >= timeout) throw std::runtime_error("Timeout waiting for frames");
    }

    // The frame time will be the most recent timestamp available on any stream
    int frame_time = INT_MIN;
    for(int i=0; i<RS_STREAM_COUNT; ++i)
    {
        if(streams[i])
        {
            frame_time = std::max(frame_time, streams[i]->get_front_number());
        }
    }

    // Wait for any stream which expects a new frame prior to the frame time
    for(int i=0; i<RS_STREAM_COUNT; ++i)
    {
        if(streams[i] && streams[i]->get_front_number() + streams[i]->get_front_delta() <= frame_time)
        {
            while(true)
            {
                if(streams[i]->swap_front()) break;
                std::this_thread::sleep_for(std::chrono::microseconds(100)); // TODO: Use a condition variable or something to avoid this
                if(std::chrono::high_resolution_clock::now() >= timeout) throw std::runtime_error("Timeout waiting for frames");
            }
        }
    }
}

rsimpl::stream_mode rs_device::get_stream_mode(rs_stream stream) const
{
    if(streams[stream]) return streams[stream]->get_mode();
    if(requests[stream].enabled)
    {
        if(auto subdevice_mode = device_info.select_mode(requests, device_info.stream_subdevices[stream]))
        {
            for(auto & mode : subdevice_mode->streams)
            {
                if(mode.stream == stream) return mode;
            }
        }   
        throw std::logic_error("no mode found"); // Should never happen, select_mode should throw if no mode can be found
    }
    throw std::runtime_error("stream not enabled");
}

rs_extrinsics rs_device::get_stream_extrinsics(rs_stream from, rs_stream to) const
{
    auto transform = inverse(device_info.stream_poses[from]) * device_info.stream_poses[to];
    rs_extrinsics extrin;
    (float3x3 &)extrin.rotation = transform.orientation;
    (float3 &)extrin.translation = transform.position;
    return extrin;
}