#include "types.h"
#include "image.h"
#include "device.h"

#include <cstring>
#include <algorithm>
#include <array>

namespace rsimpl
{
    const char * get_string(rs_stream value)
    {
        #define CASE(X) case RS_STREAM_##X: return #X;
        switch(value)
        {
        CASE(DEPTH)
        CASE(COLOR)
        CASE(INFRARED)
        CASE(INFRARED2)
        CASE(RECTIFIED_COLOR)
        CASE(COLOR_ALIGNED_TO_DEPTH)
        CASE(DEPTH_ALIGNED_TO_COLOR)
        CASE(DEPTH_ALIGNED_TO_RECTIFIED_COLOR)
        default: assert(!is_valid(value)); return nullptr;
        }
        #undef CASE
    }

    const char * get_string(rs_format value)
    {
        #define CASE(X) case RS_FORMAT_##X: return #X;
        switch(value)
        {
        CASE(ANY)
        CASE(Z16)
        CASE(YUYV)
        CASE(RGB8)
        CASE(BGR8)
        CASE(RGBA8)
        CASE(BGRA8)
        CASE(Y8)
        CASE(Y16)
        default: assert(!is_valid(value)); return nullptr;
        }
        #undef CASE
    }

    const char * get_string(rs_preset value)
    {
        #define CASE(X) case RS_PRESET_##X: return #X;
        switch(value)
        {
        CASE(BEST_QUALITY)
        CASE(LARGEST_IMAGE)
        CASE(HIGHEST_FRAMERATE)
        default: assert(!is_valid(value)); return nullptr;
        }
        #undef CASE
    }

    const char * get_string(rs_distortion value)
    {
        #define CASE(X) case RS_DISTORTION_##X: return #X;
        switch(value)
        {
        CASE(NONE)
        CASE(MODIFIED_BROWN_CONRADY)
        CASE(INVERSE_BROWN_CONRADY)
        default: assert(!is_valid(value)); return nullptr;
        }
        #undef CASE
    }

    const char * get_string(rs_option value)
    {
        #define CASE(X) case RS_OPTION_##X: return #X;
        switch(value)
        {
        CASE(COLOR_BACKLIGHT_COMPENSATION)
        CASE(COLOR_BRIGHTNESS)
        CASE(COLOR_CONTRAST)
        CASE(COLOR_EXPOSURE)
        CASE(COLOR_GAIN)
        CASE(COLOR_GAMMA)
        CASE(COLOR_HUE)
        CASE(COLOR_SATURATION)
        CASE(COLOR_SHARPNESS)
        CASE(COLOR_WHITE_BALANCE)
        CASE(F200_LASER_POWER)
        CASE(F200_ACCURACY)
        CASE(F200_MOTION_RANGE)
        CASE(F200_FILTER_OPTION)
        CASE(F200_CONFIDENCE_THRESHOLD)
        CASE(F200_DYNAMIC_FPS)
        CASE(R200_LR_AUTO_EXPOSURE_ENABLED)
        CASE(R200_LR_GAIN)
        CASE(R200_LR_EXPOSURE)
        CASE(R200_EMITTER_ENABLED)
        CASE(R200_DEPTH_CONTROL_PRESET)
        CASE(R200_DEPTH_UNITS)
        CASE(R200_DEPTH_CLAMP_MIN)
        CASE(R200_DEPTH_CLAMP_MAX)
        CASE(R200_DISPARITY_MODE_ENABLED)
        CASE(R200_DISPARITY_MULTIPLIER)
        CASE(R200_DISPARITY_SHIFT)
        default: assert(!is_valid(value)); return nullptr;
        }
        #undef CASE
    }

    ////////////////////////
    // static_device_info //
    ////////////////////////

    static_device_info::static_device_info()
    {
        for(auto & s : stream_subdevices) s = -1;
        for(auto & s : presets) for(auto & p : s) p = stream_request();
        for(auto & o : option_supported) o = false;
        for(auto & p : stream_poses)
        {
            p = {{{1,0,0},{0,1,0},{0,0,1}}, {0,0,0}};
        }
    }

