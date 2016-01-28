// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef LIBREALSENSE_TYPES_H
#define LIBREALSENSE_TYPES_H

#include "../include/librealsense/rs.h"

#include <cassert>                          // For assert
#include <cstring>                          // For memcmp
#include <vector>                           // For vector
#include <memory>                           // For shared_ptr
#include <sstream>                          // For ostringstream
#include <atomic>
#include <mutex>
#include <condition_variable>

#define RS_STREAM_NATIVE_COUNT 4

namespace rsimpl
{
    void log(rs_log_severity severity, const std::string & message);
    void log_to_console(rs_log_severity min_severity);
    void log_to_file(rs_log_severity min_severity, const char * file_path);
    extern rs_log_severity minimum_log_severity;

    #define LOG(SEVERITY, ...) do { if(static_cast<int>(SEVERITY) >= rsimpl::minimum_log_severity) { std::ostringstream ss; ss << __VA_ARGS__; rsimpl::log(SEVERITY, ss.str()); } } while(false)
    #define LOG_DEBUG(...)   LOG(RS_LOG_SEVERITY_DEBUG, __VA_ARGS__)
    #define LOG_INFO(...)    LOG(RS_LOG_SEVERITY_INFO,  __VA_ARGS__)
    #define LOG_WARNING(...) LOG(RS_LOG_SEVERITY_WARN,  __VA_ARGS__)
    #define LOG_ERROR(...)   LOG(RS_LOG_SEVERITY_ERROR, __VA_ARGS__)
    #define LOG_FATAL(...)   LOG(RS_LOG_SEVERITY_FATAL, __VA_ARGS__)

    enum class byte : uint8_t {};

    struct pixel_format_unpacker
    {
        void (* unpack)(byte * const dest[], const byte * source, int count);
        std::vector<std::pair<rs_stream, rs_format>> outputs;

        bool provides_stream(rs_stream stream) const { for(auto & o : outputs) if(o.first == stream) return true; return false; }
        rs_format get_format(rs_stream stream) const { for(auto & o : outputs) if(o.first == stream) return o.second; throw std::logic_error("missing output"); }
    };

    struct native_pixel_format
    {
        uint32_t fourcc;
        int plane_count;
        size_t bytes_per_pixel;
        std::vector<pixel_format_unpacker> unpackers;

        size_t get_image_size(int width, int height) const { return width * height * plane_count * bytes_per_pixel; }
    };

