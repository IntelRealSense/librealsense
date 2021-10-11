// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

// This header defines vocabulary types and utility mechanisms used ubiquitously by the
// rest of the library. As clearer module boundaries form, declarations might be moved
// out of this file and into more appropriate locations.

#pragma once

#include "basics.h"

#include <librealsense2/hpp/rs_types.hpp>

#include "backend.h"
#include "concurrency.h"
#include "librealsense-exception.h"
#include "easyloggingpp.h"

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <cmath>                            // For acos
#include <ctime>
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
#include <limits>
#include <iomanip>

typedef unsigned char byte;

const int RS2_USER_QUEUE_SIZE = 128;

// Usage of non-standard C++ PI derivatives is prohibitive, use local definitions
static const double pi = std::acos(-1);
static const double d2r = pi / 180;
static const double r2d = 180 / pi;
template<typename T> T deg2rad(T val) { return T(val * d2r); }
template<typename T> T rad2deg(T val) { return T(val * r2d); }

// global abs() is only defined for int for some GCC impl on Linux, meaning we may
// get unwanted behavior without any warning whatsoever. Instead, we want to use the
// C++ version in std!
using std::abs;

#pragma warning(disable: 4250)

#ifdef ANDROID
#include "../common/android_helpers.h"
#endif

#define UNKNOWN_VALUE "UNKNOWN"

namespace librealsense
{
#pragma pack (push, 1)

    struct hid_data
    {
        short x;
        byte reserved1[2];
        short y;
        byte reserved2[2];
        short z;
        byte reserved3[2];
    };

#pragma pack(pop)

    static const double TIMESTAMP_USEC_TO_MSEC = 0.001;

    ///////////////////////////////////
    // Utility types for general use //
    ///////////////////////////////////
    struct to_string
    {
        std::ostringstream ss;
        template<class T> to_string & operator << (const T & val) { ss << val; return *this; }
        operator std::string() const { return ss.str(); }
    };

    template<typename T>
    constexpr size_t arr_size(T const&) { return 1; }

    template<typename T, size_t sz>
    constexpr size_t arr_size(T(&arr)[sz])
    {
        return sz * arr_size(arr[0]);
    }

    template<typename T, size_t sz>
    constexpr size_t arr_size_bytes(T(&arr)[sz])
    {
        return arr_size(arr) * sizeof(T);
    }

    template<typename T>
    std::string array2str(T& data)
    {
        std::stringstream ss;
        for (auto i = 0; i < arr_size(data); i++)
            ss << " [" << i << "] = " << data[i] << "\t";
        return ss.str();
    }

    // Auxillary function for compiler to highlight the invoking user code
    template <typename T, typename S>
    struct isNarrowing { static bool constexpr value = std::numeric_limits<S>::max() > std::numeric_limits<T>::max(); };

    template<const bool force_narrowing=false, typename T, typename S, size_t size_tgt, size_t size_src>
    inline size_t copy_array(T(&dst)[size_tgt], const S(&src)[size_src])
    {
        static_assert((size_tgt && (size_tgt == size_src)), "copy_array requires similar non-zero size for target and source containers");
        static_assert((std::is_arithmetic<S>::value) && (std::is_arithmetic<T>::value), "copy_array supports arithmetic types only");
        if (!force_narrowing && isNarrowing<T, S>::value)
        {
            static_assert(!(isNarrowing<T, S>::value && !force_narrowing), "Passing narrowing conversion to copy_array requires setting the force flag on");
        }

        assert(dst != nullptr && src != nullptr);
        for (size_t i = 0; i < size_tgt; i++)
        {
            dst[i] = static_cast<T>(src[i]);
        }
        return size_tgt;
    }

