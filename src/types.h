// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

// This header defines vocabulary types and utility mechanisms used ubiquitously by the
// rest of the library. As clearer module boundaries form, declarations might be moved
// out of this file and into more appropriate locations.

#pragma once
#ifndef LIBREALSENSE_TYPES_H
#define LIBREALSENSE_TYPES_H

#include "../include/librealsense2/hpp/rs_types.hpp"

#include <stdint.h>
#include <cassert>                          // For assert
#include <cstring>                          // For memcmp
#include <vector>                           // For vector
#include <sstream>                          // For ostringstream
#include <mutex>                            // For mutex, unique_lock
#include <memory>                           // For unique_ptr
#include <map>
#include <limits>
#include <algorithm>
#include <condition_variable>
#include <functional>
#include <utility>                          // For std::forward
#include "backend.h"
#include "concurrency.h"
#if BUILD_EASYLOGGINGPP
#include "../third-party/easyloggingpp/src/easylogging++.h"
#endif // BUILD_EASYLOGGINGPP

typedef unsigned char byte;

const int RS2_USER_QUEUE_SIZE = 128;

#ifndef DBL_EPSILON
const double DBL_EPSILON = 2.2204460492503131e-016;  // smallest such that 1.0+DBL_EPSILON != 1.0
#endif

#pragma warning(disable: 4250)

#ifdef ANDROID
#include "../common/android_helpers.h"
#endif

namespace librealsense
{
    #define UNKNOWN_VALUE "UNKNOWN"

    ///////////////////////////////////
    // Utility types for general use //
    ///////////////////////////////////
    struct to_string
    {
        std::ostringstream ss;
        template<class T> to_string & operator << (const T & val) { ss << val; return *this; }
        operator std::string() const { return ss.str(); }
    };

    template<typename T, size_t size>
    inline size_t copy_array(T(&dst)[size], const T(&src)[size])
    {
        assert(dst != nullptr && src != nullptr);
        for (size_t i = 0; i < size; i++)
        {
            dst[i] = src[i];
        }
        return size;
    }

    template<typename T, size_t sizem, size_t sizen>
    inline size_t copy_2darray(T(&dst)[sizem][sizen], const T(&src)[sizem][sizen])
    {
        assert(dst != nullptr && src != nullptr);
        for (size_t i = 0; i < sizem; i++)
        {
            for (size_t j = 0; j < sizen; j++)
            {
                dst[i][j] = src[i][j];
            }
        }
        return sizem * sizen;
    }

    void copy(void* dst, void const* src, size_t size);

    std::string make_less_screamy(const char* str);

    ///////////////////////
    // Logging mechanism //
    ///////////////////////

    void log_to_console(rs2_log_severity min_severity);
    void log_to_file(rs2_log_severity min_severity, const char * file_path);

#if BUILD_EASYLOGGINGPP

#define LOG_DEBUG(...)   do { CLOG(DEBUG   ,"librealsense") << __VA_ARGS__; } while(false)
#define LOG_INFO(...)    do { CLOG(INFO    ,"librealsense") << __VA_ARGS__; } while(false)
#define LOG_WARNING(...) do { CLOG(WARNING ,"librealsense") << __VA_ARGS__; } while(false)
#define LOG_ERROR(...)   do { CLOG(ERROR   ,"librealsense") << __VA_ARGS__; } while(false)
#define LOG_FATAL(...)   do { CLOG(FATAL   ,"librealsense") << __VA_ARGS__; } while(false)

#else // BUILD_EASYLOGGINGPP

#define LOG_DEBUG(...)   do { ; } while(false)
#define LOG_INFO(...)    do { ; } while(false)
#define LOG_WARNING(...) do { ; } while(false)
#define LOG_ERROR(...)   do { ; } while(false)
#define LOG_FATAL(...)   do { ; } while(false)

#endif // BUILD_EASYLOGGINGPP

    // Enhancement for debug mode that incurs performance penalty with STL
    // std::clamp to be introduced with c++17
    template< typename T>
    inline T clamp_val(T val, const T& min, const T& max)
    {
        static_assert((std::is_arithmetic<T>::value), "clamping supports arithmetic built-in types only");
#ifdef _DEBUG
        const T t = val < min ? min : val;
        return t > max ? max : t;
#else
        return std::min(std::max(val, min), max);
#endif
    }

    //////////////////////////
    // Exceptions mechanism //
    //////////////////////////


    class librealsense_exception : public std::exception
    {
    public:
        const char* get_message() const noexcept
        {
            return _msg.c_str();
        }

        rs2_exception_type get_exception_type() const noexcept
        {
            return _exception_type;
        }

        const char* what() const noexcept override
        {
            return _msg.c_str();
        }

