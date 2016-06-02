// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

// This header defines vocabulary types and utility mechanisms used ubiquitously by the
// rest of the library. As clearer module boundaries form, declarations might be moved
// out of this file and into more appropriate locations.

#pragma once
#ifndef LIBREALSENSE_TYPES_H
#define LIBREALSENSE_TYPES_H

#include "../include/librealsense/rs.h"     // Inherit all type definitions in the public API

#include <cassert>                          // For assert
#include <cstring>                          // For memcmp
#include <vector>                           // For vector
#include <sstream>                          // For ostringstream
#include <mutex>                            // For mutex, unique_lock
#include <condition_variable>               // For condition_variable

#define RS_STREAM_NATIVE_COUNT 5
#define RS_CHANNEL_NATIVE_COUNT 1

#define RS_USER_QUEUE_SIZE 64


namespace rsimpl
{
    ///////////////////////////////////
    // Utility types for general use //
    ///////////////////////////////////

    enum class byte : uint8_t {};

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
            for (unsigned int i = 0; i < sizeof(T); ++i) reinterpret_cast<char *>(&le_value)[i] = reinterpret_cast<const char *>(&be_value)[sizeof(T) - i - 1];
            return le_value;
        }
    };
#pragma pack(pop)

    ///////////////////////
    // Logging mechanism //
    ///////////////////////

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

    /////////////////////////////
    // Enumerated type support //
    /////////////////////////////

