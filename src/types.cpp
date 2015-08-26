#include "types.h"
#include "image.h"
#include "rs-internal.h"

#include <algorithm>

namespace rsimpl
{
    static size_t get_pixel_size(int format)
    {
        switch(format)
        {
        case RS_Z16: return sizeof(uint16_t);
        case RS_Y8: return sizeof(uint8_t);
        case RS_RGB8: return sizeof(uint8_t) * 3;
        default: assert(false);
        }
    }

    void unpack_strided_image(void * dest[], const SubdeviceMode & mode, const void * source)
    {
        assert(mode.streams.size() == 1);
        copy_strided_image(dest[0], mode.streams[0].width * get_pixel_size(mode.streams[0].format), source, mode.width * get_pixel_size(mode.streams[0].format), mode.streams[0].height);
    }

    void unpack_rly12_to_y8(void * dest[], const SubdeviceMode & mode, const void * frame)
    {
        assert(mode.format == UVC_FRAME_FORMAT_Y12I && mode.streams.size() == 2 && mode.streams[0].format == RS_Y8 && mode.streams[1].format == RS_Y8);
        convert_rly12_to_y8_y8(dest[0], dest[1], mode.streams[0].width, mode.streams[0].height, frame, 3*mode.width);
    }

    void unpack_yuyv_to_rgb(void * dest[], const SubdeviceMode & mode, const void * source)
    {
        assert(mode.format == UVC_FRAME_FORMAT_YUYV && mode.streams.size() == 1 && mode.streams[0].format == RS_RGB8);
        convert_yuyv_to_rgb((uint8_t *) dest[0], mode.width, mode.height, source);
    }

    const SubdeviceMode * StaticCameraInfo::select_mode(const std::array<StreamRequest,MAX_STREAMS> & requests, int subdevice_index) const
    {
        // Determine if the user has requested any streams which are supplied by this subdevice
        bool any_stream_requested = false;
        std::array<bool, MAX_STREAMS> stream_requested = {};
        for(int j = 0; j < MAX_STREAMS; ++j)
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
        for(int j = 0; j < MAX_STREAMS; ++j)
        {
            if(!stream_requested[j]) continue;
            ss << (first ? " " : " and ");

            ss << requests[j].width << 'x' << requests[j].height << ':';
            switch(requests[j].format)
            {
            case RS_Z16: ss << "Z16"; break;
            case RS_RGB8: ss << "RGB8"; break;
            case RS_Y8: ss << "Y8"; break;
            }
            ss << '@' << requests[j].fps << "Hz ";
            switch(j)
            {
            case RS_DEPTH: ss << "DEPTH"; break;
            case RS_COLOR: ss << "COLOR"; break;
            case RS_INFRARED: ss << "INFRARED"; break;
            case RS_INFRARED_2: ss << "INFRARED_2"; break;
            }
            first = false;
        }
        throw std::runtime_error(ss.str());
    }
}