    protected:
        librealsense_exception(const std::string& msg,
                               rs2_exception_type exception_type) noexcept
            : _msg(msg),
              _exception_type(exception_type)
        {}

    private:
        std::string _msg;
        rs2_exception_type _exception_type;
    };

    class recoverable_exception : public librealsense_exception
    {
    public:
        recoverable_exception(const std::string& msg,
            rs2_exception_type exception_type) noexcept;
    };

    class unrecoverable_exception : public librealsense_exception
    {
    public:
        unrecoverable_exception(const std::string& msg,
                                rs2_exception_type exception_type) noexcept
            : librealsense_exception(msg, exception_type)
        {
            LOG_ERROR(msg);
        }
    };
    class io_exception : public unrecoverable_exception
    {
    public:
        io_exception(const std::string& msg) noexcept
            : unrecoverable_exception(msg, RS2_EXCEPTION_TYPE_IO)
        {}
    };
    class camera_disconnected_exception : public unrecoverable_exception
    {
    public:
        camera_disconnected_exception(const std::string& msg) noexcept
            : unrecoverable_exception(msg, RS2_EXCEPTION_TYPE_CAMERA_DISCONNECTED)
        {}
    };

    class backend_exception : public unrecoverable_exception
    {
    public:
        backend_exception(const std::string& msg,
                          rs2_exception_type exception_type) noexcept
            : unrecoverable_exception(msg, exception_type)
        {}
    };

    class linux_backend_exception : public backend_exception
    {
    public:
        linux_backend_exception(const std::string& msg) noexcept
            : backend_exception(generate_last_error_message(msg), RS2_EXCEPTION_TYPE_BACKEND)
        {}

    private:
        std::string generate_last_error_message(const std::string& msg) const
        {
            return msg + " Last Error: " + strerror(errno);
        }
    };

    class windows_backend_exception : public backend_exception
    {
    public:
        // TODO: get last error
        windows_backend_exception(const std::string& msg) noexcept
            : backend_exception(msg, RS2_EXCEPTION_TYPE_BACKEND)
        {}
    };

    class invalid_value_exception : public recoverable_exception
    {
    public:
        invalid_value_exception(const std::string& msg) noexcept
            : recoverable_exception(msg, RS2_EXCEPTION_TYPE_INVALID_VALUE)
        {}
    };

    class wrong_api_call_sequence_exception : public recoverable_exception
    {
    public:
        wrong_api_call_sequence_exception(const std::string& msg) noexcept
            : recoverable_exception(msg, RS2_EXCEPTION_TYPE_WRONG_API_CALL_SEQUENCE)
        {}
    };

    class not_implemented_exception : public recoverable_exception
    {
    public:
        not_implemented_exception(const std::string& msg) noexcept
            : recoverable_exception(msg, RS2_EXCEPTION_TYPE_NOT_IMPLEMENTED)
        {}
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

    template <class T>
    class lazy
    {
    public:
        lazy() : _init([]() { T t{}; return t; }) {}
        lazy(std::function<T()> initializer) : _init(std::move(initializer)) {}

        T* operator->() const
        {
            return operate();
        }

        T& operator*()
        {
            return *operate();
        }

        const T& operator*() const
        {
            return *operate();
        }

        lazy(lazy&& other) noexcept
        {
            std::lock_guard<std::mutex> lock(other._mtx);
            if (!other._was_init)
            {
                _init = move(other._init);
                _was_init = false;
            }
            else
            {
                _init = move(other._init);
                _was_init = true;
                _ptr = move(other._ptr);
            }
        }

        lazy& operator=(std::function<T()> func) noexcept
        {
            return *this = lazy<T>(std::move(func));
        }

        lazy& operator=(lazy&& other) noexcept
        {
            std::lock_guard<std::mutex> lock1(_mtx);
            std::lock_guard<std::mutex> lock2(other._mtx);
            if (!other._was_init)
            {
                _init = move(other._init);
                _was_init = false;
            }
            else
            {
                _init = move(other._init);
                _was_init = true;
                _ptr = move(other._ptr);
            }

            return *this;
        }

    private:
        T* operate() const
        {
            std::lock_guard<std::mutex> lock(_mtx);
            if (!_was_init)
            {
                _ptr = std::unique_ptr<T>(new T(_init()));
                _was_init = true;
            }
            return _ptr.get();
        }

        mutable std::mutex _mtx;
        mutable bool _was_init = false;
        std::function<T()> _init;
        mutable std::unique_ptr<T> _ptr;
    };

    class unique_id
    {
    public:
        static uint64_t generate_id()
        {
            static std::atomic<uint64_t> id(0);
            return ++id;
        }

        unique_id(const unique_id&) = delete;
        unique_id& operator=(const unique_id&) = delete;
    };

