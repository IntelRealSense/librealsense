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
    rs2::decimation_filter  dec_filter;  // Decimation - reduces depth frame density
    rs2::spatial_filter     spat_filter;    // Spatial    - edge-preserving spatial smoothing
    rs2::temporal_filter    temp_filter;   // Temporal   - reduces temporal noise

    // Declare disparity transform from depth to disparity and vice versa
    rs2::disparity_transform depth_to_disparity;
    rs2::disparity_transform disparity_to_depth;

    bool dec_pb = false;
    bool spat_pb = false;
    bool temp_pb = false;

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
}

rs2::frame post_processing_filters::process(rs2::frame input)
{
    auto processed = input;

    // The filters are applied in the same order as recommended by the reference design
    // Decimation -> Depth2Disparity -> Spatial ->Temporal -> Disparity2Depth

    if (dec_pb)
        processed = dec_filter.process(processed);

    // Domain transform is mandatory according to the reference design
    processed = depth_to_disparity.process(processed);

    if (spat_pb)
        processed = spat_filter.process(processed);

    if (temp_pb)
        processed = temp_filter.process(processed);

    return  disparity_to_depth.process(processed);
}

bool validate_ppf_results(rs2::frame origin_depth, rs2::frame result_depth, const ppf_test_config& reference_data)
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
    auto v_src = reinterpret_cast<const uint16_t*>(origin_depth.get_data());
    auto v1 = reinterpret_cast<const uint16_t*>(result_depth.get_data());
    auto v2 = reinterpret_cast<const uint16_t*>(reference_data._output_data.data());

    for (auto i = 0; i < pixels; i++)
    {
        uint16_t diff = std::abs(*v1++ - *v2++);
        diff2ref[i] = diff;
    }

    // Basic sanity scenario with no filters applied.
    // validating domain transform in/out conversion.
    //if (domain_transform_only)
    //    profile_diffs("./DomainTransform.txt",diff2orig, 0, 0);

    return profile_diffs("./Filterstransform.txt", diff2ref, 0, 0);
}

TEST_CASE("Post-Processing Filters validation", "[software-device][post-processing-filters]") {
    rs2::context ctx;

    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        // Test file name  , Filters configuraiton
        const std::map< std::string, std::string> ppf_test_cases = {
            //{ "152342844",  "D415_Downsample2+Temp(Defaults)" },// Run with a "recursively"-generated source to neutralize the temporal history
            { "152346214",  "D415_Downsample_2+Spat(A=0.7,D=8,Iter=3)" },
            { "152347981",  "D415_Downsample_3+Spat(A=0.7,D=8,Iter=3)" },
            { "152347976",  "D415_Downsample_1+Spat(A=0.85,D=32,Iter=3)" },
            { "152347975",  "D415_Downsample_3+Spat(A=0.85,D=32,Iter=3)" },
            { "152347974",  "D415_Downsample_2+Spat(A=0.85,D=32,Iter=3)" },
            { "152348222",  "D415_Downsample_3+Spat(A=0.7,D=10,Iter=3)" },
            { "152348234",  "D415_Downsample_3+Spat(A=0.3,D=20,Iter=3)" },
            { "152348138",  "D415_Downsample_2" },
            { "152348137",  "D415_Downsample_3" },
            { "152348165",  "D415_Downsample_1" },
        };

        ppf_test_config test_cfg;

        for (auto& ppf_test : ppf_test_cases)
        {
            CAPTURE(ppf_test.first);
            CAPTURE(ppf_test.second);

            WARN("Testing pattern " << ppf_test.first << "," << ppf_test.second);
            //INTERNAL_CATCH_INFO(msg, "INFO")

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

            // Inject input frame
            depth_sensor.on_video_frame({ test_cfg._input_data.data(), // Frame pixels from capture API
                    [](void*) {},                   // Custom deleter (if required)
                (int)test_cfg.input_res_x *depth_bpp,    // Stride
                depth_bpp,                          // Bytes-per-pixels
                (rs2_time_t)frame_number * 16,      // Timestamp
                RS2_TIMESTAMP_DOMAIN_SYSTEM_TIME,   // Clock Domain 
                frame_number,                       // Frame# for potential sync services
                depth_stream_profile });            // Depth stream profile

            /// ... here the actual filters are being applied
            rs2::frameset fset = sync.wait_for_frames();
            REQUIRE(fset);
            rs2::frame depth = fset.first_or_default(RS2_STREAM_DEPTH);
            REQUIRE(depth);

            auto filtered_depth = ppf.process(depth);

            // Compare the resulted frame versus input
            REQUIRE_NOTHROW(validate_ppf_results(depth,filtered_depth, test_cfg));
        }
    }
}
