// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

// This header defines vocabulary types and utility mechanisms used ubiquitously by the
// rest of the library. As clearer module boundaries form, declarations might be moved
// out of this file and into more appropriate locations.

#pragma once
#ifndef LIBREALSENSE_TYPES_H
#define LIBREALSENSE_TYPES_H

#include "../include/librealsense/rs.h"     // Inherit all type definitions in the public API
#include "../include/librealsense/rscore.hpp"

#include <cassert>                          // For assert
#include <cstring>                          // For memcmp
#include <vector>                           // For vector
#include <sstream>                          // For ostringstream
#include <mutex>                            // For mutex, unique_lock
#include <memory>                           // For unique_ptr
#include <map>          
#include <algorithm>
#include <condition_variable>
#include "backend.h"

#define ELPP_THREAD_SAFE
#define NOMINMAX
#include "../third_party/easyloggingpp/src/easylogging++.h"

typedef unsigned char byte;

const uint8_t RS_STREAM_NATIVE_COUNT = 5;
const int RS_USER_QUEUE_SIZE = 20;
const int RS_MAX_EVENT_QUEUE_SIZE = 500;
const int RS_MAX_EVENT_TINE_OUT = 10;

#ifndef DBL_EPSILON
const double DBL_EPSILON = 2.2204460492503131e-016;  // smallest such that 1.0+DBL_EPSILON != 1.0
#endif

namespace rsimpl
{
    ///////////////////////////////////
    // Utility types for general use //
    ///////////////////////////////////
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

    void log_to_console(rs_log_severity min_severity);
    void log_to_file(rs_log_severity min_severity, const char * file_path);

#define LOG_DEBUG(...)   do { CLOG(DEBUG   ,"librealsense") << __VA_ARGS__; } while(false)
#define LOG_INFO(...)    do { CLOG(INFO    ,"librealsense") << __VA_ARGS__; } while(false)
#define LOG_WARNING(...) do { CLOG(WARNING ,"librealsense") << __VA_ARGS__; } while(false)
#define LOG_ERROR(...)   do { CLOG(ERROR   ,"librealsense") << __VA_ARGS__; } while(false)
#define LOG_FATAL(...)   do { CLOG(FATAL   ,"librealsense") << __VA_ARGS__; } while(false)

