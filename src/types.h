#pragma once
#ifndef LIBREALSENSE_TYPES_H
#define LIBREALSENSE_TYPES_H

#include "../include/librealsense/rs.h"
#include "libuvc/libuvc.h"                  // For uvc_frame_format

#include <cassert>                          // For assert
#include <vector>                           // For vector
#include <memory>                           // For shared_ptr
#include <sstream>                          // For ostringstream

//#define ENABLE_DEBUG_SPAM

namespace rsimpl
{
    // Enumerated type support
    #define RS_ENUM_HELPERS(TYPE, PREFIX) const char * get_string(TYPE value); \
        inline bool is_valid(TYPE value) { return value >= RS_##PREFIX##_BEGIN_RANGE && value <= RS_##PREFIX##_END_RANGE; } \
        inline std::ostream & operator << (std::ostream & out, TYPE value) { if(is_valid(value)) return out << get_string(value); else return out << (int)value; }
    RS_ENUM_HELPERS(rs_stream, STREAM)
    RS_ENUM_HELPERS(rs_format, FORMAT)
    RS_ENUM_HELPERS(rs_preset, PRESET)
    RS_ENUM_HELPERS(rs_distortion, DISTORTION)
    RS_ENUM_HELPERS(rs_option, OPTION)
    #undef RS_ENUM_HELPERS

    // World's tiniest linear algebra library
    struct float3 { float x,y,z; float & operator [] (int i) { return (&x)[i]; } };
    struct float3x3 { float3 x,y,z; float & operator () (int i, int j) { return (&x)[j][i]; } }; // column-major
    struct pose { float3x3 orientation; float3 position; };
    inline float3 operator + (const float3 & a, const float3 & b) { return {a.x+b.x, a.y+b.y, a.z+b.z}; }
    inline float3 operator * (const float3 & a, float b) { return {a.x*b, a.y*b, a.z*b}; }
    inline float3 operator * (const float3x3 & a, const float3 & b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
    inline float3x3 operator * (const float3x3 & a, const float3x3 & b) { return {a*b.x, a*b.y, a*b.z}; }
    inline float3x3 transpose(const float3x3 & a) { return {{a.x.x,a.y.x,a.z.x}, {a.x.y,a.y.y,a.z.y}, {a.x.z,a.y.z,a.z.z}}; }
    inline float3 operator * (const pose & a, const float3 & b) { return a.orientation * b + a.position; }
    inline pose operator * (const pose & a, const pose & b) { return {a.orientation * b.orientation, a.position + a * b.position}; }
    inline pose inverse(const pose & a) { auto inv = transpose(a.orientation); return {inv, inv * a.position * -1}; }

    // Static camera info
    struct stream_request
    {
        bool enabled;
        int width, height;
        rs_format format;
        int fps;
    };

    struct stream_mode
    {
        rs_stream stream;           // RS_DEPTH, RS_COLOR, RS_INFRARED, RS_INFRARED_2, etc.
        int width, height;          // Resolution visible to the client library
        rs_format format;           // Pixel format visible to the client library
        int fps;                    // Framerate visible to the client library
        int intrinsics_index;       // Index of image intrinsics
    };

    struct subdevice_mode
    {
        int subdevice;                      // 0, 1, 2, etc...
        int width, height;                  // Resolution advertised over UVC
        uvc_frame_format format;            // Pixel format advertised over UVC
        int fps;                            // Framerate advertised over UVC
        std::vector<stream_mode> streams;   // Modes for streams which can be supported by this device mode
        void (* unpacker)(void * dest[], const subdevice_mode & mode, const void * frame);
        int (* frame_number_decoder)(const subdevice_mode & mode, const void * frame);
    };
    void unpack_strided_image(void * dest[], const subdevice_mode & mode, const void * frame);
    void unpack_rly12_to_y8(void * dest[], const subdevice_mode & mode, const void * frame);
    void unpack_yuyv_to_rgb(void * dest[], const subdevice_mode & mode, const void * frame);

    struct static_camera_info
    {
        int stream_subdevices[RS_STREAM_NUM];                   // Which subdevice is used to support each stream, or -1 if stream is unavailable
        std::vector<subdevice_mode> subdevice_modes;            // A list of available modes each subdevice can be put into
        stream_request presets[RS_STREAM_NUM][RS_PRESET_NUM];   // Presets available for each stream
        bool option_supported[RS_OPTION_NUM];                   // Whether or not a given option is supported on this camera

        static_camera_info();

        const subdevice_mode * select_mode(const stream_request (&requests)[RS_STREAM_NUM], int subdevice_index) const;
    };

    // Calibration info
    struct calibration_info
    {
        std::vector<rs_intrinsics> intrinsics;
        pose stream_poses[RS_STREAM_NUM];
        float depth_scale;
    };

    // Utilities
    struct to_string
    {
        std::ostringstream ss;
        template<class T> to_string & operator << (const T & val) { ss << val; return *this; }
        operator std::string() const { return ss.str(); }
    };
}

#endif
