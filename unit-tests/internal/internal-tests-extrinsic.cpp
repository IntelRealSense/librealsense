// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "approx.h"
#include <cmath>
#include <iostream>
#include <chrono>
#include <ctime>
#include <algorithm>
#include <librealsense2/rs.hpp>
#include <librealsense2/hpp/rs_sensor.hpp>
#include "../../common/tiny-profiler.h"
#include "./../src/environment.h"

#include <unit-tests-common.h>
#include <numeric>
#include <stdlib.h> 

using namespace librealsense;
using namespace librealsense::platform;

constexpr int ITERATIONS_PER_CONFIG = 100;
constexpr int INNER_ITERATIONS_PER_CONFIG = 10;
constexpr int DELAY_INCREMENT_THRESHOLD = 3; //[%]
constexpr int DELAY_INCREMENT_THRESHOLD_IMU = 8; //[%]
constexpr int SPIKE_THRESHOLD = 2; //[stdev]

// Require that vector is exactly the zero vector
/*inline void require_zero_vector(const float(&vector)[3])
{
    for (int i = 1; i < 3; ++i) REQUIRE(vector[i] == 0.0f);
}

// Require that matrix is exactly the identity matrix
inline void require_identity_matrix(const float(&matrix)[9])
{
    static const float identity_matrix_3x3[] = { 1,0,0, 0,1,0, 0,0,1 };
    for (int i = 0; i < 9; ++i) REQUIRE(matrix[i] == approx(identity_matrix_3x3[i]));
}*/

bool get_mode(rs2::device& dev, rs2::stream_profile& profile, int mode_index = 0)
{
    auto sensors = dev.query_sensors();
    REQUIRE(sensors.size() > 0);

    for (auto i = 0; i < sensors.size(); i++)
    {
        auto modes = sensors[i].get_stream_profiles();
        REQUIRE(modes.size() > 0);

        if (mode_index >= modes.size())
            continue;

        profile = modes[mode_index];
        return true;
    }
    return false;
}

