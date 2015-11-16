#include "device.h"
#include "image.h"

#include <cstring>
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
        if(mode.pf->fourcc == 'YUY2' && mode.unpacker == &unpack_subrect && mode.width == mode.streams[0].width && mode.height == mode.streams[0].height)
        {
            auto m = mode;
            m.streams[0].format = RS_FORMAT_RGB8;
            m.unpacker = &unpack_rgb_from_yuy2;
            info.subdevice_modes.push_back(m);

            m.streams[0].format = RS_FORMAT_BGR8;
            m.unpacker = &unpack_bgr_from_yuy2;
            info.subdevice_modes.push_back(m);

            m.streams[0].format = RS_FORMAT_RGBA8;
            m.unpacker = &unpack_rgba_from_yuy2;
            info.subdevice_modes.push_back(m);

            m.streams[0].format = RS_FORMAT_BGRA8;
            m.unpacker = &unpack_bgra_from_yuy2;
            info.subdevice_modes.push_back(m);
        }
    }

    // Flag all standard options as supported
    for(int i=0; i<RS_OPTION_COUNT; ++i)
    {
        if(uvc::is_pu_control((rs_option)i))
        {
            info.option_supported[i] = true;
        }
    }

    return info;
}

rs_device::rs_device(std::shared_ptr<rsimpl::uvc::device> device, const rsimpl::static_device_info & info) : device(device), device_info(add_standard_unpackers(info)), capturing(false)
{
    for(auto & req : requests) req = rsimpl::stream_request();
    for(auto & t : synthetic_timestamps) t = 0;
}

rs_device::~rs_device()
{

}

void rs_device::enable_stream(rs_stream stream, int width, int height, rs_format format, int fps)
{
    if(capturing) throw std::runtime_error("streams cannot be reconfigured after having called rs_start_device()");
    if(device_info.stream_subdevices[stream] == -1) throw std::runtime_error("unsupported stream");

    requests[stream] = {true, width, height, format, fps};
    for(auto & s : native_streams) s.buffer.reset(); // Changing stream configuration invalidates the current stream info
}

void rs_device::enable_stream_preset(rs_stream stream, rs_preset preset)
{
    if(capturing) throw std::runtime_error("streams cannot be reconfigured after having called rs_start_device()");
    if(!device_info.presets[stream][preset].enabled) throw std::runtime_error("unsupported stream");

    requests[stream] = device_info.presets[stream][preset];
    for(auto & s : native_streams) s.buffer.reset(); // Changing stream configuration invalidates the current stream info
}

void rs_device::disable_stream(rs_stream stream)
{
    if(capturing) throw std::runtime_error("streams cannot be reconfigured after having called rs_start_device()");
    if(device_info.stream_subdevices[stream] == -1) throw std::runtime_error("unsupported stream");

    requests[stream] = {};
    for(auto & s : native_streams) s.buffer.reset(); // Changing stream configuration invalidates the current stream info
}

// Where do the intrinsics for a given (possibly synthetic) stream come from?
static rs_stream get_stream_intrinsics_native_stream(rs_stream stream)
{
    switch(stream)
    {
    case RS_STREAM_COLOR_ALIGNED_TO_DEPTH: return RS_STREAM_DEPTH;
    case RS_STREAM_DEPTH_ALIGNED_TO_COLOR: return RS_STREAM_COLOR;
    case RS_STREAM_DEPTH_ALIGNED_TO_RECTIFIED_COLOR: return RS_STREAM_RECTIFIED_COLOR;
    default: assert(stream <= RS_STREAM_RECTIFIED_COLOR); return stream;
    }
}

// Where does the pixel data for a given (possibly synthetic) stream come from?
static rs_stream get_stream_data_native_stream(rs_stream stream)
{
    switch(stream)
    {
    case RS_STREAM_RECTIFIED_COLOR: return RS_STREAM_COLOR;
    case RS_STREAM_COLOR_ALIGNED_TO_DEPTH: return RS_STREAM_COLOR;
    case RS_STREAM_DEPTH_ALIGNED_TO_COLOR: return RS_STREAM_DEPTH;
    case RS_STREAM_DEPTH_ALIGNED_TO_RECTIFIED_COLOR: return RS_STREAM_DEPTH;
    default: assert(stream < RS_STREAM_NATIVE_COUNT); return stream;
    }
}



bool rs_device::is_stream_enabled(rs_stream stream) const 
{ 
    if(stream < RS_STREAM_NATIVE_COUNT) return requests[stream].enabled;
    else return true; // Synthetic streams always enabled?
}

