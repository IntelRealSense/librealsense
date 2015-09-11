#include "device.h"
#include "image.h"

#include <climits>
#include <thread>
#include <algorithm>

using namespace rsimpl;

static_device_info rsimpl::add_standard_unpackers(const static_device_info & device_info)
{
    static_device_info info = device_info;
    for(auto & mode : device_info.subdevice_modes)
    {
        // Unstrided YUYV modes can be unpacked into RGB and BGR
        if(mode.fourcc == 'YUY2' && mode.unpacker == &unpack_strided_image && mode.width == mode.streams[0].width && mode.height == mode.streams[0].height)
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

rs_device::rs_device(rsimpl::uvc::device device) : device(device), capturing()
{
    for(auto & req : requests) req = rsimpl::stream_request();
}

rs_device::~rs_device()
{

}

void rs_device::enable_stream(rs_stream stream, int width, int height, rs_format format, int fps)
{
    if(capturing) throw std::runtime_error("streams cannot be reconfigured after having called rs_start_device()");
    if(device_info.stream_subdevices[stream] == -1) throw std::runtime_error("unsupported stream");

    requests[stream] = {true, width, height, format, fps};
    for(auto & s : streams) s.reset(); // Changing stream configuration invalidates the current stream info
}

void rs_device::enable_stream_preset(rs_stream stream, rs_preset preset)
{
    if(capturing) throw std::runtime_error("streams cannot be reconfigured after having called rs_start_device()");
    if(!device_info.presets[stream][preset].enabled) throw std::runtime_error("unsupported stream");

    requests[stream] = device_info.presets[stream][preset];
    for(auto & s : streams) s.reset(); // Changing stream configuration invalidates the current stream info
}

void rs_device::disable_stream(rs_stream stream)
{
    if(capturing) throw std::runtime_error("streams cannot be reconfigured after having called rs_start_device()");
    if(device_info.stream_subdevices[stream] == -1) throw std::runtime_error("unsupported stream");

    requests[stream] = {};
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
    bool stream_enabled[RS_STREAM_COUNT] = {};
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
                stream_enabled[stream_mode.stream] = true;
                    
                // If this is one of the streams requested by the user, store the buffer so they can access it
                if(requests[stream_mode.stream].enabled) streams[stream_mode.stream] = stream;
            }
                
            // Initialize the subdevice and set it to the selected mode
            device.set_subdevice_mode(i, mode.width, mode.height, mode.fourcc, mode.fps, [mode, stream_list](const void * frame) mutable
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
    
    set_stream_intent(stream_enabled);
    device.start_streaming();
    capture_started = std::chrono::high_resolution_clock::now();
    capturing = true;
    base_timestamp = 0;
}

void rs_device::stop()
{
    if(!capturing) throw std::runtime_error("cannot stop device without first starting device");
    device.stop_streaming();
    capturing = false;
}

void rs_device::wait_all_streams()
{
    if(!capturing) return;

    // Determine timeout time
    const auto timeout = std::max(std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(250), capture_started + std::chrono::seconds(2));

    // Check if any of our streams do not have data yet, if so, wait for them to have data, and remember that we are on the first frame
    bool first_frame = false;
    for(int i=0; i<RS_STREAM_COUNT; ++i)
    {
        if(streams[i] && !streams[i]->is_front_valid())
        {
            while(!streams[i]->swap_front())
            {
                std::this_thread::sleep_for(std::chrono::microseconds(100)); // TODO: Use a condition variable or something to avoid this
                //if(std::chrono::high_resolution_clock::now() >= timeout) throw std::runtime_error("Timeout waiting for frames");
            }
            assert(streams[i]->is_front_valid());
            last_stream_timestamp = streams[i]->get_front_number();
            first_frame = true;
        }
    }

    // If this is not the first frame, check for new data on all streams, make sure that at least one stream provides an update
    if(!first_frame)
    {
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
            //if(std::chrono::high_resolution_clock::now() >= timeout) throw std::runtime_error("Timeout waiting for frames");
        }
    }

    // Determine the largest change from the previous timestamp
    int frame_delta = INT_MIN;    
    for(int i=0; i<RS_STREAM_COUNT; ++i)
    {
        if(!streams[i]) continue;
        int delta = streams[i]->get_front_number() - last_stream_timestamp; // Relying on undefined behavior: 32-bit integer overflow -> wraparound
        frame_delta = std::max(frame_delta, delta);
    }

    // Wait for any stream which expects a new frame prior to the frame time
    for(int i=0; i<RS_STREAM_COUNT; ++i)
    {
        if(!streams[i]) continue;
        int next_timestamp = streams[i]->get_front_number() + streams[i]->get_front_delta();
        int next_delta = next_timestamp - last_stream_timestamp;
        if(next_delta > frame_delta) continue;
        while(!streams[i]->swap_front())
        {
            std::this_thread::sleep_for(std::chrono::microseconds(100)); // TODO: Use a condition variable or something to avoid this
            //if(std::chrono::high_resolution_clock::now() >= timeout) throw std::runtime_error("Timeout waiting for frames");
        }
    }

    if(first_frame)
    {
        // Set time 0 to the least recent of the current frames
        base_timestamp = 0;
        for(int i=0; i<RS_STREAM_COUNT; ++i)
        {
            if(!streams[i]) continue;
            int delta = streams[i]->get_front_number() - last_stream_timestamp; // Relying on undefined behavior: 32-bit integer overflow -> wraparound
            if(delta < 0) last_stream_timestamp = streams[i]->get_front_number();
        }
    }
    else
    {
        base_timestamp += frame_delta;
        last_stream_timestamp += frame_delta;
    }    
}

int rs_device::get_frame_timestamp(rs_stream stream) const 
{ 
    if(!streams[stream]) throw std::runtime_error("stream not enabled"); 
    return base_timestamp == -1 ? 0 : convert_timestamp(base_timestamp + streams[stream]->get_front_number() - last_stream_timestamp);
}

rsimpl::stream_mode rs_device::get_current_stream_mode(rs_stream stream) const
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

rs_extrinsics rs_device::get_extrinsics(rs_stream from, rs_stream to) const
{
    auto transform = inverse(device_info.stream_poses[from]) * device_info.stream_poses[to];
    rs_extrinsics extrin;
    (float3x3 &)extrin.rotation = transform.orientation;
    (float3 &)extrin.translation = transform.position;
    return extrin;
}

namespace rsimpl
{
    std::vector<stream_mode> enumerate_stream_modes(const static_device_info & device_info, rs_stream stream)
    {
        std::vector<stream_mode> modes;
        for(auto & subdevice_mode : device_info.subdevice_modes)
        {
            for(auto & stream_mode : subdevice_mode.streams)
            {
                if(stream_mode.stream == stream)
                {
                    modes.push_back(stream_mode);
                }
            }
        }

        std::sort(begin(modes), end(modes), [](const stream_mode & a, const stream_mode & b)
        {
            return std::make_tuple(-a.width, -a.height, -a.fps, a.format) < std::make_tuple(-b.width, -b.height, -b.fps, b.format);
        });

        auto it = std::unique(begin(modes), end(modes), [](const stream_mode & a, const stream_mode & b)
        {
            return std::make_tuple(a.width, a.height, a.fps, a.format) == std::make_tuple(b.width, b.height, b.fps, b.format);
        });
        if(it != end(modes)) modes.erase(it, end(modes));

        return modes;
    }
}

int rs_device::get_stream_mode_count(rs_stream stream) const
{
    return enumerate_stream_modes(device_info, stream).size();
}

void rs_device::get_stream_mode(rs_stream stream, int mode, int * width, int * height, rs_format * format, int * framerate) const
{
    auto m = enumerate_stream_modes(device_info, stream)[mode];
    if(width) *width = m.width;
    if(height) *height = m.height;
    if(format) *format = m.format;
    if(framerate) *framerate = m.fps;
}