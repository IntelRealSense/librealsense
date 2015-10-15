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
        if(mode.fourcc == 'YUY2' && mode.unpacker == &unpack_subrect && mode.width == mode.streams[0].width && mode.height == mode.streams[0].height)
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

// Where do the intrinsics for a given (possibly synthetic) stream come from?
static rs_stream get_stream_intrinsics_native_stream(rs_stream stream)
{
    switch(stream)
    {
    case RS_STREAM_COLOR_ALIGNED_TO_DEPTH: return RS_STREAM_DEPTH;
    case RS_STREAM_DEPTH_ALIGNED_TO_COLOR: return RS_STREAM_COLOR;
    case RS_STREAM_DEPTH_ALIGNED_TO_RECTIFIED_COLOR: return RS_STREAM_RECTIFIED_COLOR;
    default: assert(stream < RS_STREAM_RECTIFIED_COLOR); return stream;
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
    std::lock_guard<std::mutex> lock(intrinsics_mutex);

    stream = get_stream_intrinsics_native_stream(stream);
    if(stream == RS_STREAM_RECTIFIED_COLOR)
    {
        auto intrin = intrinsics[get_current_stream_mode(RS_STREAM_COLOR).intrinsics_index];
        intrin.model = RS_DISTORTION_NONE;
        memset(&intrin.coeffs, 0, sizeof(intrin.coeffs));
        return intrin;
    }
    else return intrinsics[get_current_stream_mode(stream).intrinsics_index];
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

    for(auto & s : streams) s.reset(); // Starting capture invalidates the current stream info, if any exists from previous capture

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
            if(requests[stream_mode.stream].enabled) streams[stream_mode.stream] = stream;
        }
                
        // Initialize the subdevice and set it to the selected mode
        int serial_frame_no = 0;
        bool only_stream = selected_modes.size() == 1;
        set_subdevice_mode(*device, mode.subdevice, mode.width, mode.height, mode.fourcc, mode.fps, [mode, stream_list, only_stream, serial_frame_no](const void * frame) mutable
        {
            // Unpack the image into the user stream interface back buffer
            std::vector<void *> dest;
            for(auto & stream : stream_list) dest.push_back(stream->get_back_data());
            mode.unpacker(dest.data(), frame, mode);
            const int frame_number = (mode.use_serial_numbers_if_unique && only_stream) ? serial_frame_no++ : mode.frame_number_decoder(mode, frame);
                
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
        if(streams[i] && !streams[i]->is_front_valid())
        {
            while(!streams[i]->swap_front())
            {
                std::this_thread::sleep_for(std::chrono::microseconds(100)); // TODO: Use a condition variable or something to avoid this
                if(std::chrono::high_resolution_clock::now() >= timeout) throw std::runtime_error("Timeout waiting for frames");
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
            for(int i=0; i<RS_STREAM_NATIVE_COUNT; ++i)
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
    }

    // Determine the largest change from the previous timestamp
    int frame_delta = INT_MIN;    
    for(int i=0; i<RS_STREAM_NATIVE_COUNT; ++i)
    {
        if(!streams[i]) continue;
        int delta = streams[i]->get_front_number() - last_stream_timestamp; // Relying on undefined behavior: 32-bit integer overflow -> wraparound
        frame_delta = std::max(frame_delta, delta);
    }

    // Wait for any stream which expects a new frame prior to the frame time
    for(int i=0; i<RS_STREAM_NATIVE_COUNT; ++i)
    {
        if(!streams[i]) continue;
        int next_timestamp = streams[i]->get_front_number() + streams[i]->get_front_delta();
        int next_delta = next_timestamp - last_stream_timestamp;
        if(next_delta > frame_delta) continue;
        while(!streams[i]->swap_front())
        {
            std::this_thread::sleep_for(std::chrono::microseconds(100)); // TODO: Use a condition variable or something to avoid this
            if(std::chrono::high_resolution_clock::now() >= timeout) throw std::runtime_error("Timeout waiting for frames");
        }
    }

    if(first_frame)
    {
        // Set time 0 to the least recent of the current frames
        base_timestamp = 0;
        for(int i=0; i<RS_STREAM_NATIVE_COUNT; ++i)
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
    stream = get_stream_data_native_stream(stream);
    if(!streams[stream]) throw std::runtime_error(to_string() << "stream not enabled: " << stream); 
    return base_timestamp == -1 ? 0 : convert_timestamp(base_timestamp + streams[stream]->get_front_number() - last_stream_timestamp);
}

#define RSUTIL_IMPLEMENTATION
#include "../include/librealsense/rsutil.h"

void rs_align_images(void * depth_aligned_to_color, void * color_aligned_to_depth, const void * depth_pixels, rs_format depth_format, float depth_scale, const rs_intrinsics * depth_intrin, const rs_extrinsics * depth_to_color, const rs_intrinsics * color_intrin, const void * color_pixels, rs_format color_format)
{
    assert(depth_format == RS_FORMAT_Z16);
    assert(color_format == RS_FORMAT_RGB8);
    const uint16_t * in_depth = (const uint16_t *)(depth_pixels);
    const rs_byte3 * in_color = (const rs_byte3 *)(color_pixels);
    uint16_t * out_depth = (uint16_t *)(depth_aligned_to_color);
    rs_byte3 * out_color = (rs_byte3 *)(color_aligned_to_depth);
    int depth_x, depth_y, depth_pixel_index, color_x, color_y, color_pixel_index;

    // Iterate over the pixels of the depth image       
    for(depth_y = 0; depth_y < depth_intrin->height; ++depth_y)
    {
        for(depth_x = 0; depth_x < depth_intrin->width; ++depth_x)
        {
            // Skip over depth pixels with the value of zero, we have no depth data so we will not write anything into our aligned images
            depth_pixel_index = depth_y * depth_intrin->width + depth_x;
            if(in_depth[depth_pixel_index])
            {
                // Determine the corresponding pixel location in our color image
                float depth_pixel[2] = {depth_x, depth_y}, depth_point[3], color_point[3], color_pixel[2];
                rs_deproject_pixel_to_point(depth_point, depth_intrin, depth_pixel, in_depth[depth_pixel_index] * depth_scale);
                rs_transform_point_to_point(color_point, depth_to_color, depth_point);
                rs_project_point_to_pixel(color_pixel, color_intrin, color_point);
                
                // If the location is outside the bounds of the image, skip to the next pixel
                color_x = (int)roundf(color_pixel[0]);
                color_y = (int)roundf(color_pixel[1]);
                if(color_x < 0 || color_y < 0 || color_x >= color_intrin->width || color_y >= color_intrin->height)
                {
                    continue;
                }

                // Transfer data from original images into corresponding aligned images
                color_pixel_index = color_y * color_intrin->width + color_x;
                out_color[depth_pixel_index] = in_color[color_pixel_index];
                out_depth[color_pixel_index] = in_depth[depth_pixel_index];
            }
        }
    }
}

const void * rs_device::get_frame_data(rs_stream stream) const 
{ 
    if(stream < RS_STREAM_NATIVE_COUNT)
    {
        if(!streams[stream]) throw std::runtime_error(to_string() << "stream not enabled: " << stream);
        return streams[stream]->get_front_data(); 
    }

    if(synthetic_images[stream - RS_STREAM_NATIVE_COUNT].empty() || synthetic_timestamps[stream - RS_STREAM_NATIVE_COUNT] != get_frame_timestamp(get_stream_data_native_stream(stream)))
    {
        if(stream == RS_STREAM_RECTIFIED_COLOR)
        {
            const auto rect_intrin = get_stream_intrinsics(RS_STREAM_RECTIFIED_COLOR);
            if(rectification_table.empty())
            {
                const auto unrect_intrin = get_stream_intrinsics(RS_STREAM_COLOR);
                const auto rect_extrin = get_extrinsics(RS_STREAM_DEPTH, RS_STREAM_RECTIFIED_COLOR), unrect_extrin = get_extrinsics(RS_STREAM_DEPTH, RS_STREAM_COLOR);
                rectification_table.resize(rect_intrin.width * rect_intrin.height);
                rs_compute_rectification_table(rectification_table.data(), &rect_intrin, &rect_extrin, &unrect_intrin, &unrect_extrin);
            }
            
            const auto format = get_stream_format(RS_STREAM_COLOR);
            auto & rectified_color = synthetic_images[RS_STREAM_RECTIFIED_COLOR - RS_STREAM_NATIVE_COUNT];
            rectified_color.resize(get_image_size(rect_intrin.width, rect_intrin.height, format));
            rs_rectify_image(rectified_color.data(), &rect_intrin, rectification_table.data(), get_frame_data(RS_STREAM_COLOR), format);
        }
        else
        {
            auto depth_intrin = get_stream_intrinsics(RS_STREAM_DEPTH);
            auto color_intrin = get_stream_intrinsics(RS_STREAM_COLOR);
            auto depth_to_color = get_extrinsics(RS_STREAM_DEPTH, RS_STREAM_COLOR);

            auto & depth_aligned_to_color = synthetic_images[RS_STREAM_DEPTH_ALIGNED_TO_COLOR - RS_STREAM_NATIVE_COUNT];
            depth_aligned_to_color.resize(rsimpl::get_image_size(color_intrin.width, color_intrin.height, get_stream_format(RS_STREAM_DEPTH)));
            memset(depth_aligned_to_color.data(), 0, depth_aligned_to_color.size());

            auto & color_aligned_to_depth = synthetic_images[RS_STREAM_COLOR_ALIGNED_TO_DEPTH - RS_STREAM_NATIVE_COUNT];
            color_aligned_to_depth.resize(rsimpl::get_image_size(depth_intrin.width, depth_intrin.height, get_stream_format(RS_STREAM_COLOR)));
            memset(color_aligned_to_depth.data(), 0, color_aligned_to_depth.size());

            rs_align_images(depth_aligned_to_color.data(), color_aligned_to_depth.data(),
                get_frame_data(RS_STREAM_DEPTH), get_stream_format(RS_STREAM_DEPTH), get_depth_scale(), &depth_intrin,
                &depth_to_color, &color_intrin, get_frame_data(RS_STREAM_COLOR), get_stream_format(RS_STREAM_COLOR));    

            synthetic_timestamps[RS_STREAM_DEPTH_ALIGNED_TO_COLOR - RS_STREAM_NATIVE_COUNT] = get_frame_timestamp(RS_STREAM_DEPTH);
            synthetic_timestamps[RS_STREAM_COLOR_ALIGNED_TO_DEPTH - RS_STREAM_NATIVE_COUNT] = get_frame_timestamp(RS_STREAM_COLOR);
        }
    }
    return synthetic_images[stream - RS_STREAM_NATIVE_COUNT].data();
} 

rsimpl::stream_mode rs_device::get_current_stream_mode(rs_stream stream) const
{
    if(streams[stream]) return streams[stream]->get_mode();
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
    if(stream == RS_STREAM_RECTIFIED_COLOR) return {device_info.stream_poses[RS_STREAM_DEPTH].orientation, device_info.stream_poses[RS_STREAM_COLOR].position};
    else return device_info.stream_poses[get_stream_intrinsics_native_stream(stream)];
}

rs_extrinsics rs_device::get_extrinsics(rs_stream from, rs_stream to) const
{
    auto transform = inverse(get_pose(from)) * get_pose(to);
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
    switch(stream)
    {
    case RS_STREAM_COLOR_ALIGNED_TO_DEPTH: return 0; // Synthetic streams are not configurable
    case RS_STREAM_DEPTH_ALIGNED_TO_COLOR: return 0; // Synthetic streams are not configurable
    default: assert(stream < RS_STREAM_NATIVE_COUNT); return enumerate_stream_modes(device_info, stream).size();
    }
}

void rs_device::get_stream_mode(rs_stream stream, int mode, int * width, int * height, rs_format * format, int * framerate) const
{
    auto m = enumerate_stream_modes(device_info, stream)[mode];
    if(width) *width = m.width;
    if(height) *height = m.height;
    if(format) *format = m.format;
    if(framerate) *framerate = m.fps;
}

void rs_device::set_intrinsics_thread_safe(std::vector<rs_intrinsics> new_intrinsics)
{
    std::lock_guard<std::mutex> lock(intrinsics_mutex);
    intrinsics.swap(new_intrinsics);
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