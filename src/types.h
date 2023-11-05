// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

// This header defines vocabulary types and utility mechanisms used ubiquitously by the
// rest of the library. As clearer module boundaries form, declarations might be moved
// out of this file and into more appropriate locations.

#pragma once

#include "basics.h"

#include <librealsense2/hpp/rs_types.hpp>

#include "core/enum-helpers.h"
#include <rsutils/concurrency/concurrency.h>
#include "librealsense-exception.h"
#include <rsutils/easylogging/easyloggingpp.h>
#include <rsutils/version.h>

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

// Usage of non-standard C++ PI derivatives is prohibitive, use local definitions
static const double pi = std::acos(-1);
static const double d2r = pi / 180;
static const double r2d = 180 / pi;
template<typename T> T deg2rad(T val) { return T(val * d2r); }
template<typename T> T rad2deg(T val) { return T(val * r2d); }

#ifdef ANDROID
#include "../common/android_helpers.h"
#endif


namespace librealsense {


    static const double TIMESTAMP_USEC_TO_MSEC = 0.001;

    ///////////////////////////////////
    // Utility types for general use //
    ///////////////////////////////////

    template<typename T>
    constexpr size_t arr_size(T const&) { return 1; }

    template<typename T, size_t sz>
    constexpr size_t arr_size(T(&arr)[sz])
    {
        return sz * arr_size(arr[0]);
    }

    template<typename T>
    std::string array2str(T& data)
    {
        std::stringstream ss;
        for (auto i = 0; i < arr_size(data); i++)
            ss << " [" << i << "] = " << data[i] << "\t";
        return ss.str();
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

    typedef float float_4[4];

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


    using firmware_version = rsutils::version;



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


    ////////////////////////////////////////
    // Helper functions for library types //
    ////////////////////////////////////////

    inline bool operator == (const rs2_intrinsics & a, const rs2_intrinsics & b) { return std::memcmp(&a, &b, sizeof(a)) == 0; }

    inline uint32_t pack(uint8_t c0, uint8_t c1, uint8_t c2, uint8_t c3)
    {
        return (c0 << 24) | (c1 << 16) | (c2 << 8) | c3;
    }

    bool file_exists(const char* filename);

    ///////////////////////////////////////////
    // Extrinsic auxillary routines routines //
    ///////////////////////////////////////////

    // Auxillary function that calculates standard 32bit CRC code. used in verificaiton
    uint32_t calc_crc32(const uint8_t *buf, size_t bufsize);




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

template<typename T>
uint32_t rs_fourcc(const T a, const T b, const  T c, const T d)
{
    static_assert((std::is_integral<T>::value), "rs_fourcc supports integral built-in types only");
    return ((static_cast<uint32_t>(a) << 24) |
            (static_cast<uint32_t>(b) << 16) |
            (static_cast<uint32_t>(c) << 8) |
            (static_cast<uint32_t>(d) << 0));
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
