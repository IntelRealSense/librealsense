// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This set of tests is valid for any number and combination of RealSense cameras, including R200 and F200 //
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <cmath>
#include "unit-tests-common.h"
#include "../include/librealsense2/rs_advanced_mode.hpp"
#include <librealsense2/hpp/rs_frame.hpp>
#include <iostream>

using namespace rs2;

# define SECTION_FROM_TEST_NAME space_to_underscore(Catch::getCurrentContext().getResultCapture()->getCurrentTestName()).c_str()

//// disable in one place options that are sensitive to frame content
//// this is done to make sure unit-tests are deterministic
void disable_sensitive_options_for(sensor& sen)
{
    if (sen.supports(RS2_OPTION_ERROR_POLLING_ENABLED))
        REQUIRE_NOTHROW(sen.set_option(RS2_OPTION_ERROR_POLLING_ENABLED, 0));

    if (sen.supports(RS2_OPTION_ENABLE_AUTO_EXPOSURE))
        REQUIRE_NOTHROW(sen.set_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE, 0));

    if (sen.supports(RS2_OPTION_EXPOSURE))
    {
        option_range range;
        REQUIRE_NOTHROW(range = sen.get_option_range(RS2_OPTION_EXPOSURE)); // TODO: fails sometimes with "Element Not Found!"
        REQUIRE_NOTHROW(sen.set_option(RS2_OPTION_EXPOSURE, range.def));
    }
}

void disable_sensitive_options_for(rs2::device& dev)
{
    for (auto&& s : dev.query_sensors())
        disable_sensitive_options_for(s);
}

bool wait_for_reset(std::function<bool(void)> func, std::shared_ptr<device> dev)
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


struct stream_request
{
    rs2_stream stream;
    rs2_format format;
    int width;
    int height;
    int fps;
    int index;

    bool operator==(const video_stream_profile& other) const
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
struct device_profiles
{
    std::vector<profile> streams;
    int fps;
    bool sync;
};

std::vector<profile>  configure_all_supported_streams(rs2::sensor& sensor, int width = 640, int height = 480, int fps = 60)
{
    std::vector<profile> all_profiles =
    {
        { RS2_STREAM_DEPTH, RS2_FORMAT_Z16, width, height, 0 },
        { RS2_STREAM_COLOR, RS2_FORMAT_RGB8, width, height, 0 },
        { RS2_STREAM_INFRARED, RS2_FORMAT_Y8, width, height, 1 },
        { RS2_STREAM_INFRARED, RS2_FORMAT_Y8, width, height, 2 },
        { RS2_STREAM_FISHEYE, RS2_FORMAT_RAW8, width, height, 0 },
        //        {RS2_STREAM_GYRO, 0, 0, 0, RS2_FORMAT_MOTION_XYZ32F, 0},
        //        {RS2_STREAM_ACCEL, 0,  0, 0, RS2_FORMAT_MOTION_XYZ32F, 0}
    };

    std::vector<profile> profiles;
    std::vector<rs2::stream_profile> modes;
    auto all_modes = sensor.get_stream_profiles();

    for (auto profile : all_profiles)
    {
        if (std::find_if(all_modes.begin(), all_modes.end(), [&](rs2::stream_profile p)
        {
            auto  video = p.as<rs2::video_stream_profile>();
            if (!video) return false;

            if (p.fps() == fps &&
                p.stream_index() == profile.index &&
                p.stream_type() == profile.stream &&
                p.format() == profile.format &&
                video.width() == profile.width &&
                video.height() == profile.height)
            {
                modes.push_back(p);
                return true;
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

std::pair<std::vector<sensor>, std::vector<profile>> configure_all_supported_streams(rs2::device& dev, int width = 640, int height = 480, int fps = 30)
{
    std::vector<profile > profiles;
    std::vector<sensor > sensors;
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

TEST_CASE("Sync sanity", "[live]") {

    const double DELTA = 20;
    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        auto list = ctx.query_devices();
        REQUIRE(list.size());

        auto dev = list[0];
        disable_sensitive_options_for(dev);

       
        rs2::syncer sync;
        auto profiles = configure_all_supported_streams(dev);
      
        for (auto s : dev.query_sensors())
        {
            s.start(sync);
        }

        std::vector<std::vector<double>> all_timestamps;

        for (auto i = 0; i < 200; i++)
        {
            auto frames = sync.wait_for_frames(5000);
            REQUIRE(frames.size() > 0);


            std::vector<double> timestamps;
            for (auto&& f : frames)
            {
                timestamps.push_back(f.get_timestamp());
            }
            all_timestamps.push_back(timestamps);

        }
        for (auto i = 0; i < 30; i++)
        {
            auto frames = sync.wait_for_frames(500);
        }
        auto num_of_partial_sync_sets = 0;
        for (auto set_timestamps : all_timestamps)
        {
            if (set_timestamps.size() < profiles.second.size())
                num_of_partial_sync_sets++;

            if (set_timestamps.size() <= 1)
                continue;

            std::sort(set_timestamps.begin(), set_timestamps.end());
            REQUIRE(set_timestamps[set_timestamps.size() - 1] - set_timestamps[0] <= DELTA);
        }

        REQUIRE((float)num_of_partial_sync_sets / (float)all_timestamps.size() < 0.90);
    }
}

TEST_CASE("Sync different fps", "[live][!mayfail]") {

    const double DELTA = 20;
    rs2::context ctx;

    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        auto list = ctx.query_devices();
        REQUIRE(list.size());
        auto dev = list[0];

        list = ctx.query_devices();
        REQUIRE(list.size());
        dev = list[0];

        syncer syncer;
        disable_sensitive_options_for(dev);

        auto sensors = dev.query_sensors();

        for (auto s : sensors)
        {
            if (s.supports(RS2_OPTION_EXPOSURE))
            {
                auto range = s.get_option_range(RS2_OPTION_EXPOSURE);
                REQUIRE_NOTHROW(s.set_option(RS2_OPTION_EXPOSURE, range.min));
            }

        }

        std::map<rs2_stream, rs2::sensor*> profiles_sensors;
        std::map<rs2::sensor*, rs2_stream> sensors_profiles;

        std::vector<int> fps(sensors.size(), 0);
        for (auto i = 0; i < fps.size(); i++)
        {
            fps[i] = (fps.size() - i - 1) * 30 % 90 + 30;
        }
        std::vector<std::vector<rs2::sensor*>> streams_groups(fps.size());
        for (auto i = 0; i < sensors.size(); i++)
        {
            auto profs = configure_all_supported_streams(sensors[i], 640, 480, fps[i]);
            for (auto p : profs)
            {
                profiles_sensors[p.stream] = &sensors[i];
                sensors_profiles[&sensors[i]] = p.stream;
            }
            if (profs.size() > 0)
                sensors[i].start(syncer);
        }

        for (auto i = 0; i < sensors.size(); i++)
        {
            for (auto j = 0; j < sensors.size(); j++)
            {
                if ((float)fps[j] / (float)fps[i] >= 1)
                {
                    streams_groups[i].push_back(&sensors[j]);
                }

            }
        }


        std::vector<std::vector<rs2_stream>> frames_arrived;

        for (auto i = 0; i < 200; i++)
        {
            auto frames = syncer.wait_for_frames(5000);
            REQUIRE(frames.size() > 0);
            std::vector<rs2_stream> streams_arrived;
            for (auto&& f : frames)
            {
                auto s = f.get_profile().stream_type();
                streams_arrived.push_back(s);
            }
            frames_arrived.push_back(streams_arrived);
        }

        std::vector<int> streams_groups_arrrived(streams_groups.size(), 0);

        for (auto streams : frames_arrived)
        {
            std::set<rs2::sensor*> sensors;

            for (auto s : streams)
            {
                sensors.insert(profiles_sensors[s]);
            }
            std::vector<rs2::sensor*> sensors_vec(sensors.size());
            std::copy(sensors.begin(), sensors.end(), sensors_vec.begin());
            auto it = std::find(streams_groups.begin(), streams_groups.end(), sensors_vec);
            if (it != streams_groups.end())
            {
                auto ind = std::distance(streams_groups.begin(), it);
                streams_groups_arrrived[ind]++;
            }
        }


        for (auto i = 0; i < streams_groups_arrrived.size(); i++)
        {
            for (auto j = 0; j < streams_groups_arrrived.size(); j++)
            {
                REQUIRE(streams_groups_arrrived[j]);
                auto num1 = streams_groups_arrrived[i];
                auto num2 = streams_groups_arrrived[j];
                CAPTURE(sensors_profiles[&sensors[i]]);
                CAPTURE(num1);
                CAPTURE(sensors_profiles[&sensors[j]]);
                CAPTURE(num2);
                REQUIRE((float)num1 / (float)num2 <= 5 * (float)fps[i] / (float)fps[j]);
            }

        }
    }
}


bool get_mode(rs2::device& dev, stream_profile* profile, int mode_index = 0)
{
    auto sensors = dev.query_sensors();
    REQUIRE(sensors.size() > 0);

    for (auto i = 0; i < sensors.size(); i++)
    {
        auto modes = sensors[i].get_stream_profiles();
        REQUIRE(modes.size() > 0);

        if (mode_index >= modes.size())
            continue;

        *profile = modes[mode_index];
        return true;
    }
    return false;
}

TEST_CASE("Sync start stop", "[live]") {
    rs2::context ctx;

    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        auto list = ctx.query_devices();
        REQUIRE(list.size() > 0);
        auto dev = list[0];
        disable_sensitive_options_for(dev);

        syncer sync;

        rs2::stream_profile mode;
        auto mode_index = 0;

        do
        {
            REQUIRE(get_mode(dev, &mode, mode_index));
            mode_index++;
        } while (mode.fps() != 60);

        auto video = mode.as<rs2::video_stream_profile>();
        auto res = configure_all_supported_streams(dev, video.width(), video.height(), mode.fps());

        for (auto s : res.first)
        {
            REQUIRE_NOTHROW(s.start(sync));
        }
        

        rs2::frameset frames;
        for (auto i = 0; i < 30; i++)
        {
            frames = sync.wait_for_frames(10000);
            REQUIRE(frames.size() > 0);
        }

        frameset old_frames;

        while (sync.poll_for_frames(&old_frames));

        stream_profile other_mode;
        mode_index = 0;

        REQUIRE(get_mode(dev, &other_mode, mode_index));
        auto other_video = other_mode.as<rs2::video_stream_profile>();
        while ((other_video.height() == video.height() && other_video.width() == video.width()) || other_video.fps() != 60)
        {
            REQUIRE(get_mode(dev, &other_mode, mode_index));
            mode_index++;
            other_video = other_mode.as<rs2::video_stream_profile>();
            REQUIRE(other_video);
        }

        for (auto s : res.first)
        {
            REQUIRE_NOTHROW(s.stop());
            REQUIRE_NOTHROW(s.close());
        }
        res = configure_all_supported_streams(dev, other_video.width(), other_video.height(), other_mode.fps());

        for (auto s : res.first)
        {
            REQUIRE_NOTHROW(s.start(sync));
        }

        for (auto i = 0; i < 10; i++)
            frames = sync.wait_for_frames(10000);

        REQUIRE(frames.size() > 0);
        auto f = frames[0];
        auto image = f.as<rs2::video_frame>();
        REQUIRE(image);

        REQUIRE(image.get_width() == other_video.width());

        REQUIRE(image.get_height() == other_video.height());
    }
}
TEST_CASE("Device metadata enumerates correctly", "[live]")
{
    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {       // Require at least one device to be plugged in
        std::vector<rs2::device> list;
        REQUIRE_NOTHROW(list = ctx.query_devices());
        REQUIRE(list.size() > 0);

        // For each device
        for (auto&& dev : list)
        {
            SECTION("supported device metadata strings are nonempty, unsupported ones throw")
            {
                for (auto j = 0; j < RS2_CAMERA_INFO_COUNT; ++j) {
                    auto is_supported = false;
                    REQUIRE_NOTHROW(is_supported = dev.supports(rs2_camera_info(j)));
                    if (is_supported) REQUIRE(dev.get_info(rs2_camera_info(j)) != nullptr);
                    else REQUIRE_THROWS(dev.get_info(rs2_camera_info(j)));
                }
            }
        }
    }
}

////////////////////////////////////////////
////// Test basic streaming functionality //
////////////////////////////////////////////

TEST_CASE("Start-Stop stream sequence", "[live]")
{
    // Require at least one device to be plugged in
    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        std::vector<sensor> list;
        REQUIRE_NOTHROW(list = ctx.query_all_sensors());
        REQUIRE(list.size() > 0);

        pipeline pipe(ctx);
        device dev;
        // Configure all supported streams to run at 30 frames per second

        for (auto i = 0; i < 5; i++)
        {
            REQUIRE_NOTHROW(pipe.open());
            REQUIRE_NOTHROW(dev = pipe.get_device());
            disable_sensitive_options_for(dev);

            // Test sequence
            REQUIRE_NOTHROW(pipe.start());

            REQUIRE_NOTHROW(pipe.stop());
        }
    }
}

/////////////////////////////////////////
//////// Calibration information tests //
/////////////////////////////////////////

TEST_CASE("No extrinsic transformation between a stream and itself", "[live]")
{
    // Require at least one device to be plugged in
    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        std::vector<sensor> list;
        REQUIRE_NOTHROW(list = ctx.query_all_sensors());
        const size_t device_count = list.size();
        REQUIRE(device_count > 0);

        // For each device
        for (auto&& dev : list)
        {
            std::vector<rs2::stream_profile> profs;
            REQUIRE_NOTHROW(profs = dev.get_stream_profiles());
            REQUIRE(profs.size()>0);

            rs2_extrinsics extrin;
            try {
                auto prof = profs[0];
                extrin = prof.get_extrinsics_to(prof);
            }
            catch (error &e) {
                // if device isn't calibrated, get_extrinsics must error out (according to old comment. Might not be true under new API)
                WARN(e.what());
                continue;
            }

            require_identity_matrix(extrin.rotation);
            require_zero_vector(extrin.translation);
        }
    }
}

TEST_CASE("Extrinsic transformation between two streams is a rigid transform", "[live]")
{
    // Require at least one device to be plugged in
    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        std::vector<sensor> list;
        REQUIRE_NOTHROW(list = ctx.query_all_sensors());
        const size_t device_count = list.size();
        REQUIRE(device_count > 0);

        // For each device
        for (int i = 0; i < device_count; ++i)
        {
            auto dev = list[i];
            auto adj_devices = ctx.get_sensor_parent(dev).query_sensors();

            //REQUIRE(dev != nullptr);

            // For every pair of streams
            for (auto j = 0; j < adj_devices.size(); ++j)
            {
                for (int k = j + 1; k < adj_devices.size(); ++k)
                {
                    std::vector<rs2::stream_profile> profs_a, profs_b;
                    REQUIRE_NOTHROW(profs_a = adj_devices[j].get_stream_profiles());
                    REQUIRE(profs_a.size()>0);

                    REQUIRE_NOTHROW(profs_b = adj_devices[k].get_stream_profiles());
                    REQUIRE(profs_b.size()>0);

                    // Extrinsics from A to B should have an orthonormal 3x3 rotation matrix and a translation vector of magnitude less than 10cm
                    rs2_extrinsics a_to_b;

                    try {
                        a_to_b = profs_a[0].get_extrinsics_to(profs_b[0]);
                    }
                    catch (error &e) {
                        WARN(e.what());
                        continue;
                    }

                    require_rotation_matrix(a_to_b.rotation);
                    REQUIRE(vector_length(a_to_b.translation) < 0.1f);

                    // Extrinsics from B to A should be the inverse of extrinsics from A to B
                    rs2_extrinsics b_to_a;
                    REQUIRE_NOTHROW(b_to_a = profs_b[0].get_extrinsics_to(profs_a[0]));

                    require_transposed(a_to_b.rotation, b_to_a.rotation);
                    REQUIRE(b_to_a.rotation[0] * a_to_b.translation[0] + b_to_a.rotation[3] * a_to_b.translation[1] + b_to_a.rotation[6] * a_to_b.translation[2] == Approx(-b_to_a.translation[0]));
                    REQUIRE(b_to_a.rotation[1] * a_to_b.translation[0] + b_to_a.rotation[4] * a_to_b.translation[1] + b_to_a.rotation[7] * a_to_b.translation[2] == Approx(-b_to_a.translation[1]));
                    REQUIRE(b_to_a.rotation[2] * a_to_b.translation[0] + b_to_a.rotation[5] * a_to_b.translation[1] + b_to_a.rotation[8] * a_to_b.translation[2] == Approx(-b_to_a.translation[2]));
                }
            }
        }
    }
}

TEST_CASE("Extrinsic transformations are transitive", "[live]")
{
    // Require at least one device to be plugged in
    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        std::vector<sensor> list;
        REQUIRE_NOTHROW(list = ctx.query_all_sensors());
        REQUIRE(list.size() > 0);

        // For each device
        for (auto&& dev : list)
        {
            auto adj_devices = ctx.get_sensor_parent(dev).query_sensors();

            // For every set of subdevices
            for (auto a = 0; a < adj_devices.size(); ++a)
            {
                for (auto b = 0; b < adj_devices.size(); ++b)
                {
                    for (auto c = 0; c < adj_devices.size(); ++c)
                    {
                        std::vector<rs2::stream_profile> profs_a, profs_b, profs_c ;
                        REQUIRE_NOTHROW(profs_a = adj_devices[a].get_stream_profiles());
                        REQUIRE(profs_a.size()>0);

                        REQUIRE_NOTHROW(profs_b = adj_devices[b].get_stream_profiles());
                        REQUIRE(profs_b.size()>0);

                        REQUIRE_NOTHROW(profs_c = adj_devices[c].get_stream_profiles());
                        REQUIRE(profs_c.size()>0);

                        // Require that the composition of a_to_b and b_to_c is equal to a_to_c
                        rs2_extrinsics a_to_b, b_to_c, a_to_c;

                        try {
                            a_to_b = profs_a[0].get_extrinsics_to(profs_b[0]);
                            b_to_c = profs_b[0].get_extrinsics_to(profs_c[0]);
                            a_to_c = profs_a[0].get_extrinsics_to(profs_c[0]);
                        }
                        catch (error &e)
                        {
                            WARN(e.what());
                            continue;
                        }

                        // a_to_c.rotation == a_to_b.rotation * b_to_c.rotation
                        REQUIRE(a_to_c.rotation[0] == Approx(a_to_b.rotation[0] * b_to_c.rotation[0] + a_to_b.rotation[3] * b_to_c.rotation[1] + a_to_b.rotation[6] * b_to_c.rotation[2]));
                        REQUIRE(a_to_c.rotation[2] == Approx(a_to_b.rotation[2] * b_to_c.rotation[0] + a_to_b.rotation[5] * b_to_c.rotation[1] + a_to_b.rotation[8] * b_to_c.rotation[2]));
                        REQUIRE(a_to_c.rotation[1] == Approx(a_to_b.rotation[1] * b_to_c.rotation[0] + a_to_b.rotation[4] * b_to_c.rotation[1] + a_to_b.rotation[7] * b_to_c.rotation[2]));
                        REQUIRE(a_to_c.rotation[3] == Approx(a_to_b.rotation[0] * b_to_c.rotation[3] + a_to_b.rotation[3] * b_to_c.rotation[4] + a_to_b.rotation[6] * b_to_c.rotation[5]));
                        REQUIRE(a_to_c.rotation[4] == Approx(a_to_b.rotation[1] * b_to_c.rotation[3] + a_to_b.rotation[4] * b_to_c.rotation[4] + a_to_b.rotation[7] * b_to_c.rotation[5]));
                        REQUIRE(a_to_c.rotation[5] == Approx(a_to_b.rotation[2] * b_to_c.rotation[3] + a_to_b.rotation[5] * b_to_c.rotation[4] + a_to_b.rotation[8] * b_to_c.rotation[5]));
                        REQUIRE(a_to_c.rotation[6] == Approx(a_to_b.rotation[0] * b_to_c.rotation[6] + a_to_b.rotation[3] * b_to_c.rotation[7] + a_to_b.rotation[6] * b_to_c.rotation[8]));
                        REQUIRE(a_to_c.rotation[7] == Approx(a_to_b.rotation[1] * b_to_c.rotation[6] + a_to_b.rotation[4] * b_to_c.rotation[7] + a_to_b.rotation[7] * b_to_c.rotation[8]));
                        REQUIRE(a_to_c.rotation[8] == Approx(a_to_b.rotation[2] * b_to_c.rotation[6] + a_to_b.rotation[5] * b_to_c.rotation[7] + a_to_b.rotation[8] * b_to_c.rotation[8]));

                        // a_to_c.translation = a_to_b.transform(b_to_c.translation)
                        REQUIRE(a_to_c.translation[0] == Approx(a_to_b.rotation[0] * b_to_c.translation[0] + a_to_b.rotation[3] * b_to_c.translation[1] + a_to_b.rotation[6] * b_to_c.translation[2] + a_to_b.translation[0]));
                        REQUIRE(a_to_c.translation[1] == Approx(a_to_b.rotation[1] * b_to_c.translation[0] + a_to_b.rotation[4] * b_to_c.translation[1] + a_to_b.rotation[7] * b_to_c.translation[2] + a_to_b.translation[1]));
                        REQUIRE(a_to_c.translation[2] == Approx(a_to_b.rotation[2] * b_to_c.translation[0] + a_to_b.rotation[5] * b_to_c.translation[1] + a_to_b.rotation[8] * b_to_c.translation[2] + a_to_b.translation[2]));
                    }
                }
            }
        }
    }
}

std::shared_ptr<device> do_with_waiting_for_camera_connection(rs2::context ctx, std::shared_ptr<device> dev, std::string serial, std::function<void()> operation)
{
    std::mutex m;
    bool disconnected = false;
    bool connected = false;
    std::shared_ptr<device> result;
    std::condition_variable cv;

    ctx.set_devices_changed_callback([&result, dev, &disconnected, &connected, &m, &cv, &serial](rs2::event_information info) mutable
    {
        if (info.was_removed(*dev))
        {
            std::unique_lock<std::mutex> lock(m);
            disconnected = true;
            cv.notify_all();
        }
        auto list = info.get_new_devices();
        if (list.size() > 0)
        {
            for (auto cam : list)
            {
                if (serial == cam.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER))
                {
                    std::unique_lock<std::mutex> lock(m);
                    connected = true;

                    result = std::make_shared<device>(cam);


                    disable_sensitive_options_for(*result);
                    cv.notify_all();
                    break;
                }
            }
        }
    });