    template<const bool force_narrowing=false, typename T, typename S, size_t tgt_m, size_t tgt_n, size_t src_m, size_t src_n>
    inline size_t copy_2darray(T(&dst)[tgt_m][tgt_n], const S(&src)[src_m][src_n])
    {
        static_assert((src_m && src_n && (tgt_n == src_n) && (tgt_m == src_m)), "copy_array requires similar non-zero size for target and source containers");
        static_assert((std::is_arithmetic<S>::value) && (std::is_arithmetic<T>::value), "copy_2darray supports arithmetic types only");
        if (isNarrowing<T, S>::value && !force_narrowing)
        {
            static_assert(!(isNarrowing<T, S>::value && !force_narrowing), "Passing narrowing conversion to copy_2darray requires setting the force flag on");
        }

        assert(dst != nullptr && src != nullptr);
        for (size_t i = 0; i < src_m; i++)
        {
            for (size_t j = 0; j < src_n; j++)
            {
                dst[i][j] = static_cast<T>(src[i][j]);
            }
        }
        return src_m * src_n;
    }

    // Comparing parameter against a range of values of the same type
    // https://stackoverflow.com/questions/15181579/c-most-efficient-way-to-compare-a-variable-to-multiple-values
    template <typename T>
    bool val_in_range(const T& val, const std::initializer_list<T>& list)
    {
        for (const auto& i : list) {
            if (val == i) {
                return true;
            }
        }
        return false;
    }

    template<class T>
    std::string hexify(const T& val)
    {
        static_assert((std::is_integral<T>::value), "hexify supports integral built-in types only");

        std::ostringstream oss;
        oss << std::setw(sizeof(T)*2) << std::setfill('0') << std::uppercase << std::hex << val;
        return oss.str().c_str();
    }

    void copy(void* dst, void const* src, size_t size);

    std::string make_less_screamy(const char* str);

    ///////////////////////
    // Logging mechanism //
    ///////////////////////

    typedef std::shared_ptr< rs2_log_callback > log_callback_ptr;

