// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This set of tests is valid for any number and combination of RealSense cameras, including R200 and F200 //
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "unit-tests-common.h"
#include "unit-tests-post-processing.h"
#include "../include/librealsense2/rs_advanced_mode.hpp"
#include <librealsense2/hpp/rs_frame.hpp>
#include <cmath>
#include <iostream>
#include <chrono>
#include <ctime>
#include <algorithm>


# define SECTION_FROM_TEST_NAME space_to_underscore(Catch::getCurrentContext().getResultCapture()->getCurrentTestName()).c_str()

class post_processing_filters
{
public:
    post_processing_filters(void) : depth_to_disparity(true),disparity_to_depth(false) {};
    ~post_processing_filters() noexcept {};

    void configure(const ppf_test_config& filters_cfg);
    rs2::frame process(rs2::frame input_frame);

private:
    post_processing_filters(const post_processing_filters& other);
    post_processing_filters(post_processing_filters&& other);

    // Declare filters
    rs2::decimation_filter  dec_filter;     // Decimation - frame downsampling using median filter
    rs2::spatial_filter     spat_filter;    // Spatial    - edge-preserving spatial smoothing
    rs2::temporal_filter    temp_filter;    // Temporal   - reduces temporal noise
    rs2::hole_filling_filter hole_filling_filter; // try reconstruct the missing data

    // Declare disparity transform from depth to disparity and vice versa
    rs2::disparity_transform depth_to_disparity;
    rs2::disparity_transform disparity_to_depth;

    bool dec_pb = false;
    bool spat_pb = false;
    bool temp_pb = false;
    bool holes_pb = false;

};

void post_processing_filters::configure(const ppf_test_config& filters_cfg)
{
    // Reconfigure the post-processing according to the test spec
    dec_pb = (filters_cfg.downsample_scale != 1);
    dec_filter.set_option(RS2_OPTION_FILTER_MAGNITUDE, (float)filters_cfg.downsample_scale);

    if (spat_pb = filters_cfg.spatial_filter)
    {
        spat_filter.set_option(RS2_OPTION_FILTER_SMOOTH_ALPHA, filters_cfg.spatial_alpha);
        spat_filter.set_option(RS2_OPTION_FILTER_SMOOTH_DELTA, filters_cfg.spatial_delta);
        spat_filter.set_option(RS2_OPTION_FILTER_MAGNITUDE, (float)filters_cfg.spatial_iterations);
        //spat_filter.set_option(RS2_OPTION_HOLES_FILL, filters_cfg.holes_filling_mode);      // Currently disabled
    }

    if (temp_pb = filters_cfg.temporal_filter)
    {
        temp_filter.set_option(RS2_OPTION_FILTER_SMOOTH_ALPHA, filters_cfg.temporal_alpha);
        temp_filter.set_option(RS2_OPTION_FILTER_SMOOTH_DELTA, filters_cfg.temporal_delta);
        temp_filter.set_option(RS2_OPTION_HOLES_FILL, filters_cfg.temporal_persistence);
    }

    if (holes_pb = filters_cfg.holes_filter)
    {
        hole_filling_filter.set_option(RS2_OPTION_HOLES_FILL, filters_cfg.holes_filling_mode);
    }
}

rs2::frame post_processing_filters::process(rs2::frame input)
{
    auto processed = input;

    // The filters are applied in the order mandated by reference design to enable byte-by-byte results verification
    // Decimation -> Depth2Disparity -> Spatial ->Temporal -> Disparity2Depth -> HolesFilling

    if (dec_pb)
        processed = dec_filter.process(processed);

    // Domain transform is mandatory according to the reference design
    processed = depth_to_disparity.process(processed);

    if (spat_pb)
        processed = spat_filter.process(processed);

    if (temp_pb)
        processed = temp_filter.process(processed);

    processed= disparity_to_depth.process(processed);

    if (holes_pb)
        processed = hole_filling_filter.process(processed);

    return processed;
}

bool validate_ppf_results(rs2::frame origin_depth, rs2::frame result_depth, const ppf_test_config& reference_data, size_t frame_idx)
{
    std::vector<uint16_t> diff2orig;
    std::vector<uint16_t> diff2ref;

    // Basic sanity scenario with no filters applied.
    // validating domain transform in/out conversion. Requiring input=output
    bool domain_transform_only = (reference_data.downsample_scale == 1) &&
        (!reference_data.spatial_filter) && (!reference_data.temporal_filter);

    auto result_profile = result_depth.get_profile().as<rs2::video_stream_profile>();
    REQUIRE(result_profile);
    CAPTURE(result_profile.width());
    CAPTURE(result_profile.height());

    REQUIRE(result_profile.width() == reference_data.output_res_x);
    REQUIRE(result_profile.height() == reference_data.output_res_y);

    auto pixels = result_profile.width()*result_profile.height();
    diff2ref.resize(pixels);
    if (domain_transform_only)
        diff2orig.resize(pixels);

    // Pixel-by-pixel comparison of the resulted filtered depth vs data ercorded with external tool
    auto v1 = reinterpret_cast<const uint16_t*>(result_depth.get_data());
    auto v2 = reinterpret_cast<const uint16_t*>(reference_data._output_frames[frame_idx].data());

    for (auto i = 0; i < pixels; i++)
    {
        uint16_t diff = std::abs(*v1++ - *v2++);
        diff2ref[i] = diff;
    }

    // validating depth<->disparity domain transformation is lostless.
    if (domain_transform_only)
        REQUIRE(profile_diffs("./DomainTransform.txt",diff2orig, 0, 0, frame_idx));

    // Validate the filters
    // The differences between the reference code and librealsense implementation are byte-compared below
    return profile_diffs("./Filterstransform.txt", diff2ref, 0.f, 0, frame_idx);
}

