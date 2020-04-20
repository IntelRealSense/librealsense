// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
#pragma once
#ifndef LIBREALSENSE_UNITTESTS_COMMON_H
#define LIBREALSENSE_UNITTESTS_COMMON_H

#include "catch/catch.hpp"
#include "../include/librealsense2/rs.hpp"
#include "../include/librealsense2/hpp/rs_context.hpp"
#include "../include/librealsense2/hpp/rs_internal.hpp"
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
#include "../src/types.h"

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

inline std::vector<profile>  configure_all_supported_streams(rs2::sensor& sensor, int width = 640, int height = 480, int fps = 60)
{
    std::vector<profile> all_profiles =
    {
        { RS2_STREAM_DEPTH,     RS2_FORMAT_Z16,           width, height,    0, fps},
        { RS2_STREAM_COLOR,     RS2_FORMAT_RGB8,          width, height,    0, fps},
        { RS2_STREAM_INFRARED,  RS2_FORMAT_Y8,            width, height,    1, fps},
        { RS2_STREAM_INFRARED,  RS2_FORMAT_Y8,            width, height,    2, fps},
        { RS2_STREAM_FISHEYE,   RS2_FORMAT_RAW8,          width, height,    0, fps},
        { RS2_STREAM_GYRO,      RS2_FORMAT_MOTION_XYZ32F,   1,      1,      0, 200},
        { RS2_STREAM_ACCEL,     RS2_FORMAT_MOTION_XYZ32F,   1,      1,      0, 250}
    };

    std::vector<profile> profiles;
    std::vector<rs2::stream_profile> modes;
    auto all_modes = sensor.get_stream_profiles();

    for (auto profile : all_profiles)
    {
        if (std::find_if(all_modes.begin(), all_modes.end(), [&](rs2::stream_profile p)
            {
                if (auto  video = p.as<rs2::video_stream_profile>())
                {
                    if (p.fps() == profile.fps &&
                        p.stream_index() == profile.index &&
                        p.stream_type() == profile.stream &&
                        p.format() == profile.format &&
                        video.width() == profile.width &&
                        video.height() == profile.height)
                    {
                        modes.push_back(p);
                        return true;
                    }
                }
                else
                {
                    if (auto  motion = p.as<rs2::motion_stream_profile>())
                    {
                        if (p.fps() == profile.fps &&
                            p.stream_index() == profile.index &&
                            p.stream_type() == profile.stream &&
                            p.format() == profile.format)
                        {
                            modes.push_back(p);
                            return true;
                        }
                    }
                    else
                        return false;
                }

                return false;
            }) != all_modes.end())
        {
            profiles.push_back(profile);

        }
    }
    if (modes.size() > 0)
        REQUIRE_NOTHROW(sensor.open(modes));
    return profiles;
}

inline std::pair<std::vector<rs2::sensor>, std::vector<profile>> configure_all_supported_streams(rs2::device& dev, int width = 640, int height = 480, int fps = 30)
{
    std::vector<profile> profiles;
    std::vector<rs2::sensor> sensors;
    auto sens = dev.query_sensors();
    for (auto s : sens)
    {
        auto res = configure_all_supported_streams(s, width, height, fps);
        profiles.insert(profiles.end(), res.begin(), res.end());
        if (res.size() > 0)
        {
            sensors.push_back(s);
        }
    }

    return{ sensors, profiles };
}

inline std::string space_to_underscore(const std::string& text) {
    const std::string from = " ";
    const std::string to = "__";
    auto temp = text;
    size_t start_pos = 0;
    while ((start_pos = temp.find(from, start_pos)) != std::string::npos) {
        temp.replace(start_pos, from.size(), to);
        start_pos += to.size();
    }
    return temp;
}

#define SECTION_FROM_TEST_NAME space_to_underscore(Catch::getCurrentContext().getResultCapture()->getCurrentTestName()).c_str()

//inline long long current_time()
//{
//    return (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() % 10000);
//}

