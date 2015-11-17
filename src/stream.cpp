#include "stream.h"
#include "image.h"
#include <algorithm>    // For sort
#include <tuple>        // For make_tuple

using namespace rsimpl;

rs_extrinsics stream_interface::get_extrinsics_to(const stream_interface & r) const
{
    auto from = get_pose(), to = r.get_pose();
    if(from == to) return {{1,0,0,0,1,0,0,0,1},{0,0,0}};
    auto transform = inverse(from) * to;
    rs_extrinsics extrin;
    (float3x3 &)extrin.rotation = transform.orientation;
    (float3 &)extrin.translation = transform.position;
    return extrin;
}

native_stream::native_stream(device_config & config, rs_stream stream) : config(config), stream(stream) 
{
    for(auto & subdevice_mode : config.info.subdevice_modes)
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
}

stream_mode native_stream::get_mode() const
{
    if(buffer) return buffer->get_mode();
    if(config.requests[stream].enabled)
    {
        for(auto subdevice_mode : config.select_modes())
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

const rsimpl::byte * rectified_stream::get_frame_data() const
{
    if(image.empty() || number != get_frame_number())
    {
        if(table.empty()) table = compute_rectification_table(get_intrinsics(), get_extrinsics_to(source), source.get_intrinsics());
        image.resize(get_image_size(get_intrinsics().width, get_intrinsics().height, get_format()));
        rectify_image(image.data(), table, source.get_frame_data(), get_format());
        number = get_frame_number();
    }
    return image.data();
}

const rsimpl::byte * aligned_stream::get_frame_data() const
{
    if(image.empty() || number != get_frame_number())
    {
        image.resize(get_image_size(get_intrinsics().width, get_intrinsics().height, get_format()));
        memset(image.data(), 0, image.size());
        if(from.get_format() == RS_FORMAT_Z16) align_depth_to_color(image.data(), (const uint16_t *)from.get_frame_data(), from.get_depth_scale(), from.get_intrinsics(), from.get_extrinsics_to(to), to.get_intrinsics());
        else if(to.get_format() == RS_FORMAT_Z16) align_color_to_depth(image.data(), (const uint16_t *)to.get_frame_data(), to.get_depth_scale(), to.get_intrinsics(), to.get_extrinsics_to(from), from.get_intrinsics(), from.get_frame_data(), from.get_format());
        else assert(false && "Cannot align two images if neither have depth data");
        number = get_frame_number();
    }
    return image.data();
}