    void log_to_console(rs2_log_severity min_severity);
    void log_to_file( rs2_log_severity min_severity, const char* file_path );
    void log_to_callback( rs2_log_severity min_severity, log_callback_ptr callback );
    void reset_logger();
    void enable_rolling_log_file( unsigned max_size );

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


#pragma pack(push, 1)
    template<class T> class big_endian
    {
        T be_value;
    public:
        operator T () const
        {
            T le_value = 0;
            for (unsigned int i = 0; i < sizeof(T); ++i) *(reinterpret_cast<char*>(&le_value) + i) = *(reinterpret_cast<const char*>(&be_value) + sizeof(T) - i - 1);
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

        void reset() const
        {
            std::lock_guard<std::mutex> lock(_mtx);
            if (_was_init)
            {
                _ptr.reset();
                _was_init = false;
            }
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

    typedef float float_4[4];

    /////////////////////////////
    // Enumerated type support //
    /////////////////////////////

// Require the last enumerator value to be in format of RS2_#####_COUNT
#define RS2_ENUM_HELPERS( TYPE, PREFIX )                                                           \
    RS2_ENUM_HELPERS_CUSTOMIZED( TYPE, 0, RS2_##PREFIX##_COUNT - 1 )

// This macro can be used directly if needed to support enumerators with no RS2_#####_COUNT last value
#define RS2_ENUM_HELPERS_CUSTOMIZED( TYPE, FIRST, LAST )                                           \
    LRS_EXTENSION_API const char * get_string( TYPE value );                                       \
    inline bool is_valid( TYPE value ) { return value >= FIRST && value <= LAST; }                 \
    inline std::ostream & operator<<( std::ostream & out, TYPE value )                             \
    {                                                                                              \
        if( is_valid( value ) )                                                                    \
            return out << get_string( value );                                                     \
        else                                                                                       \
            return out << (int)value;                                                              \
    }                                                                                              \
    inline bool try_parse( const std::string & str, TYPE & res )                                   \
    {                                                                                              \
        for( int i = FIRST; i <= LAST; i++ )                                                       \
        {                                                                                          \
            auto v = static_cast< TYPE >( i );                                                     \
            if( str == get_string( v ) )                                                           \
            {                                                                                      \
                res = v;                                                                           \
                return true;                                                                       \
            }                                                                                      \
        }                                                                                          \
        return false;                                                                              \
    }

    RS2_ENUM_HELPERS(rs2_stream, STREAM)
    RS2_ENUM_HELPERS(rs2_format, FORMAT)
    RS2_ENUM_HELPERS(rs2_distortion, DISTORTION)
    RS2_ENUM_HELPERS(rs2_option, OPTION)
    RS2_ENUM_HELPERS(rs2_camera_info, CAMERA_INFO)
    RS2_ENUM_HELPERS(rs2_frame_metadata_value, FRAME_METADATA)
    RS2_ENUM_HELPERS(rs2_timestamp_domain, TIMESTAMP_DOMAIN)
    RS2_ENUM_HELPERS(rs2_calib_target_type, CALIB_TARGET)
    RS2_ENUM_HELPERS(rs2_sr300_visual_preset, SR300_VISUAL_PRESET)
    RS2_ENUM_HELPERS(rs2_extension, EXTENSION)
    RS2_ENUM_HELPERS(rs2_exception_type, EXCEPTION_TYPE)
    RS2_ENUM_HELPERS(rs2_log_severity, LOG_SEVERITY)
    RS2_ENUM_HELPERS(rs2_notification_category, NOTIFICATION_CATEGORY)
    RS2_ENUM_HELPERS(rs2_playback_status, PLAYBACK_STATUS)
    RS2_ENUM_HELPERS(rs2_matchers, MATCHER)
    RS2_ENUM_HELPERS(rs2_sensor_mode, SENSOR_MODE)
    RS2_ENUM_HELPERS(rs2_l500_visual_preset, L500_VISUAL_PRESET)
    RS2_ENUM_HELPERS(rs2_calibration_type, CALIBRATION_TYPE)
    RS2_ENUM_HELPERS_CUSTOMIZED(rs2_calibration_status, RS2_CALIBRATION_STATUS_FIRST, RS2_CALIBRATION_STATUS_LAST )
    RS2_ENUM_HELPERS_CUSTOMIZED(rs2_ambient_light, RS2_AMBIENT_LIGHT_NO_AMBIENT, RS2_AMBIENT_LIGHT_LOW_AMBIENT)
    RS2_ENUM_HELPERS_CUSTOMIZED(rs2_digital_gain, RS2_DIGITAL_GAIN_HIGH, RS2_DIGITAL_GAIN_LOW)
    RS2_ENUM_HELPERS(rs2_host_perf_mode, HOST_PERF)


    ////////////////////////////////////////////
    // World's tiniest linear algebra library //
    ////////////////////////////////////////////
#pragma pack( push, 1 )
    struct int2
    {
        int x, y;
    };
    struct float2
    {
        float x, y;
        float & operator[]( int i )
        {
            assert( i >= 0 );
            assert( i < 2 );
            return *( &x + i );
        }
    };
    struct float3
    {
        float x, y, z;
        float & operator[]( int i )
        {
            assert( i >= 0 );
            assert( i < 3 );
            return ( *( &x + i ) );
        }
    };
    struct float4
    {
        float x, y, z, w;
        float & operator[]( int i )
        {
            assert( i >= 0 );
            assert( i < 4 );
            return ( *( &x + i ) );
        }
    };
    struct float3x3
    {
        float3 x, y, z;
        float & operator()( int i, int j )
        {
            assert( i >= 0 );
            assert( i < 3 );
            assert( j >= 0 );
            assert( j < 3 );
            return ( *( &x[0] + j * sizeof( float3 ) / sizeof( float ) + i ) );
        }
    };  // column-major
    struct pose
    {
        float3x3 orientation;
        float3 position;
    };
#pragma pack(pop)
    inline bool operator == (const float3 & a, const float3 & b) { return a.x == b.x && a.y == b.y && a.z == b.z; }
    inline float3 operator + (const float3 & a, const float3 & b) { return{ a.x + b.x, a.y + b.y, a.z + b.z }; }
    inline float3 operator - (const float3 & a, const float3 & b) { return{ a.x - b.x, a.y - b.y, a.z - b.z }; }
    inline float3 operator * (const float3 & a, float b) { return{ a.x*b, a.y*b, a.z*b }; }
    inline bool operator == (const float4 & a, const float4 & b) { return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w; }
    inline float4 operator + (const float4 & a, const float4 & b) { return{ a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w }; }
    inline float4 operator - (const float4 & a, const float4 & b) { return{ a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w }; }
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

    // The extrinsics on the camera ("raw extrinsics") are in milimeters, but LRS works in meters
    // Additionally, LRS internal algorithms are
    // written with a transposed matrix in mind! (see rs2_transform_point_to_point)
    rs2_extrinsics to_raw_extrinsics(rs2_extrinsics);
    rs2_extrinsics from_raw_extrinsics(rs2_extrinsics);

    inline std::ostream& operator <<(std::ostream& stream, const float3& elem)
    {
        return stream << elem.x << " " << elem.y << " " << elem.z;
    }

    inline std::ostream& operator <<(std::ostream& stream, const float4& elem)
    {
        return stream << elem.x << " " << elem.y << " " << elem.z << " " << elem.w;
    }

    inline std::ostream& operator <<(std::ostream& stream, const float3x3& elem)
    {
        return stream << elem.x << "\n" << elem.y << "\n" << elem.z;
    }

    inline std::ostream& operator <<(std::ostream& stream, const pose& elem)
    {
        return stream << "Position:\n " << elem.position  << "\n Orientation :\n" << elem.orientation;
    }

    ///////////////////
    // Pixel formats //
    ///////////////////

    struct resolution
    {
        uint32_t width, height;
    };
    using resolution_func = std::function<resolution(resolution res)>;

    struct stream_profile
    {
        stream_profile(rs2_format fmt = RS2_FORMAT_ANY,
            rs2_stream strm = RS2_STREAM_ANY,
            int idx = 0,
            uint32_t w = 0, uint32_t h = 0,
            uint32_t framerate = 0,
            resolution_func res_func = [](resolution res) { return res; }) :
            format(fmt), stream(strm), index(idx), width(w), height(h), fps(framerate), stream_resolution(res_func)
        {}

        rs2_format format;
        rs2_stream stream;
        int index;
        uint32_t width, height, fps;
        resolution_func stream_resolution; // Calculates the relevant resolution from the given backend resolution.

        std::pair< uint32_t, uint32_t > width_height() const { return std::make_pair( width, height ); }
    };


    inline bool operator==(const stream_profile& a,
        const stream_profile& b)
    {
        return (a.width == b.width) &&
            (a.height == b.height) &&
            (a.fps == b.fps) &&
            (a.format == b.format) &&
            (a.index == b.index) &&
            (a.stream == b.stream);
    }

    inline bool operator<(const stream_profile & lhs,
        const stream_profile & rhs)
    {
        if (lhs.format != rhs.format) return lhs.format < rhs.format;
        if (lhs.index != rhs.index)   return lhs.index  < rhs.index;
        return lhs.stream < rhs.stream;
    }

    struct stream_descriptor
    {
        stream_descriptor() : type(RS2_STREAM_ANY), index(0) {}
        stream_descriptor(rs2_stream type, int index = 0) : type(type), index(index) {}

        rs2_stream type;
        int index;
    };

    struct stream_output {
        stream_output(stream_descriptor stream_desc_in,
                      rs2_format format_in,
                      resolution_func res_func = [](resolution res) {return res; })
            : stream_desc(stream_desc_in),
              format(format_in),
              stream_resolution(res_func)
        {}

        stream_descriptor stream_desc;
        rs2_format format;
        resolution_func stream_resolution;
    };

    class stream_profile_interface;

    class frame_interface;

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

        // CTO experimental firmware versions are marked with build >= 90
        bool experimental() const { return m_build >= 90; }

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
                    LOG_ERROR("Received an exception from frame callback!");
                }
            }
        }
        void release() override { delete this; }
    };

