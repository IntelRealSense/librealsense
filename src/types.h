#ifndef LIBREALSENSE_TYPES_H
#define LIBREALSENSE_TYPES_H

#include <array>            // For array
#include <vector>           // For vector
#include <libuvc/libuvc.h>  // For uvc_frame_format

namespace rsimpl
{
    const int MAX_STREAMS = 4;

    struct StreamRequest
    {
        bool enabled;
        int width, height, format, fps;
    };

    struct StreamMode
    {
        int stream;                 // RS_DEPTH, RS_COLOR, RS_INFRARED, RS_INFRARED_2, etc.
        int width, height;          // Resolution visible to the client library
        int format, fps;            // Pixel format and framerate visible to the client library
        int intrinsics_index;       // Index of image intrinsics
    };

    struct SubdeviceMode
    {
        int subdevice;                      // 0, 1, 2, etc...
        int width, height;                  // Resolution advertised over UVC
        uvc_frame_format format;            // Pixel format advertised over UVC
        int fps;                            // Framerate advertised over UVC
        std::vector<StreamMode> streams;    // Modes for streams which can be supported by this device mode
        void (* unpacker)(void * dest[], const SubdeviceMode & mode, const void * frame);
    };
    void unpack_strided_image(void * dest[], const SubdeviceMode & mode, const void * frame);
    void unpack_rly12_to_y8(void * dest[], const SubdeviceMode & mode, const void * frame);
    void unpack_yuyv_to_rgb(void * dest[], const SubdeviceMode & mode, const void * frame);

    struct StaticCameraInfo
    {
        int stream_subdevices[MAX_STREAMS];             // Which subdevice is used to support each stream, or -1 if stream is unavailable
        std::vector<SubdeviceMode> subdevice_modes;     // A list of available modes each subdevice can be put into

        StaticCameraInfo() { for(auto & s : stream_subdevices) s = -1; }

        const SubdeviceMode * select_mode(const std::array<StreamRequest,MAX_STREAMS> & requests, int subdevice_index) const;
    };


}

#endif