#define RS_ENUM_HELPERS(TYPE, PREFIX) const char * get_string(TYPE value); \
        inline bool is_valid(TYPE value) { return value >= 0 && value < RS_##PREFIX##_COUNT; } \
        inline std::ostream & operator << (std::ostream & out, TYPE value) { if(is_valid(value)) return out << get_string(value); else return out << (int)value; }
    RS_ENUM_HELPERS(rs_stream, STREAM)
        RS_ENUM_HELPERS(rs_format, FORMAT)
        RS_ENUM_HELPERS(rs_preset, PRESET)
        RS_ENUM_HELPERS(rs_distortion, DISTORTION)
    RS_ENUM_HELPERS(rs_option, OPTION)
        RS_ENUM_HELPERS(rs_capabilities, CAPABILITIES)
        RS_ENUM_HELPERS(rs_source, SOURCE)
#undef RS_ENUM_HELPERS

        ////////////////////////////////////////////
        // World's tiniest linear algebra library //
        ////////////////////////////////////////////

    struct int2 { int x, y; };
    struct float3 { float x, y, z; float & operator [] (int i) { return (&x)[i]; } };
    struct float3x3 { float3 x, y, z; float & operator () (int i, int j) { return (&x)[j][i]; } }; // column-major
    struct pose { float3x3 orientation; float3 position; };
    inline bool operator == (const float3 & a, const float3 & b) { return a.x == b.x && a.y == b.y && a.z == b.z; }
    inline float3 operator + (const float3 & a, const float3 & b) { return{ a.x + b.x, a.y + b.y, a.z + b.z }; }
    inline float3 operator * (const float3 & a, float b) { return{ a.x*b, a.y*b, a.z*b }; }
    inline bool operator == (const float3x3 & a, const float3x3 & b) { return a.x == b.x && a.y == b.y && a.z == b.z; }
    inline float3 operator * (const float3x3 & a, const float3 & b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
    inline float3x3 operator * (const float3x3 & a, const float3x3 & b) { return{ a*b.x, a*b.y, a*b.z }; }
    inline float3x3 transpose(const float3x3 & a) { return{ {a.x.x,a.y.x,a.z.x}, {a.x.y,a.y.y,a.z.y}, {a.x.z,a.y.z,a.z.z} }; }
    inline bool operator == (const pose & a, const pose & b) { return a.orientation == b.orientation && a.position == b.position; }
    inline float3 operator * (const pose & a, const float3 & b) { return a.orientation * b + a.position; }
    inline pose operator * (const pose & a, const pose & b) { return{ a.orientation * b.orientation, a * b.position }; }
    inline pose inverse(const pose & a) { auto inv = transpose(a.orientation); return{ inv, inv * a.position * -1 }; }

    ///////////////////
    // Pixel formats //
    ///////////////////

    struct pixel_format_unpacker
    {
        bool requires_processing;
        void(*unpack)(byte * const dest[], const byte * source, int count);
        std::vector<std::pair<rs_stream, rs_format>> outputs;

        bool provides_stream(rs_stream stream) const { for (auto & o : outputs) if (o.first == stream) return true; return false; }
        rs_format get_format(rs_stream stream) const { for (auto & o : outputs) if (o.first == stream) return o.second; throw std::logic_error("missing output"); }
    };

    struct native_pixel_format
    {
        uint32_t fourcc;
        int plane_count;
        size_t bytes_per_pixel;
        std::vector<pixel_format_unpacker> unpackers;

        size_t get_image_size(int width, int height) const { return width * height * plane_count * bytes_per_pixel; }
    };

    ////////////////////////
    // Static camera info //
    ////////////////////////

    struct subdevice_mode
    {
        int subdevice;                          // 0, 1, 2, etc...
        int2 native_dims;                       // Resolution advertised over UVC
        native_pixel_format pf;                 // Pixel format advertised over UVC
        int fps;                                // Framerate advertised over UVC
        rs_intrinsics native_intrinsics;        // Intrinsics structure corresponding to the content of image (Note: width,height may be subset of native_dims)
        std::vector<rs_intrinsics> rect_modes;  // Potential intrinsics of image after being rectified in software by librealsense
        std::vector<int> pad_crop_options;      // Acceptable padding/cropping values
    };

    struct stream_request
    {
        bool enabled;
        int width, height;
        rs_format format;
        int fps;
    };

    struct interstream_rule // Requires a.*field + delta == b.*field OR a.*field + delta2 == b.*field
    {
        rs_stream a, b;
        int stream_request::* field;
        int delta, delta2;
    };

    struct supported_option
    {
        rs_option option;
        double min, max, step, def;
    };

    struct data_polling_request
    {
        bool        enabled = false;
    };

    struct static_device_info
    {
        std::string name;                                                   // Model name of the camera        
        int stream_subdevices[RS_STREAM_NATIVE_COUNT];                      // Which subdevice is used to support each stream, or -1 if stream is unavailable
        int data_subdevices[RS_STREAM_NATIVE_COUNT];                        // Specify whether the subdevice supports events pipe in addition to streaming, -1 if data channels are unavailable
        std::vector<subdevice_mode> subdevice_modes;                        // A list of available modes each subdevice can be put into
        std::vector<interstream_rule> interstream_rules;                    // Rules which constrain the set of available modes
        stream_request presets[RS_STREAM_NATIVE_COUNT][RS_PRESET_COUNT];    // Presets available for each stream
        std::vector<supported_option> options;
        pose stream_poses[RS_STREAM_NATIVE_COUNT];                          // Static pose of each camera on the device
        int num_libuvc_transfer_buffers;                                    // Number of transfer buffers to use in LibUVC backend
        std::string firmware_version;                                       // Firmware version string
        std::string serial;                                                 // Serial number of the camera (from USB or from SPI memory)
        float nominal_depth_scale;                                          // Default scale
        std::vector<rs_capabilities> capabilities_vector;

        static_device_info();
    };

    struct motion_event
    {
        unsigned short      error_state;
        unsigned short      status;
        unsigned short      imu_entries_num;
        unsigned short      non_imu_entries_num;
        unsigned long       timestamp;
        rs_motion_data      imu_packets[4];
        rs_timestamp_data   non_imu_packets[8];
    };

    //////////////////////////////////
    // Runtime device configuration //
    //////////////////////////////////

    struct subdevice_mode_selection
    {
        subdevice_mode mode;                    // The streaming mode in which to place the hardware
        int pad_crop;                           // The number of pixels of padding (positive values) or cropping (negative values) to apply to all four edges of the image
        int unpacker_index;                     // The specific unpacker used to unpack the encoded format into the desired output formats

        subdevice_mode_selection() : mode({}), pad_crop(), unpacker_index() {}
        subdevice_mode_selection(const subdevice_mode & mode, int pad_crop, int unpacker_index) : mode(mode), pad_crop(pad_crop), unpacker_index(unpacker_index) {}

        const pixel_format_unpacker & get_unpacker() const { return mode.pf.unpackers[unpacker_index]; }
        const std::vector<std::pair<rs_stream, rs_format>> & get_outputs() const { return get_unpacker().outputs; }
        int get_width() const { return mode.native_intrinsics.width + pad_crop * 2; }
        int get_height() const { return mode.native_intrinsics.height + pad_crop * 2; }
        size_t get_image_size(rs_stream stream) const;
        bool provides_stream(rs_stream stream) const { return get_unpacker().provides_stream(stream); }
        rs_format get_format(rs_stream stream) const { return get_unpacker().get_format(stream); }
        int get_framerate(rs_stream stream) const { return mode.fps; }
        void unpack(byte * const dest[], const byte * source) const;

        bool requires_processing() const { return mode.pf.unpackers[unpacker_index].requires_processing || get_width() != mode.native_dims.x; }
    };

    class frame_callback
    {
        void(*on_frame)(rs_device * dev, rs_frame_ref * frame, void * user);
        void * user;
        rs_device * device;
    public:
        frame_callback() : frame_callback(nullptr, nullptr, nullptr) {}
        frame_callback(rs_device * dev, void(*on_frame)(rs_device *, rs_frame_ref *, void *), void * user) : on_frame(on_frame), user(user), device(dev) {}

        operator bool() { return on_frame != nullptr; }
        void operator () (rs_frame_ref * frame) const { if (on_frame) on_frame(device, frame, user); }
    };

    class motion_events_callback
    {
        void(*on_event)(rs_device * dev, rs_motion_data data, void * user);
        void        * user;
        rs_device   * device;
    public:
        motion_events_callback() : motion_events_callback(nullptr, nullptr, nullptr) {}
        motion_events_callback(rs_device * dev, void(*on_event)(rs_device *, rs_motion_data, void *), void * user) : on_event(on_event), user(user), device(dev) {}

        operator bool() { return on_event != nullptr; }
        void operator () (rs_motion_data data) const { if (on_event) on_event(device, data, user); }
    };

    class timestamp_events_callback
    {
        void(*on_event)(rs_device * dev, rs_timestamp_data data, void * user);
        void        * user;
        rs_device   * device;
    public:
        timestamp_events_callback() : timestamp_events_callback(nullptr, nullptr, nullptr) {}
        timestamp_events_callback(rs_device * dev, void(*on_event)(rs_device *, rs_timestamp_data, void *), void * user) : on_event(on_event), user(user), device(dev) {}

        operator bool() { return on_event != nullptr; }
        void operator () (rs_timestamp_data data) const { if (on_event) on_event(device, data, user); }
    };

    struct device_config
    {
        const static_device_info    info;
        stream_request              requests[RS_STREAM_NATIVE_COUNT];   // Modified by enable/disable_stream calls
        frame_callback callbacks[RS_STREAM_NATIVE_COUNT];   // Modified by set_frame_callback calls
        data_polling_request        data_requests;                      // Modified by enable/disable_events calls
        motion_events_callback      motion_callback;                   // Modified by set_events_callback calls
        timestamp_events_callback   timestamp_callback;
        float depth_scale;                                              // Scale of depth values

        device_config(const rsimpl::static_device_info & info) : info(info), depth_scale(info.nominal_depth_scale)
        {
            for (auto & req : requests) req = rsimpl::stream_request();
        }

        subdevice_mode_selection select_mode(const stream_request(&requests)[RS_STREAM_NATIVE_COUNT], int subdevice_index) const;
        std::vector<subdevice_mode_selection> select_modes(const stream_request(&requests)[RS_STREAM_NATIVE_COUNT]) const;
        std::vector<subdevice_mode_selection> select_modes() const { return select_modes(requests); }
    };

    ////////////////////////////////////////
    // Helper functions for library types //
    ////////////////////////////////////////

    inline rs_intrinsics pad_crop_intrinsics(const rs_intrinsics & i, int pad_crop)
    {
        return{ i.width + pad_crop * 2, i.height + pad_crop * 2, i.ppx + pad_crop, i.ppy + pad_crop, i.fx, i.fy, i.model, {i.coeffs[0], i.coeffs[1], i.coeffs[2], i.coeffs[3], i.coeffs[4]} };
    }

    inline rs_intrinsics scale_intrinsics(const rs_intrinsics & i, int width, int height)
    {
        const float sx = (float)width / i.width, sy = (float)height / i.height;
        return{ width, height, i.ppx*sx, i.ppy*sy, i.fx*sx, i.fy*sy, i.model, {i.coeffs[0], i.coeffs[1], i.coeffs[2], i.coeffs[3], i.coeffs[4]} };
    }

    inline bool operator == (const rs_intrinsics & a, const rs_intrinsics & b) { return std::memcmp(&a, &b, sizeof(a)) == 0; }

    inline uint32_t pack(uint8_t c0, uint8_t c1, uint8_t c2, uint8_t c3)
    {
        return (c0 << 24) | (c1 << 16) | (c2 << 8) | c3;
    }

    template<class T, int C>
    class small_heap
    {
        T buffer[C];
        bool is_free[C];
        std::mutex mutex;
        bool keep_allocating = true;
        std::condition_variable cv;
        int size = 0;

    public:
        small_heap()
        {
            for (auto i = 0; i < C; i++)
            {
                is_free[i] = true;
                buffer[i] = std::move(T());
            }
        }

        T * allocate()
        {
            std::unique_lock<std::mutex> lock(mutex);
            if (!keep_allocating) return nullptr;

            for (auto i = 0; i < C; i++)
            {
                if (is_free[i])
                {
                    is_free[i] = false;
                    size++;
                    return &buffer[i];
                }
            }
            return nullptr;
        }

        void deallocate(T * item)
        {
             if (item < buffer || item >= buffer + C)
            {
                throw std::runtime_error("Trying to return item to a heap that didn't allocate it!");
            }
            auto i = item - buffer;
            buffer[i] = std::move(T());


          

            
            {
                std::unique_lock<std::mutex> lock(mutex);

                is_free[i] = true;
                size--;

                if (size == 0)
                {
                    lock.unlock();
                    cv.notify_one();
                }
            }
        }

        void stop_allocation()
        {
            std::unique_lock<std::mutex> lock(mutex);
            keep_allocating = false;
        }

        void wait_until_empty()
        {
            std::unique_lock<std::mutex> lock(mutex);

            const auto ready = [this]()
            {
                return size == 0;
            };
            if (!ready() && !cv.wait_for(lock, std::chrono::hours(1000), ready)) // for some reason passing std::chrono::duration::max makes it return instantly
            {
                throw std::runtime_error("Could not flush one of the user controlled objects!");
            }
        }
    };

    class frame_continuation
    {
        std::function<void()> continuation;
        const void* protected_data = nullptr;

        frame_continuation(const frame_continuation &) = delete;
        frame_continuation & operator=(const frame_continuation &) = delete;
    public:
        frame_continuation() : continuation([]() {}) {}

        explicit frame_continuation(std::function<void()> continuation, const void* protected_data) : continuation(continuation), protected_data(protected_data) {}
        

        frame_continuation(frame_continuation && other) : continuation(std::move(other.continuation)), protected_data(other.protected_data)
        {
            other.continuation = []() {};
            other.protected_data = nullptr;
        }

        void operator()()
        {
            continuation();
            continuation = []() {};
            protected_data = nullptr;
        }

        const void* get_data() const { return protected_data; }

        frame_continuation & operator=(frame_continuation && other)
        {
            continuation();
            protected_data = other.protected_data;
            continuation = other.continuation;
            other.continuation = []() {};
            other.protected_data = nullptr;
            return *this;
        }

        ~frame_continuation()
        {
            continuation();
        }
    };
}

#endif
