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






    ////////////////////////////////////////
    // Helper functions for library types //
    ////////////////////////////////////////

    inline bool operator == (const rs2_intrinsics & a, const rs2_intrinsics & b) { return std::memcmp(&a, &b, sizeof(a)) == 0; }


    ///////////////////////////////////////////
    // Extrinsic auxillary routines routines //
    ///////////////////////////////////////////


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
