#pragma once
#ifndef LIBREALSENSE_TYPES_H
#define LIBREALSENSE_TYPES_H

#include "../include/librealsense/rs.h"

#include <cassert>                          // For assert
#include <vector>                           // For vector
#include <memory>                           // For shared_ptr
#include <sstream>                          // For ostringstream
#include <mutex>

//#define ENABLE_DEBUG_SPAM

#ifdef _DEBUG
#define ENABLE_DEBUG_OUTPUT
#endif

#ifdef ENABLE_DEBUG_OUTPUT
#include <iostream> // For cout, cerr, endl. DO NOT INCLUDE ANYWHERE ELSE IN LIBREALSENSE 
#define DEBUG_OUT(...) std::cout << "[debug] " << __VA_ARGS__ << std::endl
#define DEBUG_ERR(...) std::cerr << "[error] " << __VA_ARGS__ << std::endl
#else
#define DEBUG_OUT(...) do {} while(false)
#define DEBUG_ERR(...) do {} while(false)
#endif

namespace rsimpl
{
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

    // World's tiniest linear algebra library
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

    inline uint32_t pack(uint8_t c0, uint8_t c1, uint8_t c2, uint8_t c3)
    {
        return (c0 << 24) | (c1 << 16) | (c2 << 8) | c3;
    }

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
        uint32_t fourcc;                    // Pixel format advertised over UVC
        int fps;                            // Framerate advertised over UVC
        std::vector<stream_mode> streams;   // Modes for streams which can be supported by this device mode
        void (* unpacker)(void * dest[], const void * source, const subdevice_mode & mode);
        int (* frame_number_decoder)(const subdevice_mode & mode, const void * frame);
        bool use_serial_numbers_if_unique;  // If true, ignore frame_number_decoder and use a serial frame count if this is the only mode set
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
        bool option_supported[RS_OPTION_COUNT];                             // Whether or not a given option is supported on this camera
        pose stream_poses[RS_STREAM_NATIVE_COUNT];                          // Static pose of each camera on the device
        float depth_scale;                                                  // Scale of depth values
        int num_libuvc_transfer_buffers;                                    // Number of transfer buffers to use in LibUVC backend
        std::string firmware_version;                                       // Firmware version string
        std::string serial;                                                 // Serial number of the camera (from USB or from SPI memory)

        static_device_info();

        const subdevice_mode * select_mode(const stream_request (&requests)[RS_STREAM_NATIVE_COUNT], int subdevice_index) const;
        std::vector<subdevice_mode> select_modes(const stream_request (&requests)[RS_STREAM_NATIVE_COUNT]) const;
    };
    static_device_info add_standard_unpackers(const static_device_info & device_info);

    // Buffer for storing images provided by a given stream
    class stream_buffer
    {
        struct frame
        {
            std::vector<uint8_t>    data;
            int                     number;
            int                     delta;

                                    frame(const stream_mode & m);
            void                    swap(frame & r) { data.swap(r.data); std::swap(number, r.number); std::swap(delta, r.delta); }
        };

        stream_mode                 mode;
        frame                       front, middle, back;
        std::mutex                  mutex;
        volatile bool               updated = false;
        bool                        has_front = false;
        int                         last_frame_number;
        bool                        first = true;
    public:
                                    stream_buffer(const stream_mode & mode);

        const stream_mode &         get_mode() const { return mode; }
        const void *                get_front_data() const { return front.data.data(); }
        int                         get_front_number() const { return front.number; }
        int                         get_front_delta() const { return front.delta; }
        bool                        is_front_valid() const { return has_front; }

        void *                      get_back_data() { return back.data.data(); }
        void                        swap_back(int frame_number);
        bool                        swap_front();
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