    operation();

    std::unique_lock<std::mutex> lock(m);
    REQUIRE(wait_for_reset([&]() {
        return cv.wait_for(lock, std::chrono::seconds(20), [&]() {return disconnected; });
    }, dev));
    REQUIRE(cv.wait_for(lock, std::chrono::seconds(20), [&]() {return connected; }));
    REQUIRE(result);
    return result;
}

TEST_CASE("Toggle Advanced Mode", "[live][AdvMd]") {
    for (int i = 0; i < 10; ++i)
    {
        rs2::context ctx;
        if (make_context(SECTION_FROM_TEST_NAME, &ctx))
        {
            device_list list;
            REQUIRE_NOTHROW(list = ctx.query_devices());
            REQUIRE(list.size() > 0);

            auto dev = std::make_shared<device>(list.front());

            disable_sensitive_options_for(*dev);

            std::string serial;
            REQUIRE_NOTHROW(serial = dev->get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));

            if (dev->is<rs400::advanced_mode>())
            {
                auto advanced = dev->as<rs400::advanced_mode>();

                if (!advanced.is_enabled())
                {
                    dev = do_with_waiting_for_camera_connection(ctx, dev, serial, [&]()
                    {
                        REQUIRE_NOTHROW(advanced.toggle_advanced_mode(true));
                    });
                }
                disable_sensitive_options_for(*dev);
                advanced = dev->as<rs400::advanced_mode>();

                REQUIRE(advanced.is_enabled());

                dev = do_with_waiting_for_camera_connection(ctx, dev, serial, [&]()
                {
                    REQUIRE_NOTHROW(advanced.toggle_advanced_mode(false));
                });
                advanced = dev->as<rs400::advanced_mode>();
                REQUIRE(!advanced.is_enabled());
            }
        }
    }
}


TEST_CASE("Advanced Mode presets", "[live][AdvMd]")
{
    static const std::vector<res_type> resolutions = { low_resolution,
                                                       medium_resolution,
                                                       high_resolution };

    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        device_list list;
        REQUIRE_NOTHROW(list = ctx.query_devices());
        REQUIRE(list.size() > 0);

        auto dev = std::make_shared<device>(list.front());

        disable_sensitive_options_for(*dev);

        std::string serial;
        REQUIRE_NOTHROW(serial = dev->get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));

        if (dev->is<rs400::advanced_mode>())
        {
            auto advanced = dev->as<rs400::advanced_mode>();

            if (!advanced.is_enabled())
            {
                dev = do_with_waiting_for_camera_connection(ctx, dev, serial, [&]()
                {
                    REQUIRE_NOTHROW(advanced.toggle_advanced_mode(true));
                });
            }
            disable_sensitive_options_for(*dev);
            advanced = dev->as<rs400::advanced_mode>();

            REQUIRE(advanced.is_enabled());
            auto sensors = dev->query_sensors();

            sensor presets_sensor;
            for (sensor& elem : sensors)
            {
                auto supports = false;
                REQUIRE_NOTHROW(supports = elem.supports(RS2_OPTION_VISUAL_PRESET));
                if (supports)
                {
                    presets_sensor = elem;
                    break;
                }
            }


            for (auto& res : resolutions)
            {
                std::vector<rs2::stream_profile> sp = {get_profile_by_resolution_type(presets_sensor, res)};
                presets_sensor.open(sp);
                presets_sensor.start([](rs2::frame) {});
                for (auto i = 0; i < RS2_RS400_VISUAL_PRESET_COUNT; ++i)
                {
                    auto preset = (rs2_rs400_visual_preset)i;
                    CAPTURE(res);
                    CAPTURE(preset);
                    REQUIRE_NOTHROW(presets_sensor.set_option(RS2_OPTION_VISUAL_PRESET, (float)preset));
                    float ret_preset;
                    REQUIRE_NOTHROW(ret_preset = presets_sensor.get_option(RS2_OPTION_VISUAL_PRESET));
                    REQUIRE(preset == (rs2_rs400_visual_preset)((int)ret_preset));
                }
                presets_sensor.stop();
                presets_sensor.close();
            }

            dev = do_with_waiting_for_camera_connection(ctx, dev, serial, [&]()
            {
                REQUIRE_NOTHROW(advanced.toggle_advanced_mode(false));
            });
            disable_sensitive_options_for(*dev);
            advanced = dev->as<rs400::advanced_mode>();
            REQUIRE(!advanced.is_enabled());
        }
    }
}

TEST_CASE("Advanced Mode JSON", "[live][AdvMd]") {
    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        device_list list;
        REQUIRE_NOTHROW(list = ctx.query_devices());
        REQUIRE(list.size() > 0);


        auto dev = std::make_shared<device>(list.front());

        disable_sensitive_options_for(*dev);

        std::string serial;
        REQUIRE_NOTHROW(serial = dev->get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));

        if (dev->is<rs400::advanced_mode>())
        {
            auto advanced = dev->as<rs400::advanced_mode>();

            if (!advanced.is_enabled())
            {
                dev = do_with_waiting_for_camera_connection(ctx, dev, serial, [&]()
                {
                    REQUIRE_NOTHROW(advanced.toggle_advanced_mode(true));
                });
            }

            disable_sensitive_options_for(*dev);
            advanced = dev->as<rs400::advanced_mode>();
            REQUIRE(advanced.is_enabled());

            auto sensors = dev->query_sensors();
            sensor presets_sensor;
            for (sensor& elem : sensors)
            {
                auto supports = false;
                REQUIRE_NOTHROW(supports = elem.supports(RS2_OPTION_VISUAL_PRESET));
                if (supports)
                {
                    presets_sensor = elem;
                    break;
                }
            }

            std::string json1, json2;
            REQUIRE_NOTHROW(json1 = advanced.serialize_json());
            REQUIRE_NOTHROW(presets_sensor.set_option(RS2_OPTION_VISUAL_PRESET, RS2_RS400_VISUAL_PRESET_COUNT - 1));
            REQUIRE_NOTHROW(advanced.load_json(json1));
            REQUIRE_NOTHROW(json2 = advanced.serialize_json());
            REQUIRE_NOTHROW(json1 == json2);

            dev = do_with_waiting_for_camera_connection(ctx, dev, serial, [&]()
            {
                REQUIRE_NOTHROW(advanced.toggle_advanced_mode(false));
            });
            disable_sensitive_options_for(*dev);
            advanced = dev->as<rs400::advanced_mode>();
            REQUIRE(!advanced.is_enabled());
        }
    }
}