rs_intrinsics rs_device::get_stream_intrinsics(rs_stream stream) const 
{
    stream = get_stream_intrinsics_native_stream(stream);
    if(stream == RS_STREAM_RECTIFIED_COLOR)
    {
        auto intrin = intrinsics.get(get_current_stream_mode(RS_STREAM_COLOR).intrinsics_index);
        intrin.model = RS_DISTORTION_NONE;
        memset(&intrin.coeffs, 0, sizeof(intrin.coeffs));
        return intrin;
    }
    else return intrinsics.get(get_current_stream_mode(stream).intrinsics_index);
}

rs_format rs_device::get_stream_format(rs_stream stream) const
{ 
    return get_current_stream_mode(get_stream_data_native_stream(stream)).format;
}

int rs_device::get_stream_framerate(rs_stream stream) const
{ 
    return get_current_stream_mode(get_stream_data_native_stream(stream)).fps; 
}

void rs_device::start()
{
    if(capturing) throw std::runtime_error("cannot restart device without first stopping device");
        
    auto selected_modes = device_info.select_modes(requests);

    for(auto & s : native_streams) s.buffer.reset(); // Starting capture invalidates the current stream info, if any exists from previous capture

    // Satisfy stream_requests as necessary for each subdevice, calling set_mode and
    // dispatching the uvc configuration for a requested stream to the hardware
    for(auto mode : selected_modes)
    {
        // Create a stream buffer for each stream served by this subdevice mode
        std::vector<std::shared_ptr<stream_buffer>> stream_list;
        for(auto & stream_mode : mode.streams)
        {
            // Create a buffer to receive the images from this stream
            auto stream = std::make_shared<stream_buffer>(stream_mode);
            stream_list.push_back(stream);
                    
            // If this is one of the streams requested by the user, store the buffer so they can access it
            if(requests[stream_mode.stream].enabled) native_streams[stream_mode.stream].buffer = stream;
        }
                
        // Initialize the subdevice and set it to the selected mode
        int serial_frame_no = 0;
        bool only_stream = selected_modes.size() == 1;
        set_subdevice_mode(*device, mode.subdevice, mode.width, mode.height, mode.pf->fourcc, mode.fps, [mode, stream_list, only_stream, serial_frame_no](const void * frame) mutable
        {
            // Unpack the image into the user stream interface back buffer
            std::vector<byte *> dest;
            for(auto & stream : stream_list) dest.push_back(stream->get_back_data());
            mode.unpacker(dest.data(), reinterpret_cast<const byte *>(frame), mode);
            int frame_number = (mode.use_serial_numbers_if_unique && only_stream) ? serial_frame_no++ : mode.frame_number_decoder(mode, frame);
            if(frame_number == 0) frame_number = ++serial_frame_no; // No dinghy on LibUVC backend?
                
            // Swap the backbuffer to the middle buffer and indicate that we have updated
            for(auto & stream : stream_list) stream->swap_back(frame_number);
        });
    }
    
    on_before_start(selected_modes);
    start_streaming(*device, device_info.num_libuvc_transfer_buffers);
    capture_started = std::chrono::high_resolution_clock::now();
    capturing = true;
    base_timestamp = 0;
}

void rs_device::stop()
{
    if(!capturing) throw std::runtime_error("cannot stop device without first starting device");
    stop_streaming(*device);
    capturing = false;
}