    template<typename T, int sz>
    int arr_size(T(&)[sz])
    {
        return sz;
    }

    template<typename T>
    std::string array2str(T& data)
    {
        std::stringstream ss;
        for (auto i = 0; i < arr_size(data); i++)
            ss << " [" << i << "] = " << data[i] << "\t";
        return ss.str();
    }

    typedef float float_4[4];

    /////////////////////////////
    // Enumerated type support //
    /////////////////////////////


#define RS2_ENUM_HELPERS(TYPE, PREFIX) const char* get_string(TYPE value); \
        inline bool is_valid(TYPE value) { return value >= 0 && value < RS2_##PREFIX##_COUNT; } \
        inline std::ostream & operator << (std::ostream & out, TYPE value) { if(is_valid(value)) return out << get_string(value); else return out << (int)value; } \
        inline bool try_parse(const std::string& str, TYPE& res)       \
        {                                                            \
            for (int i = 0; i < static_cast<int>(RS2_ ## PREFIX ## _COUNT); i++) {  \
                auto v = static_cast<TYPE>(i);                       \
                if(str == get_string(v)) { res = v; return true; }   \
            }                                                        \
            return false;                                            \
        }

    RS2_ENUM_HELPERS(rs2_stream, STREAM)
    RS2_ENUM_HELPERS(rs2_format, FORMAT)
    RS2_ENUM_HELPERS(rs2_distortion, DISTORTION)
    RS2_ENUM_HELPERS(rs2_option, OPTION)
    RS2_ENUM_HELPERS(rs2_camera_info, CAMERA_INFO)
    RS2_ENUM_HELPERS(rs2_frame_metadata_value, FRAME_METADATA)
    RS2_ENUM_HELPERS(rs2_timestamp_domain, TIMESTAMP_DOMAIN)
    RS2_ENUM_HELPERS(rs2_sr300_visual_preset, SR300_VISUAL_PRESET)
    RS2_ENUM_HELPERS(rs2_extension, EXTENSION)
    RS2_ENUM_HELPERS(rs2_exception_type, EXCEPTION_TYPE)
    RS2_ENUM_HELPERS(rs2_log_severity, LOG_SEVERITY)
    RS2_ENUM_HELPERS(rs2_notification_category, NOTIFICATION_CATEGORY)
    RS2_ENUM_HELPERS(rs2_playback_status, PLAYBACK_STATUS)

    ////////////////////////////////////////////
    // World's tiniest linear algebra library //
    ////////////////////////////////////////////
#pragma pack(push, 1)
    struct int2 { int x, y; };
    struct float2 { float x, y; float & operator [] (int i) { return (&x)[i]; } };
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
    inline pose to_pose(const rs2_extrinsics& a)
    {
        pose r{};
        for (int i = 0; i < 3; i++) r.position[i] = a.translation[i];
        for (int j = 0; j < 3; j++)
            for (int i = 0; i < 3; i++)
                r.orientation(i, j) = a.rotation[j * 3 + i];
        return r;
    }
    inline rs2_extrinsics from_pose(pose a)
    {
        rs2_extrinsics r;
        for (int i = 0; i < 3; i++) r.translation[i] = a.position[i];
        for (int j = 0; j < 3; j++)
            for (int i = 0; i < 3; i++)
                r.rotation[j * 3 + i] = a.orientation(i, j);
        return r;
    }
    inline rs2_extrinsics identity_matrix() {
        rs2_extrinsics r;
        // Do it the silly way to avoid infite warnings about the dangers of memset
        for (int i = 0; i < 3; i++) r.translation[i] = 0.f;
        for (int j = 0; j < 3; j++)
            for (int i = 0; i < 3; i++)
                r.rotation[j * 3 + i] = (i == j) ? 1.f : 0.f;
        return r;
    }
    inline rs2_extrinsics inverse(const rs2_extrinsics& a) { auto p = to_pose(a); return from_pose(inverse(p)); }

    ///////////////////
    // Pixel formats //
    ///////////////////

    typedef std::tuple<uint32_t, int, size_t> native_pixel_format_tuple;
    typedef std::tuple<rs2_stream, int, rs2_format> output_tuple;
    typedef std::tuple<platform::stream_profile_tuple, native_pixel_format_tuple, std::vector<output_tuple>> request_mapping_tuple;

    struct stream_profile
    {
        rs2_stream stream;
        int index;
        uint32_t width, height, fps;
        rs2_format format;
    };


    inline bool operator==(const stream_profile& a,
        const stream_profile& b)
    {
        return (a.width == b.width) &&
            (a.height == b.height) &&
            (a.fps == b.fps) &&
            (a.format == b.format) &&
            (a.index == b.index);
    }

    struct stream_descriptor
    {
        stream_descriptor() : type(RS2_STREAM_ANY), index(0) {}
        stream_descriptor(rs2_stream type, int index = 0) : type(type), index(index) {}

        rs2_stream type;
        int index;
    };

    struct pixel_format_unpacker
    {
        bool requires_processing;
        void(*unpack)(byte * const dest[], const byte * source, int count);
        std::vector<std::pair<stream_descriptor, rs2_format>> outputs;

        bool satisfies(const stream_profile& request) const
        {
            return provides_stream(request.stream, request.index) &&
                get_format(request.stream, request.index) == request.format;
        }

        bool provides_stream(rs2_stream stream, int index) const
        {
            for (auto & o : outputs)
                if (o.first.type == stream && o.first.index == index)
                    return true;

            return false;
        }
        rs2_format get_format(rs2_stream stream, int index) const
        {
            for (auto & o : outputs)
                if (o.first.type == stream && o.first.index == index)
                    return o.second;

            throw invalid_value_exception("missing output");
        }

        operator std::vector<output_tuple>()
        {
            std::vector<output_tuple> tuple_outputs;

            for (auto output : outputs)
            {
                tuple_outputs.push_back(std::make_tuple(output.first.type, output.first.index, output.second));
            }
            return tuple_outputs;
        }

    };

    struct native_pixel_format
    {
        uint32_t fourcc;
        int plane_count;
        size_t bytes_per_pixel;
        std::vector<pixel_format_unpacker> unpackers;

        size_t get_image_size(int width, int height) const { return width * height * plane_count * bytes_per_pixel; }

        operator native_pixel_format_tuple() const
        {
            return std::make_tuple(fourcc, plane_count, bytes_per_pixel);
        }
    };

    class stream_profile_interface;

    struct request_mapping
    {
        platform::stream_profile profile;
        native_pixel_format* pf;
        pixel_format_unpacker* unpacker;

        // The request lists is there just for lookup and is not involved in object comparison
        mutable std::vector<std::shared_ptr<stream_profile_interface>> original_requests;

        operator request_mapping_tuple() const
        {
            return std::make_tuple(profile, *pf, *unpacker);
        }

        bool requires_processing() const { return unpacker->requires_processing; }

    };

    inline bool operator< (const request_mapping& first, const request_mapping& second)
    {
        return request_mapping_tuple(first) < request_mapping_tuple(second);
    }

    inline bool operator==(const request_mapping& a,
        const request_mapping& b)
    {
        return (a.profile == b.profile) && (a.pf == b.pf) && (a.unpacker == b.unpacker);
    }

    class frame_interface;

    struct frame_holder
    {
        frame_interface* frame;

        frame_interface* operator->()
        {
            return frame;
        }

        operator bool() const { return frame != nullptr; }

        operator frame_interface*() const { return frame; }

        frame_holder(frame_interface* f)
        {
            frame = f;
        }

        ~frame_holder();

        frame_holder(frame_holder&& other)
            : frame(other.frame)
        {
            other.frame = nullptr;
        }

        frame_holder() : frame(nullptr) {}


        frame_holder& operator=(frame_holder&& other);

        frame_holder clone() const;
    private:
        frame_holder& operator=(const frame_holder& other) = delete;
        frame_holder(const frame_holder& other);
    };

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

        operator std::string() const
        {
            return string_representation.c_str();
        }
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

    // Provides an efficient wraparound for built-in arithmetic times, for use-cases such as a rolling timestamp
    template <typename T, typename S>
    class arithmetic_wraparound
    {
    public:
        arithmetic_wraparound() :
            last_input(std::numeric_limits<T>::lowest()), accumulated(0) {
            static_assert(
                (std::is_arithmetic<T>::value) &&
                (std::is_arithmetic<S>::value) &&
                (std::numeric_limits<T>::max() < std::numeric_limits<S>::max()) &&
                (std::numeric_limits<T>::lowest() >= std::numeric_limits<S>::lowest())
                , "Wraparound class requirements are not met");
        }

        S calc(const T input)
        {
            accumulated += static_cast<T>(input - last_input); // Automatically resolves wraparounds
            last_input = input;
            return (accumulated);
        }

        void reset() { last_input = std::numeric_limits<T>::lowest();  accumulated = 0; }

    private:
        T last_input;
        S accumulated;
    };

    typedef void(*frame_callback_function_ptr)(rs2_frame * frame, void * user);

    class frame_callback : public rs2_frame_callback
    {
        frame_callback_function_ptr fptr;
        void * user;
    public:
        frame_callback() : frame_callback(nullptr, nullptr) {}
        frame_callback(frame_callback_function_ptr on_frame, void * user) : fptr(on_frame), user(user) {}

        operator bool() const { return fptr != nullptr; }
        void on_frame (rs2_frame * frame) override {
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

    template<class T>
    class internal_frame_callback : public rs2_frame_callback
    {
        T on_frame_function; //Callable of type: void(frame_interface* frame)
    public:
        explicit internal_frame_callback(T on_frame) : on_frame_function(on_frame) {}

        void on_frame(rs2_frame* fref) override
        {
            on_frame_function((frame_interface*)(fref));
        }

        void release() override { delete this; }
    };

    typedef void(*notifications_callback_function_ptr)(rs2_notification * notification, void * user);

    class notifications_callback : public rs2_notifications_callback
    {
        notifications_callback_function_ptr nptr;
        void * user;
    public:
        notifications_callback() : notifications_callback(nullptr, nullptr) {}
        notifications_callback(notifications_callback_function_ptr on_notification, void * user) : nptr(on_notification), user(user) {}

        operator bool() const { return nptr != nullptr; }
        void on_notification(rs2_notification * notification) override {
            if (nptr)
            {
                try { nptr(notification, user); }
                catch (...)
                {
                    LOG_ERROR("Received an execption from frame callback!");
                }
            }
        }
        void release() override { delete this; }
    };

    typedef void(*devices_changed_function_ptr)(rs2_device_list* removed, rs2_device_list* added, void * user);

    class devices_changed_callback: public rs2_devices_changed_callback
    {
        devices_changed_function_ptr nptr;
        void * user;
    public:
        devices_changed_callback() : devices_changed_callback(nullptr, nullptr) {}
        devices_changed_callback(devices_changed_function_ptr on_devices_changed, void * user) : nptr(on_devices_changed), user(user) {}

        operator bool() const { return nptr != nullptr; }
        void on_devices_changed(rs2_device_list* removed, rs2_device_list* added) override {
            if (nptr)
            {
                try { nptr(removed, added, user); }
                catch (...)
                {
                    LOG_ERROR("Received an execption from frame callback!");
                }
            }
        }
        void release() override { delete this; }
    };

    typedef std::unique_ptr<rs2_log_callback, void(*)(rs2_log_callback*)> log_callback_ptr;
    typedef std::shared_ptr<rs2_frame_callback> frame_callback_ptr;
    typedef std::shared_ptr<rs2_frame_processor_callback> frame_processor_callback_ptr;
    typedef std::shared_ptr<rs2_notifications_callback> notifications_callback_ptr;
    typedef std::shared_ptr<rs2_devices_changed_callback> devices_changed_callback_ptr;

    using internal_callback = std::function<void(rs2_device_list* removed, rs2_device_list* added)>;
    class devices_changed_callback_internal : public rs2_devices_changed_callback
    {
        internal_callback _callback;

    public:
        explicit devices_changed_callback_internal(internal_callback callback)
            : _callback(callback)
        {}

        void on_devices_changed(rs2_device_list* removed, rs2_device_list* added) override
        {
            _callback(removed , added);
        }

        void release() override { delete this; }
    };


    struct notification
    {
        notification(rs2_notification_category category, int type, rs2_log_severity severity, std::string description)
            :category(category), type(type), severity(severity), description(description)
        {
            timestamp = std::chrono::duration<double, std::milli>(std::chrono::system_clock::now().time_since_epoch()).count();
            LOG_INFO(description);
        }

        rs2_notification_category category;
        int type;
        rs2_log_severity severity;
        std::string description;
        double timestamp;
        std::string serialized_data;
    };


    class notification_decoder
    {
    public:
        virtual ~notification_decoder() = default;
        virtual notification decode(int value) = 0;
    };

    class notifications_proccessor
    {
    public:
        notifications_proccessor();
        ~notifications_proccessor();

        void set_callback(notifications_callback_ptr callback);
        notifications_callback_ptr get_callback() const;
        void raise_notification(const notification);

    private:
        notifications_callback_ptr _callback;
        std::mutex _callback_mutex;
        dispatcher _dispatcher;
    };
    ////////////////////////////////////////
    // Helper functions for library types //
    ////////////////////////////////////////

    inline rs2_intrinsics pad_crop_intrinsics(const rs2_intrinsics & i, int pad_crop)
    {
        return{ i.width + pad_crop * 2, i.height + pad_crop * 2, i.ppx + pad_crop, i.ppy + pad_crop,
            i.fx, i.fy, i.model, {i.coeffs[0], i.coeffs[1], i.coeffs[2], i.coeffs[3], i.coeffs[4]} };
    }

    inline rs2_intrinsics scale_intrinsics(const rs2_intrinsics & i, int width, int height)
    {
        const float sx = static_cast<float>(width) / i.width, sy = static_cast<float>(height) / i.height;
        return{ width, height, i.ppx*sx, i.ppy*sy, i.fx*sx, i.fy*sy, i.model,
                {i.coeffs[0], i.coeffs[1], i.coeffs[2], i.coeffs[3], i.coeffs[4]} };
    }

    inline bool operator == (const rs2_intrinsics & a, const rs2_intrinsics & b) { return std::memcmp(&a, &b, sizeof(a)) == 0; }

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
                throw invalid_value_exception("Trying to return item to a heap that didn't allocate it!");
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
                throw invalid_value_exception("Could not flush one of the user controlled objects!");
            }
        }

        bool is_empty() const { return size == 0; }
        int get_size() const { return size; }
    };

    struct uvc_device_info
    {
        std::string id = ""; // to distinguish between different pins of the same device
        uint16_t vid;
        uint16_t pid;
        uint16_t mi;
        std::string unique_id;
        std::string device_path;

        operator std::string()
        {
            std::stringstream s;
            s << "id- " << id <<
                "\nvid- " << std::hex << vid <<
                "\npid- " << std::hex << pid <<
                "\nmi- " << mi <<
                "\nunique_id- " << unique_id <<
                "\npath- " << device_path;

            return s.str();
        }
    };

    inline bool operator==(const uvc_device_info& a,
                    const uvc_device_info& b)
    {
        return (a.vid == b.vid) &&
               (a.pid == b.pid) &&
               (a.mi == b.mi) &&
               (a.unique_id == b.unique_id) &&
               (a.id == b.id) &&
               (a.device_path == b.device_path);
    }

    struct usb_device_info
    {
        std::string id;

        uint16_t vid;
        uint16_t pid;
        uint16_t mi;
        std::string unique_id;

        operator std::string()
        {
            std::stringstream s;

            s << "vid- " << std::hex << vid <<
                "\npid- " << std::hex << pid <<
                "\nmi- " << mi <<
                "\nunique_id- " << unique_id;

            return s.str();
        }
    };

    inline bool operator==(const usb_device_info& a,
        const usb_device_info& b)
    {
        return  (a.id == b.id) &&
            (a.vid == b.vid) &&
            (a.pid == b.pid) &&
            (a.mi == b.mi) &&
            (a.unique_id == b.unique_id);
    }

    struct hid_device_info
    {
        std::string id;
        std::string vid;
        std::string pid;
        std::string unique_id;
        std::string device_path;

        operator std::string()
        {
            std::stringstream s;
            s << "id- " << id <<
                "\nvid- " << std::hex << vid <<
                "\npid- " << std::hex << pid <<
                "\nunique_id- " << unique_id <<
                "\npath- " << device_path;

            return s.str();
        }
    };

    inline bool operator==(const hid_device_info& a,
        const hid_device_info& b)
    {
        return  (a.id == b.id) &&
            (a.vid == b.vid) &&
            (a.pid == b.pid) &&
            (a.unique_id == b.unique_id) &&
            (a.device_path == b.device_path);
    }


    struct devices_data
    {
        devices_data(std::vector<uvc_device_info>  uvc_devices, std::vector<usb_device_info> usb_devices, std::vector<hid_device_info> hid_devices)
            :_uvc_devices(uvc_devices), _usb_devices(usb_devices), _hid_devices(hid_devices) {}

        devices_data(std::vector<usb_device_info> usb_devices)
            :_usb_devices(usb_devices) {}

        devices_data(std::vector<uvc_device_info> uvc_devices, std::vector<usb_device_info> usb_devices)
            :_uvc_devices(uvc_devices), _usb_devices(usb_devices) {}

        std::vector<uvc_device_info> _uvc_devices;
        std::vector<usb_device_info> _usb_devices;
        std::vector<hid_device_info> _hid_devices;

        bool operator == (const devices_data& other)
        {
            return !list_changed(_uvc_devices, other._uvc_devices) &&
                !list_changed(_hid_devices, other._hid_devices);
        }

        operator std::string()
        {
            std::string s;
            s = _uvc_devices.size()>0 ? "uvc devices:\n" : "";
            for (auto uvc : _uvc_devices)
            {
                s += uvc;
                s += "\n\n";
            }

            s += _usb_devices.size()>0 ? "usb devices:\n" : "";
            for (auto usb : _usb_devices)
            {
                s += usb;
                s += "\n\n";
            }

            s += _hid_devices.size()>0 ? "hid devices: \n" : "";
            for (auto hid : _hid_devices)
            {
                s += hid;
                s += "\n\n";
            }
            return s;
        }
    };


    typedef std::function<void(devices_data old, devices_data curr)> device_changed_callback;
    struct callback_invocation
    {
        std::chrono::high_resolution_clock::time_point started;
        std::chrono::high_resolution_clock::time_point ended;
    };

    typedef librealsense::small_heap<callback_invocation, 1> callbacks_heap;

    struct callback_invocation_holder
    {
        callback_invocation_holder() : invocation(nullptr), owner(nullptr) {}
        callback_invocation_holder(const callback_invocation_holder&) = delete;
        callback_invocation_holder& operator=(const callback_invocation_holder&) = delete;

        callback_invocation_holder(callback_invocation_holder&& other)
            : invocation(other.invocation), owner(other.owner)
        {
            other.invocation = nullptr;
        }

        callback_invocation_holder(callback_invocation* invocation, callbacks_heap* owner)
            : invocation(invocation), owner(owner)
        { }

        ~callback_invocation_holder()
        {
            if (invocation) owner->deallocate(invocation);
        }

        callback_invocation_holder& operator=(callback_invocation_holder&& other)
        {
            invocation = other.invocation;
            owner = other.owner;
            other.invocation = nullptr;
            return *this;
        }

        operator bool()
        {
            return invocation != nullptr;
        }

    private:
        callback_invocation* invocation;
        callbacks_heap* owner;
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
        calibration_validator(std::function<bool(rs2_stream, rs2_stream)> extrinsic_validator,
                              std::function<bool(rs2_stream)>            intrinsic_validator);
        calibration_validator();

        bool validate_extrinsics(rs2_stream from_stream, rs2_stream to_stream) const;
        bool validate_intrinsics(rs2_stream stream) const;

    private:
        std::function<bool(rs2_stream from_stream, rs2_stream to_stream)> extrinsic_validator;
        std::function<bool(rs2_stream stream)> intrinsic_validator;
    };

    inline bool check_not_all_zeros(std::vector<byte> data)
    {
        return std::find_if(data.begin(), data.end(), [](byte b){ return b!=0; }) != data.end();
    }

    std::string datetime_string();

    bool file_exists(const char* filename);

    ///////////////////////////////////////////
    // Extrinsic auxillary routines routines //
    ///////////////////////////////////////////
    float3x3 calc_rotation_from_rodrigues_angles(const std::vector<double> rot);
    // Auxillary function that calculates standard 32bit CRC code. used in verificaiton
    uint32_t calc_crc32(const uint8_t *buf, size_t bufsize);


    class polling_device_watcher: public librealsense::platform::device_watcher
    {
    public:
        polling_device_watcher(const platform::backend* backend_ref):
            _backend(backend_ref),_active_object([this](dispatcher::cancellable_timer cancellable_timer)
        {
            polling(cancellable_timer);
        }), _devices_data()
        {
        }

        ~polling_device_watcher()
        {
            stop();
        }

        void polling(dispatcher::cancellable_timer cancellable_timer)
        {
            if(cancellable_timer.try_sleep(100))
            {
               platform::backend_device_group curr(_backend->query_uvc_devices(), _backend->query_usb_devices(), _backend->query_hid_devices());

                if(list_changed(_devices_data.uvc_devices, curr.uvc_devices ) ||
                   list_changed(_devices_data.usb_devices, curr.usb_devices ) ||
                   list_changed(_devices_data.hid_devices, curr.hid_devices ))
                {
                    callback_invocation_holder callback = { _callback_inflight.allocate(), &_callback_inflight };
                    if(callback)
                    {
                        _callback(_devices_data, curr);
                        _devices_data = curr;
                    }
                }
            }
        }

        void start(platform::device_changed_callback callback) override
        {
            stop();
            _callback = std::move(callback);
            _devices_data = {   _backend->query_uvc_devices(),
                                _backend->query_usb_devices(),
                                _backend->query_hid_devices() };

            _active_object.start();
        }

        void stop() override
        {
            _active_object.stop();

            _callback_inflight.wait_until_empty();
        }

    private:
        active_object<> _active_object;

        callbacks_heap _callback_inflight;
        const platform::backend* _backend;

        platform::backend_device_group _devices_data;
        platform::device_changed_callback _callback;

    };


    template<typename HostingClass, typename... Args>
    class signal
    {
        friend HostingClass;
    public:
        signal()
        {
        }

        signal(signal&& other)
        {
            std::lock_guard<std::mutex> locker(other.m_mutex);
            m_subscribers = std::move(other.m_subscribers);

            other.m_subscribers.clear();
        }

        signal& operator=(signal&& other)
        {
            std::lock_guard<std::mutex> locker(other.m_mutex);
            m_subscribers = std::move(other.m_subscribers);

            other.m_subscribers.clear();
            return *this;
        }

        int subscribe(const std::function<void(Args...)>& func)
        {
            std::lock_guard<std::mutex> locker(m_mutex);

            int token = -1;
            for (int i = 0; i < (std::numeric_limits<int>::max)(); i++)
            {
                if (m_subscribers.find(i) == m_subscribers.end())
                {
                    token = i;
                    break;
                }
            }

            if (token != -1)
            {
                m_subscribers.emplace(token, func);
            }

            return token;
        }

        bool unsubscribe(int token)
        {
            std::lock_guard<std::mutex> locker(m_mutex);

            bool retVal = false;
            typename std::map<int, std::function<void(Args...)>>::iterator it = m_subscribers.find(token);
            if (it != m_subscribers.end())
            {
                m_subscribers.erase(token);
                retVal = true;
            }

            return retVal;
        }

        int operator+=(const std::function<void(Args...)>& func)
        {
            return subscribe(func);
        }

        bool operator-=(int token)
        {
            return unsubscribe(token);
        }

    private:
        signal(const signal& other);            // non construction-copyable
        signal& operator=(const signal&);       // non copyable

        bool raise(Args... args)
        {
            std::vector<std::function<void(Args...)>> functions;
            bool retVal = false;

            std::unique_lock<std::mutex> locker(m_mutex);
            if (m_subscribers.size() > 0)
            {
                typename std::map<int, std::function<void(Args...)>>::iterator it = m_subscribers.begin();
                while (it != m_subscribers.end())
                {
                    functions.emplace_back(it->second);
                    ++it;
                }
            }
            locker.unlock();

            if (functions.size() > 0)
            {
                for (auto func : functions)
                {
                    func(std::forward<Args>(args)...);
                }

                retVal = true;
            }

            return retVal;
        }

        bool operator()(Args... args)
        {
            return raise(std::forward<Args>(args)...);
        }

        std::mutex m_mutex;
        std::map<int, std::function<void(Args...)>> m_subscribers;
    };

    template <typename T>
    class optional_value
    {
    public:
        optional_value() : _valid(false) {}
        explicit optional_value(const T& v) : _valid(true), _value(v) {}

        operator bool() const
        {
            return has_value();
        }
        bool has_value() const
        {
            return _valid;
        }

        T& operator=(const T& v)
        {
            _valid = true;
            _value = v;
            return _value;
        }

        T& value() &
        {
            if (!_valid) throw std::runtime_error("bad optional access");
            return _value;
        }

        T&& value() &&
        {
            if (!_valid) throw std::runtime_error("bad optional access");
            return std::move(_value);
        }

        const T* operator->() const
        {
            return &_value;
        }
        T* operator->()
        {
            return &_value;
        }
        const T& operator*() const&
        {
            return _value;
        }
        T& operator*() &
        {
            return _value;
        }
        T&& operator*() &&
        {
            return std::move(_value);
        }
    private:
        bool _valid;
        T _value;
    };
}

namespace std {