//// disable in one place options that are sensitive to frame content
//// this is done to make sure unit-tests are deterministic
inline void disable_sensitive_options_for(rs2::sensor& sen)
{
    if (sen.supports(RS2_OPTION_ERROR_POLLING_ENABLED))
        REQUIRE_NOTHROW(sen.set_option(RS2_OPTION_ERROR_POLLING_ENABLED, 0));

    if (sen.supports(RS2_OPTION_ENABLE_AUTO_EXPOSURE))
        REQUIRE_NOTHROW(sen.set_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE, 0));

    if (sen.supports(RS2_OPTION_GLOBAL_TIME_ENABLED))
        REQUIRE_NOTHROW(sen.set_option(RS2_OPTION_GLOBAL_TIME_ENABLED, 0));

    if (sen.supports(RS2_OPTION_EXPOSURE))
    {
        rs2::option_range range;
        REQUIRE_NOTHROW(range = sen.get_option_range(RS2_OPTION_EXPOSURE)); // TODO: fails sometimes with "Element Not Found!"
        REQUIRE_NOTHROW(sen.set_option(RS2_OPTION_EXPOSURE, range.def));
    }
}

inline void disable_sensitive_options_for(rs2::device& dev)
{
    for (auto&& s : dev.query_sensors())
        disable_sensitive_options_for(s);
}

inline bool wait_for_reset(std::function<bool(void)> func, std::shared_ptr<rs2::device> dev)
{
    if (func())
        return true;

    WARN("Reset workaround");

    try {
        dev->hardware_reset();
    }
    catch (...)
    {
    }
    return func();
}

inline bool is_usb3(const rs2::device& dev)
{
    bool usb3_device = true;
    try
    {
        std::string usb_type(dev.get_info(RS2_CAMERA_INFO_USB_TYPE_DESCRIPTOR));
        // Device types "3.x" and also "" (for legacy playback records) will be recognized as USB3
        usb3_device = (std::string::npos == usb_type.find("2."));
    }
    catch (...) // In case the feature not provided assume USB3 device
    {
    }

    return usb3_device;
}

// Provides for Device PID , USB3/2 (true/false)
typedef  std::pair<std::string, bool > dev_type;

inline dev_type get_PID(rs2::device& dev)
{
    dev_type designator{ "",true };
    std::string usb_type{};
    bool usb_descriptor = false;

    REQUIRE_NOTHROW(designator.first = dev.get_info(RS2_CAMERA_INFO_PRODUCT_ID));
    REQUIRE_NOTHROW(usb_descriptor = dev.supports(RS2_CAMERA_INFO_USB_TYPE_DESCRIPTOR));
    if (usb_descriptor)
    {
        REQUIRE_NOTHROW(usb_type = dev.get_info(RS2_CAMERA_INFO_USB_TYPE_DESCRIPTOR));
        designator.second = (std::string::npos == usb_type.find("2."));
    }

    return designator;
}

class command_line_params
{
public:
    command_line_params(int argc, char * const argv[])
    {
        for (auto i = 0; i < argc; i++)
        {
            _params.push_back(argv[i]);
        }
    }

    char * const * get_argv() const { return _params.data(); }
    char * get_argv(int i) const { return _params[i]; }
    size_t get_argc() const { return _params.size(); }

    static command_line_params& instance(int argc = 0, char * const argv[] = 0)
    {
        static command_line_params params(argc, argv);
        return params;
    }

    bool _found_any_section = false;
private:

    std::vector<char*> _params;
};

inline bool file_exists(const std::string& filename)
{
    std::ifstream f(filename);
    return f.good();
}

inline bool make_context(const char* id, rs2::context* ctx, std::string min_api_version = "0.0.0")
{
    rs2::log_to_file(RS2_LOG_SEVERITY_DEBUG);

    static std::map<std::string, int> _counters;

    _counters[id]++;

    auto argc = command_line_params::instance().get_argc();
    auto argv = command_line_params::instance().get_argv();


    std::string base_filename;
    bool record = false;
    bool playback = false;
    for (auto i = 0u; i < argc; i++)
    {
        std::string param(argv[i]);
        if (param == "into")
        {
            i++;
            if (i < argc)
            {
                base_filename = argv[i];
                record = true;
            }
        }
        else if (param == "from")
        {
            i++;
            if (i < argc)
            {
                base_filename = argv[i];
                playback = true;
            }
        }
    }

    std::stringstream ss;
    ss << id << "." << _counters[id] << ".test";
    auto section = ss.str();


    try
    {
        if (record)
        {
            *ctx = rs2::recording_context(base_filename, section);
        }
        else if (playback)
        {
            *ctx = rs2::mock_context(base_filename, section, min_api_version);
        }
        command_line_params::instance()._found_any_section = true;
        return true;
    }
    catch (...)
    {
        return false;
    }

}