TEST_CASE("Advanced Mode controls", "[live][AdvMd]") {
    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        device_list list;
        REQUIRE_NOTHROW(list = ctx.query_devices());
        REQUIRE(list.size() > 0);

        std::shared_ptr<device> dev = std::make_shared<device>(list.front());
        if (dev->is<rs400::advanced_mode>())
        {
            disable_sensitive_options_for(*dev);
            auto info = dev->get_info(RS2_CAMERA_INFO_NAME);
            CAPTURE(info);

            std::string serial;
            REQUIRE_NOTHROW(serial = dev->get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));


            auto advanced = dev->as<rs400::advanced_mode>();
            if (!advanced.is_enabled())
            {

                dev = do_with_waiting_for_camera_connection(ctx, dev, serial, [&]()
                {
                    REQUIRE_NOTHROW(advanced.toggle_advanced_mode(true));
                });
            }



            disable_sensitive_options_for(*dev);
            advanced = dev->as<rs400::advanced_mode>();
            REQUIRE(advanced.is_enabled());

            {
                STDepthControlGroup ctrl_curr{};
                REQUIRE_NOTHROW(ctrl_curr = advanced.get_depth_control(0));
                STDepthControlGroup ctrl_min{};
                REQUIRE_NOTHROW(ctrl_min = advanced.get_depth_control(1));
                STDepthControlGroup ctrl_max{};
                REQUIRE_NOTHROW(ctrl_max = advanced.get_depth_control(2));
                REQUIRE_NOTHROW(advanced.set_depth_control(ctrl_min));
                REQUIRE_NOTHROW(ctrl_curr = advanced.get_depth_control(0));
                REQUIRE(ctrl_curr == ctrl_min);
            }

            {
                STRsm ctrl_curr{};
                REQUIRE_NOTHROW(ctrl_curr = advanced.get_rsm(0));
                STRsm ctrl_min{};
                REQUIRE_NOTHROW(ctrl_min = advanced.get_rsm(1));
                STRsm ctrl_max{};
                REQUIRE_NOTHROW(ctrl_max = advanced.get_rsm(2));
                REQUIRE_NOTHROW(advanced.set_rsm(ctrl_min));
                REQUIRE_NOTHROW(ctrl_curr = advanced.get_rsm(0));
                REQUIRE(ctrl_curr == ctrl_min);
            }

            {
                STRauSupportVectorControl ctrl_curr{};
                REQUIRE_NOTHROW(ctrl_curr = advanced.get_rau_support_vector_control(0));
                STRauSupportVectorControl ctrl_min{};
                REQUIRE_NOTHROW(ctrl_min = advanced.get_rau_support_vector_control(1));
                STRauSupportVectorControl ctrl_max{};
                REQUIRE_NOTHROW(ctrl_max = advanced.get_rau_support_vector_control(2));
                REQUIRE_NOTHROW(advanced.set_rau_support_vector_control(ctrl_min));
                REQUIRE_NOTHROW(ctrl_curr = advanced.get_rau_support_vector_control(0));
                REQUIRE(ctrl_curr == ctrl_min);
            }

            {
                STColorControl ctrl_curr{};
                REQUIRE_NOTHROW(ctrl_curr = advanced.get_color_control(0));
                STColorControl ctrl_min{};
                REQUIRE_NOTHROW(ctrl_min = advanced.get_color_control(1));
                STColorControl ctrl_max{};
                REQUIRE_NOTHROW(ctrl_max = advanced.get_color_control(2));
                REQUIRE_NOTHROW(advanced.set_color_control(ctrl_min));
                REQUIRE_NOTHROW(ctrl_curr = advanced.get_color_control(0));
                REQUIRE(ctrl_curr == ctrl_min);
            }

            {
                STRauColorThresholdsControl ctrl_curr{};
                REQUIRE_NOTHROW(ctrl_curr = advanced.get_rau_thresholds_control(0));
                STRauColorThresholdsControl ctrl_min{};
                REQUIRE_NOTHROW(ctrl_min = advanced.get_rau_thresholds_control(1));
                STRauColorThresholdsControl ctrl_max{};
                REQUIRE_NOTHROW(ctrl_max = advanced.get_rau_thresholds_control(2));
                REQUIRE_NOTHROW(advanced.set_rau_thresholds_control(ctrl_min));
                REQUIRE_NOTHROW(ctrl_curr = advanced.get_rau_thresholds_control(0));
                REQUIRE(ctrl_curr == ctrl_min);
            }

            {
                STSloColorThresholdsControl ctrl_curr{};
                REQUIRE_NOTHROW(ctrl_curr = advanced.get_slo_color_thresholds_control(0));
                STSloColorThresholdsControl ctrl_min{};
                REQUIRE_NOTHROW(ctrl_min = advanced.get_slo_color_thresholds_control(1));
                STSloColorThresholdsControl ctrl_max{};
                REQUIRE_NOTHROW(ctrl_max = advanced.get_slo_color_thresholds_control(2));
                REQUIRE_NOTHROW(advanced.set_slo_color_thresholds_control(ctrl_min));
                REQUIRE_NOTHROW(ctrl_curr = advanced.get_slo_color_thresholds_control(0));
                REQUIRE(ctrl_curr == ctrl_min);
            }

            {
                STSloPenaltyControl ctrl_curr{};
                REQUIRE_NOTHROW(ctrl_curr = advanced.get_slo_penalty_control(0));
                STSloPenaltyControl ctrl_min{};
                REQUIRE_NOTHROW(ctrl_min = advanced.get_slo_penalty_control(1));
                STSloPenaltyControl ctrl_max{};
                REQUIRE_NOTHROW(ctrl_max = advanced.get_slo_penalty_control(2));
                REQUIRE_NOTHROW(advanced.set_slo_penalty_control(ctrl_min));
                REQUIRE_NOTHROW(ctrl_curr = advanced.get_slo_penalty_control(0));
                REQUIRE(ctrl_curr == ctrl_min);
            }

            {
                STHdad ctrl_curr1{};
                REQUIRE_NOTHROW(ctrl_curr1 = advanced.get_hdad(0));
                REQUIRE_NOTHROW(advanced.set_hdad(ctrl_curr1));
                STHdad ctrl_curr2{};
                REQUIRE_NOTHROW(ctrl_curr2 = advanced.get_hdad(0));
                REQUIRE(ctrl_curr1 == ctrl_curr2);
            }

            {
                STColorCorrection ctrl_curr{};
                REQUIRE_NOTHROW(ctrl_curr = advanced.get_color_correction(0));
                STColorCorrection ctrl_min{};
                REQUIRE_NOTHROW(ctrl_min = advanced.get_color_correction(1));
                STColorCorrection ctrl_max{};
                REQUIRE_NOTHROW(ctrl_max = advanced.get_color_correction(2));
                REQUIRE_NOTHROW(advanced.set_color_correction(ctrl_min));
                REQUIRE_NOTHROW(ctrl_curr = advanced.get_color_correction(0));
                REQUIRE(ctrl_curr == ctrl_min);
            }

            {
                STAEControl ctrl_curr1{};
                REQUIRE_NOTHROW(ctrl_curr1 = advanced.get_ae_control(0));
                REQUIRE_NOTHROW(advanced.set_ae_control(ctrl_curr1));
                STAEControl ctrl_curr2{};
                REQUIRE_NOTHROW(ctrl_curr2 = advanced.get_ae_control(0));
                REQUIRE(ctrl_curr1 == ctrl_curr2);
            }

            {
                STDepthTableControl ctrl_curr{};
                REQUIRE_NOTHROW(ctrl_curr = advanced.get_depth_table(0));
                STDepthTableControl ctrl_min{};
                REQUIRE_NOTHROW(ctrl_min = advanced.get_depth_table(1));
                STDepthTableControl ctrl_max{};
                REQUIRE_NOTHROW(ctrl_max = advanced.get_depth_table(2));
                REQUIRE_NOTHROW(advanced.set_depth_table(ctrl_min));
                REQUIRE_NOTHROW(ctrl_curr = advanced.get_depth_table(0));
                REQUIRE(ctrl_curr == ctrl_min);
            }

            {
                STCensusRadius ctrl_curr{};
                REQUIRE_NOTHROW(ctrl_curr = advanced.get_census(0));
                STCensusRadius ctrl_min{};
                REQUIRE_NOTHROW(ctrl_min = advanced.get_census(1));
                STCensusRadius ctrl_max{};
                REQUIRE_NOTHROW(ctrl_max = advanced.get_census(2));
                REQUIRE_NOTHROW(advanced.set_census(ctrl_min));
                REQUIRE_NOTHROW(ctrl_curr = advanced.get_census(0));
                REQUIRE(ctrl_curr == ctrl_min);
            }


            dev = do_with_waiting_for_camera_connection(ctx, dev, serial, [&]()
            {
                REQUIRE_NOTHROW(advanced.toggle_advanced_mode(false));
            });

            disable_sensitive_options_for(*dev);
            advanced = dev->as<rs400::advanced_mode>();

            REQUIRE(!advanced.is_enabled());
        }
    }
}

//////// break up
TEST_CASE("Streaming modes sanity check", "[live]")
{
    // Require at least one device to be plugged in
    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        std::vector<sensor> list;
        REQUIRE_NOTHROW(list = ctx.query_all_sensors());
        REQUIRE(list.size() > 0);

        // For each device
        for (auto&& dev : list)
        {
            disable_sensitive_options_for(dev);

            std::string PID;
            REQUIRE_NOTHROW(PID = dev.get_info(RS2_CAMERA_INFO_PRODUCT_ID));

            // make sure they provide at least one streaming mode
            std::vector<rs2::stream_profile> stream_profiles;
            REQUIRE_NOTHROW(stream_profiles = dev.get_stream_profiles());
            REQUIRE(stream_profiles.size() > 0);

            SECTION("check stream profile settings are sane")
            {
                // for each stream profile provided:
                for (auto profile : stream_profiles) {

                    // require that the settings are sane
                    REQUIRE(profile.format() > RS2_FORMAT_ANY);
                    REQUIRE(profile.format() < RS2_FORMAT_COUNT);
                    REQUIRE(profile.fps() >= 2);
                    REQUIRE(profile.fps() <= 300);

                    if (auto video = profile.as<video_stream_profile>())
                    {
                        REQUIRE(video.width() >= 320);
                        REQUIRE(video.width() <= 1920);
                        REQUIRE(video.height() >= 180);
                        REQUIRE(video.height() <= 1080);
                    }

                    // require that we can start streaming this mode
                    REQUIRE_NOTHROW(dev.open({ profile }));
                    // TODO: make callback confirm stream format/dimensions/framerate
                    REQUIRE_NOTHROW(dev.start([](rs2::frame fref) {}));

                    // Require that we can disable the stream afterwards
                    REQUIRE_NOTHROW(dev.stop());
                    REQUIRE_NOTHROW(dev.close());
                }
            }
            SECTION("check stream intrinsics are sane")
            {
                for (auto profile : stream_profiles)
                {
                    if (auto video = profile.as<video_stream_profile>())
                    {
                        rs2_intrinsics intrin;
                        CAPTURE(video.format());
                        CAPTURE(video.width());
                        CAPTURE(video.height());

                        if ((PID == "0AA5") || (RS2_FORMAT_Y16 != video.format())) // Y16 format intrinsics are available for SR300 only
                        {
                            REQUIRE_NOTHROW(intrin = video.get_intrinsics());

                            // Intrinsic width/height must match width/height of streaming mode we requested
                            REQUIRE(intrin.width == video.width());
                            REQUIRE(intrin.height == video.height());

                            // Principal point must be within center 20% of image
                            REQUIRE(intrin.ppx > video.width() * 0.4f);
                            REQUIRE(intrin.ppx < video.width() * 0.6f);
                            REQUIRE(intrin.ppy > video.height() * 0.4f);
                            REQUIRE(intrin.ppy < video.height() * 0.6f);

                            // Focal length must be non-negative (todo - Refine requirements based on known expected FOV)
                            REQUIRE(intrin.fx > 0.0f);
                            REQUIRE(intrin.fy > 0.0f);
                        }
                        else
                        {
                            REQUIRE_THROWS(intrin = video.get_intrinsics());
                        }
                    }
                }
            }
        }
    }
}

TEST_CASE("Motion profiles sanity", "[live]")
{
    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        std::vector<sensor> list;
        REQUIRE_NOTHROW(list = ctx.query_all_sensors());
        REQUIRE(list.size() > 0);

        // For each device
        for (auto&& dev : list)
        {
            disable_sensitive_options_for(dev);

            // make sure they provide at least one streaming mode
            std::vector<rs2::stream_profile> stream_profiles;
            REQUIRE_NOTHROW(stream_profiles = dev.get_stream_profiles());
            REQUIRE(stream_profiles.size() > 0);

            // for each stream profile provided:
            for (auto profile : stream_profiles)
            {
                SECTION("check motion intrisics") {

                    auto stream = profile.stream_type();
                    rs2_motion_device_intrinsic mm_int;

                    CAPTURE(stream);

                    if (stream != RS2_STREAM_ACCEL && stream != RS2_STREAM_GYRO)
                    {
                        REQUIRE_THROWS(dev.get_motion_intrinsics(stream));
                    }
                    else
                    {
                        REQUIRE_NOTHROW(mm_int = dev.get_motion_intrinsics(stream));

                        for (int j = 0; j < 3; j++)
                        {
                            auto scale = mm_int.data[j][j];
                            CAPTURE(scale);
                            // Make sure scale value is "sane"
                            // We don't expect Motion Device to require adjustment of more then 20%
                            REQUIRE(scale > 0.8);
                            REQUIRE(scale < 1.2);

                            auto bias = mm_int.data[0][3];
                            CAPTURE(bias);
                            // Make sure bias is "sane"
                            REQUIRE(bias > -0.5);
                            REQUIRE(bias < 0.5);
                        }
                    }
                }
            }
        }
    }
}

