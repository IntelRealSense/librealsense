// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

//#cmake: static!
//#test:device D400*
//#test:donotrun:!nightly
//#test:timeout 480

#include <numeric>
#include <stdlib.h> 
#include <cmath>
#include "../../catch.h"
#include "../../unit-tests-common.h"
#include <librealsense2/hpp/rs_sensor.hpp>
#include "./../src/environment.h"


using namespace librealsense;
using namespace librealsense::platform;

constexpr int DELAY_INCREMENT_THRESHOLD = 3; //[%]
constexpr int DELAY_INCREMENT_THRESHOLD_IMU = 8; //[%]
constexpr int SPIKE_THRESHOLD = 2; //[stdev]

constexpr int ITERATIONS_PER_CONFIG = 60;
constexpr int INNER_ITERATIONS_PER_CONFIG = 10;

// Input:     vector that represent samples of delay to first frame of one stream
// Output:  - slope of the fitted line
// reference: https://en.wikipedia.org/wiki/Least_squares
double line_fitting(const std::vector<double>& y_vec, std::vector<double> y_fit = std::vector<double>())
{
    double ysum = std::accumulate(y_vec.begin(), y_vec.end(), 0.0);  //calculate sigma(yi)
    double xsum = 0;
    double x2sum = 0;
    double xysum = 0;
    double a = 1;
    auto y_vec_it = y_vec.begin();
    size_t n = y_vec.size();
    for (auto i = 0; i < n; i++)
    {
        xsum = xsum + i;  //sigma(xi)                                   
        x2sum = x2sum + pow(i, 2); //sigma(x^2i)
        xysum = xysum + i * *(y_vec_it + i);
    }
    a = (n * xysum - xsum * ysum) / (n * x2sum - xsum * xsum);      //slope
    double b = (x2sum * ysum - xsum * xysum) / (x2sum * n - xsum * xsum);  //intercept   
    if (y_fit.size() > 0)
    {
        auto it = y_fit.begin();
        for (auto i = 0; i < n; i++)
        {
            *(it + i) = a * i + b;
        }
    }
    return a; // slope is used when checking delay increment 
}
// Input:  vector that represent samples of time delay to first frame of one stream
// Output: vector of filtered-out spikes 
void data_filter(const std::vector<double>& stream_vec, std::vector<double>& filtered_stream_vec)
{
    std::vector<double> y_fit(stream_vec.size());
    double slope = line_fitting(stream_vec, y_fit);

    auto y_fit_it = y_fit.begin();
    auto stream_vec_it = stream_vec.begin();
    std::vector<double> diff_y_fit;
    for (auto i = 0; i < stream_vec.size(); i++)
    {
        double diff = abs(*(y_fit_it + i) - *(stream_vec_it + i));
        diff_y_fit.push_back(diff);
    }
    // calc stdev from fitted linear line
    double sq_sum = std::inner_product(diff_y_fit.begin(), diff_y_fit.end(), diff_y_fit.begin(), 0.0);
    double stdev = std::sqrt(sq_sum / diff_y_fit.size());

    // calc % of each sample from stdev
    std::vector<double> samples_stdev(diff_y_fit.size());
    auto v_size = diff_y_fit.size();
    std::transform(diff_y_fit.begin(), diff_y_fit.end(), samples_stdev.begin(), [stdev](double d) {
        auto val = d / stdev;
        return  val;
        }
    );

    // filter spikes
    auto samples_stdev_it = samples_stdev.begin();
    stream_vec_it = stream_vec.begin();
    for (auto i = 0; i < samples_stdev.size(); i++)
    {
        if (*(samples_stdev_it + i) > SPIKE_THRESHOLD) continue;
        filtered_stream_vec.push_back(*(stream_vec_it + i));
    }
}

