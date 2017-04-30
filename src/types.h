// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

// This header defines vocabulary types and utility mechanisms used ubiquitously by the
// rest of the library. As clearer module boundaries form, declarations might be moved
// out of this file and into more appropriate locations.

#pragma once
#ifndef LIBREALSENSE_TYPES_H
#define LIBREALSENSE_TYPES_H

#include "../include/librealsense/rs2.h"     // Inherit all type definitions in the public API
#include "../include/librealsense/rscore2.hpp"

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
#include "concurrency.h"

#define NOMINMAX
#include "../third_party/easyloggingpp/src/easylogging++.h"

typedef unsigned char byte;

const int RS2_USER_QUEUE_SIZE = 64;
const int RS2_MAX_EVENT_QUEUE_SIZE = 500;
const int RS2_MAX_EVENT_TINE_OUT = 10;

#ifndef DBL_EPSILON
const double DBL_EPSILON = 2.2204460492503131e-016;  // smallest such that 1.0+DBL_EPSILON != 1.0
#endif

namespace rsimpl2
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

    inline void copy(void* dst, void const* src, size_t size)
    {
        auto from = reinterpret_cast<uint8_t const*>(src);
        std::copy(from, from + size, reinterpret_cast<uint8_t*>(dst));
    }

    ///////////////////////
    // Logging mechanism //
    ///////////////////////

    void log_to_console(rs2_log_severity min_severity);
    void log_to_file(rs2_log_severity min_severity, const char * file_path);

#define LOG_DEBUG(...)   do { CLOG(DEBUG   ,"librealsense") << __VA_ARGS__; } while(false)
#define LOG_INFO(...)    do { CLOG(INFO    ,"librealsense") << __VA_ARGS__; } while(false)
#define LOG_WARNING(...) do { CLOG(WARNING ,"librealsense") << __VA_ARGS__; } while(false)
#define LOG_ERROR(...)   do { CLOG(ERROR   ,"librealsense") << __VA_ARGS__; } while(false)
#define LOG_FATAL(...)   do { CLOG(FATAL   ,"librealsense") << __VA_ARGS__; } while(false)


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
                              rs2_exception_type exception_type) noexcept
            : librealsense_exception(msg, exception_type)
        {
            LOG_WARNING(msg);
        }
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
        lazy() : _init([]() { T t; return t; }) {}
        lazy(std::function<T()> initializer) : _init(std::move(initializer)) {}

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