// The test is intended to check the results of filters applied on a sequence of frames, specifically the temporal filter
// that preserves an internal state. The test utilizes rosbag recordings
TEST_CASE("Post-Processing Filters sequence validation", "[software-device][post-processing-filters]")
{
    rs2::context ctx;

    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        // Test file name  , Filters configuraiton
        const std::vector< std::pair<std::string, std::string>> ppf_test_cases = {
            // All the tests below include depth-disparity domain transformation
            // Downsample scales 2/3
            { "1525186403504",  "D415_DS(2)" },
            { "1525186407536",  "D415_DS(3)" },
            // Downsample + Hole-Filling modes 0/1/2
            { "1525072818314",  "D415_DS(1)_HoleFill(0)" },
            { "1525072823227",  "D415_DS(1)_HoleFill(1)" },
            { "1524668713358",  "D435_DS(3)_HoleFill(2)" },
            // Downsample + Spatial Filter parameters
            { "1525267760676",  "D415_DS(2)+Spat(A:0.85/D:32/I:3)" },
            // Downsample + Temporal Filter
            { "1525266028697",  "D415_DS(2)+Temp(A:0.25/D:15/P:1)" },
            { "1525265554250",  "D415_DS(2)+Temp(A:0.25/D:15/P:3)" },
            { "1525266069476",  "D415_DS(2)+Temp(A:0.25/D:15/P:5)" },
            { "1525266120520",  "D415_DS(3)+Temp(A:0.25/D:15/P:7)" },
            // Downsample + Spatial + Temporal (+ Hole-Filling)
            { "1525267168585",  "D415_DS(2)_Spat(A:0.85/D:32/I:3)_Temp(A:0.25/D:15/P:0)" },
            { "1525089539880",  "D415_DS(2)_Spat(A:0.85/D:32/I:3)_Temp(A:0.25/D:15/P:0)_HoleFill(1)" },
        };

        ppf_test_config test_cfg;

        for (auto& ppf_test : ppf_test_cases)
        {
            CAPTURE(ppf_test.first);
            CAPTURE(ppf_test.second);

            WARN("PPF test " << ppf_test.first << "[" << ppf_test.second << "]");

            // Load the data from configuration and raw frame files
            if (!load_test_configuration(ppf_test.first, test_cfg))
                continue;

            post_processing_filters ppf;

            // Apply the retrieved configuration onto a local post-processing chain of filters
            REQUIRE_NOTHROW(ppf.configure(test_cfg));

            rs2::software_device dev; // Create software-only device
            auto depth_sensor = dev.add_sensor("Depth");

            int width = test_cfg.input_res_x;
            int height = test_cfg.input_res_y;
            int depth_bpp = 2; //16bit unsigned
            int frame_number = 1;
            rs2_intrinsics depth_intrinsics = { width, height,
                width / 2.f, height / 2.f,                      // Principal point (N/A in this test)
                test_cfg.focal_length ,test_cfg.focal_length,   // Focal Length
                RS2_DISTORTION_BROWN_CONRADY ,{ 0,0,0,0,0 } };

            auto depth_stream_profile = depth_sensor.add_video_stream({ RS2_STREAM_DEPTH, 0, 0, width, height, 30, depth_bpp, RS2_FORMAT_Z16, depth_intrinsics });
            depth_sensor.add_read_only_option(RS2_OPTION_DEPTH_UNITS, test_cfg.depth_units);
            depth_sensor.add_read_only_option(RS2_OPTION_STEREO_BASELINE, test_cfg.stereo_baseline);

            // Establish the required chain of filters
            dev.create_matcher(RS2_MATCHER_DLR_C);
            rs2::syncer sync;

            depth_sensor.open(depth_stream_profile);
            depth_sensor.start(sync);

            size_t frames = (test_cfg.frames_sequence_size > 1) ? test_cfg.frames_sequence_size : 1;
            for (auto i = 0; i < frames; i++)
            {
                // Inject input frame
                depth_sensor.on_video_frame({ test_cfg._input_frames[i].data(), // Frame pixels from capture API
                    [](void*) {},                   // Custom deleter (if required)
                    (int)test_cfg.input_res_x *depth_bpp,    // Stride
                    depth_bpp,                          // Bytes-per-pixels
                    (rs2_time_t)frame_number + i,      // Timestamp
                    RS2_TIMESTAMP_DOMAIN_SYSTEM_TIME,   // Clock Domain
                    frame_number,                       // Frame# for potential sync services
                    depth_stream_profile });            // Depth stream profile

                rs2::frameset fset = sync.wait_for_frames();
                REQUIRE(fset);
                rs2::frame depth = fset.first_or_default(RS2_STREAM_DEPTH);
                REQUIRE(depth);

                // ... here the actual filters are being applied
                auto filtered_depth = ppf.process(depth);

                // Compare the resulted frame versus input
                validate_ppf_results(depth, filtered_depth, test_cfg, i);
            }
        }
    }
}

