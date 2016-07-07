// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
#pragma once
#ifndef LIBREALSENSE_UNITTESTS_COMMON_H
#define LIBREALSENSE_UNITTESTS_COMMON_H

#include "catch/catch.hpp"
#include <librealsense/rs.h>

#include <cmath> // For std::sqrt
#include <cassert> // For assert
#include <thread> // For std::this_thread::sleep_for

// noexcept is not accepted by Visual Studio 2013 yet, but noexcept(false) is require on throwing destructors on gcc and clang
// It is normally advisable not to throw in a destructor, however, this usage is safe for require_error/require_no_error because
// they will only ever be created as temporaries immediately before being passed to a C ABI function. All parameters and return
// types are vanilla C types, and thus nothrow-copyable, and the function itself cannot throw because it is a C ABI function.
// Therefore, when a temporary require_error/require_no_error is destructed immediately following one of these C ABI function
// calls, we should not have any exceptions in flight, and can freely throw (perhaps indirectly by calling Catch's REQUIRE() 
// macro) to indicate postcondition violations.
#ifdef WIN32
#define NOEXCEPT_FALSE
#else
#define NOEXCEPT_FALSE noexcept(false)
#endif

// Can be passed to rs_error ** parameters, requires that an error is indicated with the specific provided message
class require_error
{
    std::string message;
    bool validate_error_message;      // Messages content may vary , subject to backend selection
    rs_error * err;
public:
    require_error(std::string message,bool message_validation=true) : message(move(message)), validate_error_message(message_validation), err() {}
    require_error(const require_error &) = delete;
    ~require_error() NOEXCEPT_FALSE
    {
        assert(!std::uncaught_exception());
        REQUIRE(err != nullptr);
        if (validate_error_message)
        {
            REQUIRE(rs_get_error_message(err) == std::string(message));
        }
    }
    require_error &  operator = (const require_error &) = delete;
    operator rs_error ** () { return &err; }
};

// Can be passed to rs_error ** parameters, requires that no error is indicated
class require_no_error
{
    rs_error * err;
public:
    require_no_error() : err() {}
    require_no_error(const require_error &) = delete;
    ~require_no_error() NOEXCEPT_FALSE 
    { 
        assert(!std::uncaught_exception());
        REQUIRE(rs_get_error_message(err) == rs_get_error_message(nullptr)); // Perform this check first. If an error WAS indicated, Catch will display it, making our debugging easier.
        REQUIRE(err == nullptr);
    }
    require_no_error &  operator = (const require_no_error &) = delete;
    operator rs_error ** () { return &err; }
};

// RAII wrapper to ensure that contexts are always cleaned up. rs_context has singleton
// semantics, and creation will fail if an undeleted previous context still exists.
class safe_context
{
    rs_context * context;
    safe_context(int) : context() {}
public:
    safe_context() : safe_context(1)
    {
        context = rs_create_context(RS_API_VERSION, require_no_error());
        REQUIRE(context != nullptr);
    }

    ~safe_context()
    {
        if(context)
        {
            rs_delete_context(context, nullptr);
        }
    }

    bool operator == (const safe_context& other) const { return context == other.context; }

    operator rs_context * () const { return context; }
};


// Compute dot product of a and b
inline float dot_product(const float (& a)[3], const float (& b)[3])
{ 
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2]; 
}

// Compute length of v
inline float vector_length(const float (& v)[3])
{ 
    return std::sqrt(dot_product(v, v));
}

// Require that r = cross(a, b)
inline void require_cross_product(const float (& r)[3], const float (& a)[3], const float (& b)[3])
{
    REQUIRE( r[0] == Approx(a[1]*b[2] - a[2]*b[1]) );
    REQUIRE( r[1] == Approx(a[2]*b[0] - a[0]*b[2]) );
    REQUIRE( r[2] == Approx(a[0]*b[1] - a[1]*b[0]) );
}

// Require that vector is exactly the zero vector
inline void require_zero_vector(const float (& vector)[3])
{
    for(int i=1; i<3; ++i) REQUIRE( vector[i] == 0.0f );
}

// Require that a == transpose(b)
inline void require_transposed(const float (& a)[9], const float (& b)[9])
{
    REQUIRE( a[0] == Approx(b[0]) );
    REQUIRE( a[1] == Approx(b[3]) );
    REQUIRE( a[2] == Approx(b[6]) );
    REQUIRE( a[3] == Approx(b[1]) );
    REQUIRE( a[4] == Approx(b[4]) );
    REQUIRE( a[5] == Approx(b[7]) );
    REQUIRE( a[6] == Approx(b[2]) );
    REQUIRE( a[7] == Approx(b[5]) );
    REQUIRE( a[8] == Approx(b[8]) );
}