#define RS2_ENUM_HELPERS(TYPE, PREFIX) const char * get_string(TYPE value); \
        inline bool is_valid(TYPE value) { return value >= 0 && value < RS2_##PREFIX##_COUNT; } \
        inline std::ostream & operator << (std::ostream & out, TYPE value) { if(is_valid(value)) return out << get_string(value); else return out << (int)value; }
    RS2_ENUM_HELPERS(rs2_stream, STREAM)
    RS2_ENUM_HELPERS(rs2_format, FORMAT)
    RS2_ENUM_HELPERS(rs2_distortion, DISTORTION)
    RS2_ENUM_HELPERS(rs2_option, OPTION)
    RS2_ENUM_HELPERS(rs2_camera_info, CAMERA_INFO)
    RS2_ENUM_HELPERS(rs2_timestamp_domain, TIMESTAMP_DOMAIN)
    RS2_ENUM_HELPERS(rs2_visual_preset, VISUAL_PRESET)
    RS2_ENUM_HELPERS(rs2_exception_type, EXCEPTION_TYPE)
    RS2_ENUM_HELPERS(rs2_log_severity, LOG_SEVERITY)
    RS2_ENUM_HELPERS(rs2_notification_category, NOTIFICATION_CATEGORY)
    #undef RS2_ENUM_HELPERS

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

    typedef std::tuple<uint32_t, int, size_t> native_pixel_format_tuple;
    typedef std::tuple<rs2_stream, rs2_format> output_tuple;
    typedef std::tuple<uvc::stream_profile_tuple, native_pixel_format_tuple, std::vector<output_tuple>> request_mapping_tuple;

    struct stream_profile
    {
        rs2_stream stream;
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
            (a.stream == b.stream);
    }

    struct pixel_format_unpacker
    {
        bool requires_processing;
        void(*unpack)(byte * const dest[], const byte * source, int count);
        std::vector<std::pair<rs2_stream, rs2_format>> outputs;

        bool satisfies(const stream_profile& request) const
        {
            return provides_stream(request.stream) &&
                get_format(request.stream) == request.format;
        }

        bool provides_stream(rs2_stream stream) const
        {
            for (auto & o : outputs)
                if (o.first == stream)
                    return true;

            return false;
        }
        rs2_format get_format(rs2_stream stream) const
        {
            for (auto & o : outputs)
                if (o.first == stream)
                    return o.second;

            throw invalid_value_exception("missing output");
        }

        operator std::vector<output_tuple>()
        {
            std::vector<output_tuple> tuple_outputs;

            for (auto output : outputs)
            {
                tuple_outputs.push_back(std::make_tuple(output.first, output.second));
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

    struct request_mapping
    {
        uvc::stream_profile profile;
        native_pixel_format* pf;
        pixel_format_unpacker* unpacker;

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

    class device;

    struct frame_holder
    {
        rs2_frame* frame;

        rs2_frame* operator->()
        {
            return frame;
        }

        operator bool() const { return frame != nullptr; }

        operator rs2_frame*() const { return frame; }

        frame_holder(rs2_frame* f)
        {
            frame = f;
        }

        ~frame_holder();

        frame_holder(const frame_holder&) = delete;
        frame_holder(frame_holder&& other)
            : frame(other.frame)
        {
            other.frame = nullptr;
        }

        frame_holder() : frame(nullptr) {}

        frame_holder& operator=(const frame_holder&) = delete;
        frame_holder& operator=(frame_holder&& other);

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
    };

    struct supported_capability
    {
        rs2_stream           capability;
        firmware_version    from;
        firmware_version    until;
        rs2_camera_info      firmware_type;

        supported_capability(rs2_stream capability, firmware_version from,
            firmware_version until, rs2_camera_info firmware_type = RS2_CAMERA_INFO_CAMERA_FIRMWARE_VERSION)
            : capability(capability), from(from), until(until), firmware_type(firmware_type) {}

        explicit supported_capability(rs2_stream capability)
            : capability(capability), from(), until(), firmware_type(RS2_CAMERA_INFO_CAMERA_FIRMWARE_VERSION) {}
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
        std::vector<rs2_frame_metadata> supported_metadata_vector;
        std::vector<supported_capability> capabilities_vector;
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

    typedef std::unique_ptr<rs2_log_callback, void(*)(rs2_log_callback*)> log_callback_ptr;
    typedef std::shared_ptr<rs2_frame_callback> frame_callback_ptr;
    typedef std::unique_ptr<rs2_notifications_callback, void(*)(rs2_notifications_callback*)> notifications_callback_ptr;



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
    };

    class notification_decoder
    {
    public:
        virtual notification decode(int value) = 0;
    };

    class notifications_proccessor
    {
    public:
        notifications_proccessor();
        ~notifications_proccessor();

        void set_callback(notifications_callback_ptr callback);
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
    float3x3 calc_rodrigues_matrix(const std::vector<double> rot);
    // Auxillary function that calculates standard 32bit CRC code. used in verificaiton
    uint32_t calc_crc32(const uint8_t *buf, size_t bufsize);
}

namespace std {

    template <>
    struct hash<rsimpl2::stream_profile>
    {
        size_t operator()(const rsimpl2::stream_profile& k) const
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
    struct hash<rsimpl2::uvc::stream_profile>
    {
        size_t operator()(const rsimpl2::uvc::stream_profile& k) const
        {
            using std::hash;

            return (hash<uint32_t>()(k.height))
                ^ (hash<uint32_t>()(k.width))
                ^ (hash<uint32_t>()(k.fps))
                ^ (hash<uint32_t>()(k.format));
        }
    };

    template <>
    struct hash<rsimpl2::request_mapping>
    {
        size_t operator()(const rsimpl2::request_mapping& k) const
        {
            using std::hash;

            return (hash<rsimpl2::uvc::stream_profile>()(k.profile))
                ^ (hash<rsimpl2::pixel_format_unpacker*>()(k.unpacker))
                ^ (hash<rsimpl2::native_pixel_format*>()(k.pf));
        }
    };
}

#endif