// Can be passed to rs2_error ** parameters, requires that an error is indicated with the specific provided message
class require_error
{
    std::string message;
    bool validate_error_message;      // Messages content may vary , subject to backend selection
    rs2_error * err;
public:
    require_error(std::string message, bool message_validation = true) : message(move(message)), validate_error_message(message_validation), err() {}
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
    REQUIRE(r[0] == Approx(a[1] * b[2] - a[2] * b[1]));
    REQUIRE(r[1] == Approx(a[2] * b[0] - a[0] * b[2]));
    REQUIRE(r[2] == Approx(a[0] * b[1] - a[1] * b[0]));
}

// Require that vector is exactly the zero vector
inline void require_zero_vector(const float(&vector)[3])
{
    for (int i = 1; i < 3; ++i) REQUIRE(vector[i] == 0.0f);
}

// Require that a == transpose(b)
inline void require_transposed(const float(&a)[9], const float(&b)[9])
{
    REQUIRE(a[0] == Approx(b[0]));
    REQUIRE(a[1] == Approx(b[3]));
    REQUIRE(a[2] == Approx(b[6]));
    REQUIRE(a[3] == Approx(b[1]));
    REQUIRE(a[4] == Approx(b[4]));
    REQUIRE(a[5] == Approx(b[7]));
    REQUIRE(a[6] == Approx(b[2]));
    REQUIRE(a[7] == Approx(b[5]));
    REQUIRE(a[8] == Approx(b[8]));
}

// Require that matrix is an orthonormal 3x3 matrix
inline void require_rotation_matrix(const float(&matrix)[9])
{
    const float row0[] = { matrix[0], matrix[3], matrix[6] };
    const float row1[] = { matrix[1], matrix[4], matrix[7] };
    const float row2[] = { matrix[2], matrix[5], matrix[8] };
    CAPTURE(row0[0]);
    CAPTURE(row0[1]);
    CAPTURE(row0[2]);
    CAPTURE(row1[0]);
    CAPTURE(row1[1]);
    CAPTURE(row1[2]);
    CAPTURE(row2[0]);
    CAPTURE(row2[1]);
    CAPTURE(row2[2]);
    REQUIRE(dot_product(row0, row0) == Approx(1));
    REQUIRE(dot_product(row1, row1) == Approx(1));
    REQUIRE(dot_product(row2, row2) == Approx(1));
    REQUIRE(dot_product(row0, row1) == Approx(0));
    REQUIRE(dot_product(row1, row2) == Approx(0));
    REQUIRE(dot_product(row2, row0) == Approx(0));
    require_cross_product(row0, row1, row2);
    require_cross_product(row0, row1, row2);
    require_cross_product(row0, row1, row2);
}

// Require that matrix is exactly the identity matrix
inline void require_identity_matrix(const float(&matrix)[9])
{
    static const float identity_matrix_3x3[] = { 1,0,0, 0,1,0, 0,0,1 };
    for (int i = 0; i < 9; ++i) REQUIRE(matrix[i] == Approx(identity_matrix_3x3[i]));
}

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