TEST_CASE("Check width and height of stream intrinsics", "[live][AdvMd]")
{
    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        std::vector<device> devs;
        REQUIRE_NOTHROW(devs = ctx.query_devices());

        for (auto&& dev : devs)
        {
            auto shared_dev = std::make_shared<device>(dev);
            disable_sensitive_options_for(dev);
            std::string serial;
            REQUIRE_NOTHROW(serial = dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));
            if (shared_dev->is<rs400::advanced_mode>())
            {
                auto advanced = shared_dev->as<rs400::advanced_mode>();

                if (advanced.is_enabled())
                {
                    shared_dev = do_with_waiting_for_camera_connection(ctx, shared_dev, serial, [&]()
                    {
                        REQUIRE_NOTHROW(advanced.toggle_advanced_mode(false));
                    });
                }
                disable_sensitive_options_for(*shared_dev);
                advanced = shared_dev->as<rs400::advanced_mode>();

                REQUIRE(advanced.is_enabled() == false);
            }

            std::vector<sensor> list;
            REQUIRE_NOTHROW(list = shared_dev->query_sensors());
            REQUIRE(list.size() > 0);

            for (auto&& dev : list)
            {
                disable_sensitive_options_for(dev);
                auto module_name = dev.get_info(RS2_CAMERA_INFO_NAME);
                // TODO: if FE
                std::vector<rs2::stream_profile> stream_profiles;
                REQUIRE_NOTHROW(stream_profiles = dev.get_stream_profiles());
                REQUIRE(stream_profiles.size() > 0);

                // for each stream profile provided:
                for (auto&& profile : stream_profiles)
                {
                    if (auto video = profile.as<video_stream_profile>())
                    {
                        rs2_intrinsics intrin;


                        CAPTURE(video.width());
                        CAPTURE(video.height());

                        REQUIRE_NOTHROW(intrin = video.get_intrinsics());

                        // Intrinsic width/height must match width/height of streaming mode we requested
                        REQUIRE(intrin.width == video.width());
                        REQUIRE(intrin.height == video.height());
                    }
                }
            }
        }
    }
}

TEST_CASE("Check option API", "[live][options]")
{
    // Require at least one device to be plugged in
    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        std::vector<sensor> list;
        REQUIRE_NOTHROW(list = ctx.query_all_sensors());
        REQUIRE(list.size() > 0);

        // for each device
        for (auto&& dev : list)
        {

            // for each option
            for (auto i = 0; i < RS2_OPTION_COUNT; ++i) {
                auto opt = rs2_option(i);
                bool is_opt_supported;
                REQUIRE_NOTHROW(is_opt_supported = dev.supports(opt));


                SECTION("Ranges are sane")
                {
                    if (!is_opt_supported)
                    {
                        REQUIRE_THROWS_AS(dev.get_option_range(opt), error);
                    }
                    else
                    {
                        rs2::option_range range;
                        REQUIRE_NOTHROW(range = dev.get_option_range(opt));

                        // a couple sanity checks
                        REQUIRE(range.min < range.max);
                        REQUIRE(range.min + range.step <= range.max);
                        REQUIRE(range.step > 0);
                        REQUIRE(range.def <= range.max);
                        REQUIRE(range.min <= range.def);

                        // TODO: check that range.def == range.min + k*range.step for some k?
                        // TODO: some sort of bounds checking against constants?
                    }
                }
                SECTION("get_option returns a legal value")
                {
                    if (!is_opt_supported)
                    {
                        REQUIRE_THROWS_AS(dev.get_option(opt), error);
                    }
                    else
                    {
                        auto range = dev.get_option_range(opt);
                        float value;
                        REQUIRE_NOTHROW(value = dev.get_option(opt));

                        // value in range. Do I need to account for epsilon in lt[e]/gt[e] comparisons?
                        REQUIRE(value >= range.min);
                        REQUIRE(value <= range.max);

                        // value doesn't change between two gets (if no additional threads are calling set)
                        REQUIRE(dev.get_option(opt) == Approx(value));

                        // REQUIRE(value == Approx(range.def)); // Not sure if this is a reasonable check
                        // TODO: make sure value == range.min + k*range.step for some k?
                    }
                }
                SECTION("set opt doesn't like bad values") {
                    if (!is_opt_supported)
                    {
                        REQUIRE_THROWS_AS(dev.set_option(opt, 1), error);
                    }
                    else
                    {
                        auto range = dev.get_option_range(opt);

                        // minimum should work, as should maximum
                        REQUIRE_NOTHROW(dev.set_option(opt, range.min));
                        REQUIRE_NOTHROW(dev.set_option(opt, range.max));

                        int n_steps = int((range.max - range.min) / range.step);

                        // check a few arbitrary points along the scale
                        REQUIRE_NOTHROW(dev.set_option(opt, range.min + (1 % n_steps)*range.step));
                        REQUIRE_NOTHROW(dev.set_option(opt, range.min + (11 % n_steps)*range.step));
                        REQUIRE_NOTHROW(dev.set_option(opt, range.min + (111 % n_steps)*range.step));
                        REQUIRE_NOTHROW(dev.set_option(opt, range.min + (1111 % n_steps)*range.step));

                        // below min and above max shouldn't work
                        REQUIRE_THROWS_AS(dev.set_option(opt, range.min - range.step), error);
                        REQUIRE_THROWS_AS(dev.set_option(opt, range.max + range.step), error);

                        // make sure requesting value in the range, but not a legal step doesn't work
                        // TODO: maybe something for range.step < 1 ?
                        for (auto j = 1; j < range.step; j++) {
                            CAPTURE(range.step);
                            CAPTURE(j);
                            REQUIRE_THROWS_AS(dev.set_option(opt, range.min + j), error);
                        }
                    }
                }
                SECTION("check get/set sequencing works as expected") {
                    if (!is_opt_supported) continue;
                    auto range = dev.get_option_range(opt);

                    // setting a valid value lets you get that value back
                    dev.set_option(opt, range.min);
                    REQUIRE(dev.get_option(opt) == Approx(range.min));

                    // setting an invalid value returns the last set valid value.
                    REQUIRE_THROWS(dev.set_option(opt, range.max + range.step));
                    REQUIRE(dev.get_option(opt) == Approx(range.min));

                    dev.set_option(opt, range.max);
                    REQUIRE_THROWS(dev.set_option(opt, range.min - range.step));
                    REQUIRE(dev.get_option(opt) == Approx(range.max));

                }
                SECTION("get_description returns a non-empty, non-null string") {
                    if (!is_opt_supported) {
                        REQUIRE_THROWS_AS(dev.get_option_description(opt), error);
                    }
                    else
                    {
                        REQUIRE(dev.get_option_description(opt) != nullptr);
                        REQUIRE(std::string(dev.get_option_description(opt)) != std::string(""));
                    }
                }
                // TODO: tests for get_option_value_description? possibly too open a function to have useful tests
            }
        }
    }
}

TEST_CASE("Multiple devices", "[live][multicam]")
{
    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        // Require at least one device to be plugged in
        std::vector<sensor> list;
        REQUIRE_NOTHROW(list = ctx.query_all_sensors());
        REQUIRE(list.size() > 0);

        SECTION("subdevices on a single device")
        {
            for (auto & dev : list)
            {
                disable_sensitive_options_for(dev);

                SECTION("opening the same subdevice multiple times")
                {
                    auto modes = dev.get_stream_profiles();
                    REQUIRE(modes.size() > 0);
                    CAPTURE(modes.front().stream_type());
                    REQUIRE_NOTHROW(dev.open(modes.front()));

                    SECTION("same mode")
                    {
                        // selected, but not streaming
                        REQUIRE_THROWS_AS(dev.open({ modes.front() }), error);

                        // streaming
                        REQUIRE_NOTHROW(dev.start([](rs2::frame fref) {}));
                        REQUIRE_THROWS_AS(dev.open({ modes.front() }), error);
                    }

                    SECTION("different modes")
                    {
                        if (modes.size() == 1)
                        {
                            WARN("device " << dev.get_info(RS2_CAMERA_INFO_NAME) << " S/N: " << dev.get_info(
                                RS2_CAMERA_INFO_SERIAL_NUMBER) << " w/ FW v" << dev.get_info(
                                    RS2_CAMERA_INFO_FIRMWARE_VERSION) << ":");
                            WARN("subdevice has only 1 supported streaming mode. Skipping Same Subdevice, different modes test.");
                        }
                        else
                        {
                            // selected, but not streaming
                            REQUIRE_THROWS_AS(dev.open({ modes[1] }), error);

                            // streaming
                            REQUIRE_NOTHROW(dev.start([](rs2::frame fref) {}));
                            REQUIRE_THROWS_AS(dev.open({ modes[1] }), error);
                        }
                    }

                    REQUIRE_NOTHROW(dev.stop());
                }
                // TODO: Move
                SECTION("opening different subdevices") {
                    for (auto&& subdevice1 : ctx.get_sensor_parent(dev).query_sensors())
                    {
                        disable_sensitive_options_for(subdevice1);
                        for (auto&& subdevice2 : ctx.get_sensor_parent(dev).query_sensors())
                        {
                            disable_sensitive_options_for(subdevice2);

                            if (subdevice1 == subdevice2)
                                continue;

                            // get first lock
                            REQUIRE_NOTHROW(subdevice1.open(subdevice1.get_stream_profiles().front()));

                            // selected, but not streaming
                            {
                                CAPTURE(subdevice2.get_stream_profiles().front().stream_type());
                                REQUIRE_NOTHROW(subdevice2.open(subdevice2.get_stream_profiles().front()));
                                REQUIRE_NOTHROW(subdevice2.start([](rs2::frame fref) {}));
                                REQUIRE_NOTHROW(subdevice2.stop());
                                REQUIRE_NOTHROW(subdevice2.close());
                            }

                            // streaming
                            {
                                REQUIRE_NOTHROW(subdevice1.start([](rs2::frame fref) {}));
                                REQUIRE_NOTHROW(subdevice2.open(subdevice2.get_stream_profiles().front()));
                                REQUIRE_NOTHROW(subdevice2.start([](rs2::frame fref) {}));
                                // stop streaming in opposite order just to be sure that works too
                                REQUIRE_NOTHROW(subdevice1.stop());
                                REQUIRE_NOTHROW(subdevice2.stop());
                                REQUIRE_NOTHROW(subdevice2.close());
                            }

                            REQUIRE_NOTHROW(subdevice1.close());
                        }
                    }
                }
            }
        }
        SECTION("multiple devices")
        {
            if (list.size() == 1)
            {
                WARN("Only one device connected. Skipping multi-device test");
            }
            else
            {
                for (auto & dev1 : list)
                {
                    disable_sensitive_options_for(dev1);
                    for (auto & dev2 : list)
                    {
                        disable_sensitive_options_for(dev2);

                        // couldn't think of a better way to compare the two...
                        if (dev1 == dev2)
                            continue;

                        REQUIRE_NOTHROW(dev1.open(dev1.get_stream_profiles().front()));
                        REQUIRE_NOTHROW(dev2.open(dev2.get_stream_profiles().front()));

                        REQUIRE_NOTHROW(dev1.start([](rs2::frame fref) {}));
                        REQUIRE_NOTHROW(dev2.start([](rs2::frame fref) {}));
                        REQUIRE_NOTHROW(dev1.stop());
                        REQUIRE_NOTHROW(dev2.stop());

                        REQUIRE_NOTHROW(dev1.close());
                        REQUIRE_NOTHROW(dev2.close());
                    }
                }
            }
        }
    }
}

// On Windows 10 RS2 there is an anusual behaviour failingt this test:
// When trying to enable the second instance of Source Reader, instead of failing, the Media Foundation allows it
// and sends an HR to the first Source Reader instead (something about the application being preempted)
TEST_CASE("Multiple applications", "[live][multicam][!mayfail]")
{
    rs2::context ctx1;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx1))
    {
        // Require at least one device to be plugged in
        std::vector<sensor> list1;
        REQUIRE_NOTHROW(list1 = ctx1.query_all_sensors());
        REQUIRE(list1.size() > 0);

        rs2::context ctx2;
        REQUIRE(make_context("two_contexts", &ctx2));
        std::vector<sensor> list2;
        REQUIRE_NOTHROW(list2 = ctx2.query_all_sensors());
        REQUIRE(list2.size() == list1.size());
        SECTION("subdevices on a single device")
        {
            for (auto&& dev1 : list1)
            {
                disable_sensitive_options_for(dev1);
                for (auto&& dev2 : list2)
                {
                    disable_sensitive_options_for(dev2);

                    if (dev1 == dev2)
                    {
                        SECTION("same subdevice") {
                            // get modes
                            std::vector<rs2::stream_profile> modes1, modes2;
                            REQUIRE_NOTHROW(modes1 = dev1.get_stream_profiles());
                            REQUIRE_NOTHROW(modes2 = dev2.get_stream_profiles());
                            REQUIRE(modes1.size() > 0);
                            REQUIRE(modes1.size() == modes2.size());
                            // require that the lists are the same (disregarding order)
                            for (auto profile : modes1) {
                                REQUIRE(std::any_of(begin(modes2), end(modes2), [&profile](const rs2::stream_profile & p)
                                {
                                    return profile == p;
                                }));
                            }

                            // grab first lock
                            CAPTURE(modes1.front().stream_name());
                            REQUIRE_NOTHROW(dev1.open(modes1.front()));

                            SECTION("same mode")
                            {
                                // selected, but not streaming
                                REQUIRE_THROWS_AS(dev2.open({ modes1.front() }), error);

                                // streaming
                                REQUIRE_NOTHROW(dev1.start([](rs2::frame fref) {}));
                                REQUIRE_THROWS_AS(dev2.open({ modes1.front() }), error);
                            }
                            SECTION("different modes")
                            {
                                if (modes1.size() == 1)
                                {
                                    WARN("device " << dev1.get_info(RS2_CAMERA_INFO_NAME) << " S/N: " << dev1.get_info(
                                        RS2_CAMERA_INFO_SERIAL_NUMBER) << " w/ FW v" << dev1.get_info(
                                            RS2_CAMERA_INFO_FIRMWARE_VERSION) << ":");
                                    WARN("Device has only 1 supported streaming mode. Skipping Same Subdevice, different modes test.");
                                }
                                else
                                {
                                    // selected, but not streaming
                                    REQUIRE_THROWS_AS(dev2.open({ modes1[1] }), error);

                                    // streaming
                                    REQUIRE_NOTHROW(dev1.start([](rs2::frame fref) {}));
                                    REQUIRE_THROWS_AS(dev2.open({ modes1[1] }), error);
                                }
                            }
                            REQUIRE_NOTHROW(dev1.stop());
                        }
                    }
                    else
                    {
                        SECTION("different subdevice")
                        {
                            // get first lock
                            REQUIRE_NOTHROW(dev1.open(dev1.get_stream_profiles().front()));

                            // selected, but not streaming
                            {
                                CAPTURE(dev2.get_stream_profiles().front().stream_type());
                                REQUIRE_NOTHROW(dev2.open(dev2.get_stream_profiles().front()));
                                REQUIRE_NOTHROW(dev2.start([](rs2::frame fref) {}));
                                REQUIRE_NOTHROW(dev2.stop());
                                REQUIRE_NOTHROW(dev2.close());
                            }

                            // streaming
                            {
                                REQUIRE_NOTHROW(dev1.start([](rs2::frame fref) {}));
                                REQUIRE_NOTHROW(dev2.open(dev2.get_stream_profiles().front()));
                                REQUIRE_NOTHROW(dev2.start([](rs2::frame fref) {}));
                                // stop streaming in opposite order just to be sure that works too
                                REQUIRE_NOTHROW(dev1.stop());
                                REQUIRE_NOTHROW(dev2.stop());

                                REQUIRE_NOTHROW(dev1.close());
                                REQUIRE_NOTHROW(dev2.close());
                            }
                        }
                    }
                }
            }
        }
        SECTION("subdevices on separate devices")
        {
            if (list1.size() == 1)
            {
                WARN("Only one device connected. Skipping multi-device test");
            }
            else
            {
                for (auto & dev1 : list1)
                {
                    disable_sensitive_options_for(dev1);
                    for (auto & dev2 : list2)
                    {
                        disable_sensitive_options_for(dev2);

                        if (dev1 == dev2)
                            continue;

                        // get modes
                        std::vector<rs2::stream_profile> modes1, modes2;
                        REQUIRE_NOTHROW(modes1 = dev1.get_stream_profiles());
                        REQUIRE_NOTHROW(modes2 = dev2.get_stream_profiles());
                        REQUIRE(modes1.size() > 0);
                        REQUIRE(modes2.size() > 0);

                        // grab first lock
                        CAPTURE(modes1.front().stream_type());
                        CAPTURE(dev1.get_info(RS2_CAMERA_INFO_NAME));
                        CAPTURE(dev1.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));
                        CAPTURE(dev2.get_info(RS2_CAMERA_INFO_NAME));
                        CAPTURE(dev2.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));
                        REQUIRE_NOTHROW(dev1.open(modes1.front()));

                        // try to acquire second lock

                        // selected, but not streaming
                        {
                            REQUIRE_NOTHROW(dev2.open({ modes2.front() }));
                            REQUIRE_NOTHROW(dev2.start([](rs2::frame fref) {}));
                            REQUIRE_NOTHROW(dev2.stop());
                            REQUIRE_NOTHROW(dev2.close());
                        }

                        // streaming
                        {
                            REQUIRE_NOTHROW(dev1.start([](rs2::frame fref) {}));
                            REQUIRE_NOTHROW(dev2.open({ modes2.front() }));
                            REQUIRE_NOTHROW(dev2.start([](rs2::frame fref) {}));
                            // stop streaming in opposite order just to be sure that works too
                            REQUIRE_NOTHROW(dev1.stop());
                            REQUIRE_NOTHROW(dev2.stop());
                            REQUIRE_NOTHROW(dev2.close());
                        }

                        REQUIRE_NOTHROW(dev1.close());
                    }
                }
            }
        }
    }
}