void rs_device::wait_all_streams()
{
    if(!capturing) return;

    // Determine timeout time
    const auto timeout = std::max(std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(500), capture_started + std::chrono::seconds(5));

    // Check if any of our streams do not have data yet, if so, wait for them to have data, and remember that we are on the first frame
    bool first_frame = false;
    for(int i=0; i<RS_STREAM_NATIVE_COUNT; ++i)
    {
        if(native_streams[i].buffer && !native_streams[i].buffer->is_front_valid())
        {
            while(!native_streams[i].buffer->swap_front())
            {
                std::this_thread::sleep_for(std::chrono::microseconds(100)); // todo - Use a condition variable or something to avoid this
                if(std::chrono::high_resolution_clock::now() >= timeout) throw std::runtime_error("Timeout waiting for frames");
            }
            assert(native_streams[i].buffer->is_front_valid());
            last_stream_timestamp = native_streams[i].buffer->get_front_number();
            first_frame = true;
        }
    }

    // If this is not the first frame, check for new data on all streams, make sure that at least one stream provides an update
    if(!first_frame)
    {
        bool updated = false;
        while(true)
        {
            for(int i=0; i<RS_STREAM_NATIVE_COUNT; ++i)
            {
                if(native_streams[i].buffer && native_streams[i].buffer->swap_front())
                {
                    updated = true;
                }
            }
            if(updated) break;

            std::this_thread::sleep_for(std::chrono::microseconds(100)); // todo - Use a condition variable or something to avoid this
            if(std::chrono::high_resolution_clock::now() >= timeout) throw std::runtime_error("Timeout waiting for frames");
        }
    }

    // Determine the largest change from the previous timestamp
    int frame_delta = INT_MIN;    
    for(int i=0; i<RS_STREAM_NATIVE_COUNT; ++i)
    {
        if(!native_streams[i].buffer) continue;
        int delta = native_streams[i].buffer->get_front_number() - last_stream_timestamp; // Relying on undefined behavior: 32-bit integer overflow -> wraparound
        frame_delta = std::max(frame_delta, delta);
    }

    // Wait for any stream which expects a new frame prior to the frame time
    for(int i=0; i<RS_STREAM_NATIVE_COUNT; ++i)
    {
        if(!native_streams[i].buffer) continue;
        int next_timestamp = native_streams[i].buffer->get_front_number() + native_streams[i].buffer->get_front_delta();
        int next_delta = next_timestamp - last_stream_timestamp;
        if(next_delta > frame_delta) continue;
        while(!native_streams[i].buffer->swap_front())
        {
            std::this_thread::sleep_for(std::chrono::microseconds(100)); // todo - Use a condition variable or something to avoid this
            if(std::chrono::high_resolution_clock::now() >= timeout) throw std::runtime_error("Timeout waiting for frames");
        }
    }

    if(first_frame)
    {
        // Set time 0 to the least recent of the current frames
        base_timestamp = 0;
        for(int i=0; i<RS_STREAM_NATIVE_COUNT; ++i)
        {
            if(!native_streams[i].buffer) continue;
            int delta = native_streams[i].buffer->get_front_number() - last_stream_timestamp; // Relying on undefined behavior: 32-bit integer overflow -> wraparound
            if(delta < 0) last_stream_timestamp = native_streams[i].buffer->get_front_number();
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
    stream = get_stream_data_native_stream(stream);
    if(!native_streams[stream].is_enabled()) throw std::runtime_error(to_string() << "stream not enabled: " << stream); 
    return base_timestamp == -1 ? 0 : convert_timestamp(base_timestamp + native_streams[stream].get_frame_number() - last_stream_timestamp);
}

const byte * rs_device::get_aligned_image(rs_stream stream, rs_stream from, rs_stream to) const
{
    const int index = stream - RS_STREAM_NATIVE_COUNT;
    if(synthetic_images[index].empty() || synthetic_timestamps[index] != get_frame_timestamp(from))
    {
        synthetic_images[index].resize(rsimpl::get_image_size(get_stream_intrinsics(to).width, get_stream_intrinsics(to).height, get_stream_format(from)));
        memset(synthetic_images[index].data(), 0, synthetic_images[index].size());
        synthetic_timestamps[index] = get_frame_timestamp(from);

        if(get_stream_format(from) == RS_FORMAT_Z16)
        {
            align_depth_to_color(synthetic_images[index].data(), (const uint16_t *)get_frame_data(from), get_depth_scale(), get_stream_intrinsics(from), get_extrinsics(from, to), get_stream_intrinsics(to));    
        }
        else if(get_stream_format(to) == RS_FORMAT_Z16)
        {
            align_color_to_depth(synthetic_images[index].data(), (const uint16_t *)get_frame_data(to), get_depth_scale(), get_stream_intrinsics(to), get_extrinsics(to, from), get_stream_intrinsics(from), get_frame_data(from), get_stream_format(from));      
        }
        else
        {
            assert(false && "Cannot align two images if neither have depth data");
        }
    }
    return synthetic_images[index].data();
}

const byte * rs_device::get_frame_data(rs_stream stream) const 
{ 
    if(stream < RS_STREAM_NATIVE_COUNT)
    {
        if(!native_streams[stream].is_enabled()) throw std::runtime_error(to_string() << "stream not enabled: " << stream);
        return native_streams[stream].get_frame_data();
    }

    if(synthetic_images[stream - RS_STREAM_NATIVE_COUNT].empty() || synthetic_timestamps[stream - RS_STREAM_NATIVE_COUNT] != get_frame_timestamp(get_stream_data_native_stream(stream)))
    {
        if(stream == RS_STREAM_RECTIFIED_COLOR)
        {
            auto rect_intrin = get_stream_intrinsics(RS_STREAM_RECTIFIED_COLOR);
            if(rectification_table.empty())
            {
                rectification_table = compute_rectification_table(rect_intrin, get_extrinsics(RS_STREAM_RECTIFIED_COLOR, RS_STREAM_COLOR), get_stream_intrinsics(RS_STREAM_COLOR));
            }
            
            const auto format = get_stream_format(RS_STREAM_COLOR);
            auto & rectified_color = synthetic_images[RS_STREAM_RECTIFIED_COLOR - RS_STREAM_NATIVE_COUNT];
            rectified_color.resize(get_image_size(rect_intrin.width, rect_intrin.height, format));
            rectify_image(rectified_color.data(), rectification_table, get_frame_data(RS_STREAM_COLOR), format);
        }
        
        switch(stream)
        {
        case RS_STREAM_COLOR_ALIGNED_TO_DEPTH: return get_aligned_image(stream, RS_STREAM_COLOR, RS_STREAM_DEPTH);
        case RS_STREAM_DEPTH_ALIGNED_TO_COLOR: return get_aligned_image(stream, RS_STREAM_DEPTH, RS_STREAM_COLOR);
        case RS_STREAM_DEPTH_ALIGNED_TO_RECTIFIED_COLOR: return get_aligned_image(stream, RS_STREAM_DEPTH, RS_STREAM_RECTIFIED_COLOR);
        }
    }
    return synthetic_images[stream - RS_STREAM_NATIVE_COUNT].data();
} 

rsimpl::stream_mode rs_device::get_current_stream_mode(rs_stream stream) const
{
    if(native_streams[stream].buffer) return native_streams[stream].buffer->get_mode();
    if(requests[stream].enabled)
    {
        for(auto subdevice_mode : device_info.select_modes(requests))
        {
            for(auto stream_mode : subdevice_mode.streams)
            {
                if(stream_mode.stream == stream) return stream_mode;
            }
        }   
        throw std::logic_error("no mode found"); // Should never happen, select_modes should throw if no mode can be found
    }
    throw std::runtime_error(to_string() << "stream not enabled: " << stream);
}

rsimpl::pose rs_device::get_pose(rs_stream stream) const
{
    if(stream < RS_STREAM_NATIVE_COUNT) return device_info.stream_poses[stream];
    if(stream == RS_STREAM_RECTIFIED_COLOR) return {device_info.stream_poses[RS_STREAM_DEPTH].orientation, device_info.stream_poses[RS_STREAM_COLOR].position};
    return get_pose(get_stream_intrinsics_native_stream(stream));
}

rs_extrinsics rs_device::get_extrinsics(rs_stream from, rs_stream to) const
{
    auto from_pose = get_pose(from), to_pose = get_pose(to);
    if(from_pose == to_pose) return {{1,0,0,0,1,0,0,0,1},{0,0,0}};
    auto transform = inverse(from_pose) * to_pose;
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
    if(stream < RS_STREAM_NATIVE_COUNT) return enumerate_stream_modes(device_info, stream).size();
    return 0; // Synthetic streams have no modes and cannot have enable_stream called on it
}

void rs_device::get_stream_mode(rs_stream stream, int mode, int * width, int * height, rs_format * format, int * framerate) const
{
    auto m = enumerate_stream_modes(device_info, stream)[mode];
    if(width) *width = m.width;
    if(height) *height = m.height;
    if(format) *format = m.format;
    if(framerate) *framerate = m.fps;
}

void rs_device::set_option(rs_option option, int value)
{
    if(!supports_option(option)) throw std::runtime_error(to_string() << "option not supported by this device - " << option);
    if(uvc::is_pu_control(option))
    {
        uvc::set_pu_control(get_device(), device_info.stream_subdevices[RS_STREAM_COLOR], option, value);
    }
    else
    {
        set_xu_option(option, value);
    }
}

int rs_device::get_option(rs_option option) const
{
    if(!supports_option(option)) throw std::runtime_error(to_string() << "option not supported by this device - " << option);
    if(uvc::is_pu_control(option))
    {
        return uvc::get_pu_control(get_device(), device_info.stream_subdevices[RS_STREAM_COLOR], option);
    }
    else
    {
        return get_xu_option(option);
    }
}
