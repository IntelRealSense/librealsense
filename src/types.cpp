#include "types.h"
#include "image.h"
#include "camera.h"

#include <cstring>
#include <algorithm>
#include <array>
#include <iostream>

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
        CASE(INFRARED_2)
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

    size_t get_image_size(int width, int height, rs_format format)
    {
        switch(format)
        {
        case RS_FORMAT_Z16: return width * height * 2;
        case RS_FORMAT_YUYV: assert(width % 2 == 0); return width * height * 2;
        case RS_FORMAT_RGB8: return width * height * 3;
        case RS_FORMAT_BGR8: return width * height * 3;
        case RS_FORMAT_RGBA8: return width * height * 4;
        case RS_FORMAT_BGRA8: return width * height * 4;
        case RS_FORMAT_Y8: return width * height * 2;
        case RS_FORMAT_Y16: return width * height * 2;
        default: assert(false); return 0;
        }    
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

    void unpack_strided_image(void * dest[], const subdevice_mode & mode, const void * source)
    {
        assert(mode.streams.size() == 1);
        copy_strided_image(dest[0], get_image_size(mode.streams[0].width, 1, mode.streams[0].format), source, get_image_size(mode.width, 1, mode.streams[0].format), mode.streams[0].height);
    }

    void unpack_y12i_to_y8(void * dest[], const subdevice_mode & mode, const void * frame)
    {
        assert(mode.format == uvc::frame_format::Y12I && mode.streams.size() == 2 && mode.streams[0].format == RS_FORMAT_Y8 && mode.streams[1].format == RS_FORMAT_Y8);
        convert_y12i_to_y8_y8(dest[0], dest[1], mode.streams[0].width, mode.streams[0].height, frame, 3*mode.width);
    }

    void unpack_y12i_to_y16(void * dest[], const subdevice_mode & mode, const void * frame)
    {
        assert(mode.format == uvc::frame_format::Y12I && mode.streams.size() == 2 && mode.streams[0].format == RS_FORMAT_Y16 && mode.streams[1].format == RS_FORMAT_Y16);
        convert_y12i_to_y16_y16(dest[0], dest[1], mode.streams[0].width, mode.streams[0].height, frame, 3*mode.width);
    }

    void unpack_yuyv_to_rgb(void * dest[], const subdevice_mode & mode, const void * source)
    {
        assert(mode.format == uvc::frame_format::YUYV && mode.streams.size() == 1 && mode.streams[0].format == RS_FORMAT_RGB8);
        convert_yuyv_to_rgb(dest[0], mode.width, mode.height, source);
    }

    void unpack_yuyv_to_rgba(void * dest[], const subdevice_mode & mode, const void * source)
    {
        assert(mode.format == uvc::frame_format::YUYV && mode.streams.size() == 1 && mode.streams[0].format == RS_FORMAT_RGBA8);
        convert_yuyv_to_rgba(dest[0], mode.width, mode.height, source);
    }

    void unpack_yuyv_to_bgr(void * dest[], const subdevice_mode & mode, const void * source)
    {
        assert(mode.format == uvc::frame_format::YUYV && mode.streams.size() == 1 && mode.streams[0].format == RS_FORMAT_BGR8);
        convert_yuyv_to_bgr(dest[0], mode.width, mode.height, source);
    }

    void unpack_yuyv_to_bgra(void * dest[], const subdevice_mode & mode, const void * source)
    {
        assert(mode.format == uvc::frame_format::YUYV && mode.streams.size() == 1 && mode.streams[0].format == RS_FORMAT_BGRA8);
        convert_yuyv_to_bgra(dest[0], mode.width, mode.height, source);
    }

    static_camera_info::static_camera_info()
    {
        for(auto & s : stream_subdevices) s = -1;
        for(auto & s : presets) for(auto & p : s) p = stream_request();
        for(auto & o : option_supported) o = false;
    }

    const subdevice_mode * static_camera_info::select_mode(const stream_request (& requests)[RS_STREAM_COUNT], int subdevice_index) const
    {
        // Determine if the user has requested any streams which are supplied by this subdevice
        bool any_stream_requested = false;
        std::array<bool, RS_STREAM_COUNT> stream_requested = {};
        for(int j = 0; j < RS_STREAM_COUNT; ++j)
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
        for(int j = 0; j < RS_STREAM_COUNT; ++j)
        {
            if(!stream_requested[j]) continue;
            ss << (first ? " " : " and ");
            ss << requests[j].width << 'x' << requests[j].height << ':' << get_string(requests[j].format);
            ss << '@' << requests[j].fps << "Hz " << get_string((rs_stream)j);
            first = false;
        }
        throw std::runtime_error(ss.str());
    }
}
