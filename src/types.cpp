#include "types.h"
#include "image.h"
#include "camera.h"

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
        CASE(Y8)
        CASE(RGB8)
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
        CASE(GORDON_BROWN_CONRADY)
        CASE(INVERSE_BROWN_CONRADY)
        default: assert(!is_valid(value)); return nullptr;
        }
        #undef CASE
    }

    static size_t get_pixel_size(int format)
    {
        switch(format)
        {
        case RS_FORMAT_Z16: return sizeof(uint16_t);
        case RS_FORMAT_Y8: return sizeof(uint8_t);
        case RS_FORMAT_RGB8: return sizeof(uint8_t) * 3;
        default: assert(false);
        }
    }

    void unpack_strided_image(void * dest[], const subdevice_mode & mode, const void * source)
    {
        assert(mode.streams.size() == 1);
        copy_strided_image(dest[0], mode.streams[0].width * get_pixel_size(mode.streams[0].format), source, mode.width * get_pixel_size(mode.streams[0].format), mode.streams[0].height);
    }

    void unpack_rly12_to_y8(void * dest[], const subdevice_mode & mode, const void * frame)
    {
        assert(mode.format == UVC_FRAME_FORMAT_Y12I && mode.streams.size() == 2 && mode.streams[0].format == RS_FORMAT_Y8 && mode.streams[1].format == RS_FORMAT_Y8);
        convert_rly12_to_y8_y8(dest[0], dest[1], mode.streams[0].width, mode.streams[0].height, frame, 3*mode.width);
    }

    void unpack_yuyv_to_rgb(void * dest[], const subdevice_mode & mode, const void * source)
    {
        assert(mode.format == UVC_FRAME_FORMAT_YUYV && mode.streams.size() == 1 && mode.streams[0].format == RS_FORMAT_RGB8);
        convert_yuyv_to_rgb((uint8_t *) dest[0], mode.width, mode.height, source);
    }

    static_camera_info::static_camera_info()
    {
        for(auto & s : stream_subdevices) s = -1;
        for(auto & s : presets) for(auto & p : s) p = {};
    }

    const subdevice_mode * static_camera_info::select_mode(const stream_request (& requests)[RS_STREAM_NUM], int subdevice_index) const
    {
        // Determine if the user has requested any streams which are supplied by this subdevice
        bool any_stream_requested = false;
        std::array<bool, RS_STREAM_NUM> stream_requested = {};
        for(int j = 0; j < RS_STREAM_NUM; ++j)
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
                if(req.enabled && req.width == stream_mode.width && req.height == stream_mode.height && req.format == stream_mode.format && req.fps == stream_mode.fps)
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
        for(int j = 0; j < RS_STREAM_NUM; ++j)
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