    const subdevice_mode * static_device_info::select_mode(const stream_request (& requests)[RS_STREAM_NATIVE_COUNT], int subdevice_index) const
    {
        // Determine if the user has requested any streams which are supplied by this subdevice
        bool any_stream_requested = false;
        std::array<bool, RS_STREAM_NATIVE_COUNT> stream_requested = {};
        for(int j = 0; j < RS_STREAM_NATIVE_COUNT; ++j)
        {
            if(requests[j].enabled && stream_subdevices[j] == subdevice_index)
            {
                stream_requested[j] = true;
                any_stream_requested = true;
            }
        }

        // If no streams were requested, skip to the next subdevice
        if(!any_stream_requested) return nullptr;

        // Look for an appropriate mode
        for(auto & subdevice_mode : subdevice_modes)
        {
            // Skip modes that apply to other subdevices
            if(subdevice_mode.subdevice != subdevice_index) continue;

            // Determine if this mode satisfies the requirements on our requested streams
            auto stream_unsatisfied = stream_requested;
            for(auto & stream_mode : subdevice_mode.streams)
            {
                const auto & req = requests[stream_mode.stream];
                if(req.enabled && (req.width == 0 || req.width == stream_mode.width) 
                               && (req.height == 0 || req.height == stream_mode.height)
                               && (req.format == RS_FORMAT_ANY || req.format == stream_mode.format)
                               && (req.fps == 0 || req.fps == stream_mode.fps))
                {
                    stream_unsatisfied[stream_mode.stream] = false;
                }
            }

            // If any requested streams are still unsatisfied, skip to the next mode
            if(std::any_of(begin(stream_unsatisfied), end(stream_unsatisfied), [](bool b) { return b; })) continue;
            return &subdevice_mode;
        }

        // If we did not find an appropriate mode, report an error
        std::ostringstream ss;
        ss << "uvc subdevice " << subdevice_index << " cannot provide";
        bool first = true;
        for(int j = 0; j < RS_STREAM_NATIVE_COUNT; ++j)
        {
            if(!stream_requested[j]) continue;
            ss << (first ? " " : " and ");
            ss << requests[j].width << 'x' << requests[j].height << ':' << get_string(requests[j].format);
            ss << '@' << requests[j].fps << "Hz " << get_string((rs_stream)j);
            first = false;
        }
        throw std::runtime_error(ss.str());
    }

    std::vector<subdevice_mode> static_device_info::select_modes(const stream_request (&reqs)[RS_STREAM_NATIVE_COUNT]) const
    {
        // Make a mutable copy of our array
        stream_request requests[RS_STREAM_NATIVE_COUNT];
        for(int i=0; i<RS_STREAM_NATIVE_COUNT; ++i) requests[i] = reqs[i];

        // Check and modify requests to enforce all interstream constraints
        for(auto & rule : interstream_rules)
        {
            auto & a = requests[rule.a], & b = requests[rule.b]; auto f = rule.field;
            if(a.enabled && b.enabled)
            {
                // Check for incompatibility if both values specified
                if(a.*f != 0 && b.*f != 0 && a.*f + rule.delta != b.*f && a.*f + rule.delta2 != b.*f)
                {
                    throw std::runtime_error(to_string() << "requested " << rule.a << " and " << rule.b << " settings are incompatible");
                }

                // If only one value is specified, modify the other request to match
                if(a.*f != 0 && b.*f == 0) b.*f = a.*f + rule.delta;
                if(a.*f == 0 && b.*f != 0) a.*f = b.*f - rule.delta;
            }
        }

        // Select subdevice modes needed to satisfy our requests
        int num_subdevices = 0;
        for(auto & mode : subdevice_modes) num_subdevices = std::max(num_subdevices, mode.subdevice+1);
        std::vector<subdevice_mode> selected_modes;
        for(int i = 0; i < num_subdevices; ++i) if(auto * m = select_mode(requests, i)) selected_modes.push_back(*m);
        return selected_modes;
    }

    ///////////////////
    // stream_buffer //
    ///////////////////

    triple_buffer::triple_buffer(const frame & value) : front(0), middle(1), back(2)
    {
        for(auto & f : buffers)
        {
            f.f = value;
            f.count = 0;
        }
    }

    bool triple_buffer::swap_front()
    {
        // If the "front" buffer currently has the most recent frame, return false
        auto count = frame_counter.load(std::memory_order_acquire); // Perform this load first to force UVC thread's writes to become visible
        if(buffers[front].count == count) return false;

        // Otherwise, there is a frame more recent than the "front" buffer, swap with "middle" until we have it
        while(buffers[front].count < count) front = middle.exchange(front);
        return true;
    }

    void triple_buffer::swap_back()
    {
        // Compute and store the new frame counter
        int count = frame_counter.load(std::memory_order_relaxed) + 1;
        buffers[back].count = count;
        back = middle.exchange(back);
        frame_counter.store(count, std::memory_order_release); // Perform this store last to force writes to become visible to app thread
    }

    stream_buffer::stream_buffer(const stream_mode & mode) : mode(mode), frames({std::vector<uint8_t>(get_image_size(mode.width, mode.height, mode.format))}), last_frame_number() {}

    void stream_buffer::swap_back(int frame_number) 
    {
        if(frames.get_count() == 0) last_frame_number = frame_number;
        frames.get_back().timestamp = frame_number;
        frames.get_back().delta = frame_number - last_frame_number;
        last_frame_number = frame_number;     
        frames.swap_back();
    }
}