//
///* Apply heuristic test to check metadata attributes for sanity*/
void metadata_verification(const std::vector<internal_frame_additional_data>& data)
{
    // Heuristics that we use to verify metadata
    // Metadata sanity
    // Frame numbers and timestamps increase monotonically
    // Sensor timestamp should be less or equal to frame timestamp
    // Exposure time and gain values are greater than zero
    // Sensor framerate is bounded to >0 and < 200 fps for uvc streams
    int64_t last_val[3] = { -1, -1, -1 };

    for (size_t i = 0; i < data.size(); i++)
    {

        // Check that Frame/Sensor timetamps, frame number rise monotonically
        for (int j = RS2_FRAME_METADATA_FRAME_COUNTER; j <= RS2_FRAME_METADATA_SENSOR_TIMESTAMP; j++)
        {
            if (data[i].frame_md.md_attributes[j].first)
            {
                int64_t value = data[i].frame_md.md_attributes[j].second;
                CAPTURE(value);
                CAPTURE(last_val[j]);

                REQUIRE_NOTHROW((value > last_val[0]));
                if (RS2_FRAME_METADATA_FRAME_COUNTER == j) // In addition, there shall be no frame number gaps
                {
                    REQUIRE_NOTHROW((1 == (value - last_val[j])));
                }

                last_val[j] = data[i].frame_md.md_attributes[j].second;
            }
        }

        //        // Exposure time and gain values are greater than zero
        //        if (data[i].frame_md.md_attributes[RS2_FRAME_METADATA_ACTUAL_EXPOSURE].first)
        //            REQUIRE(data[i].frame_md.md_attributes[RS2_FRAME_METADATA_ACTUAL_EXPOSURE].second > 0);
        //        if (data[i].frame_md.md_attributes[RS2_FRAME_METADATA_GAIN_LEVEL].first)
        //            REQUIRE(data[i].frame_md.md_attributes[RS2_FRAME_METADATA_GAIN_LEVEL].second > 0);
    }
}


////serialize_json
void triger_error(const rs2::device& dev, int num)
{
    std::vector<uint8_t> raw_data(24, 0);
    raw_data[0] = 0x14;
    raw_data[2] = 0xab;
    raw_data[3] = 0xcd;
    raw_data[4] = 0x4d;
    raw_data[8] = num;
    if (auto debug = dev.as<debug_protocol>())
        debug.send_and_receive_raw_data(raw_data);
}

TEST_CASE("Error handling sanity", "[live][!mayfail]") {

    //Require at least one device to be plugged in
    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        const int num_of_errors = 4;

        std::vector<sensor> list;
        REQUIRE_NOTHROW(list = ctx.query_all_sensors());
        REQUIRE(list.size() > 0);

        std::string notification_description;
        rs2_log_severity severity;
        std::condition_variable cv;
        std::mutex m;

        std::map<int, std::string> notification_descriptions;

        notification_descriptions[1] = "Hot laser power reduce";
        notification_descriptions[2] = "Hot laser disable";
        notification_descriptions[3] = "Flag B laser disable";

        //enable error polling
        for (auto && subdevice : list) {
            // disable_sensitive_options_for(subdevice);

            for (auto i = 1; i < num_of_errors; i++)
            {
                if (subdevice.supports(RS2_OPTION_ERROR_POLLING_ENABLED))
                {
                    disable_sensitive_options_for(subdevice);

                    subdevice.set_notifications_callback([&](rs2::notification n)
                    {

                        std::unique_lock<std::mutex> lock(m);

                        notification_description = n.get_description();
                        severity = n.get_severity();
                        cv.notify_one();
                    });

                    triger_error(ctx.get_sensor_parent(subdevice), i);
                    REQUIRE_NOTHROW(subdevice.set_option(RS2_OPTION_ERROR_POLLING_ENABLED, 1));
                    std::unique_lock<std::mutex> lock(m);
                    CAPTURE(notification_description);
                    CAPTURE(severity);
                    CAPTURE(i);
                    auto pred = [&]() {
                        return notification_description.compare(notification_descriptions[i]) == 0
                            && severity == RS2_LOG_SEVERITY_ERROR;
                    };
                    REQUIRE(cv.wait_for(lock, std::chrono::seconds(10), pred));
                    REQUIRE_NOTHROW(subdevice.set_option(RS2_OPTION_ERROR_POLLING_ENABLED, 0));

                }
            }
        }


        //disable error polling

        auto got_error = false;
        for (auto && subdevice : list) {
            // disable_sensitive_options_for(subdevice);

            for (auto i = 1; i < 4; i++)
            {
                if (subdevice.supports(RS2_OPTION_ERROR_POLLING_ENABLED))
                {
                    disable_sensitive_options_for(subdevice);
                    REQUIRE_NOTHROW(subdevice.set_option(RS2_OPTION_ERROR_POLLING_ENABLED, 0));
                    subdevice.set_notifications_callback([&](rs2::notification n)
                    {
                        got_error = true;
                        notification_description = n.get_description();

                    });

                    triger_error(ctx.get_sensor_parent(subdevice), i);
                    std::this_thread::sleep_for(std::chrono::seconds(2));
                    CAPTURE(notification_description);
                    REQUIRE(got_error == false);

                }
            }
        }
    }
}

TEST_CASE("Auto disabling control behavior", "[live]") {
    //Require at least one device to be plugged in
    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        std::vector<sensor> list;
        REQUIRE_NOTHROW(list = ctx.query_all_sensors());
        REQUIRE(list.size() > 0);

        for (auto && subdevice : list)
        {
            disable_sensitive_options_for(subdevice);
            auto info = subdevice.get_info(RS2_CAMERA_INFO_NAME);
            CAPTURE(info);

            rs2::option_range range{};
            float val{};
            if (subdevice.supports(RS2_OPTION_ENABLE_AUTO_EXPOSURE))
            {
                SECTION("Disable auto exposure when setting a value")
                {
                    REQUIRE_NOTHROW(subdevice.set_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE, 1));
                    REQUIRE_NOTHROW(range = subdevice.get_option_range(RS2_OPTION_EXPOSURE));
                    REQUIRE_NOTHROW(subdevice.set_option(RS2_OPTION_EXPOSURE, range.max));
                    CAPTURE(range.max);
                    REQUIRE_NOTHROW(val = subdevice.get_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE));
                    REQUIRE(val == 0);
                }
            }

            if (subdevice.supports(RS2_OPTION_EMITTER_ENABLED))
            {
                SECTION("Disable emitter when setting a value")
                {
                    for (auto elem : { 0.f, 2.f })
                    {
                        REQUIRE_NOTHROW(subdevice.set_option(RS2_OPTION_EMITTER_ENABLED, elem));
                        REQUIRE_NOTHROW(range = subdevice.get_option_range(RS2_OPTION_LASER_POWER));
                        REQUIRE_NOTHROW(subdevice.set_option(RS2_OPTION_LASER_POWER, range.max));
                        CAPTURE(range.max);
                        REQUIRE_NOTHROW(val = subdevice.get_option(RS2_OPTION_EMITTER_ENABLED));
                        REQUIRE(val == 1);
                    }
                }
            }

            if (subdevice.supports(RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE)) // TODO: Add auto-disabling to SR300 options
            {
                SECTION("Disable white balance when setting a value")
                {
                    if (subdevice.supports(RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE) && subdevice.supports(RS2_OPTION_WHITE_BALANCE))
                    {

                        REQUIRE_NOTHROW(subdevice.set_option(RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE, 1));
                        REQUIRE_NOTHROW(range = subdevice.get_option_range(RS2_OPTION_WHITE_BALANCE));
                        REQUIRE_NOTHROW(subdevice.set_option(RS2_OPTION_WHITE_BALANCE, range.max));
                        CAPTURE(range.max);
                        REQUIRE_NOTHROW(val = subdevice.get_option(RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE));
                        REQUIRE(val == 0);
                    }
                }
            }
        }
    }
}


std::pair<std::shared_ptr<rs2::device>, std::weak_ptr<rs2::device>> make_device(device_list& list)
{
    REQUIRE(list.size() > 0);

    std::shared_ptr<rs2::device> dev;
    REQUIRE_NOTHROW(dev = std::make_shared<rs2::device>(list[0]));
    std::weak_ptr<rs2::device> weak_dev(dev);

    disable_sensitive_options_for(*dev);

    return std::pair<std::shared_ptr<rs2::device>, std::weak_ptr<rs2::device>>(dev, weak_dev);
}

void reset_device(std::shared_ptr<rs2::device>& strong, std::weak_ptr<rs2::device>& weak, device_list& list, const rs2::device& new_dev)
{
    strong.reset();
    weak.reset();
    list = nullptr;
    strong = std::make_shared<rs2::device>(new_dev);
    weak = strong;
    disable_sensitive_options_for(*strong);
}

TEST_CASE("Disconnect events works", "[live]") {

    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        device_list list;
        REQUIRE_NOTHROW(list = ctx.query_devices());

        auto dev = make_device(list);
        auto dev_strong = dev.first;
        auto dev_weak = dev.second;


        auto disconnected = false;
        auto connected = false;

        std::string serial;

        REQUIRE_NOTHROW(serial = dev_strong->get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));

        std::condition_variable cv;
        std::mutex m;

        //Setting up devices change callback to notify the test about device disconnection
        REQUIRE_NOTHROW(ctx.set_devices_changed_callback([&, dev_weak](event_information& info) mutable
        {
            auto&& strong = dev_weak.lock();
            {
                if (strong)
                {
                    if (info.was_removed(*strong))
                    {
                        std::unique_lock<std::mutex> lock(m);
                        disconnected = true;
                        cv.notify_one();
                    }


                    for (auto d : info.get_new_devices())
                    {
                        for (auto&& s : d.query_sensors())
                            disable_sensitive_options_for(s);

                        if (serial == d.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER))
                        {
                            try
                            {
                                std::unique_lock<std::mutex> lock(m);
                                connected = true;
                                cv.notify_one();
                                break;
                            }
                            catch (...)
                            {

                            }
                        }
                    }
                }

            }}));

        //forcing hardware reset to simulate device disconnection
        do_with_waiting_for_camera_connection(ctx, dev_strong, serial, [&]()
        {
            dev_strong->hardware_reset();
        });

        //Check that after the library reported device disconnection, operations on old device object will return error
        REQUIRE_THROWS(dev_strong->query_sensors().front().close());
    }
}

TEST_CASE("Connect events works", "[live]") {


    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        device_list list;
        REQUIRE_NOTHROW(list = ctx.query_devices());

        auto dev = make_device(list);
        auto dev_strong = dev.first;
        auto dev_weak = dev.second;

        std::string serial;

        REQUIRE_NOTHROW(serial = dev_strong->get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));

        auto disconnected = false;
        auto connected = false;
        std::condition_variable cv;
        std::mutex m;

        //Setting up devices change callback to notify the test about device disconnection and connection
        REQUIRE_NOTHROW(ctx.set_devices_changed_callback([&, dev_weak](event_information& info) mutable
        {
            auto&& strong = dev_weak.lock();
            {
                if (strong)
                {
                    if (info.was_removed(*strong))
                    {
                        std::unique_lock<std::mutex> lock(m);
                        disconnected = true;
                        cv.notify_one();
                    }


                    for (auto d : info.get_new_devices())
                    {
                        if (serial == d.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER))
                        {
                            try
                            {
                                std::unique_lock<std::mutex> lock(m);

                                reset_device(dev_strong, dev_weak, list, d);

                                connected = true;
                                cv.notify_one();
                                break;
                            }
                            catch (...)
                            {

                            }
                        }
                    }
                }

            }}));

        //forcing hardware reset to simulate device disconnection
        do_with_waiting_for_camera_connection(ctx, dev_strong, serial, [&]()
        {
            dev_strong->hardware_reset();
        });
    }
}

