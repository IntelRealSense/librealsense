// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "device.h"
#include "sync.h"

#include <array>

using namespace rsimpl;

rs_device::rs_device(std::shared_ptr<rsimpl::uvc::device> device, const rsimpl::static_device_info & info) : device(device), config(info), capturing(false),
    depth(config, RS_STREAM_DEPTH), color(config, RS_STREAM_COLOR), infrared(config, RS_STREAM_INFRARED), infrared2(config, RS_STREAM_INFRARED2),
    points(depth), rect_color(color), color_to_depth(color, depth), depth_to_color(depth, color), depth_to_rect_color(depth, rect_color), infrared2_to_depth(infrared2,depth), depth_to_infrared2(depth,infrared2)
{
    streams[RS_STREAM_DEPTH    ] = native_streams[RS_STREAM_DEPTH]     = &depth;
    streams[RS_STREAM_COLOR    ] = native_streams[RS_STREAM_COLOR]     = &color;
    streams[RS_STREAM_INFRARED ] = native_streams[RS_STREAM_INFRARED]  = &infrared;
    streams[RS_STREAM_INFRARED2] = native_streams[RS_STREAM_INFRARED2] = &infrared2;
    streams[RS_STREAM_POINTS]                                          = &points;
    streams[RS_STREAM_RECTIFIED_COLOR]                                 = &rect_color;
    streams[RS_STREAM_COLOR_ALIGNED_TO_DEPTH]                          = &color_to_depth;
    streams[RS_STREAM_DEPTH_ALIGNED_TO_COLOR]                          = &depth_to_color;
    streams[RS_STREAM_DEPTH_ALIGNED_TO_RECTIFIED_COLOR]                = &depth_to_rect_color;
    streams[RS_STREAM_INFRARED2_ALIGNED_TO_DEPTH]                      = &infrared2_to_depth;
    streams[RS_STREAM_DEPTH_ALIGNED_TO_INFRARED2]                      = &depth_to_infrared2;
}

rs_device::~rs_device()
{

}

bool rs_device::supports_option(rs_option option) const 
{ 
    if(uvc::is_pu_control(option)) return true;
    for(auto & o : config.info.options) if(o.option == option) return true;
    return false; 
}

void rs_device::enable_stream(rs_stream stream, int width, int height, rs_format format, int fps)
{
    if(capturing) throw std::runtime_error("streams cannot be reconfigured after having called rs_start_device()");
    if(config.info.stream_subdevices[stream] == -1) throw std::runtime_error("unsupported stream");

    config.requests[stream] = {true, width, height, format, fps};
    for(auto & s : native_streams) s->archive.reset(); // Changing stream configuration invalidates the current stream info
}

void rs_device::enable_stream_preset(rs_stream stream, rs_preset preset)
{
    if(capturing) throw std::runtime_error("streams cannot be reconfigured after having called rs_start_device()");
    if(!config.info.presets[stream][preset].enabled) throw std::runtime_error("unsupported stream");

    config.requests[stream] = config.info.presets[stream][preset];
    for(auto & s : native_streams) s->archive.reset(); // Changing stream configuration invalidates the current stream info
}

void rs_device::disable_stream(rs_stream stream)
{
    if(capturing) throw std::runtime_error("streams cannot be reconfigured after having called rs_start_device()");
    if(config.info.stream_subdevices[stream] == -1) throw std::runtime_error("unsupported stream");

    config.requests[stream] = {};
    for(auto & s : native_streams) s->archive.reset(); // Changing stream configuration invalidates the current stream info
}

void rs_device::set_stream_callback(rs_stream stream, void (*on_frame)(void * data, int timestamp, void * user), void * user)
{
    config.callbacks[stream] = {on_frame, user};
}