// Require that matrix is an orthonormal 3x3 matrix
inline void require_rotation_matrix(const float (& matrix)[9])
{
    const float row0[] = {matrix[0], matrix[3], matrix[6]};
    const float row1[] = {matrix[1], matrix[4], matrix[7]};
    const float row2[] = {matrix[2], matrix[5], matrix[8]};
    REQUIRE( dot_product(row0, row0) == Approx(1) );
    REQUIRE( dot_product(row1, row1) == Approx(1) );
    REQUIRE( dot_product(row2, row2) == Approx(1) );
    REQUIRE( dot_product(row0, row1) == Approx(0) );
    REQUIRE( dot_product(row1, row2) == Approx(0) );
    REQUIRE( dot_product(row2, row0) == Approx(0) );
    require_cross_product(row0, row1, row2);
    require_cross_product(row0, row1, row2);
    require_cross_product(row0, row1, row2); 
}

// Require that matrix is exactly the identity matrix
inline void require_identity_matrix(const float (& matrix)[9])
{
    static const float identity_matrix_3x3[] = {1,0,0, 0,1,0, 0,0,1};
    for(int i=0; i<9; ++i) REQUIRE( matrix[i] == identity_matrix_3x3[i] );
}

// Provide support for doing basic streaming tests on a set of specified modes
struct stream_mode { rs_stream stream; int width, height; rs_format format; int framerate; };
inline void test_streaming(rs_device * device, std::initializer_list<stream_mode> modes)
{
    for(auto & mode : modes)
    {
        rs_enable_stream(device, mode.stream, mode.width, mode.height, mode.format, mode.framerate, require_no_error());
        REQUIRE( rs_is_stream_enabled(device, mode.stream, require_no_error()) == 1 );
    }
    rs_start_device(device, require_no_error());
    REQUIRE( rs_is_device_streaming(device, require_no_error()) == 1 );

    rs_wait_for_frames(device, require_no_error());
    for(auto & mode : modes)
    {
        REQUIRE( rs_is_stream_enabled(device, mode.stream, require_no_error()) == 1 );
        REQUIRE( rs_get_frame_data(device, mode.stream, require_no_error()) != nullptr );
        REQUIRE( rs_get_frame_timestamp(device, mode.stream, require_no_error()) >= 0 );
    }

    for(int i=0; i<100; ++i)
    {
        rs_wait_for_frames(device, require_no_error());
    }
    for(auto & mode : modes)
    {
        REQUIRE( rs_is_stream_enabled(device, mode.stream, require_no_error()) == 1 );
        REQUIRE( rs_get_frame_data(device, mode.stream, require_no_error()) != nullptr );
        REQUIRE( rs_get_frame_timestamp(device, mode.stream, require_no_error()) >= 0 );
    }

    rs_stop_device(device, require_no_error());
    REQUIRE( rs_is_device_streaming(device, require_no_error()) == 0 );
    for(auto & mode : modes)
    {
        rs_disable_stream(device, mode.stream, require_no_error());
        REQUIRE( rs_is_stream_enabled(device, mode.stream, require_no_error()) == 0 );
    }
}

inline void test_option(rs_device * device, rs_option option, std::initializer_list<int> good_values, std::initializer_list<int> bad_values)
{
    // Test reading the current value
    const auto first_value = rs_get_device_option(device, option, require_no_error());
    
    // todo - Check that the first value is something sane?

    // Test setting good values, and that each value set can be subsequently get
    for(auto value : good_values)
    {
        rs_set_device_option(device, option, value, require_no_error());
        REQUIRE( rs_get_device_option(device, option, require_no_error()) == value );
    }

    // Test setting bad values, and verify that they do not change the value of the option
    const auto last_good_value = rs_get_device_option(device, option, require_no_error());
    for(auto value : bad_values)
    {
        rs_set_device_option(device, option, value, require_error("foo",false));
        REQUIRE( rs_get_device_option(device, option, require_no_error()) == last_good_value );
    }

    // Test that we can reset the option to its original value
    rs_set_device_option(device, option, first_value, require_no_error());
    REQUIRE( rs_get_device_option(device, option, require_no_error()) == first_value );
}
#endif