std::shared_ptr<std::function<void(rs2::frame fref)>> check_stream_sanity(const context& ctx, const sensor& sub, int num_of_frames, bool infinite = false)
{
    std::shared_ptr<std::condition_variable> cv = std::make_shared<std::condition_variable>();
    std::shared_ptr<std::mutex> m = std::make_shared<std::mutex>();
    std::shared_ptr<std::map<rs2_stream, int>> streams_frames = std::make_shared<std::map<rs2_stream, int>>();

    std::shared_ptr<std::function<void(rs2::frame fref)>>  func;

    std::vector<rs2::stream_profile> modes;
    REQUIRE_NOTHROW(modes = sub.get_stream_profiles());

    auto streaming = false;

    for (auto p : modes)
    {
        if (auto video = p.as<video_stream_profile>())
        {
            if (video.width() == 640 && video.height() == 480 && video.fps() == 60 && video.format())
            {
                if ((video.stream_type() == RS2_STREAM_DEPTH && video.format() == RS2_FORMAT_Z16) ||
                    (video.stream_type() == RS2_STREAM_FISHEYE && video.format() == RS2_FORMAT_RAW8))
                {
                    streaming = true;
                    (*streams_frames)[p.stream_type()] = 0;

                    REQUIRE_NOTHROW(sub.open(p));

                    func = std::make_shared< std::function<void(frame fref)>>([num_of_frames, m, streams_frames, cv](frame fref) mutable
                    {
                        std::unique_lock<std::mutex> lock(*m);
                        auto stream = fref.get_profile().stream_type();
                        streams_frames->at(stream)++;
                        if (streams_frames->at(stream) >= num_of_frames)
                            cv->notify_one();

                    });
                    REQUIRE_NOTHROW(sub.start(*func));
                }
            }
        }
    }


    std::unique_lock<std::mutex> lock(*m);
    cv->wait_for(lock, std::chrono::seconds(30), [&]
    {
        for (auto f : (*streams_frames))
        {
            if (f.second < num_of_frames)
                return false;
        }
        return true;
    });

    if (!infinite && streaming)
    {
        REQUIRE_NOTHROW(sub.stop());
        REQUIRE_NOTHROW(sub.close());

    }

    return func;
}

TEST_CASE("Connect Disconnect events while streaming", "[live]") {
    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        device_list list;
        REQUIRE_NOTHROW(list = ctx.query_devices());

        std::string serial;

        auto dev = make_device(list);
        auto dev_strong = dev.first;
        auto dev_weak = dev.second;


        REQUIRE_NOTHROW(serial = dev_strong->get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));


        auto disconnected = false;
        auto connected = false;
        std::condition_variable cv;
        std::mutex m;

        //Setting up devices change callback to notify the test about device disconnection and connection
        REQUIRE_NOTHROW(ctx.set_devices_changed_callback([&, dev_weak](event_information& info) mutable
        {
            auto&& strong = dev_weak.lock();
            {
                if (strong)
                {
                    if (info.was_removed(*strong))
                    {
                        std::unique_lock<std::mutex> lock(m);
                        disconnected = true;
                        cv.notify_one();
                    }


                    for (auto d : info.get_new_devices())
                    {
                        if (serial == d.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER))
                        {
                            try
                            {
                                std::unique_lock<std::mutex> lock(m);

                                reset_device(dev_strong, dev_weak, list, d);

                                connected = true;
                                cv.notify_one();
                                break;
                            }
                            catch (...)
                            {

                            }
                        }
                    }
                }

            }}));

        for (auto&& s : dev_strong->query_sensors())
            auto func = check_stream_sanity(ctx, s, 1, true);

        for (auto i = 0; i < 3; i++)
        {
            //forcing hardware reset to simulate device disconnection
            dev_strong = do_with_waiting_for_camera_connection(ctx, dev_strong, serial, [&]()
            {
                dev_strong->hardware_reset();
            });

            for (auto&& s : dev_strong->query_sensors())
                auto func = check_stream_sanity(ctx, s, 10);

            disconnected = connected = false;
        }

    }
}

void check_controls_sanity(const context& ctx, const sensor& dev)
{
    for (auto d : ctx.get_sensor_parent(dev).query_sensors())
    {
        for (auto i = 0; i < RS2_OPTION_COUNT; i++)
        {
            if (d.supports((rs2_option)i))
                REQUIRE_NOTHROW(d.get_option((rs2_option)i));
        }
    }
}
//
TEST_CASE("Connect Disconnect events while controls", "[live]")
{
    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        device_list list;
        REQUIRE_NOTHROW(list = ctx.query_devices());

        auto dev = make_device(list);
        auto dev_strong = dev.first;
        auto dev_weak = dev.second;

        std::string serial;

        REQUIRE_NOTHROW(serial = dev_strong->get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));


        auto disconnected = false;
        auto connected = false;
        std::condition_variable cv;
        std::mutex m;

        //Setting up devices change callback to notify the test about device disconnection and connection
        REQUIRE_NOTHROW(ctx.set_devices_changed_callback([&, dev_weak](event_information& info) mutable
        {
            auto&& strong = dev_weak.lock();
            {
                if (strong)
                {
                    if (info.was_removed(*strong))
                    {
                        std::unique_lock<std::mutex> lock(m);
                        disconnected = true;
                        cv.notify_one();
                    }

                    for (auto d : info.get_new_devices())
                    {
                        if (serial == d.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER))
                        {
                            try
                            {
                                std::unique_lock<std::mutex> lock(m);

                                reset_device(dev_strong, dev_weak, list, d);
                                connected = true;
                                cv.notify_one();
                                break;
                            }
                            catch (...)
                            {

                            }
                        }
                    }
                }

            }}));

        //forcing hardware reset to simulate device disconnection
        dev_strong = do_with_waiting_for_camera_connection(ctx, dev_strong, serial, [&]()
        {
            dev_strong->hardware_reset();
        });

        for (auto&& s : dev_strong->query_sensors())
            check_controls_sanity(ctx, s);
    }

}

TEST_CASE("Basic device_hub flow", "[live][!mayfail]") {

    rs2::context ctx;

    std::shared_ptr<rs2::device> dev;

    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        device_hub hub(ctx);
        REQUIRE_NOTHROW(dev = std::make_shared<rs2::device>(hub.wait_for_device()));

        std::weak_ptr<rs2::device> weak(dev);

        disable_sensitive_options_for(*dev);

        dev->hardware_reset();
        int i = 300;
        while (i > 0 && hub.is_connected(*dev))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            --i;
        }
        if (i == 0)
        {
            WARN("Reset workaround");
            dev->hardware_reset();
            while (hub.is_connected(*dev))
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        // Don't exit the test in unknown state
        REQUIRE_NOTHROW(hub.wait_for_device());
    }
}


struct stream_format
{
    rs2_stream stream_type;
    int width;
    int height;
    int fps;
    rs2_format format;
    int index;
};

TEST_CASE("Auto-complete feature works", "[offline][util::config]") {
    // dummy device can provide the following profiles:
    rs2::context ctx;

    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        struct Test {
            std::vector<stream_format> given;      // We give these profiles to the config class
            std::vector<stream_format> expected;    // pool of profiles the config class can return. Leave empty if auto-completer is expected to fail
        };
        std::vector<Test> tests = {
            // Test 0 (Depth always has RS2_FORMAT_Z16)
           { { { RS2_STREAM_DEPTH   ,   0,   0,   0, RS2_FORMAT_ANY, 0 } },   // given
             { { RS2_STREAM_DEPTH   ,   0,   0,   0, RS2_FORMAT_Z16, 0 } } }, // expected
            // Test 1 (IR always has RS2_FORMAT_Y8)
           { { { RS2_STREAM_INFRARED,   0,   0,   0, RS2_FORMAT_ANY, 1 } },   // given
             { { RS2_STREAM_INFRARED,   0,   0,   0, RS2_FORMAT_Y8 , 1 } } }, // expected
            // Test 2 (No 200 fps depth)
           { { { RS2_STREAM_DEPTH   ,   0,   0, 200, RS2_FORMAT_ANY, 0 } },   // given
             { } },                                                      // expected
            // Test 3 (Can request 60 fps IR)
           { { { RS2_STREAM_INFRARED,   0,   0, 60, RS2_FORMAT_ANY, 1 } },   // given
             { { RS2_STREAM_INFRARED,   0,   0, 60, RS2_FORMAT_ANY, 1 } } }, // expected
            // Test 4 (requesting IR@200fps + depth fails
           { { { RS2_STREAM_INFRARED,   0,   0, 200, RS2_FORMAT_ANY, 1 }, { RS2_STREAM_DEPTH   ,   0,   0,   0, RS2_FORMAT_ANY, 0 } },   // given
             { } },                                                                                                            // expected
            // Test 5 (Can't do 640x480@110fps a)
           { { { RS2_STREAM_INFRARED, 640, 480, 110, RS2_FORMAT_ANY, 1 } },   // given
             { } },                                                      // expected
            // Test 6 (Can't do 640x480@110fps b)
           { { { RS2_STREAM_DEPTH   , 640, 480,   0, RS2_FORMAT_ANY, 0 }, { RS2_STREAM_INFRARED,   0,   0, 110, RS2_FORMAT_ANY, 1 } },   // given
             { } },                                                                                                            // expected
            // Test 7 (Pull extra details from second stream a)
           { { { RS2_STREAM_DEPTH   , 640, 480,   0, RS2_FORMAT_ANY, 0 }, { RS2_STREAM_INFRARED,   0,   0,  30, RS2_FORMAT_ANY, 1 } },   // given
             { { RS2_STREAM_DEPTH   , 640, 480,  30, RS2_FORMAT_ANY, 0 }, { RS2_STREAM_INFRARED, 640, 480,  30, RS2_FORMAT_ANY, 1 } } }, // expected
            // Test 8 (Pull extra details from second stream b) [IR also supports 200, could fail if that gets selected]
           { { { RS2_STREAM_INFRARED, 640, 480,   0, RS2_FORMAT_ANY, 1 }, { RS2_STREAM_DEPTH   ,   0,   0,   0, RS2_FORMAT_ANY , 0 } },   // given
             { { RS2_STREAM_INFRARED, 640, 480,  10, RS2_FORMAT_ANY, 1 }, { RS2_STREAM_INFRARED, 640, 480,  30, RS2_FORMAT_ANY , 1 },     // expected - options for IR stream
               { RS2_STREAM_DEPTH   , 640, 480,  10, RS2_FORMAT_ANY, 0 }, { RS2_STREAM_DEPTH   , 640, 480,  30, RS2_FORMAT_ANY , 0 } } }  // expected - options for depth stream

        };

        pipeline pipe(ctx);
        for (int i = 0; i < tests.size(); ++i)
        {
            pipe.disable_all();
            for (auto & profile : tests[i].given) {
                REQUIRE_NOTHROW(pipe.enable_stream(profile.stream_type, profile.index, profile.width, profile.height, profile.format, profile.fps));
            }

            CAPTURE(i);
            if (tests[i].expected.size() == 0) {
                REQUIRE_THROWS_AS(pipe.start(), std::runtime_error);
            }
            else
            {
                REQUIRE_NOTHROW(pipe.start());
                //REQUIRE()s are in here

                REQUIRE_NOTHROW(pipe.stop());
            }
        }
    }
}




//TODO: make it work
//TEST_CASE("Sync connect disconnect", "[live]") {
//    rs2::context ctx;
//
//    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
//    {
//        auto list = ctx.query_devices();
//        REQUIRE(list.size());
//        pipeline pipe(ctx);
//        auto dev = pipe.get_device();
//
//        disable_sensitive_options_for(dev);
//
//
//        auto profiles = configure_all_supported_streams(dev, dev);
//
//       
//        pipe.start();
//
//        std::string serial;
//        REQUIRE_NOTHROW(serial = dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));
//
//        auto disconnected = false;
//        auto connected = false;
//        std::condition_variable cv;
//        std::mutex m;
//
//        //Setting up devices change callback to notify the test about device disconnection and connection
//        REQUIRE_NOTHROW(ctx.set_devices_changed_callback([&](event_information& info) mutable
//        {
//            std::unique_lock<std::mutex> lock(m);
//            if (info.was_removed(dev))
//            {
//
//                try {
//                    pipe.stop();
//                    pipe.disable_all();
//                    dev = nullptr;
//                }
//                catch (...) {};
//                disconnected = true;
//                cv.notify_one();
//            }
//
//            auto devs = info.get_new_devices();
//            if (devs.size() > 0)
//            {
//                dev = pipe.get_device();
//                std::string new_serial;
//                REQUIRE_NOTHROW(new_serial = dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));
//                if (serial == new_serial)
//                {
//                    disable_sensitive_options_for(dev);
//
//                    auto profiles = configure_all_supported_streams(dev, pipe);
//
//                    pipe.start();
//
//                    connected = true;
//                    cv.notify_one();
//                }
//            }
//
//        }));
//
//
//        for (auto i = 0; i < 5; i++)
//        {
//            auto frames = pipe.wait_for_frames(10000);
//            REQUIRE(frames.size() > 0);
//        }
//
//        {
//            std::unique_lock<std::mutex> lock(m);
//            disconnected = connected = false;
//            auto shared_dev = std::make_shared<rs2::device>(dev);
//            dev.hardware_reset();
//
//            REQUIRE(wait_for_reset([&]() {
//                return cv.wait_for(lock, std::chrono::seconds(20), [&]() {return disconnected; });
//            }, shared_dev));
//            REQUIRE(cv.wait_for(lock, std::chrono::seconds(20), [&]() {return connected; }));
//        }
//
//        for (auto i = 0; i < 5; i++)
//        {
//            auto frames = pipe.wait_for_frames(10000);
//            REQUIRE(frames.size() > 0);
//
//        }
//    }
//}





void validate(std::vector<std::vector<stream_profile>> frames, std::vector<std::vector<double>> timestamps, device_profiles requsets)
{
    REQUIRE(frames.size() > 0);

    int successful = 0;

    auto gap = 1000 / requsets.fps;

    auto ts = 0;

    for (auto i = 0; i < frames.size(); i++)
    {
        auto frame = frames[i];
        auto ts = timestamps[i];
        if (frame.size() == 0)
            continue;


        std::vector<profile> stream_arrived;

        for (auto f : frame)
        {
            auto image = f.as<rs2::video_stream_profile>();

            stream_arrived.push_back({ image.stream_type(), image.format(), image.width(), image.height() });
            REQUIRE(image.fps());
        }

        std::sort(ts.begin(), ts.end());

        if (ts[ts.size() - 1] - ts[0] > gap / 2)
        {
            continue;
        }

        if (stream_arrived.size() != requsets.streams.size())
            continue;

        std::sort(stream_arrived.begin(), stream_arrived.end());
        std::sort(requsets.streams.begin(), requsets.streams.end());

        auto equals = true;
        for (auto i = 0; i < requsets.streams.size(); i++)
        {
            if (stream_arrived[i] != requsets.streams[i])
            {
                equals = false;
                break;
            }

        }
        if (!equals)
            continue;

        successful++;

    }
    REQUIRE(successful > 0);
}