    /////////////////////////////
    // Enumerated type support //
    /////////////////////////////

#define RS_ENUM_HELPERS(TYPE, PREFIX) const char * get_string(TYPE value); \
        inline bool is_valid(TYPE value) { return value >= 0 && value < RS_##PREFIX##_COUNT; } \
        inline std::ostream & operator << (std::ostream & out, TYPE value) { if(is_valid(value)) return out << get_string(value); else return out << (int)value; }
    RS_ENUM_HELPERS(rs_stream, STREAM)
    RS_ENUM_HELPERS(rs_format, FORMAT)
    RS_ENUM_HELPERS(rs_distortion, DISTORTION)
    RS_ENUM_HELPERS(rs_option, OPTION)
    RS_ENUM_HELPERS(rs_camera_info, CAMERA_INFO)
    RS_ENUM_HELPERS(rs_timestamp_domain, TIMESTAMP_DOMAIN)
    RS_ENUM_HELPERS(rs_subdevice, SUBDEVICE)
    RS_ENUM_HELPERS(rs_visual_preset, VISUAL_PRESET)
    #undef RS_ENUM_HELPERS

    ////////////////////////////////////////////
    // World's tiniest linear algebra library //
    ////////////////////////////////////////////
#pragma pack(push, 1)
    struct int2 { int x, y; };
    struct float3 { float x, y, z; float & operator [] (int i) { return (&x)[i]; } };
    struct float4 { float x, y, z, w; float & operator [] (int i) { return (&x)[i]; } };
    struct float3x3 { float3 x, y, z; float & operator () (int i, int j) { return (&x)[j][i]; } }; // column-major
    struct pose { float3x3 orientation; float3 position; };
#pragma pack(pop)
    inline bool operator == (const float3 & a, const float3 & b) { return a.x == b.x && a.y == b.y && a.z == b.z; }
    inline float3 operator + (const float3 & a, const float3 & b) { return{ a.x + b.x, a.y + b.y, a.z + b.z }; }
    inline float3 operator * (const float3 & a, float b) { return{ a.x*b, a.y*b, a.z*b }; }
    inline bool operator == (const float4 & a, const float4 & b) { return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w; }
    inline float4 operator + (const float4 & a, const float4 & b) { return{ a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w }; }
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

    struct stream_request
    {
        rs_stream stream;
        uint32_t width, height, fps;
        rs_format format;

        bool match(const stream_request& other) const;
        bool contradicts(const std::vector<stream_request>& requests) const;

        bool has_wildcards() const;
    };


    inline bool operator==(const stream_request& a,
        const stream_request& b)
    {
        return (a.width == b.width) &&
            (a.height == b.height) &&
            (a.fps == b.fps) &&
            (a.format == b.format) &&
            (a.stream == b.stream);
    }

    struct pixel_format_unpacker
    {
        bool requires_processing;
        void(*unpack)(byte * const dest[], const byte * source, int count);
        std::vector<std::pair<rs_stream, rs_format>> outputs;

        bool satisfies(const stream_request& request) const
        {
            return provides_stream(request.stream) &&
                get_format(request.stream) == request.format;
        }

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

    struct request_mapping
    {
        uvc::stream_profile profile;
        native_pixel_format* pf;
        pixel_format_unpacker* unpacker;
    };

    inline bool operator==(const request_mapping& a,
        const request_mapping& b)
    {
        return (a.profile == b.profile) && (a.pf == b.pf) && (a.unpacker == b.unpacker);
    }

    ////////////////////////
    // Static camera info //
    ////////////////////////

    //struct subdevice_mode
    //{
    //    int2 native_dims;                       // Resolution advertised over UVC
    //    native_pixel_format pf;                 // Pixel format advertised over UVC
    //    int fps;                                // Framerate advertised over UVC
    //    rs_intrinsics native_intrinsics;        // Intrinsics structure corresponding to the content of image (Note: width,height may be subset of native_dims)
    //    std::vector<rs_intrinsics> rect_modes;  // Potential intrinsics of image after being rectified in software by librealsense
    //    std::vector<int> pad_crop_options;      // Acceptable padding/cropping values
    //};

    //struct stream_request
    //{
    //    bool enabled;
    //    rs_stream stream;
    //    int width, height;
    //    rs_format format;
    //    int fps;
    //    rs_output_buffer_format output_format;

    //    bool contradict(stream_request req) const;
    //    bool is_filled() const;
    //};

    //struct interstream_rule
    //{
    //    rs_stream a, b;
    //    int stream_request::* field;
    //    int delta, delta2;
    //    rs_stream bigger;       // if this equals to a or b, this stream must have field value bigger then the other stream
    //    bool divides, divides2; // divides = a must divide b; divides2 = b must divide a
    //    bool same_format;
    //};

    //struct subdevice_mode_selection
    //{
    //    subdevice_mode mode;                    // The streaming mode in which to place the hardware
    //    int pad_crop;                           // The number of pixels of padding (positive values) or cropping (negative values) to apply to all four edges of the image
    //    size_t unpacker_index;                  // The specific unpacker used to unpack the encoded format into the desired output formats
    //    rs_output_buffer_format output_format = RS_OUTPUT_BUFFER_FORMAT_CONTINUOUS; // The output buffer format.

    //    subdevice_mode_selection() : mode({}), pad_crop(), unpacker_index(), output_format(RS_OUTPUT_BUFFER_FORMAT_CONTINUOUS) {}
    //    subdevice_mode_selection(const subdevice_mode & mode, int pad_crop, int unpacker_index) : mode(mode), pad_crop(pad_crop), unpacker_index(unpacker_index) {}

    //    const pixel_format_unpacker & get_unpacker() const {
    //        if ((size_t)unpacker_index < mode.pf.unpackers.size())
    //            return mode.pf.unpackers[unpacker_index];
    //        throw std::runtime_error("failed to fetch an unpakcer, most likely because enable_stream was not called!");
    //    }
    //    const std::vector<std::pair<rs_stream, rs_format>> & get_outputs() const { return get_unpacker().outputs; }
    //    int get_width() const { return mode.native_intrinsics.width + pad_crop * 2; }
    //    int get_height() const { return mode.native_intrinsics.height + pad_crop * 2; }
    //    int get_framerate() const { return mode.fps; }
    //    int get_stride_x() const { return requires_processing() ? get_width() : mode.native_dims.x; }
    //    int get_stride_y() const { return requires_processing() ? get_height() : mode.native_dims.y; }
    //    size_t get_image_size(rs_stream stream) const;
    //    bool provides_stream(rs_stream stream) const { return get_unpacker().provides_stream(stream); }
    //    rs_format get_format(rs_stream stream) const { return get_unpacker().get_format(stream); }
    //    void set_output_buffer_format(const rs_output_buffer_format in_output_format);

    //    void unpack(byte * const dest[], const byte * source) const;
    //    int get_unpacked_width() const;
    //    int get_unpacked_height() const;

    //    bool requires_processing() const { return (output_format == RS_OUTPUT_BUFFER_FORMAT_CONTINUOUS) || (mode.pf.unpackers[unpacker_index].requires_processing); }

    //};

    class device;

    //class device_config
    //{
    //    stream_request requests[RS_STREAM_NATIVE_COUNT]; // Modified by enable/disable_stream calls
    //    const device& dev;
    //    std::vector<subdevice_mode> subdevice_modes;

    //    subdevice_mode_selection select_mode(const stream_request(&requests)[RS_STREAM_NATIVE_COUNT], int subdevice_index) const;
    //    bool all_requests_filled(const stream_request(&original_requests)[RS_STREAM_NATIVE_COUNT]) const;
    //    bool find_valid_combination(stream_request(&output_requests)[RS_STREAM_NATIVE_COUNT], std::vector<stream_request> stream_requests[RS_STREAM_NATIVE_COUNT]) const;
    //    bool fill_requests(stream_request(&requests)[RS_STREAM_NATIVE_COUNT]) const;
    //    void get_all_possible_requestes(std::vector<stream_request>(&stream_requests)[RS_STREAM_NATIVE_COUNT]) const;
    //    std::vector<subdevice_mode_selection> select_modes(const stream_request(&requests)[RS_STREAM_NATIVE_COUNT]) const;
    //    bool validate_requests(stream_request(&requests)[RS_STREAM_NATIVE_COUNT], bool throw_exception = false) const;

    //public:
    //    std::vector<subdevice_mode_selection> select_modes() const { return select_modes(requests); }

    //    explicit device_config(device& dev)
    //        : dev(dev)
    //    {
    //        for (auto & req : requests) req = stream_request();
    //    }


    //};

    class firmware_version
    {
        int                 m_major, m_minor, m_patch, m_build;
        bool                is_any;
        std::string         string_representation;

        std::string to_string() const;
        static std::vector<std::string> split(const std::string& str);
        static int parse_part(const std::string& name, int part);

    public:
        firmware_version() : m_major(0), m_minor(0), m_patch(0), m_build(0), is_any(true), string_representation(to_string()) {}

        firmware_version(int major, int minor, int patch, int build, bool is_any = false)
            : m_major(major), m_minor(minor), m_patch(patch), m_build(build), is_any(is_any), string_representation(to_string()) {}

        static firmware_version any()
        {
            return{};
        }

        explicit firmware_version(const std::string& name)
            : m_major(parse_part(name, 0)), m_minor(parse_part(name, 1)), m_patch(parse_part(name, 2)), m_build(parse_part(name, 3)), is_any(false), string_representation(to_string()) {}

        bool operator<=(const firmware_version& other) const
        {
            if (is_any || other.is_any) return true;
            if (m_major > other.m_major) return false;
            if ((m_major == other.m_major) && (m_minor > other.m_minor)) return false;
            if ((m_major == other.m_major) && (m_minor == other.m_minor) && (m_patch > other.m_patch)) return false;
            if ((m_major == other.m_major) && (m_minor == other.m_minor) && (m_patch == other.m_patch) && (m_build > other.m_build)) return false;
            return true;
        }
        bool operator==(const firmware_version& other) const
        {
            return is_any || (other.m_major == m_major && other.m_minor == m_minor && other.m_patch == m_patch && other.m_build == m_build);
        }

        bool operator> (const firmware_version& other) const { return !(*this < other) || is_any; }
        bool operator!=(const firmware_version& other) const { return !(*this == other); }
        bool operator<(const firmware_version& other) const { return !(*this == other) && (*this <= other); }
        bool operator>=(const firmware_version& other) const { return (*this == other) || (*this > other); }

        bool is_between(const firmware_version& from, const firmware_version& until) const
        {
            return (from <= *this) && (*this <= until);
        }

        operator const char*() const
        {
            return string_representation.c_str();
        }
    };

    struct supported_capability
    {
        rs_stream           capability;
        firmware_version    from;
        firmware_version    until;
        rs_camera_info      firmware_type;

        supported_capability(rs_stream capability, firmware_version from, 
            firmware_version until, rs_camera_info firmware_type = RS_CAMERA_INFO_CAMERA_FIRMWARE_VERSION)
            : capability(capability), from(from), until(until), firmware_type(firmware_type) {}

        explicit supported_capability(rs_stream capability)
            : capability(capability), from(), until(), firmware_type(RS_CAMERA_INFO_CAMERA_FIRMWARE_VERSION) {}
    };

    // This class is used to buffer up several writes to a structure-valued XU control, and send the entire structure all at once
    // Additionally, it will ensure that any fields not set in a given struct will retain their original values
    template<class T, class R, class W> struct struct_interface
    {
        T struct_;
        R reader;
        W writer;
        bool active;

        struct_interface(R r, W w) : reader(r), writer(w), active(false) {}

        void activate() { if (!active) { struct_ = reader(); active = true; } }
        template<class U> double get(U T::* field) { activate(); return static_cast<double>(struct_.*field); }
        template<class U, class V> void set(U T::* field, V value) { activate(); struct_.*field = static_cast<U>(value); }
        void commit() { if (active) writer(struct_); }
    };

    template<class T, class R, class W> 
    std::shared_ptr<struct_interface<T, R, W>> make_struct_interface(R r, W w)
    {
        return std::make_shared<struct_interface<T, R, W>>(r, w);
    }

    template <typename T>
    class wraparound_mechanism
    {
    public:
        wraparound_mechanism(T min_value, T max_value)
            : max_number(max_value - min_value + 1), last_number(min_value), num_of_wraparounds(0)
        {}

        T fix(T number)
        {
            if ((number + (num_of_wraparounds*max_number)) < last_number)
                ++num_of_wraparounds;


            number += (num_of_wraparounds*max_number);
            last_number = number;
            return number;
        }

    private:
        T max_number;
        T last_number;
        unsigned long long num_of_wraparounds;
    };

    struct static_device_info
    {
        pose subdevice_poses[RS_SUBDEVICE_COUNT];                           // Static pose of each camera on the device
        float nominal_depth_scale;                                          // Default scale
        std::vector<rs_frame_metadata> supported_metadata_vector;
        std::map<rs_camera_info, std::string> camera_info;
        std::vector<supported_capability> capabilities_vector;
    };

    typedef void(*frame_callback_function_ptr)(rs_frame * frame, void * user);

    class frame_callback : public rs_frame_callback
    {
        frame_callback_function_ptr fptr;
        void * user;
    public:
        frame_callback() : frame_callback(nullptr, nullptr, nullptr) {}
        frame_callback(rs_streaming_lock * lock, frame_callback_function_ptr on_frame, void * user) : fptr(on_frame), user(user) {}

        operator bool() const { return fptr != nullptr; }
        void on_frame (rs_frame * frame) override { 
            if (fptr)
            {
                try { fptr(frame, user); } catch (...) 
                {
                    LOG_ERROR("Received an execption from frame callback!");
                }
            }
        }
        void release() override { delete this; }
    };

    typedef std::unique_ptr<rs_log_callback, void(*)(rs_log_callback*)> log_callback_ptr;
    typedef std::unique_ptr<rs_frame_callback, void(*)(rs_frame_callback*)> frame_callback_ptr;

    /*struct device_config
    {
        const static_device_info            info;
        stream_request                      requests[RS_STREAM_NATIVE_COUNT];                       // Modified by enable/disable_stream calls
        //frame_callback_ptr                  callbacks[RS_STREAM_NATIVE_COUNT];                      // Modified by set_frame_callback calls
        data_polling_request                data_request;                                           // Modified by enable/disable_events calls
        //motion_callback_ptr                 motion_callback{ nullptr, [](rs_motion_callback*){} };  // Modified by set_events_callback calls
       // timestamp_callback_ptr              timestamp_callback{ nullptr, [](rs_timestamp_callback*){} };
        float depth_scale;                                              // Scale of depth values

        explicit device_config(const rsimpl::static_device_info & info) : info(info), depth_scale(info.nominal_depth_scale)
        {
            for (auto & req : requests) req = rsimpl::stream_request();
        }

        subdevice_mode_selection select_mode(const stream_request(&requests)[RS_STREAM_NATIVE_COUNT], int subdevice_index) const;
        bool all_requests_filled(const stream_request(&original_requests)[RS_STREAM_NATIVE_COUNT]) const;
        bool find_valid_combination(stream_request(&output_requests)[RS_STREAM_NATIVE_COUNT], std::vector<stream_request> stream_requests[RS_STREAM_NATIVE_COUNT]) const;
        bool fill_requests(stream_request(&requests)[RS_STREAM_NATIVE_COUNT]) const;
        void get_all_possible_requestes(std::vector<stream_request> (&stream_requests)[RS_STREAM_NATIVE_COUNT]) const;
        std::vector<subdevice_mode_selection> select_modes(const stream_request(&requests)[RS_STREAM_NATIVE_COUNT]) const;
        std::vector<subdevice_mode_selection> select_modes() const { return select_modes(requests); }
        bool validate_requests(stream_request(&requests)[RS_STREAM_NATIVE_COUNT], bool throw_exception = false) const;
    };*/

    ////////////////////////////////////////
    // Helper functions for library types //
    ////////////////////////////////////////

    inline rs_intrinsics pad_crop_intrinsics(const rs_intrinsics & i, int pad_crop)
    {
        return{ i.width + pad_crop * 2, i.height + pad_crop * 2, i.ppx + pad_crop, i.ppy + pad_crop, 
            i.fx, i.fy, i.model, {i.coeffs[0], i.coeffs[1], i.coeffs[2], i.coeffs[3], i.coeffs[4]} };
    }

    inline rs_intrinsics scale_intrinsics(const rs_intrinsics & i, int width, int height)
    {
        const float sx = static_cast<float>(width) / i.width, sy = static_cast<float>(height) / i.height;
        return{ width, height, i.ppx*sx, i.ppy*sy, i.fx*sx, i.fy*sy, i.model, 
                {i.coeffs[0], i.coeffs[1], i.coeffs[2], i.coeffs[3], i.coeffs[4]} };
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
            auto old_value = std::move(buffer[i]);
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
                return is_empty();
            };
            if (!ready() && !cv.wait_for(lock, std::chrono::hours(1000), ready)) // for some reason passing std::chrono::duration::max makes it return instantly
            {
                throw std::runtime_error("Could not flush one of the user controlled objects!");
            }
        }

        bool is_empty() const { return size == 0; }
        int get_size() const { return size; }
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

        void reset()
        {
            protected_data = nullptr;
            continuation = [](){};
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

    // this class is a convinience wrapper for intrinsics / extrinsics validation methods
    class calibration_validator 
    {
    public:
        calibration_validator(std::function<bool(rs_stream, rs_stream)> extrinsic_validator,
                              std::function<bool(rs_stream)>            intrinsic_validator);
        calibration_validator();

        bool validate_extrinsics(rs_stream from_stream, rs_stream to_stream) const;
        bool validate_intrinsics(rs_stream stream) const;

    private:
        std::function<bool(rs_stream from_stream, rs_stream to_stream)> extrinsic_validator;
        std::function<bool(rs_stream stream)> intrinsic_validator;
    };

    inline bool check_not_all_zeros(std::vector<byte> data)
    {
        return std::find_if(data.begin(), data.end(), [](byte b){ return b!=0; }) != data.end();
    }

    ///////////////////////////////////////////
    // Extrinsic auxillary routines routines //
    ///////////////////////////////////////////
    float3x3 calc_rodrigues_matrix(const std::vector<double> rot);
    // Auxillary function that calculates standard 32bit CRC code. used in verificaiton
    uint32_t calc_crc32(uint8_t *buf, size_t bufsize);
}

namespace std {

    template <>
    struct hash<rsimpl::stream_request>
    {
        size_t operator()(const rsimpl::stream_request& k) const
        {
            using std::hash;

            return (hash<uint32_t>()(k.height))
                ^ (hash<uint32_t>()(k.width))
                ^ (hash<uint32_t>()(k.fps))
                ^ (hash<uint32_t>()(k.format))
                ^ (hash<uint32_t>()(k.stream));
        }
    };

    template <>
    struct hash<rsimpl::uvc::stream_profile>
    {
        size_t operator()(const rsimpl::uvc::stream_profile& k) const
        {
            using std::hash;

            return (hash<uint32_t>()(k.height))
                ^ (hash<uint32_t>()(k.width))
                ^ (hash<uint32_t>()(k.fps))
                ^ (hash<uint32_t>()(k.format));
        }
    };

    template <>
    struct hash<rsimpl::request_mapping>
    {
        size_t operator()(const rsimpl::request_mapping& k) const
        {
            using std::hash;

            return (hash<rsimpl::uvc::stream_profile>()(k.profile))
                ^ (hash<rsimpl::pixel_format_unpacker*>()(k.unpacker))
                ^ (hash<rsimpl::native_pixel_format*>()(k.pf));
        }
    };
}

#endif