    class internal_frame_processor_fptr_callback : public rs2_frame_processor_callback
    {
        rs2_frame_processor_callback_ptr fptr;
        void * user;
    public:
        internal_frame_processor_fptr_callback() : internal_frame_processor_fptr_callback(nullptr, nullptr) {}
        internal_frame_processor_fptr_callback(rs2_frame_processor_callback_ptr on_frame, void * user)
            : fptr(on_frame), user(user) {}

        operator bool() const { return fptr != nullptr; }
        void on_frame(rs2_frame * frame, rs2_source * source) override {
            if (fptr)
            {
                try { fptr(frame, source, user); }
                catch (...)
                {
                    LOG_ERROR("Received an exception from frame callback!");
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
                    LOG_ERROR("Received an exception from notification callback!");
                }
            }
        }
        void release() override { delete this; }
    };

    typedef void(*software_device_destruction_callback_function_ptr)(void * user);

    class software_device_destruction_callback : public rs2_software_device_destruction_callback
    {
        software_device_destruction_callback_function_ptr nptr;
        void * user;
    public:
        software_device_destruction_callback() : software_device_destruction_callback(nullptr, nullptr) {}
        software_device_destruction_callback(software_device_destruction_callback_function_ptr on_destruction, void * user)
            : nptr(on_destruction), user(user) {}