//inline void test_wait_for_frames(rs2_device * device, std::initializer_list<stream_mode>& modes, std::map<rs2_stream, test_duration>& duration_per_stream)
//{
//    rs2_start_device(device, require_no_error());
//    REQUIRE( rs2_is_device_streaming(device, require_no_error()) == 1 );
//
//    rs2_wait_for_frames(device, require_no_error());
//
//    int lowest_fps_mode = std::numeric_limits<int>::max();
//    for(auto & mode : modes)
//    {
//        REQUIRE( rs2_is_stream_enabled(device, mode.stream, require_no_error()) == 1 );
//        REQUIRE( rs2_get_frame_data(device, mode.stream, require_no_error()) != nullptr );
//        REQUIRE( rs2_get_frame_timestamp(device, mode.stream, require_no_error()) >= 0 );
//        if (mode.framerate < lowest_fps_mode) lowest_fps_mode = mode.framerate;
//    }
//
//    std::vector<unsigned long long> last_frame_number;
//    std::vector<unsigned long long> number_of_frames;
//    last_frame_number.resize(RS2_STREAM_COUNT);
//    number_of_frames.resize(RS2_STREAM_COUNT);
//
//    for (auto& elem : modes)
//    {
//        number_of_frames[elem.stream] = 0;
//        last_frame_number[elem.stream] = 0;
//    }
//
//    const auto actual_frames_to_receive = std::min(max_frames_to_receive, (uint32_t)(max_capture_time_sec* lowest_fps_mode));
//    const auto first_frame_to_capture = (actual_frames_to_receive*0.1);   // Skip the first 10% of frames
//    const auto frames_to_capture = (actual_frames_to_receive - first_frame_to_capture);
//
//    for (auto i = 0u; i< actual_frames_to_receive; ++i)
//    {
//        rs2_wait_for_frames(device, require_no_error());
//
//        if (i < first_frame_to_capture) continue;       // Skip some frames at the beginning to stabilize the stream output
//
//        for (auto & mode : modes)
//        {
//            if (rs2_get_frame_timestamp(device, mode.stream, require_no_error()) > 0)
//            {
//                REQUIRE( rs2_is_stream_enabled(device, mode.stream, require_no_error()) == 1);
//                REQUIRE( rs2_get_frame_data(device, mode.stream, require_no_error()) != nullptr);
//                REQUIRE( rs2_get_frame_timestamp(device, mode.stream, require_no_error()) >= 0);
//
//                auto frame_number = rs2_get_frame_number(device, mode.stream, require_no_error());
//                if (!duration_per_stream[mode.stream].is_end_time_initialized && last_frame_number[mode.stream] != frame_number)
//                {
//                    last_frame_number[mode.stream] = frame_number;
//                    ++number_of_frames[mode.stream];
//                }
//
//                if (!duration_per_stream[mode.stream].is_start_time_initialized && number_of_frames[mode.stream] >= 1)
//                {
//                    duration_per_stream[mode.stream].start_time = std::chrono::high_resolution_clock::now();
//                    duration_per_stream[mode.stream].is_start_time_initialized = true;
//                }
//
//                if (!duration_per_stream[mode.stream].is_end_time_initialized && (number_of_frames[mode.stream] > (0.9 * frames_to_capture))) // Requires additional work for streams with different fps
//                {
//                    duration_per_stream[mode.stream].end_time = std::chrono::high_resolution_clock::now();
//                    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(duration_per_stream[mode.stream].end_time - duration_per_stream[mode.stream].start_time).count();
//                    auto fps = ((double)number_of_frames[mode.stream] / duration) * 1000.;
//                    //check_fps(fps, mode.framerate);   // requires additional work to configure UVC controls in order to achieve the required fps
//                    duration_per_stream[mode.stream].is_end_time_initialized = true;
//                }
//            }
//        }
//    }
//
//    rs2_stop_device(device, require_no_error());
//    REQUIRE( rs2_is_device_streaming(device, require_no_error()) == 0 );
//}


static std::mutex m;
static std::mutex cb_mtx;
static std::condition_variable cv;
static std::atomic<bool> stop_streaming;
static int done;
struct user_data {
    std::map<rs2_stream, test_duration> duration_per_stream;
    std::map<rs2_stream, unsigned> number_of_frames_per_stream;
};


inline void frame_callback(rs2::device &dev, rs2::frame frame, void * user)
{
    std::lock_guard<std::mutex> lock(cb_mtx);

    if (stop_streaming || (frame.get_timestamp() == 0)) return;

    auto data = (user_data*)user;
    bool stop = true;

    auto stream_type = frame.get_profile().stream_type();

    for (auto& elem : data->number_of_frames_per_stream)
    {
        if (elem.second < data->duration_per_stream[stream_type].actual_frames_to_receive)
        {
            stop = false;
            break;
        }
    }


    if (stop)
    {
        stop_streaming = true;
        {
            std::lock_guard<std::mutex> lk(m);
            done = true;
        }
        cv.notify_one();
        return;
    }

    if (data->duration_per_stream[stream_type].is_end_time_initialized) return;

    unsigned num_of_frames = 0;
    num_of_frames = (++data->number_of_frames_per_stream[stream_type]);

    if (num_of_frames >= data->duration_per_stream[stream_type].actual_frames_to_receive)
    {
        if (!data->duration_per_stream[stream_type].is_end_time_initialized)
        {
            data->duration_per_stream[stream_type].end_time = std::chrono::high_resolution_clock::now();
            data->duration_per_stream[stream_type].is_end_time_initialized = true;
        }

        return;
    }

    if ((num_of_frames == data->duration_per_stream[stream_type].first_frame_to_capture) && (!data->duration_per_stream[stream_type].is_start_time_initialized))  // Skip some frames at the beginning to stabilize the stream output
    {
        data->duration_per_stream[stream_type].start_time = std::chrono::high_resolution_clock::now();
        data->duration_per_stream[stream_type].is_start_time_initialized = true;
    }

    REQUIRE(frame.get_data() != nullptr);
    REQUIRE(frame.get_timestamp() >= 0);
}

