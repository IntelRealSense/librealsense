// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "approx.h"
#include <librealsense2/rs.hpp>
#include <librealsense2/hpp/rs_context.hpp>
#include <librealsense2/hpp/rs_internal.hpp>
#include <limits> // For std::numeric_limits
#include <cmath> // For std::sqrt
#include <cassert> // For assert
#include <thread> // For std::this_thread::sleep_for
#include <map>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>
#include <fstream>
#include <array>
#include <set>
#include <src/types.h>

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



struct stream_request
{
    rs2_stream stream;
    rs2_format format;
    int width;
    int height;
    int fps;
    int index;

    bool operator==(const rs2::video_stream_profile& other) const
    {
        return stream == other.stream_type() &&
            format == other.format() &&
            width == other.width() &&
            height == other.height() &&
            index == other.stream_index();
    }
};

struct profile
{
    rs2_stream stream;
    rs2_format format;
    int width;
    int height;
    int index;
    int fps;

    bool operator==(const profile& other) const
    {
        return stream == other.stream &&
            (format == 0 || other.format == 0 || format == other.format) &&
            (width == 0 || other.width == 0 || width == other.width) &&
            (height == 0 || other.height == 0 || height == other.height) &&
            (index == 0 || other.index == 0 || index == other.index);

    }
    bool operator!=(const profile& other) const
    {
        return !(*this == other);
    }
    bool operator<(const profile& other) const
    {
        return stream < other.stream;
    }
};

inline std::ostream& operator <<(std::ostream& stream, const profile& cap)
{
    stream << cap.stream << " " << cap.stream << " " << cap.format << " "
        << cap.width << " " << cap.height << " " << cap.index << " " << cap.fps;
    return stream;
}

struct device_profiles
{
    std::vector<profile> streams;
    int fps;
    bool sync;
};

std::vector< profile >
configure_all_supported_streams( rs2::sensor & sensor, int width = 640, int height = 480, int fps = 60 );

std::pair< std::vector< rs2::sensor >, std::vector< profile > >
configure_all_supported_streams( rs2::device & dev, int width = 640, int height = 480, int fps = 30 );

std::string space_to_underscore( const std::string & text );

#define SECTION_FROM_TEST_NAME space_to_underscore(Catch::getCurrentContext().getResultCapture()->getCurrentTestName()).c_str()

//inline long long current_time();

//// disable in one place options that are sensitive to frame content
//// this is done to make sure unit-tests are deterministic
void disable_sensitive_options_for( rs2::sensor & sen );

void disable_sensitive_options_for( rs2::device & dev );

bool wait_for_reset( std::function< bool( void ) > func, std::shared_ptr< rs2::device > dev );

bool is_usb3( const rs2::device & dev );

// Provides for Device PID , USB3/2 (true/false)
typedef  std::pair<std::string, bool > dev_type;

dev_type get_PID( rs2::device & dev );

struct command_line_params
{
    static void init( int argc, char const * const * argv );
};

bool found_any_section();


inline bool file_exists(const std::string& filename)
{
    std::ifstream f(filename);
    return f.good();
}

rs2::context make_context( const char * id );

// Can be passed to rs2_error ** parameters, requires that an error is indicated with the specific provided message
class require_error
{
    std::string message;
    bool validate_error_message;      // Messages content may vary , subject to backend selection
    rs2_error * err;
public:
    require_error(std::string message, bool message_validation = true) : message(std::move(message)), validate_error_message(message_validation), err() {}
    require_error(const require_error &) = delete;
    ~require_error() NOEXCEPT_FALSE
    {
        assert(!std::uncaught_exception());
        REQUIRE(err != nullptr);
        if (validate_error_message)
        {
            REQUIRE(rs2_get_error_message(err) == std::string(message));
        }
    }
    require_error &  operator = (const require_error &) = delete;
    operator rs2_error ** () { return &err; }
};

// Can be passed to rs2_error ** parameters, requires that no error is indicated
class require_no_error
{
    rs2_error * err;
public:
    require_no_error() : err() {}
    require_no_error(const require_error &) = delete;
    ~require_no_error() NOEXCEPT_FALSE
    {
        assert(!std::uncaught_exception());
        REQUIRE(rs2_get_error_message(err) == rs2_get_error_message(nullptr)); // Perform this check first. If an error WAS indicated, Catch will display it, making our debugging easier.
        REQUIRE(err == nullptr);
    }
    require_no_error &  operator = (const require_no_error &) = delete;
    operator rs2_error ** () { return &err; }
};

// Compute dot product of a and b
inline float dot_product(const float(&a)[3], const float(&b)[3])
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