void rs_device::start()
{
    if(capturing) throw std::runtime_error("cannot restart device without first stopping device");
        
    auto selected_modes = config.select_modes();
    auto archive = std::make_shared<frame_archive>(selected_modes, select_key_stream(selected_modes));
    auto timestamp_reader = create_frame_timestamp_reader();

    for(auto & s : native_streams) s->archive.reset(); // Starting capture invalidates the current stream info, if any exists from previous capture

    // Satisfy stream_requests as necessary for each subdevice, calling set_mode and
    // dispatching the uvc configuration for a requested stream to the hardware
    for(auto mode_selection : selected_modes)
    {
        // Create a stream buffer for each stream served by this subdevice mode
        for(auto & stream_mode : mode_selection.get_outputs())
        {                    
            // If this is one of the streams requested by the user, store the buffer so they can access it
            if(config.requests[stream_mode.first].enabled) native_streams[stream_mode.first]->archive = archive;
        }

        // Copy the callbacks that apply to this stream, so that they can be captured by value
        std::vector<frame_callback> callbacks;
        for(auto & output : mode_selection.get_outputs()) callbacks.push_back(config.callbacks[output.first]);

        // Initialize the subdevice and set it to the selected mode
        set_subdevice_mode(*device, mode_selection.mode.subdevice, mode_selection.mode.native_dims.x, mode_selection.mode.native_dims.y, mode_selection.mode.pf.fourcc, mode_selection.mode.fps, 
            [mode_selection, archive, timestamp_reader, callbacks](const void * frame) mutable
        {
            // Ignore any frames which appear corrupted or invalid
            if(!timestamp_reader->validate_frame(mode_selection.mode, frame)) return;

            // Determine the timestamp for this frame
            int timestamp = timestamp_reader->get_frame_timestamp(mode_selection.mode, frame);

            // Obtain buffers for unpacking the frame
            std::vector<byte *> dest;
            for(auto & output : mode_selection.get_outputs()) dest.push_back(archive->alloc_frame(output.first, timestamp));

            // Unpack the frame
            mode_selection.unpack(dest.data(), reinterpret_cast<const byte *>(frame));

            // If any frame callbacks were specified, dispatch them now
            for(size_t i=0; i<dest.size(); ++i) callbacks[i](dest[i], timestamp);

            // Commit the frame to the archive
            for(auto & output : mode_selection.get_outputs()) archive->commit_frame(output.first);
        });
    }
    
    this->archive = archive;
    on_before_start(selected_modes);
    start_streaming(*device, config.info.num_libuvc_transfer_buffers);
    capture_started = std::chrono::high_resolution_clock::now();
    capturing = true;
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
    if(!archive) return;

    archive->wait_for_frames();
}

bool rs_device::poll_all_streams()
{
    if(!capturing) return false;
    if(!archive) return false;
    return archive->poll_for_frames();
}

rs_frameset* rs_device::wait_all_streams_safe()
{
    if (!capturing) throw std::runtime_error("Can't call wait_for_frames_safe when the device is not capturing!");
    if (!archive) throw std::runtime_error("Can't call wait_for_frames_safe when frame archive is not available!");

        return (rs_frameset*)archive->wait_for_frames_safe();
}

bool rs_device::poll_all_streams_safe(rs_frameset** frames)
{
    if (!capturing) return false;
    if (!archive) return false;

    return archive->poll_for_frames_safe((frame_archive::frameset**)frames);
}

void rs_device::release_frames(rs_frameset * frameset)
{
    archive->release_frameset((frame_archive::frameset *)frameset);
}

rs_frameset * rs_device::clone_frames(rs_frameset * frameset)
{
	auto result = archive->clone_frameset((frame_archive::frameset *)frameset);
	if (!result) throw std::runtime_error("Not enough resources to clone frameset!");
	return (rs_frameset*)result;
}


rs_frame_ref* rs_device::detach_frame(const rs_frameset* fs, rs_stream stream)
{
	auto result = archive->detach_frame_ref((frame_archive::frameset *)fs, stream);
	if (!result) throw std::runtime_error("Not enough resources to tack detached frame!");
	return (rs_frame_ref*)result;
}

void rs_device::release_frame(rs_frame_ref* ref)
{
	archive->release_frame_ref((frame_archive::frame_ref *)ref);
}

rs_frame_ref* ::rs_device::clone_frame(rs_frame_ref* frame)
{
	auto result = archive->clone_frame((frame_archive::frame_ref *)frame);
	if (!result) throw std::runtime_error("Not enough resources to clone frame!");
	return (rs_frame_ref*)result;
}

void rs_device::get_option_range(rs_option option, double & min, double & max, double & step, double & def)
{
    if(uvc::is_pu_control(option))
    {
        int mn, mx, stp, df;
        uvc::get_pu_control_range(get_device(), config.info.stream_subdevices[RS_STREAM_COLOR], option, &mn, &mx, &stp, &df);
        min  = mn;
        max  = mx;
        step = stp;
        def  = df;
        return;
    }

    for(auto & o : config.info.options)
    {
        if(o.option == option)
        {
            min = o.min;
            max = o.max;
            step = o.step;
            def = o.def;
            return;
        }
    }

    throw std::logic_error("range not specified");
}