//inline void test_frame_callback(device &device, std::initializer_list<stream_profile>& modes, std::map<rs2_stream, test_duration>& duration_per_stream)
//{
//    done = false;
//    stop_streaming = false;
//    user_data data;
//    data.duration_per_stream = duration_per_stream;
//
//    for(auto & mode : modes)
//    {
//        data.number_of_frames_per_stream[mode.stream] = 0;
//        data.duration_per_stream[mode.stream].is_start_time_initialized = false;
//        data.duration_per_stream[mode.stream].is_end_time_initialized = false;
//        data.duration_per_stream[mode.stream].actual_frames_to_receive = std::min(max_frames_to_receive, (uint32_t)(max_capture_time_sec* mode.fps));
//        data.duration_per_stream[mode.stream].first_frame_to_capture = (uint32_t)(data.duration_per_stream[mode.stream].actual_frames_to_receive*0.1);   // Skip the first 10% of frames
//        data.duration_per_stream[mode.stream].frames_to_capture = data.duration_per_stream[mode.stream].actual_frames_to_receive - data.duration_per_stream[mode.stream].first_frame_to_capture;
//        // device doesn't know which subdevice is providing which stream. could loop over all subdevices? no, because subdevices don't expose this information
//        REQUIRE( rs2_is_stream_enabled(device, mode.stream, require_no_error()) == 1 );
//        rs2_start(device, mode.stream, frame_callback, &data, require_no_error());
//    }
//
//    rs2_start_device(device, require_no_error());
//    REQUIRE( rs2_is_device_streaming(device, require_no_error()) == 1 );
//
//    {
//        std::unique_lock<std::mutex> lk(m);
//        cv.wait(lk, [&]{return done;});
//        lk.unlock();
//    }
//
//    rs2_stop_device(device, require_no_error());
//    REQUIRE( rs2_is_device_streaming(device, require_no_error()) == 0 );
//
//    for(auto & mode : modes)
//    {
//        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(data.duration_per_stream[mode.stream].end_time - data.duration_per_stream[mode.stream].start_time).count();
//        auto fps = ((float)data.number_of_frames_per_stream[mode.stream] / duration) * 1000.;
//        //check_fps(fps, mode.framerate); / TODO is not suppuorted yet. See above message
//    }
//}

//inline void motion_callback(rs2_device * , rs2_motion_data, void *)
//{
//}
//
//inline void timestamp_callback(rs2_device * , rs2_timestamp_data, void *)
//{
//}
//
//// Provide support for doing basic streaming tests on a set of specified modes
//inline void test_streaming(rs2_device * device, std::initializer_list<stream_profile> modes)
//{
//    std::map<rs2_stream, test_duration> duration_per_stream;
//    for(auto & mode : modes)
//    {
//        duration_per_stream.insert(std::pair<rs2_stream, test_duration>(mode.stream, test_duration()));
//        rs2_enable_stream(device, mode.stream, mode.width, mode.height, mode.format, mode.framerate, require_no_error());
//        REQUIRE( rs2_is_stream_enabled(device, mode.stream, require_no_error()) == 1 );
//    }
//
//    test_wait_for_frames(device, modes, duration_per_stream);
//
//    test_frame_callback(device, modes, duration_per_stream);
//
//    for(auto & mode : modes)
//    {
//        rs2_disable_stream(device, mode.stream, require_no_error());
//        REQUIRE( rs2_is_stream_enabled(device, mode.stream, require_no_error()) == 0 );
//    }
//}