        operator bool() const { return nptr != nullptr; }
        void on_destruction() override {
            if (nptr)
            {
                try { nptr(user); }
                catch (...)
                {
                    LOG_ERROR("Received an exception from software device destruction callback!");
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
                    LOG_ERROR("Received an exception from devices_changed callback!");
                }
            }
        }
        void release() override { delete this; }
    };

    class update_progress_callback : public rs2_update_progress_callback
    {
        rs2_update_progress_callback_ptr _nptr;
        void* _client_data;
    public:
        update_progress_callback() {}
        update_progress_callback(rs2_update_progress_callback_ptr on_update_progress, void* client_data = NULL)
        : _nptr(on_update_progress), _client_data(client_data){}

        operator bool() const { return _nptr != nullptr; }
        void on_update_progress(const float progress) {
            if (_nptr)
            {
                try { _nptr(progress, _client_data); }
                catch (...)
                {
                    LOG_ERROR("Received an exception from firmware update progress callback!");
                }
            }
        }
        void release() { delete this; }
    };

    typedef std::shared_ptr<rs2_frame_callback> frame_callback_ptr;
    typedef std::shared_ptr<rs2_frame_processor_callback> frame_processor_callback_ptr;
    typedef std::shared_ptr<rs2_notifications_callback> notifications_callback_ptr;
    typedef std::shared_ptr<rs2_calibration_change_callback> calibration_change_callback_ptr;
    typedef std::shared_ptr<rs2_software_device_destruction_callback> software_device_destruction_callback_ptr;
    typedef std::shared_ptr<rs2_devices_changed_callback> devices_changed_callback_ptr;
    typedef std::shared_ptr<rs2_update_progress_callback> update_progress_callback_ptr;

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

    class notifications_processor
    {
    public:
        notifications_processor();
        ~notifications_processor();

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

    inline std::string datetime_string()
    {
        auto t = time(nullptr);
        char buffer[20] = {};
        const tm* time = localtime(&t);
        if (nullptr != time)
            strftime(buffer, sizeof(buffer), "%Y-%m-%d-%H_%M_%S", time);
        return to_string() << buffer;
    }

    bool file_exists(const char* filename);

    ///////////////////////////////////////////
    // Extrinsic auxillary routines routines //
    ///////////////////////////////////////////
    float3x3 calc_rotation_from_rodrigues_angles(const std::vector<double> rot);
    // Auxillary function that calculates standard 32bit CRC code. used in verificaiton
    uint32_t calc_crc32(const uint8_t *buf, size_t bufsize);


    

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