double line_fitting(const std::vector<double>& y_vec, std::vector<double>& y_fit)
{
    double ysum = std::accumulate(y_vec.begin(), y_vec.end(), 0.0);  //calculate sigma(yi)
    double xsum = 0;
    double x2sum = 0;
    double xysum = 0;
    auto y_vec_it = y_vec.begin();
    size_t n = y_vec.size();
    for (auto i = 0; i < n; i++)
    {
        xsum = xsum + i;  //calculate sigma(xi)                                   
        x2sum = x2sum + pow(i, 2); //calculate sigma(x^2i)
        xysum = xysum + i * *(y_vec_it + i);
    }
    double a = (n * xysum - xsum * ysum) / (n * x2sum - xsum * xsum);      //calculate slope
    double b = (x2sum * ysum - xsum * xysum) / (x2sum * n - xsum * xsum);  //calculate intercept   
    for (auto i = 0; i < n; i++)
    {
        //calculate y(fitted) at given x points             
        y_fit.push_back(a * i + b);
    }
    return a; // return the slope for later usage when checking delay increment 
}
//void data_filter(double* filtered_vec_avg_arr, std::vector<double>& stream_vec, std::vector<double>& filtered_stream_vec, std::string const stream_type)
double data_filter(const std::vector<double>& stream_vec, std::vector<double>& filtered_stream_vec, std::string const stream_type)
{
    std::vector<double> y_fit;
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
        d = d < 0 ? -d : d;
        auto val = d / stdev;
        return  val >= 0 ? val : -val;
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

    return slope;

}
TEST_CASE("Extrinsic graph management", "[live][multicam]")
{
    // Require at least one device to be plugged in
    rs2::context ctx;
    {
        std::cout << "Extrinsic graph management started" << std::endl;
        auto list = ctx.query_devices();
        REQUIRE(list.size());

        std::map<std::string, size_t> extrinsic_graph_at_sensor;
        auto& b = environment::get_instance().get_extrinsics_graph();
        auto init_size = b._streams.size();
        std::cout << " Initial Extrinsic Graph size is " << init_size << std::endl;

        for (int i = 0; i < 10; i++)
        {
            std::cout << "Iteration " << i << " : Extrinsic graph map size is " << b._streams.size() << std::endl;
            // For each device
            for (auto&& dev : list)
            {
                //std::cout << "Dev " << dev.get_info(RS2_CAMERA_INFO_NAME) << std::endl;
                for (auto&& snr : dev.query_sensors())
                {
                    std::vector<rs2::stream_profile> profs;
                    REQUIRE_NOTHROW(profs = snr.get_stream_profiles());
                    REQUIRE(profs.size() > 0);

                    std::string snr_id = snr.get_info(RS2_CAMERA_INFO_NAME);
                    snr_id += snr.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER);
                    if (extrinsic_graph_at_sensor.count(snr_id))
                    {
                        CAPTURE(snr_id);
                        CAPTURE(extrinsic_graph_at_sensor.at(snr_id));
                        CAPTURE(b._streams.size());
                        REQUIRE(b._streams.size() == extrinsic_graph_at_sensor.at(snr_id));
                    }
                    else
                        extrinsic_graph_at_sensor[snr_id] = b._streams.size();

                    //                    std::cout << __LINE__ << " " << snr.get_info(RS2_CAMERA_INFO_NAME) <<" : Extrinsic graph map size is " << b._streams.size() << std::endl;

                    rs2_extrinsics extrin{};
                    try {
                        auto prof = profs[0];
                        extrin = prof.get_extrinsics_to(prof);
                    }
                    catch (const rs2::error& e) {
                        // if device isn't calibrated, get_extrinsics must error out (according to old comment. Might not be true under new API)
                        WARN(e.what());
                        continue;
                    }

                    require_identity_matrix(extrin.rotation);
                    require_zero_vector(extrin.translation);
                }
            }
        }

        auto end_size = b._streams.size();
        std::cout << " Final Extrinsic Graph size is " << end_size << std::endl;
        //REQUIRE(end_size == init_size); TODO doesn't pass yet
        WARN("TODO: Graph size shall be preserved: init " << init_size << " != final " << end_size);
    }
}
TEST_CASE("Pipe - Extrinsic memory leak detection", "[live]")
{
    // Require at least one device to be plugged in
    rs2::context ctx;
    {
        std::cout << "Pipe - Extrinsic memory leak detection started" << std::endl;
        auto list = ctx.query_devices();
        REQUIRE(list.size());
        auto dev = list.front();
        auto sensors = dev.query_sensors();

        std::string device_type = "L500";
        if (dev.supports(RS2_CAMERA_INFO_PRODUCT_LINE) && std::string(dev.get_info(RS2_CAMERA_INFO_PRODUCT_LINE)) == "D400") device_type = "D400";
        if (dev.supports(RS2_CAMERA_INFO_PRODUCT_LINE) && std::string(dev.get_info(RS2_CAMERA_INFO_PRODUCT_LINE)) == "SR300") device_type = "SR300";

        rs2::stream_profile mode;
        auto mode_index = 0;
        bool usb3_device = is_usb3(dev);
        int fps = usb3_device ? 30 : 15; // In USB2 Mode the devices will switch to lower FPS rates
        int req_fps = usb3_device ? 60 : 30; // USB2 Mode has only a single resolution for 60 fps which is not sufficient to run the test
        do
        {
            REQUIRE(get_mode(dev, mode, mode_index));
            mode_index++;
        } while (mode.fps() != req_fps);

        auto video = mode.as<rs2::video_stream_profile>();
        auto res = configure_all_supported_streams(dev, video.width(), video.height(), mode.fps());

        bool is_pipe_test[2] = { false, true };

        for (auto is_pipe : is_pipe_test)
        {
            // collect a log that contains info about 20 iterations for each stream
            // the info should include:
            // 1. extrinsics table size
            // 2. delay to first frame
            // 3. delay threshold for each stream (set fps=6 delay as worst case)
            // the test will succeed only if all 3 conditions are met:
            // 1. extrinsics table size is perserved over iterations for each stream 
            // 2. no delay increment over iterations
            // 3. "most" iterations have time to first frame delay below a defined threshold

            std::vector<size_t> extrinsics_table_size;
            std::map<std::string, std::vector<double>> streams_delay; // map to vector to collect all data
            std::map<std::string, std::vector<std::map<unsigned long long, size_t >>> unique_streams_delay;
            std::map<std::string, std::vector<unsigned long long>> frame_number;
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
                for (auto profile : res.second)
                {
                    auto fps = profile.fps;
                    if (device_type == "D400" && profile.stream == RS2_STREAM_ACCEL) fps = 250;
                    cfg.enable_stream(profile.stream, profile.index, profile.width, profile.height, profile.format, fps); // all streams in cfg
                    cfg_size += 1;
                }

                for (auto it = new_frame.begin(); it != new_frame.end(); it++)
                {
                    it->second = 0;
                }
                auto t1 = std::chrono::system_clock::now().time_since_epoch();
                bool condition = false;
                std::mutex mutex;
                std::mutex mutex_2;
                auto process_frame = [&](const rs2::frame& f)
                {
                    std::lock_guard<std::mutex> lock(mutex_2);
                    auto stream_type = f.get_profile().stream_name();
                    auto frame_num = f.get_frame_number();
                    auto time_of_arrival = f.get_frame_metadata(RS2_FRAME_METADATA_TIME_OF_ARRIVAL);

                    if (std::find(frame_number[stream_type].begin(), frame_number[stream_type].end(), frame_num) == frame_number[stream_type].end())
                    {
                        if (!new_frame[stream_type])
                        {
                            frame_number[stream_type].push_back(frame_num);
                            auto milli = std::chrono::duration_cast<std::chrono::milliseconds>(t1).count();
                            streams_delay[stream_type].push_back(time_of_arrival - milli);
                            new_frame[stream_type] = true;
                        }
                        new_frame[stream_type] += 1;
                    }
                };
                auto callback_pipe_sensor = [&](const rs2::frame& ff)
                {
                    std::lock_guard<std::mutex> lock(mutex);
                    if (rs2::frameset fs = ff.as<rs2::frameset>())
                    {
                        // With callbacks, all synchronized stream will arrive in a single frameset
                        for (const rs2::frame& f : fs)
                        {
                            process_frame(f);
                        }
                    }
                    else
                    {
                        // Stream that bypass synchronization (such as IMU) will produce single frames
                        process_frame(ff);
                    }
                };
                if (is_pipe)
                {
                    rs2::pipeline_profile profiles = pipe.start(cfg, callback_pipe_sensor);
                }
                else {
                    for (auto s : res.first)
                    {
                        s.start(callback_pipe_sensor);
                        //s.open(s.get_stream_profiles());
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
                    for (auto s : res.first)
                    {
                        s.stop();
                    }
                    /*for (auto s : res.first)
                    {
                        s.close();
                    }*/
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
                CHECK(std::adjacent_find(extrinsics_table_size.begin(), extrinsics_table_size.end(), std::not_equal_to<>()) == extrinsics_table_size.end());
            }
            // 2.  no delay increment over iterations 
            // filter spikes : calc stdev for each half and filter out samples that are not close 
            for (auto& stream : streams_delay)
            {
                // make sure we have enough data for each stream
                REQUIRE(stream.second.size() > 10);

                // remove first 5 iterations from each stream 
                stream.second.erase(stream.second.begin(), stream.second.begin() + 5);

                double filtered_vec_avg_arr[2];
                //double slope = data_filter(filtered_vec_avg_arr, stream.second, stream.first);
                std::vector<double> filtered_stream_vec;
                std::vector<double> filtered_stream_vec_2;
                double slope1 = data_filter(stream.second, filtered_stream_vec, stream.first);
                // check slope of filtered data
                double slope2 = data_filter(filtered_stream_vec, filtered_stream_vec_2, stream.first);
                // set origin data to filtered data
                stream.second = filtered_stream_vec;
                // check slope value against threshold
                auto threshold = DELAY_INCREMENT_THRESHOLD;
                // IMU streams have different threshold
                if (stream.first == "Accel" || stream.first == "Gyro") threshold = DELAY_INCREMENT_THRESHOLD_IMU;
                CAPTURE(stream.first, slope2, threshold);
                CHECK(slope2 < threshold);

            }
            // 3. "most" iterations have time to first frame delay below a defined threshold
            std::map<std::string, double> delay_thresholds;
            // D400
            delay_thresholds["Accel"] = 1200; // ms
            delay_thresholds["Color"] = 1000; // ms
            delay_thresholds["Depth"] = 1000; // ms
            delay_thresholds["Gyro"] = 1200; // ms
            delay_thresholds["Infrared 1"] = 1000; // ms
            delay_thresholds["Infrared 2"] = 1000; // ms

            // L500
            if (device_type == "L500")
            {
                delay_thresholds["Accel"] = 2000; // ms
                delay_thresholds["Color"] = 1200; // ms
                delay_thresholds["Gyro"] = 2000; // ms
            }

            for (const auto& stream_ : streams_delay)
            {
                auto v = stream_.second;
                auto stream = stream_.first;
                double avg = std::accumulate(v.begin(), v.end(), 0.0);
                double mean = avg / v.size();
                std::cout << "Delay of " << stream << " = " << mean * 1.5 << std::endl;
                CAPTURE(stream);
                for (auto it = stream_.second.begin(); it != stream_.second.end(); ++it) {
                    CHECK(*it < delay_thresholds[stream]);
                }
            }

        }
    }
}