std::map<std::string, device_profiles> get_pipeline_default_configurations()
{
    std::map<std::string, device_profiles> dev_requsets;

    dev_requsets["0B07"] =
    {
        { { RS2_STREAM_DEPTH, RS2_FORMAT_Z16, 1280, 720, 0 },{ RS2_STREAM_COLOR, RS2_FORMAT_RGB8, 1280, 720, 0 } },
        30,
        true
    };

    dev_requsets["0AA5"] =
    {

        { { RS2_STREAM_DEPTH, RS2_FORMAT_Z16, 640, 480, 0 },{ RS2_STREAM_COLOR, RS2_FORMAT_RGB8, 1920, 1080, 0 } },
        30,
        true
    };

    dev_requsets["0AD4"] =
    {
        { { RS2_STREAM_DEPTH, RS2_FORMAT_Z16, 1280, 720, 0 } },
        30,
        true
    };
    dev_requsets["0AD1"] =
    {
        { { RS2_STREAM_DEPTH, RS2_FORMAT_Z16, 1280, 720, 0 },{ RS2_STREAM_INFRARED, RS2_FORMAT_RGB8, 1280, 720, 0 } },
        30,
        true
    };
    return dev_requsets;
}

TEST_CASE("Pipeline wait_for_frames", "[live]") {
    auto dev_requsets = get_pipeline_default_configurations();

    rs2::context ctx;

    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        auto list = ctx.query_devices();
        REQUIRE(list.size());
        rs2::device dev;
        rs2::pipeline pipe(ctx);
        REQUIRE_NOTHROW(dev = list[0]);

        disable_sensitive_options_for(dev);
        std::string PID;
        REQUIRE_NOTHROW(PID = dev.get_info(RS2_CAMERA_INFO_PRODUCT_ID));
        REQUIRE(dev_requsets[PID].streams.size() > 0);

        REQUIRE_NOTHROW(pipe.start());

        std::vector<std::vector<stream_profile>> frames;
        std::vector<std::vector<double>> timestamps;

        for (auto i = 0; i < 30; i++)
            REQUIRE_NOTHROW(pipe.wait_for_frames(10000));


        while (frames.size() < 100)
        {
            frameset frame;
            REQUIRE_NOTHROW(frame = pipe.wait_for_frames(10000));
            std::vector<stream_profile> frames_set;
            std::vector<double> ts;
            for (auto f : frame)
            {
                frames_set.push_back(f.get_profile());
                ts.push_back(f.get_timestamp());
            }
            frames.push_back(frames_set);
            timestamps.push_back(ts);
        }


        REQUIRE_NOTHROW(pipe.stop());
        validate(frames, timestamps, dev_requsets[PID]);
    }
}

TEST_CASE("Pipeline poll", "[live]") {

    auto dev_requsets = get_pipeline_default_configurations();

    rs2::context ctx;

    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        auto list = ctx.query_devices();
        REQUIRE(list.size());
        rs2::pipeline pipe(ctx);
        auto dev = list[0];
        disable_sensitive_options_for(dev);

        std::string PID;
        REQUIRE_NOTHROW(PID = dev.get_info(RS2_CAMERA_INFO_PRODUCT_ID));

        REQUIRE(dev_requsets[PID].streams.size() > 0);


        REQUIRE_NOTHROW(pipe.start());

        std::vector<std::vector<stream_profile>> frames;
        std::vector<std::vector<double>> timestamps;

        for (auto i = 0; i < 30; i++)
            REQUIRE_NOTHROW(pipe.wait_for_frames(10000));


        while (frames.size() < 100)
        {
            frameset frame;
            if (pipe.poll_for_frames(&frame))
            {
                std::vector<stream_profile> frames_set;
                std::vector<double> ts;
                for (auto f : frame)
                {
                    frames_set.push_back(f.get_profile());
                    ts.push_back(f.get_timestamp());
                }
                frames.push_back(frames_set);
                timestamps.push_back(ts);
            }

        }

        REQUIRE_NOTHROW(pipe.stop());
        validate(frames, timestamps, dev_requsets[PID]);


    }
}


std::map<std::string, device_profiles> get_custom_configurations()
{
    std::map<std::string, device_profiles> dev_requsets;

    dev_requsets["0B07"] =
    {
        { { RS2_STREAM_DEPTH, RS2_FORMAT_Z16, 640, 480, 0 },{ RS2_STREAM_COLOR, RS2_FORMAT_RGB8, 1920, 1080, 0 } },
        30,
        true
    };

    dev_requsets["0AA5"] =
    {
        { { RS2_STREAM_DEPTH, RS2_FORMAT_Z16, 640, 240, 0 },{ RS2_STREAM_INFRARED, RS2_FORMAT_Y8, 640, 240, 1 },{ RS2_STREAM_COLOR, RS2_FORMAT_RGB8, 640, 480, 0 } },
        30,
        true
    };

    dev_requsets["0AD4"] =
    {
        { { RS2_STREAM_DEPTH, RS2_FORMAT_Z16, 1280, 720, 0 },{ RS2_STREAM_INFRARED, RS2_FORMAT_Y8, 640, 480, 1 } },
        30,
        true
    };
    dev_requsets["0AD1"] =
    {
        { { RS2_STREAM_DEPTH, RS2_FORMAT_Z16,  640, 480, 0 },{ RS2_STREAM_INFRARED, RS2_FORMAT_RGB8, 1280, 720, 0 } },
        30,
        true
    };
    return dev_requsets;
}

TEST_CASE("Pipeline enable stream", "[live]") {

    auto dev_requsets = get_custom_configurations();

    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        auto list = ctx.query_devices();
        REQUIRE(list.size());

        rs2::device dev;
        rs2::pipeline pipe(ctx);
        REQUIRE_NOTHROW(dev = list[0]);

        disable_sensitive_options_for(dev);

        std::string PID;
        REQUIRE_NOTHROW(PID = dev.get_info(RS2_CAMERA_INFO_PRODUCT_ID));

        REQUIRE(dev_requsets[PID].streams.size() > 0);


        for (auto req : dev_requsets[PID].streams)
            REQUIRE_NOTHROW(pipe.enable_stream(req.stream, req.index, req.width, req.height, req.format, dev_requsets[PID].fps));

        REQUIRE_NOTHROW(pipe.start());

        std::vector<std::vector<stream_profile>> frames;
        std::vector<std::vector<double>> timestamps;

        for (auto i = 0; i < 30; i++)
            REQUIRE_NOTHROW(pipe.wait_for_frames(10000));


        while (frames.size() < 100)
        {
            frameset frame;
            REQUIRE_NOTHROW(frame = pipe.wait_for_frames(10000));
            std::vector<stream_profile> frames_set;
            std::vector<double> ts;
            for (auto f : frame)
            {
                frames_set.push_back(f.get_profile());
                ts.push_back(f.get_timestamp());
            }
            frames.push_back(frames_set);
            timestamps.push_back(ts);
        }


        REQUIRE_NOTHROW(pipe.stop());
        validate(frames, timestamps, dev_requsets[PID]);


    }
}

TEST_CASE("Pipeline enable stream auto complete", "[live]") {

    std::map<std::string, device_profiles> dev_requsets;

    dev_requsets["0B07"] =
    {
        { { RS2_STREAM_DEPTH, RS2_FORMAT_ANY, 0, 0, 0 },{ RS2_STREAM_COLOR, RS2_FORMAT_RGB8, 0, 0, 0 } },
        30,
        true
    };

    dev_requsets["0AA5"] =
    {
        { { RS2_STREAM_DEPTH, RS2_FORMAT_ANY, 0, 0, 0 },{ RS2_STREAM_INFRARED, RS2_FORMAT_ANY, 0, 0, 1 },{ RS2_STREAM_COLOR, RS2_FORMAT_RGB8, 0, 0, 0 } },
        30,
        true
    };

    dev_requsets["0AD4"] =
    {
        { { RS2_STREAM_DEPTH, RS2_FORMAT_ANY, 0, 0, 0 } },
        30,
        true
    };
    dev_requsets["0AD1"] =
    {
        { { RS2_STREAM_DEPTH, RS2_FORMAT_ANY, 0, 0, 0 },{ RS2_STREAM_INFRARED, RS2_FORMAT_ANY, 0, 0, 0 } },
        30,
        true
    };
    rs2::context ctx;

    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        auto list = ctx.query_devices();
        REQUIRE(list.size());
        auto dev = list[0];
        disable_sensitive_options_for(dev);

        std::string PID = dev.get_info(RS2_CAMERA_INFO_PRODUCT_ID);

        REQUIRE(dev_requsets[PID].streams.size() > 0);

        rs2::pipeline pipe(ctx);

        for (auto req : dev_requsets[PID].streams)
            REQUIRE_NOTHROW(pipe.enable_stream(req.stream, req.index, req.width, req.height, req.format, dev_requsets[PID].fps));

        REQUIRE_NOTHROW(pipe.start());


        std::vector<std::vector<stream_profile>> frames;
        std::vector<std::vector<double>> timestamps;

        for (auto i = 0; i < 30; i++)
            REQUIRE_NOTHROW(pipe.wait_for_frames(10000));


        while (frames.size() < 100)
        {
            frameset frame;
            REQUIRE_NOTHROW(frame = pipe.wait_for_frames(10000));
            std::vector<stream_profile> frames_set;
            std::vector<double> ts;
            for (auto f : frame)
            {
                frames_set.push_back(f.get_profile());
                ts.push_back(f.get_timestamp());
            }
            frames.push_back(frames_set);
            timestamps.push_back(ts);
        }


        REQUIRE_NOTHROW(pipe.stop());
        validate(frames, timestamps, dev_requsets[PID]);
    }
}

TEST_CASE("Pipeline disable_all", "[live]") {

    auto not_default_configurations = get_custom_configurations();
    auto default_configurations = get_pipeline_default_configurations();

    rs2::context ctx;

    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        auto list = ctx.query_devices();
        REQUIRE(list.size());

        rs2::device dev;
        rs2::pipeline pipe(ctx);
        REQUIRE_NOTHROW(dev = list[0]);

        disable_sensitive_options_for(dev);

        std::string PID;
        REQUIRE_NOTHROW(PID = dev.get_info(RS2_CAMERA_INFO_PRODUCT_ID));

        REQUIRE(not_default_configurations[PID].streams.size() > 0);


        for (auto req : not_default_configurations[PID].streams)
            REQUIRE_NOTHROW(pipe.enable_stream(req.stream, req.index, req.width, req.height, req.format, not_default_configurations[PID].fps));

        REQUIRE_NOTHROW(pipe.disable_all());

        REQUIRE_NOTHROW(pipe.start());

        std::vector<std::vector<stream_profile>> frames;
        std::vector<std::vector<double>> timestamps;

        for (auto i = 0; i < 30; i++)
            REQUIRE_NOTHROW(pipe.wait_for_frames(10000));


        while (frames.size() < 100)
        {
            frameset frame;
            REQUIRE_NOTHROW(frame = pipe.wait_for_frames(10000));
            std::vector<stream_profile> frames_set;
            std::vector<double> ts;
            for (auto f : frame)
            {
                frames_set.push_back(f.get_profile());
                ts.push_back(f.get_timestamp());
            }
            frames.push_back(frames_set);
            timestamps.push_back(ts);
        }


        REQUIRE_NOTHROW(pipe.stop());
        validate(frames, timestamps, default_configurations[PID]);
    }
}

TEST_CASE("Pipeline disable stream", "[live]") {

    auto configurations = get_custom_configurations();

    rs2::context ctx;

    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        auto list = ctx.query_devices();
        REQUIRE(list.size());

        rs2::device dev;
        rs2::pipeline pipe(ctx);
        REQUIRE_NOTHROW(dev = list[0]);

        disable_sensitive_options_for(dev);

        std::string PID;
        REQUIRE_NOTHROW(PID = dev.get_info(RS2_CAMERA_INFO_PRODUCT_ID));

        REQUIRE(configurations[PID].streams.size() > 0);


        for (auto req : configurations[PID].streams)
            REQUIRE_NOTHROW(pipe.enable_stream(req.stream, req.index, req.width, req.height, req.format, configurations[PID].fps));


        auto stream_to_be_removed = configurations[PID].streams[configurations[PID].streams.size() - 1].stream;
        REQUIRE_NOTHROW(pipe.disable_stream(stream_to_be_removed));


        auto& streams = configurations[PID].streams;
        streams.erase(streams.end() - 1);


        REQUIRE_NOTHROW(pipe.start());

        std::vector<std::vector<stream_profile>> frames;
        std::vector<std::vector<double>> timestamps;

        for (auto i = 0; i < 30; i++)
            REQUIRE_NOTHROW(pipe.wait_for_frames(10000));


        while (frames.size() < 100)
        {
            frameset frame;
            REQUIRE_NOTHROW(frame = pipe.wait_for_frames(10000));
            std::vector<stream_profile> frames_set;
            std::vector<double> ts;
            for (auto f : frame)
            {
                frames_set.push_back(f.get_profile());
                ts.push_back(f.get_timestamp());
            }
            frames.push_back(frames_set);
            timestamps.push_back(ts);
        }


        REQUIRE_NOTHROW(pipe.stop());
        validate(frames, timestamps, configurations[PID]);
    }
}

TEST_CASE("Pipeline with specific device", "[live]") {

    auto configurations = get_custom_configurations();

    rs2::context ctx;

    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        auto list = ctx.query_devices();
        REQUIRE(list.size());

        rs2::device dev;
        REQUIRE_NOTHROW(dev = list[0]);
        disable_sensitive_options_for(dev);
        std::string serial, serial1;
        REQUIRE_NOTHROW(serial = dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));

        rs2::pipeline pipe(ctx);
        REQUIRE_NOTHROW(pipe.enable_device(serial));
        REQUIRE_NOTHROW(pipe.open());

        REQUIRE_NOTHROW(dev = pipe.get_device());

        disable_sensitive_options_for(dev);



        REQUIRE_NOTHROW(dev = pipe.get_device());
        REQUIRE_NOTHROW(serial1 = dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));


        CAPTURE(serial);
        CAPTURE(serial1);
        REQUIRE(serial1 == serial);

    }
}

bool operator==(std::vector<profile> streams1, std::vector<profile> streams2)
{
    if (streams1.size() != streams2.size())
        return false;

    std::sort(streams1.begin(), streams1.end());
    std::sort(streams2.begin(), streams2.end());

    auto equals = true;
    for (auto i = 0; i < streams1.size(); i++)
    {
        if (streams1[i] != streams2[i])
        {
            equals = false;
            break;
        }
    }
    return equals;
}