        bool operator==(const T& other) const
        {
            return this->_value == other;
        }

        bool operator!=(const T& other) const
        {
            return !(*this == other);
        }
    private:
        bool _valid;
        T _value;
    };
}

inline std::ostream& operator<<( std::ostream& out, rs2_extrinsics const & e )
{
    return out
        << "[ r["
        << e.rotation[0] << "," << e.rotation[1] << "," << e.rotation[2] << "," << e.rotation[3] << "," << e.rotation[4] << ","
        << e.rotation[5] << "," << e.rotation[6] << "," << e.rotation[7] << "," << e.rotation[8]
        << "]  t[" << e.translation[0] << "," << e.translation[1] << "," << e.translation[2] << "] ]";
}

inline std::ostream& operator<<( std::ostream& out, rs2_intrinsics const & i )
{
    return out
        << "[ " << i.width << "x" << i.height
        << "  p[" << i.ppx << " " << i.ppy << "]"
        << "  f[" << i.fx << " " << i.fy << "]"
        << "  " << librealsense::get_string( i.model )
        << " [" << i.coeffs[0] << " " << i.coeffs[1] << " " << i.coeffs[2]
        << " " << i.coeffs[3] << " " << i.coeffs[4]
        << "] ]";
}

inline std::ostream& operator<<( std::ostream& s, rs2_dsm_params const & self )
{
    s << "[ ";
    if( self.timestamp )
    {
        time_t t = self.timestamp;
        auto ptm = localtime( &t );
        char buf[256];
        strftime( buf, sizeof( buf ), "%F.%T ", ptm );
        s << buf;

        unsigned patch = self.version & 0xF;
        unsigned minor = (self.version >> 4) & 0xFF;
        unsigned major = (self.version >> 12);
        s << major << '.' << minor << '.' << patch << ' ';
    }
    else
    {
        s << "new: ";
    }
    switch( self.model )
    {
    case RS2_DSM_CORRECTION_NONE: break;
    case RS2_DSM_CORRECTION_AOT: s << "AoT "; break;
    case RS2_DSM_CORRECTION_TOA: s << "ToA "; break;
    }
    s << "x[" << self.h_scale << " " << self.v_scale << "] ";
    s << "+[" << self.h_offset << " " << self.v_offset;
    if( self.rtd_offset )
        s << " rtd " << self.rtd_offset;
    s << "]";
    if( self.temp_x2 )
        s << " @" << float( self.temp_x2 ) / 2 << "degC";
    s << " ]";
    return s;
}

template<typename T>
uint32_t rs_fourcc(const T a, const T b, const  T c, const T d)
{
    static_assert((std::is_integral<T>::value), "rs_fourcc supports integral built-in types only");
    return ((static_cast<uint32_t>(a) << 24) |
            (static_cast<uint32_t>(b) << 16) |
            (static_cast<uint32_t>(c) << 8) |
            (static_cast<uint32_t>(d) << 0));
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
    struct hash<rs2_format>
    {
        size_t operator()(const rs2_format& f) const
        {
            using std::hash;

            return hash<uint32_t>()(f);
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
    if (width == 256) // Crop resolution
        return res_type::high_resolution;

    if (width == 640)
        return res_type::medium_resolution;
    else if (width < 640)
        return res_type::low_resolution;

    return res_type::high_resolution;
}

inline bool operator==( const rs2_extrinsics& a, const rs2_extrinsics& b )
{
    for( int i = 0; i < 3; i++ )
        if( a.translation[i] != b.translation[i] )
            return false;
    for( int j = 0; j < 3; j++ )
        for( int i = 0; i < 3; i++ )
            if( std::fabs( a.rotation[j * 3 + i] - b.rotation[j * 3 + i] )
        > std::numeric_limits<float>::epsilon() )
                return false;
    return true;
}