    // Enumerated type support
    #define RS_ENUM_HELPERS(TYPE, PREFIX) const char * get_string(TYPE value); \
        inline bool is_valid(TYPE value) { return value >= 0 && value < RS_##PREFIX##_COUNT; } \
        inline std::ostream & operator << (std::ostream & out, TYPE value) { if(is_valid(value)) return out << get_string(value); else return out << (int)value; }
    RS_ENUM_HELPERS(rs_stream, STREAM)
    RS_ENUM_HELPERS(rs_format, FORMAT)
    RS_ENUM_HELPERS(rs_preset, PRESET)
    RS_ENUM_HELPERS(rs_distortion, DISTORTION)
    RS_ENUM_HELPERS(rs_option, OPTION)
    #undef RS_ENUM_HELPERS

    inline rs_intrinsics pad_crop_intrinsics(const rs_intrinsics & i, int pad_crop)
    {
        return {i.width+pad_crop*2, i.height+pad_crop*2, i.ppx+pad_crop, i.ppy+pad_crop, i.fx, i.fy, i.model, {i.coeffs[0], i.coeffs[1], i.coeffs[2], i.coeffs[3], i.coeffs[4]}};
    }

    inline rs_intrinsics scale_intrinsics(const rs_intrinsics & i, int width, int height)
    {
        const float sx = (float)width/i.width, sy = (float)height/i.height;
        return {width, height, i.ppx*sx, i.ppy*sy, i.fx*sx, i.fy*sy, i.model, {i.coeffs[0], i.coeffs[1], i.coeffs[2], i.coeffs[3], i.coeffs[4]}};
    }

    // World's tiniest linear algebra library
    struct int2 { int x,y; };
    struct float3 { float x,y,z; float & operator [] (int i) { return (&x)[i]; } };
    struct float3x3 { float3 x,y,z; float & operator () (int i, int j) { return (&x)[j][i]; } }; // column-major
    struct pose { float3x3 orientation; float3 position; };
    inline bool operator == (const float3 & a, const float3 & b) { return a.x==b.x && a.y==b.y && a.z==b.z; }
    inline float3 operator + (const float3 & a, const float3 & b) { return {a.x+b.x, a.y+b.y, a.z+b.z}; }
    inline float3 operator * (const float3 & a, float b) { return {a.x*b, a.y*b, a.z*b}; }
    inline bool operator == (const float3x3 & a, const float3x3 & b) { return a.x==b.x && a.y==b.y && a.z==b.z; }
    inline float3 operator * (const float3x3 & a, const float3 & b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
    inline float3x3 operator * (const float3x3 & a, const float3x3 & b) { return {a*b.x, a*b.y, a*b.z}; }
    inline float3x3 transpose(const float3x3 & a) { return {{a.x.x,a.y.x,a.z.x}, {a.x.y,a.y.y,a.z.y}, {a.x.z,a.y.z,a.z.z}}; }
    inline bool operator == (const pose & a, const pose & b) { return a.orientation==b.orientation && a.position==b.position; }
    inline float3 operator * (const pose & a, const float3 & b) { return a.orientation * b + a.position; }
    inline pose operator * (const pose & a, const pose & b) { return {a.orientation * b.orientation, a * b.position}; }
    inline pose inverse(const pose & a) { auto inv = transpose(a.orientation); return {inv, inv * a.position * -1}; }

    inline bool operator == (const rs_intrinsics & a, const rs_intrinsics & b) { return std::memcmp(&a, &b, sizeof(a)) == 0; }

    inline uint32_t pack(uint8_t c0, uint8_t c1, uint8_t c2, uint8_t c3)
    {
        return (c0 << 24) | (c1 << 16) | (c2 << 8) | c3;
    }

    struct supported_option
    { 
        rs_option option;
        double min, max, step; 
    };

    // Static camera info
    struct stream_request
    {
        bool enabled;
        int width, height;
        rs_format format;
        int fps;
    };    

    struct subdevice_mode
    {
        int subdevice;                          // 0, 1, 2, etc...
        int2 native_dims;                       // Resolution advertised over UVC
        const native_pixel_format * pf;         // Pixel format advertised over UVC
        int fps;                                // Framerate advertised over UVC
        rs_intrinsics native_intrinsics;        // Intrinsics structure corresponding to the content of image (Note: width,height may be subset of native_dims)
        std::vector<rs_intrinsics> rect_modes;  // Potential intrinsics of image after being rectified in software by librealsense
        std::vector<int> pad_crop_options;      // Acceptable padding/cropping values
    };

    struct subdevice_mode_selection
    {
        const subdevice_mode * mode;            // The streaming mode in which to place the hardware
        int pad_crop;                           // The number of pixels of padding (positive values) or cropping (negative values) to apply to all four edges of the image
        const pixel_format_unpacker * unpacker; // The specific unpacker used to unpack the encoded format into the desired output formats

        subdevice_mode_selection() : mode(), pad_crop(), unpacker() {}
        subdevice_mode_selection(const subdevice_mode * mode, int pad_crop, const pixel_format_unpacker * unpacker) : mode(mode), pad_crop(pad_crop), unpacker(unpacker) {}

        const std::vector<std::pair<rs_stream, rs_format>> & get_outputs() const { return unpacker->outputs; }
        int get_width() const { return mode->native_intrinsics.width + pad_crop * 2; }
        int get_height() const { return mode->native_intrinsics.height + pad_crop * 2; }
        size_t get_image_size(rs_stream stream) const;
        bool provides_stream(rs_stream stream) const { return unpacker->provides_stream(stream); }
        rs_format get_format(rs_stream stream) const { return unpacker->get_format(stream); }
        int get_framerate(rs_stream stream) const { return mode->fps; }
        void unpack(byte * const dest[], const byte * source) const;
    };

    struct interstream_rule // Requires a.*field + delta == b.*field OR a.*field + delta2 == b.*field
    {
        rs_stream a, b;        
        int stream_request::* field;
        int delta, delta2;
    };

    struct static_device_info
    {
        std::string name;                                                   // Model name of the camera
        int stream_subdevices[RS_STREAM_NATIVE_COUNT];                      // Which subdevice is used to support each stream, or -1 if stream is unavailable
        std::vector<subdevice_mode> subdevice_modes;                        // A list of available modes each subdevice can be put into
        std::vector<interstream_rule> interstream_rules;                    // Rules which constrain the set of available modes
        stream_request presets[RS_STREAM_NATIVE_COUNT][RS_PRESET_COUNT];    // Presets available for each stream
        std::vector<supported_option> options;
        pose stream_poses[RS_STREAM_NATIVE_COUNT];                          // Static pose of each camera on the device
        int num_libuvc_transfer_buffers;                                    // Number of transfer buffers to use in LibUVC backend
        std::string firmware_version;                                       // Firmware version string
        std::string serial;                                                 // Serial number of the camera (from USB or from SPI memory)
        float nominal_depth_scale;                                          // Default scale

        static_device_info();

        subdevice_mode_selection select_mode(const stream_request (&requests)[RS_STREAM_NATIVE_COUNT], int subdevice_index) const;
        std::vector<subdevice_mode_selection> select_modes(const stream_request (&requests)[RS_STREAM_NATIVE_COUNT]) const;
    };

    struct device_config
    {
        const static_device_info            info;
        stream_request                      requests[RS_STREAM_NATIVE_COUNT];  // Modified by enable/disable_stream calls
        float                               depth_scale;                       // Scale of depth values

                                            device_config(const rsimpl::static_device_info & info) : info(info), depth_scale(info.nominal_depth_scale) 
                                            { 
                                                for(auto & req : requests) req = rsimpl::stream_request(); 
                                            }

        std::vector<subdevice_mode_selection> select_modes() const { return info.select_modes(requests); }
    };
    
    class frame_archive
    {
        // Define a movable but explicitly noncopyable buffer type to hold our frame data
        struct frame 
        { 
            std::vector<byte> data;
            int timestamp;

            frame() : timestamp() {}
            frame(const frame & r) = delete;
            frame(frame && r) : frame() { *this = std::move(r); }

            frame & operator = (const frame & r) = delete;
            frame & operator = (frame && r) { data = move(r.data); timestamp = r.timestamp; return *this; }            
        };

        // Constant data
        subdevice_mode_selection modes[RS_STREAM_NATIVE_COUNT];
        rs_stream key_stream;
        std::vector<rs_stream> other_streams;

        // Application thread data
        frame frontbuffer[RS_STREAM_NATIVE_COUNT];

        // Synchronized data
        std::vector<frame> frames[RS_STREAM_NATIVE_COUNT];
        std::vector<frame> freelist;
        std::mutex mutex;
        std::condition_variable cv;

        // Frame callback thread data
        frame backbuffer[RS_STREAM_NATIVE_COUNT];

        void dequeue_frame(rs_stream stream);
        void discard_frame(rs_stream stream);
        void cull_frames();
    public:
        frame_archive(const std::vector<subdevice_mode_selection> & selection);

        bool is_stream_enabled(rs_stream stream) const { return modes[stream].mode != nullptr; }
        const subdevice_mode_selection & get_mode(rs_stream stream) const { return modes[stream]; }

        // Application thread API
        void wait_for_frames();
        bool poll_for_frames();
        const byte * get_frame_data(rs_stream stream) const;
        int get_frame_timestamp(rs_stream stream) const;

        // Frame callback thread API
        byte * alloc_frame(rs_stream stream, int timestamp);
        void commit_frame(rs_stream stream);  
    };

    // Utilities
    struct to_string
    {
        std::ostringstream ss;
        template<class T> to_string & operator << (const T & val) { ss << val; return *this; }
        operator std::string() const { return ss.str(); }
    };

    #pragma pack(push, 1)
    template<class T> class big_endian
    {
        T be_value;
    public:
        operator T () const
        {
            T le_value = 0;
            for(int i=0; i<sizeof(T); ++i) reinterpret_cast<char *>(&le_value)[i] = reinterpret_cast<const char *>(&be_value)[sizeof(T)-i-1];
            return le_value;
        }
    };
    #pragma pack(pop)
}

#endif