TEST_CASE("Extrinsic memory leak detection", "[live]")
{
    // Require at least one device to be plugged in

    rs2::context ctx;
    rs2::log_to_file(RS2_LOG_SEVERITY_DEBUG, "lrs_log.txt");

    std::cout << "Extrinsic memory leak detection started" << std::endl;

#ifdef LIGHT_TEST
    bool is_pipe_test[1] = { true };
#else
    bool is_pipe_test[2] = { true, false };
#endif

    for (auto is_pipe : is_pipe_test)
    {
        auto list = ctx.query_devices();
        REQUIRE(list.size());
        auto dev = list.front();
        auto sensors = dev.query_sensors();

        REQUIRE(dev.supports(RS2_CAMERA_INFO_PRODUCT_LINE));
        std::string device_type = dev.get_info(RS2_CAMERA_INFO_PRODUCT_LINE);

        if (dev.supports(RS2_CAMERA_INFO_PRODUCT_LINE) && std::string(dev.get_info(RS2_CAMERA_INFO_PRODUCT_LINE)) == "D400") device_type = "D400";
        if (dev.supports(RS2_CAMERA_INFO_PRODUCT_LINE) && std::string(dev.get_info(RS2_CAMERA_INFO_PRODUCT_LINE)) == "SR300") device_type = "SR300";

        bool usb3_device = is_usb3(dev);
        int fps = usb3_device ? 30 : 15; // In USB2 Mode the devices will switch to lower FPS rates
        int req_fps = usb3_device ? 60 : 30; // USB2 Mode has only a single resolution for 60 fps which is not sufficient to run the test

        int width = 848;
        int height = 480;

        if (device_type == "L500")
        {
            req_fps = 30;
            width = 640;
        }
        auto res = configure_all_supported_streams(dev, width, height, fps);
        for (auto& s : res.first)
        {
            s.close();
        }

        // collect a log that contains info about 20 iterations for each stream
        // the info should include:
        // 1. extrinsics table size
        // 2. delay to first frame
        // 3. delay threshold for each stream (set fps=6 delay as worst case)
        // the test will succeed only if all 3 conditions are met:
        // 1. extrinsics table size is preserved over iterations for each stream
        // 2. no delay increment over iterations
        // 3. "most" iterations have time to first frame delay below a defined threshold

        std::vector<size_t> extrinsics_table_size;
        std::map<std::string, std::vector<double>> streams_delay; // map to vector to collect all data
        std::map<std::string, std::vector<std::map<unsigned long long, size_t >>> unique_streams_delay;
        std::map<std::string, size_t> new_frame;
        std::map<std::string, size_t> extrinsic_graph_at_sensor;

        auto& b = environment::get_instance().get_extrinsics_graph();
        if (is_pipe)
        {
            std::cout << "==============================================" << std::endl;
            std::cout << "Pipe Test is running .." << std::endl;
        }
        else {
            std::cout << "==============================================" << std::endl;
            std::cout << "Sensor Test is running .." << std::endl;
        }

        for (auto i = 0; i < ITERATIONS_PER_CONFIG; i++)
        {
            rs2::config cfg;
            rs2::pipeline pipe;
            size_t cfg_size = 0;
            std::vector<rs2::stream_profile> valid_stream_profiles;
            std::map<int, std::vector<rs2::stream_profile>> sensor_stream_profiles;
            std::map<int, std::vector<rs2_stream>> ds5_sensor_stream_map;
            for (auto& profile : res.second)
            {
                auto fps = profile.fps;
                if (device_type == "D400" && profile.stream == RS2_STREAM_ACCEL) fps = 250;
                cfg.enable_stream(profile.stream, profile.index, profile.width, profile.height, profile.format, fps); // all streams in cfg
                cfg_size += 1;
                // create stream profiles data structure to open streams per sensor when testing in sensor mode
                for (auto& s : res.first)
                {
                    auto stream_profiles = s.get_stream_profiles();
                    for (auto& sp : stream_profiles)
                    {
                        if (!(sp.stream_type() == profile.stream && sp.fps() == fps && sp.stream_index() == profile.index && sp.format() == profile.format)) continue;
                        if (sp.stream_type() == RS2_STREAM_ACCEL || sp.stream_type() == RS2_STREAM_GYRO) sensor_stream_profiles[2].push_back(sp);
                        auto vid = sp.as<rs2::video_stream_profile>();
                        auto h = vid.height();
                        auto w = vid.width();
                        if (!(w == profile.width && h == profile.height)) continue;
                        if (sp.stream_type() == RS2_STREAM_DEPTH || sp.stream_type() == RS2_STREAM_INFRARED || sp.stream_type() == RS2_STREAM_CONFIDENCE) sensor_stream_profiles[0].push_back(sp);
                        if (sp.stream_type() == RS2_STREAM_COLOR) sensor_stream_profiles[1].push_back(sp);
                    }
                }
            }

            for (auto it = new_frame.begin(); it != new_frame.end(); it++)
            {
                it->second = 0;
            }
            auto start_time = std::chrono::system_clock::now().time_since_epoch();
            auto start_time_milli = std::chrono::duration_cast<std::chrono::milliseconds>(start_time).count();
            bool condition = false;
            std::mutex mutex;
            std::mutex mutex_2;
            auto process_frame = [&](const rs2::frame& f)
            {
                std::lock_guard<std::mutex> lock(mutex_2);
                auto stream_type = f.get_profile().stream_name();
                auto frame_num = f.get_frame_number();
                auto time_of_arrival = f.get_frame_metadata(RS2_FRAME_METADATA_TIME_OF_ARRIVAL);

                if (!new_frame[stream_type])
                {
                    streams_delay[stream_type].push_back((double(time_of_arrival - start_time_milli)));
                    new_frame[stream_type] = true;
                }
                new_frame[stream_type] += 1;
            };
            auto frame_callback = [&](const rs2::frame& f)
            {
                std::lock_guard<std::mutex> lock(mutex);
                if (rs2::frameset fs = f.as<rs2::frameset>())
                {
                    // With callbacks, all synchronized stream will arrive in a single frameset
                    for (const rs2::frame& ff : fs)
                    {
                        process_frame(ff);
                    }
                }
                else
                {
                    // Stream that bypass synchronization (such as IMU) will produce single frames
                    process_frame(f);
                }
            };
            if (is_pipe)
            {
                rs2::pipeline_profile profiles = pipe.start(cfg, frame_callback);
            }
            else {

                int i = 0;
                for (auto& s : res.first)
                {
                    if (sensor_stream_profiles.find(i) == sensor_stream_profiles.end()) continue;
                    s.open(sensor_stream_profiles[i]);
                    i += 1;
                }
                i = 0;
                for (auto& s : res.first)
                {
                    if (sensor_stream_profiles.find(i) == sensor_stream_profiles.end()) continue;
                    s.start(frame_callback);
                    i += 1;
                }
            }
            // to prevent FW issue, at least 20 frames per stream should arrive
            while (!condition) // the condition is set to true when at least 20 frames are received per stream
            {
                try
                {
                    if (new_frame.size() == cfg_size)
                    {
                        condition = true;
                        for (auto it = new_frame.begin(); it != new_frame.end(); it++)
                        {
                            if (it->second < INNER_ITERATIONS_PER_CONFIG)
                            {
                                condition = false;
                                break;
                            }
                        }
                        // all streams received more than 10 frames
                    }
                }
                catch (...)
                {
                    std::cout << "Iteration failed  " << std::endl;
                }
            }
            if (is_pipe)
            {
                pipe.stop();
            }
            else
            {
                // Stop & flush all active sensors. The separation is intended to semi-confirm the FPS
                for (auto& s : res.first)
                {
                    s.stop();
                }
                for (auto& s : res.first)
                {
                    s.close();
                }

            }
            extrinsics_table_size.push_back(b._extrinsics.size());

        }
        std::cout << "Analyzing info ..  " << std::endl;

        // the test will succeed only if all 3 conditions are met:
        // 1. extrinsics table size is preserved over iterations for each stream
        // 2. no delay increment over iterations
        // 3. "most" iterations have time to first frame delay below a defined threshold
        if (extrinsics_table_size.size())
        {
            CAPTURE(extrinsics_table_size);
            // 1. extrinsics table preserve its size over iterations
            CHECK(std::adjacent_find(extrinsics_table_size.begin(), extrinsics_table_size.end(), std::not_equal_to<size_t>()) == extrinsics_table_size.end());
        }
        // 2.  no delay increment over iterations
        // filter spikes : calc stdev for each half and filter out samples that are not close
        for (auto& stream : streams_delay)
        {
            // make sure we have enough data for each stream
            REQUIRE(stream.second.size() > 10);

            // remove first 5 iterations from each stream
            stream.second.erase(stream.second.begin(), stream.second.begin() + 5);
            double slope = line_fitting(stream.second);
            // check slope value against threshold
            auto threshold = DELAY_INCREMENT_THRESHOLD;
            // IMU streams have different threshold
            if (stream.first == "Accel" || stream.first == "Gyro") threshold = DELAY_INCREMENT_THRESHOLD_IMU;
            CAPTURE(stream.first, slope, threshold);
            CHECK(slope < threshold);

        }
        // 3. "most" iterations have time to first frame delay below a defined threshold
        std::map<std::string, double> delay_thresholds;
        // D400
        delay_thresholds["Accel"] = 1200; // ms
        delay_thresholds["Color"] = 1200; // ms
        delay_thresholds["Depth"] = 1200; // ms
        delay_thresholds["Gyro"] = 1200; // ms
        delay_thresholds["Infrared 1"] = 1200; // ms
        delay_thresholds["Infrared 2"] = 1200; // ms

        // L500
        if (device_type == "L500")
        {
            delay_thresholds["Accel"] = 2200;  // ms
            delay_thresholds["Color"] = 2000;  // ms
            delay_thresholds["Depth"] = 2000;  // ms
            delay_thresholds["Gyro"] = 2200;   // ms
            delay_thresholds["Confidence"] = 2000; // ms
            delay_thresholds["Infrared"] = 2000;   // ms
        }

        // SR300
        if (device_type == "SR300")
        {
            delay_thresholds["Accel"] = 1200; // ms
            delay_thresholds["Color"] = 1200; // ms
            delay_thresholds["Depth"] = 1200; // ms
            delay_thresholds["Gyro"] = 1200; // ms
            delay_thresholds["Infrared 1"] = 1200; // ms
            delay_thresholds["Infrared 2"] = 1200; // ms
        }

        for (const auto& stream_ : streams_delay)
        {
            std::vector<double> filtered_stream_vec;
            data_filter(stream_.second, filtered_stream_vec);

            auto stream = stream_.first;
            double sum = std::accumulate(filtered_stream_vec.begin(), filtered_stream_vec.end(), 0.0);
            double avg = sum / filtered_stream_vec.size();
            std::cout << "Delay of " << stream << " = " << avg * 1.5 << std::endl;
            CAPTURE(stream);
            for (auto it = stream_.second.begin(); it != stream_.second.end(); ++it) {
                CHECK(*it <= delay_thresholds[stream]);
            }
        }

    }
}