TEST_CASE("Pipeline start stop", "[live]") {

    auto configurations = get_custom_configurations();

    rs2::context ctx;

    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        auto list = ctx.query_devices();
        REQUIRE(list.size());

        rs2::device dev;
        rs2::pipeline pipe(ctx);
        REQUIRE_NOTHROW(dev = list[0]);

        disable_sensitive_options_for(dev);

        std::string PID;
        REQUIRE_NOTHROW(PID = dev.get_info(RS2_CAMERA_INFO_PRODUCT_ID));

        REQUIRE(configurations[PID].streams.size() > 0);

        for (auto req : configurations[PID].streams)
            REQUIRE_NOTHROW(pipe.enable_stream(req.stream, req.index, req.width, req.height, req.format, configurations[PID].fps));

        auto& streams = configurations[PID].streams;

        REQUIRE_NOTHROW(pipe.start());

        std::vector<device_profiles> frames;

        for (auto i = 0; i < 10; i++)
            REQUIRE_NOTHROW(pipe.wait_for_frames(10000));


        REQUIRE_NOTHROW(pipe.stop());
        REQUIRE_NOTHROW(pipe.start());

        for (auto i = 0; i < 20; i++)
            REQUIRE_NOTHROW(pipe.wait_for_frames(10000));

        std::vector<profile> profiles;
        auto equals = 0;
        for (auto i = 0; i < 30; i++)
        {
            frameset frame;
            REQUIRE_NOTHROW(frame = pipe.wait_for_frames(10000));
            REQUIRE(frame.size() > 0);

            for (auto f : frame)
            {
                auto profile = f.get_profile();
                auto video_profile = profile.as<video_stream_profile>();

                profiles.push_back({ profile.stream_type(),
                    profile.format(),
                    video_profile.width(),
                    video_profile.height(),
                    video_profile.stream_index() });

            }
            if (profiles == streams)
                equals++;
            profiles.clear();
        }

        REQUIRE_NOTHROW(pipe.stop());
        REQUIRE(equals > 1);
    }
}

bool compare(const rs2_extrinsics& first, const rs2_extrinsics& second, double delta = 0)
{
    for (auto i = 0; i < 9; i++)
    {
        if (std::abs(first.rotation[i] - second.rotation[i]) > delta)
        {
            return false;
        }
    }
    for (auto i = 0; i < 3; i++)
    {
        if (std::abs(first.translation[i] - second.translation[i]) > delta)
        {
            return false;
        }
    }
    return true;
}

std::map<std::string, device_profiles> get_configurations_for_extrinsics()
{
    std::map<std::string, device_profiles> dev_requsets;

    dev_requsets["0B07"] =
    {
        { { RS2_STREAM_DEPTH, RS2_FORMAT_Z16, 640, 480, 0 },{ RS2_STREAM_INFRARED, RS2_FORMAT_Y8, 640, 480, 1 } ,{ RS2_STREAM_COLOR, RS2_FORMAT_RGB8, 1920, 1080, 0 } },
        30,
        true
    };

    dev_requsets["0AA5"] =
    {
        { { RS2_STREAM_DEPTH, RS2_FORMAT_Z16, 640, 240, 0 },{ RS2_STREAM_INFRARED, RS2_FORMAT_Y8, 640, 240, 1 },{ RS2_STREAM_COLOR, RS2_FORMAT_RGB8, 640, 480, 0 } },
        30,
        true
    };

    dev_requsets["0AD4"] =
    {
        { { RS2_STREAM_DEPTH, RS2_FORMAT_Z16, 1280, 720, 0 },{ RS2_STREAM_INFRARED, RS2_FORMAT_Y8, 640, 480, 1 } },
        30,
        true
    };
    dev_requsets["0AD1"] =
    {
        { { RS2_STREAM_DEPTH, RS2_FORMAT_Z16,  640, 480, 0 },{ RS2_STREAM_INFRARED, RS2_FORMAT_RGB8, 1280, 720, 0 } },
        30,
        true
    };
    return dev_requsets;
}
TEST_CASE("Pipeline get selection", "[live]") {
    rs2::context ctx;
    auto configurations = get_configurations_for_extrinsics();

    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        auto list = ctx.query_devices();
        REQUIRE(list.size());

        rs2::device dev;
        rs2::pipeline pipe(ctx);
        REQUIRE_NOTHROW(dev = list[0]);

        disable_sensitive_options_for(dev);

        std::string PID;
        REQUIRE_NOTHROW(PID = dev.get_info(RS2_CAMERA_INFO_PRODUCT_ID));

        REQUIRE(configurations[PID].streams.size() > 0);

        for (auto req : configurations[PID].streams)
            REQUIRE_NOTHROW(pipe.enable_stream(req.stream, req.index, req.width, req.height, req.format, configurations[PID].fps));

        REQUIRE_NOTHROW(pipe.start());
        std::vector<stream_profile> profiles;
        REQUIRE_NOTHROW(profiles = pipe.get_active_streams());

        auto streams = configurations[PID].streams;
        std::vector<profile> pipe_streams;
        for (auto s : profiles)
        {
            REQUIRE(s.is<video_stream_profile>());
            auto video = s.as<video_stream_profile>();

            pipe_streams.push_back({ video.stream_type(), video.format(), video.width(), video.height(), video.stream_index() });
        }
        REQUIRE(pipe_streams.size() == streams.size());

        std::sort(pipe_streams.begin(), pipe_streams.end());
        std::sort(streams.begin(), streams.end());

        for (auto i = 0; i < pipe_streams.size(); i++)
        {
            REQUIRE(pipe_streams[i] == streams[i]);
        }

    }
}

TEST_CASE("Per-frame metadata sanity check", "[live][!mayfail]") {
    //Require at least one device to be plugged in
    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        std::vector<sensor> list;
        REQUIRE_NOTHROW(list = ctx.query_all_sensors());
        REQUIRE(list.size() > 0);

        const int frames_before_start_measure = 30;
        const int frames_for_fps_measure = 100;
        const double msec_to_sec = 0.001;
        const int num_of_profiles_for_each_subdevice = 2;
        const float max_diff_between_real_and_metadata_fps = 1.0f;

        for (auto && subdevice : list) {
            std::vector<rs2::stream_profile> modes;
            REQUIRE_NOTHROW(modes = subdevice.get_stream_profiles());

            REQUIRE(modes.size() > 0);
            WARN(subdevice.get_info(RS2_CAMERA_INFO_NAME));

            //the test will be done only on sub set of profile for each sub device
            for (int i = 0; i < modes.size(); i += static_cast<int>(std::ceil((float)modes.size() / (float)num_of_profiles_for_each_subdevice)))
            {
                if ((modes[i].size() == 1920 * 1080 * 60) ||           // Full-HD is often times too heavy for the build machine to handle
                    (RS2_STREAM_GPIO <= modes[i].stream_type()))   // GPIO Requires external triggers to produce events
                    continue;   // Disabling for now

                CAPTURE(modes[i].format());
                CAPTURE(modes[i].fps());
                CAPTURE(modes[i].stream_type());
                CAPTURE(modes[i].stream_index());
                if (auto video = modes[i].as<video_stream_profile>())
                {
                    CAPTURE(video.width());
                    CAPTURE(video.height());
                }

                std::vector<internal_frame_additional_data> frames_additional_data;
                auto frames = 0;
                double start;
                std::condition_variable cv;
                std::mutex m;
                auto first = true;

                REQUIRE_NOTHROW(subdevice.open({ modes[i] }));
                disable_sensitive_options_for(subdevice);

                REQUIRE_NOTHROW(subdevice.start([&](rs2::frame f)
                {

                    if ((frames >= frames_before_start_measure) && (frames_additional_data.size() < frames_for_fps_measure))
                    {
                        if (first)
                        {
                            start = internal::get_time();
                        }
                        first = false;

                        internal_frame_additional_data data{ f.get_timestamp(),
                            f.get_frame_number(),
                            f.get_frame_timestamp_domain(),
                            f.get_profile().stream_type(),
                            f.get_profile().format() };

                        // Store frame metadata attributes, verify API behavior correctness
                        for (auto i = 0; i < rs2_frame_metadata_value::RS2_FRAME_METADATA_COUNT; i++)
                        {
                            CAPTURE(i);
                            bool supported = false;
                            REQUIRE_NOTHROW(supported = f.supports_frame_metadata((rs2_frame_metadata_value)i));
                            if (supported)
                            {
                                rs2_metadata_type val{};
                                REQUIRE_NOTHROW(val = f.get_frame_metadata((rs2_frame_metadata_value)i));
                                data.frame_md.md_attributes[i] = std::make_pair(true, val);
                            }
                            else
                            {

                                REQUIRE_THROWS(f.get_frame_metadata((rs2_frame_metadata_value)i));
                                data.frame_md.md_attributes[i].first = false;
                            }
                        }


                        std::unique_lock<std::mutex> lock(m);
                        frames_additional_data.push_back(data);
                    }
                    frames++;
                    if (frames_additional_data.size() >= frames_for_fps_measure)
                    {
                        cv.notify_one();
                    }
                }));


                CAPTURE(frames_additional_data.size());
                CAPTURE(frames_for_fps_measure);
                std::unique_lock<std::mutex> lock(m);
                cv.wait_for(lock, std::chrono::seconds(30), [&] {return ((frames_additional_data.size() >= frames_for_fps_measure)); });

                REQUIRE_NOTHROW(subdevice.stop());
                REQUIRE_NOTHROW(subdevice.close());

                auto end = internal::get_time();
                lock.unlock();

                auto seconds = (end - start)*msec_to_sec;

                CAPTURE(start);
                CAPTURE(end);

                REQUIRE(seconds > 0);

                if (frames_additional_data.size())
                {
                    auto actual_fps = (double)frames_additional_data.size() / (double)seconds;
                    double metadata_seconds = frames_additional_data[frames_additional_data.size() - 1].timestamp - frames_additional_data[0].timestamp;
                    metadata_seconds *= msec_to_sec;

                    if (metadata_seconds <= 0)
                    {
                        std::cout << "Start metadata " << std::fixed << frames_additional_data[0].timestamp << "\n";
                        std::cout << "End metadata   " << std::fixed << frames_additional_data[frames_additional_data.size() - 1].timestamp << "\n";
                    }
                    REQUIRE(metadata_seconds > 0);

                    auto metadata_frames = frames_additional_data[frames_additional_data.size() - 1].frame_number - frames_additional_data[0].frame_number;
                    auto metadata_fps = (double)metadata_frames / (double)metadata_seconds;

                    for (auto i = 0; i < frames_additional_data.size() - 1; i++)
                    {
                        CAPTURE(i);
                        CAPTURE(frames_additional_data[i].timestamp_domain);
                        CAPTURE(frames_additional_data[i + 1].timestamp_domain);
                        REQUIRE((frames_additional_data[i].timestamp_domain == frames_additional_data[i + 1].timestamp_domain));

                        CAPTURE(frames_additional_data[i].frame_number);
                        CAPTURE(frames_additional_data[i + 1].frame_number);

                        REQUIRE((frames_additional_data[i].frame_number < frames_additional_data[i + 1].frame_number));
                    }
                    CAPTURE(actual_fps);
                    CAPTURE(metadata_fps);

                    //it the diff in percentage between metadata fps and actual fps is bigger than max_diff_between_real_and_metadata_fps
                    //the test will fail
                    REQUIRE(std::abs(metadata_fps / actual_fps - 1) < max_diff_between_real_and_metadata_fps);

                    // Verify per-frame metadata attributes
                    metadata_verification(frames_additional_data);
                }
            }
        }
    }
}
TEST_CASE("All suggested profiles can be opened", "[live]") {

    //Require at least one device to be plugged in
    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {

        const int num_of_profiles_for_each_subdevice = 2;

        std::vector<sensor> list;
        REQUIRE_NOTHROW(list = ctx.query_all_sensors());
        REQUIRE(list.size() > 0);

        for (auto && subdevice : list) {

            disable_sensitive_options_for(subdevice);

            std::vector<rs2::stream_profile> modes;
            REQUIRE_NOTHROW(modes = subdevice.get_stream_profiles());

            REQUIRE(modes.size() > 0);
            WARN(subdevice.get_info(RS2_CAMERA_INFO_NAME));
            //the test will be done only on sub set of profile for each sub device
            for (int i = 0; i < modes.size(); i += (int)std::ceil((float)modes.size() / (float)num_of_profiles_for_each_subdevice)) {
                //CAPTURE(rs2_subdevice(subdevice));
                CAPTURE(modes[i].format());
                CAPTURE(modes[i].fps());
                CAPTURE(modes[i].stream_type());
                REQUIRE_NOTHROW(subdevice.open({ modes[i] }));
                REQUIRE_NOTHROW(subdevice.start([](rs2::frame fref) {}));
                REQUIRE_NOTHROW(subdevice.stop());
                REQUIRE_NOTHROW(subdevice.close());
            }
        }
    }
}

TEST_CASE("Pipeline enable open start flow", "[live]") {
    rs2::context ctx;

    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        auto list = ctx.query_devices();
        REQUIRE(list.size());

        rs2::device dev;
        rs2::pipeline pipe(ctx);
        REQUIRE_NOTHROW(dev = list[0]);

        disable_sensitive_options_for(dev);
        REQUIRE_THROWS(pipe.get_device());
        REQUIRE_NOTHROW(pipe.open());
        REQUIRE_NOTHROW(pipe.open());
        REQUIRE_NOTHROW(dev = pipe.get_device());
        
        REQUIRE_THROWS(pipe.enable_stream(RS2_STREAM_DEPTH, 0, 0, 0, RS2_FORMAT_ANY, 0));
        REQUIRE_NOTHROW(pipe.start());

        std::vector<device_profiles> frames;

        for (auto i = 0; i < 10; i++)
            REQUIRE_NOTHROW(pipe.wait_for_frames(10000));


        REQUIRE_NOTHROW(pipe.stop());
        REQUIRE_NOTHROW(pipe.start());

        for (auto i = 0; i < 20; i++)
            REQUIRE_NOTHROW(pipe.wait_for_frames(10000));

        REQUIRE_NOTHROW(pipe.disable_all());

        REQUIRE_NOTHROW(pipe.enable_stream(RS2_STREAM_DEPTH, 0, 0, 0, RS2_FORMAT_ANY, 0));
        REQUIRE_NOTHROW(pipe.open());

        REQUIRE_NOTHROW(pipe.disable_all());
        REQUIRE_NOTHROW(pipe.open());
    }
}