// Compute length of v
inline float vector_length(const float(&v)[3])
{
    return std::sqrt(dot_product(v, v));
}

// Require that r = cross(a, b)
inline void require_cross_product(const float(&r)[3], const float(&a)[3], const float(&b)[3])
{
    REQUIRE(r[0] == approx(a[1] * b[2] - a[2] * b[1]));
    REQUIRE(r[1] == approx(a[2] * b[0] - a[0] * b[2]));
    REQUIRE(r[2] == approx(a[0] * b[1] - a[1] * b[0]));
}

// Require that vector is exactly the zero vector
inline void require_zero_vector(const float(&vector)[3])
{
    for (int i = 1; i < 3; ++i) REQUIRE(vector[i] == 0.0f);
}

// Require that a == transpose(b)
inline void require_transposed(const float(&a)[9], const float(&b)[9])
{
    REQUIRE(a[0] == approx(b[0]));
    REQUIRE(a[1] == approx(b[3]));
    REQUIRE(a[2] == approx(b[6]));
    REQUIRE(a[3] == approx(b[1]));
    REQUIRE(a[4] == approx(b[4]));
    REQUIRE(a[5] == approx(b[7]));
    REQUIRE(a[6] == approx(b[2]));
    REQUIRE(a[7] == approx(b[5]));
    REQUIRE(a[8] == approx(b[8]));
}

// Require that matrix is an orthonormal 3x3 matrix
void require_rotation_matrix( const float ( &matrix )[9] );

// Require that matrix is exactly the identity matrix
void require_identity_matrix( const float ( &matrix )[9] );

struct test_duration {
    bool is_start_time_initialized;
    bool is_end_time_initialized;
    std::chrono::high_resolution_clock::time_point start_time, end_time;
    uint32_t actual_frames_to_receive;
    uint32_t first_frame_to_capture;
    uint32_t frames_to_capture;
};

struct frame_metadata
{
    std::array<std::pair<bool, rs2_metadata_type>, RS2_FRAME_METADATA_COUNT> md_attributes{};
};


struct internal_frame_additional_data
{
    double                  timestamp;
    unsigned long long      frame_number;
    rs2_timestamp_domain    timestamp_domain;
    rs2_stream              stream;
    rs2_format              format;
    frame_metadata          frame_md;       // Metadata attributes

    internal_frame_additional_data(const double &ts, const unsigned long long frame_num, const rs2_timestamp_domain& ts_domain, const rs2_stream& strm, const rs2_format& fmt) :
        timestamp(ts),
        frame_number(frame_num),
        timestamp_domain(ts_domain),
        stream(strm),
        format(fmt) {}
};

inline void check_fps(double actual_fps, double configured_fps)
{
    REQUIRE(actual_fps >= configured_fps * 0.9); // allow threshold of 10 percent
}

// All streaming tests are bounded by time or number of frames, which comes first
const int max_capture_time_sec = 3;            // Each streaming test configuration shall not exceed this threshold
const uint32_t max_frames_to_receive = 50;     // Max frames to capture per streaming tests

//void test_wait_for_frames(rs2_device * device, std::initializer_list<stream_mode>& modes, std::map<rs2_stream, test_duration>& duration_per_stream);


struct user_data {
    std::map<rs2_stream, test_duration> duration_per_stream;
    std::map<rs2_stream, unsigned> number_of_frames_per_stream;
};


void frame_callback( rs2::device & dev, rs2::frame frame, void * user );

//void test_frame_callback(device &device, std::initializer_list<stream_profile>& modes, std::map<rs2_stream, test_duration>& duration_per_stream);

//void motion_callback(rs2_device * , rs2_motion_data, void *);

void test_option( rs2::sensor & device,
                  rs2_option option,
                  std::initializer_list< int > good_values,
                  std::initializer_list< int > bad_values );

rs2::stream_profile get_profile_by_resolution_type( rs2::sensor & s, res_type res );

std::shared_ptr< rs2::device > do_with_waiting_for_camera_connection( rs2::context ctx,
                                                                      std::shared_ptr< rs2::device > dev,
                                                                      std::string serial,
                                                                      std::function< void() > operation );

rs2::depth_sensor restart_first_device_and_return_depth_sensor( const rs2::context & ctx,
                                                                const rs2::device_list & devices_list );


enum special_folder
{
    user_desktop,
    user_documents,
    user_pictures,
    user_videos,
    temp_folder
};

#ifdef _WIN32
std::string get_folder_path( special_folder f );
#endif //_WIN32

#if defined __linux__ || defined __APPLE__
std::string get_folder_path( special_folder f );
#endif // defined __linux__ || defined __APPLE__