    template <>
    struct hash<librealsense::stream_profile>
    {
        size_t operator()(const librealsense::stream_profile& k) const
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
    struct hash<librealsense::platform::stream_profile>
    {
        size_t operator()(const librealsense::platform::stream_profile& k) const
        {
            using std::hash;

            return (hash<uint32_t>()(k.height))
                ^ (hash<uint32_t>()(k.width))
                ^ (hash<uint32_t>()(k.fps))
                ^ (hash<uint32_t>()(k.format));
        }
    };

    template <>
    struct hash<librealsense::request_mapping>
    {
        size_t operator()(const librealsense::request_mapping& k) const
        {
            using std::hash;

            return (hash<librealsense::platform::stream_profile>()(k.profile))
                ^ (hash<librealsense::pixel_format_unpacker*>()(k.unpacker))
                ^ (hash<librealsense::native_pixel_format*>()(k.pf));
        }
    };
}

template<class T>
bool contains(const T& first, const T& second)
{
    return first == second;
}

template<class T>
std::vector<std::shared_ptr<T>> subtract_sets(const std::vector<std::shared_ptr<T>>& first, const std::vector<std::shared_ptr<T>>& second)
{
    std::vector<std::shared_ptr<T>> results;
    std::for_each(first.begin(), first.end(), [&](std::shared_ptr<T> data)
    {
        if (std::find_if(second.begin(), second.end(), [&](std::shared_ptr<T> new_dev) {
            return contains(data, new_dev);
        }) == second.end())
        {
            results.push_back(data);
        }
    });
    return results;
}

    enum res_type {
        low_resolution,
        medium_resolution,
        high_resolution
    };

    inline res_type get_res_type(uint32_t width, uint32_t height)
    {
        if (width == 640)
            return res_type::medium_resolution;
        else if (width < 640)
            return res_type::low_resolution;

        return res_type::high_resolution;
    }

#endif