inline void test_option(rs2::sensor &device, rs2_option option, std::initializer_list<int> good_values, std::initializer_list<int> bad_values)
{
    // Test reading the current value
    float first_value;
    REQUIRE_NOTHROW(first_value = device.get_option(option));


    // check if first value is something sane (should be default?)
    rs2::option_range range;
    REQUIRE_NOTHROW(range = device.get_option_range(option));
    REQUIRE(first_value >= range.min);
    REQUIRE(first_value <= range.max);
    CHECK(first_value == range.def);

    // Test setting good values, and that each value set can be subsequently get
    for (auto value : good_values)
    {
        REQUIRE_NOTHROW(device.set_option(option, (float)value));
        REQUIRE(device.get_option(option) == value);
    }

    // Test setting bad values, and verify that they do not change the value of the option
    float last_good_value;
    REQUIRE_NOTHROW(last_good_value = device.get_option(option));
    for (auto value : bad_values)
    {
        REQUIRE_THROWS_AS(device.set_option(option, (float)value), rs2::error);
        REQUIRE(device.get_option(option) == last_good_value);
    }

    // Test that we can reset the option to its original value
    REQUIRE_NOTHROW(device.set_option(option, first_value));
    REQUIRE(device.get_option(option) == first_value);
}

inline rs2::stream_profile get_profile_by_resolution_type(rs2::sensor& s, res_type res)
{
    auto sp = s.get_stream_profiles();
    int width = 0, height = 0;
    for (auto&& p : sp)
    {
        if (auto video = p.as<rs2::video_stream_profile>())
        {
            width = video.width();
            height = video.height();
            if ((res = get_res_type(width, height)))
                return p;
        }
    }
    std::stringstream ss;
    ss << "stream profile for " << width << "," << height << " resolution is not supported!";
    throw std::runtime_error(ss.str());
}


enum special_folder
{
    user_desktop,
    user_documents,
    user_pictures,
    user_videos,
    temp_folder
};

#ifdef _WIN32
#include <windows.h>
#include <wchar.h>
#include <KnownFolders.h>
#include <shlobj.h>

inline std::string get_folder_path(special_folder f)
{
    std::string res;
    if (f == temp_folder)
    {
        TCHAR buf[MAX_PATH];
        if (GetTempPath(MAX_PATH, buf) != 0)
        {
            char str[1024];
            wcstombs(str, buf, 1023);
            res = str;
        }
    }
    else
    {
        GUID folder;
        switch (f)
        {
        case user_desktop: folder = FOLDERID_Desktop;
            break;
        case user_documents: folder = FOLDERID_Documents;
            break;
        case user_pictures: folder = FOLDERID_Pictures;
            break;
        case user_videos: folder = FOLDERID_Videos;
            break;
        default:
            throw std::invalid_argument(std::string("Value of f (") + std::to_string(f) + std::string(") is not supported"));
        }
        PWSTR folder_path = NULL;
        HRESULT hr = SHGetKnownFolderPath(folder, KF_FLAG_DEFAULT_PATH, NULL, &folder_path);
        if (SUCCEEDED(hr))
        {
            char str[1024];
            wcstombs(str, folder_path, 1023);
            CoTaskMemFree(folder_path);
            res = str;
        }
    }
    return res;
}
#endif //_WIN32

#if defined __linux__ || defined __APPLE__
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

inline std::string get_folder_path(special_folder f)
{
    std::string res;
    if (f == special_folder::temp_folder)
    {
        const char* tmp_dir = getenv("TMPDIR");
        res = tmp_dir ? tmp_dir : "/tmp/";
    }
    else
    {
        const char* home_dir = getenv("HOME");
        if (!home_dir)
        {
            struct passwd* pw = getpwuid(getuid());
            home_dir = (pw && pw->pw_dir) ? pw->pw_dir : "";
        }
        if (home_dir)
        {
            res = home_dir;
            switch (f)
            {
            case user_desktop: res += "/Desktop/";
                break;
            case user_documents: res += "/Documents/";
                break;
            case user_pictures: res += "/Pictures/";
                break;
            case user_videos: res += "/Videos/";
                break;
            default:
                throw std::invalid_argument(
                    std::string("Value of f (") + std::to_string(f) + std::string(") is not supported"));
            }
        }
    }
    return res;
}
#endif // defined __linux__ || defined __APPLE__

#endif
