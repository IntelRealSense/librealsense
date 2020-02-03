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

    if ((spat_pb = filters_cfg.spatial_filter))
    {
        spat_filter.set_option(RS2_OPTION_FILTER_SMOOTH_ALPHA, filters_cfg.spatial_alpha);
        spat_filter.set_option(RS2_OPTION_FILTER_SMOOTH_DELTA, filters_cfg.spatial_delta);
        spat_filter.set_option(RS2_OPTION_FILTER_MAGNITUDE, (float)filters_cfg.spatial_iterations);
        //spat_filter.set_option(RS2_OPTION_HOLES_FILL, filters_cfg.holes_filling_mode);      // Currently disabled
    }

    if ((temp_pb = filters_cfg.temporal_filter))
    {
        temp_filter.set_option(RS2_OPTION_FILTER_SMOOTH_ALPHA, filters_cfg.temporal_alpha);
        temp_filter.set_option(RS2_OPTION_FILTER_SMOOTH_DELTA, filters_cfg.temporal_delta);
        temp_filter.set_option(RS2_OPTION_HOLES_FILL, filters_cfg.temporal_persistence);
    }

    if ((holes_pb = filters_cfg.holes_filter))
    {
        hole_filling_filter.set_option(RS2_OPTION_HOLES_FILL, float(filters_cfg.holes_filling_mode));
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

void compare_frame_md(rs2::frame origin_depth, rs2::frame result_depth)
{
    for (auto i = 0; i < rs2_frame_metadata_value::RS2_FRAME_METADATA_COUNT; i++)
    {
        bool origin_supported = origin_depth.supports_frame_metadata((rs2_frame_metadata_value)i);
        bool result_supported = result_depth.supports_frame_metadata((rs2_frame_metadata_value)i);
        REQUIRE(origin_supported == result_supported);
        if (origin_supported && result_supported)
        {
            //FRAME_TIMESTAMP and SENSOR_TIMESTAMP metadatas are not included in post proccesing frames,
            //TIME_OF_ARRIVAL continues to increase  after post proccesing
            if (i == RS2_FRAME_METADATA_FRAME_TIMESTAMP ||
                i == RS2_FRAME_METADATA_SENSOR_TIMESTAMP ||
                i == RS2_FRAME_METADATA_TIME_OF_ARRIVAL) continue;
            rs2_metadata_type origin_val = origin_depth.get_frame_metadata((rs2_frame_metadata_value)i);
            rs2_metadata_type result_val = result_depth.get_frame_metadata((rs2_frame_metadata_value)i);
            REQUIRE(origin_val == result_val);
        }
    }
}

// Test file name  , Filters configuraiton
const std::vector< std::pair<std::string, std::string>> ppf_test_cases = {
    // All the tests below include depth-disparity domain transformation
    // Downsample scales 2 and 3 are tested only. scales 4-7 are differ in impementation from the reference code:
    // In Librealsense for all scales [2-8] the filter is the mean of depth.
    // I the reference code for [2-3] the filter uses mean of depth, and for 4-7 is switches to man of disparities doe to implementation constrains
    {"1551257764229", "D435_DS(2)"},
    {"1551257812956", "D435_DS(3)"},
    // Downsample + Hole-Filling modes 0/1/2
    { "1551257880762","D435_DS(2)_HoleFill(0)" },
    { "1551257882796","D435_DS(2)_HoleFill(1)" },
    { "1551257884097","D435_DS(2)_HoleFill(2)" },
    // Downsample + Spatial Filter parameters
    { "1551257987255",  "D435_DS(2)+Spat(A:0.85/D:32/I:3)" },
    { "1551259481873",  "D435_DS(2)+Spat(A:0.5/D:15/I:2)" },
// Downsample + Temporal Filter
{ "1551261946511",  "D435_DS(2)+Temp(A:0.25/D:15/P:0)" },
{ "1551262153516",  "D435_DS(2)+Temp(A:0.45/D:25/P:1)" },
{ "1551262256875",  "D435_DS(2)+Temp(A:0.5/D:30/P:4)" },
{ "1551262841203",  "D435_DS(2)+Temp(A:0.5/D:30/P:6)" },
{ "1551262772964",  "D435_DS(2)+Temp(A:0.5/D:30/P:8)" },
// Downsample + Spatial + Temporal
{ "1551262971309",  "D435_DS(2)_Spat(A:0.7/D:25/I:2)_Temp(A:0.6/D:15/P:6)" },
// Downsample + Spatial + Temporal (+ Hole-Filling)
{ "1551263177558",  "D435_DS(2)_Spat(A:0.7/D:25/I:2)_Temp(A:0.6/D:15/P:6))_HoleFill(1)" },
};

// The test is intended to check the results of filters applied on a sequence of frames, specifically the temporal filter
// that preserves an internal state. The test utilizes rosbag recordings
TEST_CASE("Post-Processing Filters sequence validation", "[software-device][post-processing-filters]")
{
    rs2::context ctx;

    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
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
            depth_sensor.add_read_only_option(RS2_OPTION_STEREO_BASELINE, test_cfg.stereo_baseline_mm);

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

TEST_CASE("Post-Processing Filters metadata validation", "[software-device][post-processing-filters]")
{
    rs2::context ctx;

    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
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

            // Establish the required chain of filters
            dev.create_matcher(RS2_MATCHER_DLR_C);
            rs2::syncer sync;

            depth_sensor.open(depth_stream_profile);
            depth_sensor.start(sync);

            size_t frames = (test_cfg.frames_sequence_size > 1) ? test_cfg.frames_sequence_size : 1;
            for (auto i = 0; i < frames; i++)
            {
                //set next frames metadata
                for (auto i = 0; i < rs2_frame_metadata_value::RS2_FRAME_METADATA_COUNT; i++)
                    depth_sensor.set_metadata((rs2_frame_metadata_value)i, rand());

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

                // Compare the resulted frame metadata versus input
                compare_frame_md(depth, filtered_depth);
            }
        }
    }
}

bool is_subset(rs2::frameset full, rs2::frameset sub)
{
    if (!sub.is<rs2::frameset>())
        return false;
    if (full.size() == 0 && sub.size() == 0)
        return false;
    for (auto f : full)
    {
        if (!sub.first(f.get_profile().stream_type(), f.get_profile().format()))
            return false;
    }
    return true;
}

bool is_equal(rs2::frameset org, rs2::frameset processed)
{
    if (!org.is<rs2::frameset>() || !processed.is<rs2::frameset>())
        return false;
    if (org.size() != processed.size() || org.size() == 0)
        return false;
    for (auto o : org)
    {
        auto curr_profile = o.get_profile();
        bool found = false;
        processed.foreach_rs([&curr_profile, &found](const rs2::frame& f)
        {
            auto processed_profile = f.get_profile();
            if (curr_profile.unique_id() == processed_profile.unique_id())
                found = true;
        });
        if(!found)
            return false;
    }
    return true;
}

TEST_CASE("Post-Processing expected output", "[post-processing-filters]")
{
    rs2::context ctx;

    if (!make_context(SECTION_FROM_TEST_NAME, &ctx))
        return;

    rs2::temporal_filter temporal;
    rs2::hole_filling_filter hole_filling;
    rs2::spatial_filter spatial;
    rs2::decimation_filter decimation(4);
    rs2::align aligner(RS2_STREAM_COLOR);
    rs2::colorizer depth_colorizer;
    rs2::disparity_transform to_disp;
    rs2::disparity_transform from_disp(false);

    rs2::config cfg;
    cfg.enable_all_streams();

    rs2::pipeline pipe(ctx);
    auto profile = pipe.start(cfg);

    bool supports_disparity = false;
    for (auto s : profile.get_device().query_sensors())
    {
        if (s.supports(RS2_OPTION_STEREO_BASELINE))
        {
            supports_disparity = true;
            break;
        }
    }

    rs2::frameset original = pipe.wait_for_frames();

    //set to set
    rs2::frameset temp_processed_set = original.apply_filter(temporal);
    REQUIRE(is_subset(original, temp_processed_set));
    REQUIRE(is_subset(temp_processed_set, original));

    rs2::frameset hole_processed_set = original.apply_filter(hole_filling);
    REQUIRE(is_subset(original, hole_processed_set));
    REQUIRE(is_subset(hole_processed_set, original));

    rs2::frameset spatial_processed_set = original.apply_filter(spatial);
    REQUIRE(is_subset(original, spatial_processed_set));
    REQUIRE(is_subset(spatial_processed_set, original));

    rs2::frameset decimation_processed_set = original.apply_filter(decimation);
    REQUIRE(is_subset(original, decimation_processed_set));
    REQUIRE(is_subset(decimation_processed_set, original));

    rs2::frameset align_processed_set = original.apply_filter(aligner);
    REQUIRE(is_subset(original, align_processed_set));
    REQUIRE(is_subset(align_processed_set, original));

    rs2::frameset colorizer_processed_set = original.apply_filter(depth_colorizer);
    REQUIRE(is_subset(original, colorizer_processed_set));
    REQUIRE_THROWS(is_subset(colorizer_processed_set, original));

    rs2::frameset to_disp_processed_set = original.apply_filter(to_disp);
    if(supports_disparity)
        REQUIRE_THROWS(is_subset(to_disp_processed_set, original));

    rs2::frameset from_disp_processed_set = original.apply_filter(from_disp);//should bypass
    REQUIRE(is_equal(original, from_disp_processed_set));

    //single to single
    rs2::video_frame org_depth = original.get_depth_frame();

    rs2::video_frame temp_processed_frame = org_depth.apply_filter(temporal);
    REQUIRE_FALSE(temp_processed_frame.is<rs2::frameset>());
    REQUIRE(temp_processed_frame.get_profile().stream_type() == RS2_STREAM_DEPTH);
    REQUIRE(temp_processed_frame.get_profile().format() == RS2_FORMAT_Z16);
    REQUIRE(org_depth.get_width() == temp_processed_frame.get_width());

    rs2::video_frame hole_processed_frame = org_depth.apply_filter(hole_filling);
    REQUIRE_FALSE(hole_processed_frame.is<rs2::frameset>());
    REQUIRE(hole_processed_frame.get_profile().stream_type() == RS2_STREAM_DEPTH);
    REQUIRE(hole_processed_frame.get_profile().format() == RS2_FORMAT_Z16);
    REQUIRE(org_depth.get_width() == hole_processed_frame.get_width());

    rs2::video_frame spatial_processed_frame = org_depth.apply_filter(spatial);
    REQUIRE_FALSE(spatial_processed_frame.is<rs2::frameset>());
    REQUIRE(spatial_processed_frame.get_profile().stream_type() == RS2_STREAM_DEPTH);
    REQUIRE(spatial_processed_frame.get_profile().format() == RS2_FORMAT_Z16);
    REQUIRE(org_depth.get_width() == spatial_processed_frame.get_width());

    rs2::video_frame decimation_processed_frame = org_depth.apply_filter(decimation);
    REQUIRE_FALSE(decimation_processed_frame.is<rs2::frameset>());
    REQUIRE(decimation_processed_frame.get_profile().stream_type() == RS2_STREAM_DEPTH);
    REQUIRE(decimation_processed_frame.get_profile().format() == RS2_FORMAT_Z16);
    REQUIRE(org_depth.get_width() > decimation_processed_frame.get_width());

    rs2::video_frame colorizer_processed_frame = org_depth.apply_filter(depth_colorizer);
    REQUIRE_FALSE(colorizer_processed_frame.is<rs2::frameset>());
    REQUIRE(colorizer_processed_frame.get_profile().stream_type() == RS2_STREAM_DEPTH);
    REQUIRE(colorizer_processed_frame.get_profile().format() == RS2_FORMAT_RGB8);
    REQUIRE(org_depth.get_width() == colorizer_processed_frame.get_width());

    rs2::video_frame to_disp_processed_frame = org_depth.apply_filter(to_disp);
    REQUIRE_FALSE(to_disp_processed_frame.is<rs2::frameset>());
    REQUIRE(to_disp_processed_frame.get_profile().stream_type() == RS2_STREAM_DEPTH);
    bool is_disp = to_disp_processed_frame.get_profile().format() == RS2_FORMAT_DISPARITY16 ||
        to_disp_processed_frame.get_profile().format() == RS2_FORMAT_DISPARITY32;
    if (supports_disparity)
    {
        REQUIRE(is_disp);
        REQUIRE(org_depth.get_width() == to_disp_processed_frame.get_width());
    }

    rs2::video_frame from_disp_processed_frame = org_depth.apply_filter(from_disp);//should bypass
    REQUIRE_FALSE(from_disp_processed_frame.is<rs2::frameset>());
    REQUIRE(from_disp_processed_frame.get_profile().stream_type() == RS2_STREAM_DEPTH);
    REQUIRE(from_disp_processed_frame.get_profile().format() == RS2_FORMAT_Z16);
    REQUIRE(org_depth.get_width() == from_disp_processed_frame.get_width());

    pipe.stop();
}

TEST_CASE("Post-Processing processing pipe", "[post-processing-filters]")
{
    rs2::context ctx;

    if (!make_context(SECTION_FROM_TEST_NAME, &ctx))
        return;

    rs2::temporal_filter temporal;
    rs2::hole_filling_filter hole_filling;
    rs2::spatial_filter spatial;
    rs2::decimation_filter decimation(4);
    rs2::align aligner(RS2_STREAM_COLOR);
    rs2::colorizer depth_colorizer;
    rs2::disparity_transform to_disp;
    rs2::disparity_transform from_disp(false);
    rs2::pointcloud pc(RS2_STREAM_DEPTH);

    rs2::config cfg;
    cfg.enable_all_streams();

    rs2::pipeline pipe(ctx);
    auto profile = pipe.start(cfg);

    bool supports_disparity = false;
    for (auto s : profile.get_device().query_sensors())
    {
        if (s.supports(RS2_OPTION_STEREO_BASELINE))
        {
            supports_disparity = true;
            break;
        }
    }

    rs2::frameset original = pipe.wait_for_frames();

    rs2::frameset full_pipe;
    int run_for = 10;
    std::set<int> uids;
    size_t uid_count = 0;
    while (run_for--)
    {
        full_pipe = pipe.wait_for_frames();
        full_pipe = full_pipe.apply_filter(decimation);
        full_pipe = full_pipe.apply_filter(to_disp);
        full_pipe = full_pipe.apply_filter(spatial);
        full_pipe = full_pipe.apply_filter(temporal);
        full_pipe = full_pipe.apply_filter(from_disp);
        full_pipe = full_pipe.apply_filter(aligner);
        full_pipe = full_pipe.apply_filter(hole_filling);
        full_pipe = full_pipe.apply_filter(depth_colorizer);
        full_pipe = full_pipe.apply_filter(pc);

        //printf("test frame:\n");
        full_pipe.foreach_rs([&](const rs2::frame& f) {
            uids.insert(f.get_profile().unique_id());
            //printf("stream: %s, format: %d, uid: %d\n", f.get_profile().stream_name().c_str(), f.get_profile().format(), f.get_profile().unique_id());
        });
        if (uid_count == 0)
            uid_count = uids.size();
        REQUIRE(uid_count == uids.size());
    }

    REQUIRE(is_subset(original, full_pipe));
    REQUIRE_THROWS(is_subset(full_pipe, original));
    pipe.stop();
}

TEST_CASE("Align Processing Block", "[live][pipeline][post-processing-filters][!mayfail]") {
    rs2::context ctx;

    if (make_context(SECTION_FROM_TEST_NAME, &ctx, "2.20.0"))
    {
        auto list = ctx.query_devices();
        REQUIRE(list.size());

        rs2::device dev;
        rs2::pipeline pipe(ctx);
        rs2::config cfg;
        rs2::pipeline_profile pipe_profile;

        REQUIRE_NOTHROW(cfg.enable_all_streams());
        REQUIRE_NOTHROW(pipe_profile = cfg.resolve(pipe));
        REQUIRE(pipe_profile);
        REQUIRE_NOTHROW(dev = pipe_profile.get_device());
        REQUIRE(dev);
        disable_sensitive_options_for(dev);
        dev_type PID = get_PID(dev);
        CAPTURE(PID.first);
        CAPTURE(PID.second);

        REQUIRE_NOTHROW(pipe_profile = pipe.start(cfg));
        REQUIRE(pipe_profile);

        std::vector<rs2::stream_profile> active_streams;
        REQUIRE_NOTHROW(active_streams = pipe_profile.get_streams());

        // Make sure that there is a frame for each stream opened
        rs2::frameset fs;
        for (auto i = 0; i < 300; i++)
        {
            if ((pipe.try_wait_for_frames(&fs,500)) && (fs.size() == active_streams.size()))
            {
                pipe.stop();
                break;
            }
        }

        // Sanity check and registration of all possible source and target streams for alignment process
        std::set<rs2_stream> streams_under_test;
        for (const auto &str_type : { RS2_STREAM_COLOR, RS2_STREAM_INFRARED, RS2_STREAM_FISHEYE, RS2_STREAM_CONFIDENCE })
        {
            // Currently there is no API to explicitely select target in presense of multiple candidates (IR2)
            if (auto fr = fs.first_or_default(str_type))
                streams_under_test.insert(str_type);
        }

        // Sanity check
        if (!fs.get_depth_frame() || !streams_under_test.size())
        {
            WARN("Align block test requires a device with Depth and Video sensor(s): current device "
                << "[" << PID.first << ":" <<  PID.second << "]. Test skipped");
            return;
        }

        // Check depth->{uvc} alignment.
        // Note that the test is for verification purposes and does not indicate quality of the mapping process
        WARN("Testing Depth aligned to 2D video stream");
        for (auto& tgt_stream : streams_under_test)
        {
            WARN("Testing Depth aligned to " << rs2_stream_to_string(tgt_stream));
            rs2::align align_pb(tgt_stream);
            auto aligned_fs = align_pb.process(fs);
            auto origin_depth_frame = fs.get_depth_frame();
            auto aligned_depth_frame = aligned_fs.get_depth_frame();  // Depth frame is replaced by the aligned depth
            auto reference_frame = aligned_fs.first(tgt_stream);

            auto orig_dpt_profile = origin_depth_frame.get_profile();
            auto aligned_dpt_profile = aligned_depth_frame.get_profile();
            auto aligned_dpth_video_pf = aligned_dpt_profile.as<rs2::video_stream_profile>();
            auto ref_video_profile = reference_frame.get_profile().as<rs2::video_stream_profile>();

            // Test: the depth frame retains the core attributes of depth stream
            // Stream type/format/fps, (Depth units currently not available).
            // TODO solution for Baseline ???
            REQUIRE(orig_dpt_profile == aligned_dpt_profile);
            REQUIRE(orig_dpt_profile.unique_id() != aligned_dpt_profile.unique_id());

            // Test: the resulted depth frame properties correspond to the target
            // Resolution, Intrinsic, Extrinsic
            REQUIRE(aligned_dpth_video_pf.width() == ref_video_profile.width());
            REQUIRE(aligned_dpth_video_pf.height() == ref_video_profile.height());
            const auto ref_intr = ref_video_profile.get_intrinsics();
            const auto align_dpt_intr = aligned_dpth_video_pf.get_intrinsics();
            for (auto i = 0; i < 5; i++)
            {
                REQUIRE(ref_intr.coeffs[i] == Approx(align_dpt_intr.coeffs[i]));
            }
            REQUIRE(ref_intr.fx == Approx(align_dpt_intr.fx));
            REQUIRE(ref_intr.fy == Approx(align_dpt_intr.fy));
            REQUIRE(ref_intr.ppx == Approx(align_dpt_intr.ppx));
            REQUIRE(ref_intr.ppy == Approx(align_dpt_intr.ppy));
            REQUIRE(ref_intr.model == Approx(align_dpt_intr.model));
            REQUIRE(ref_intr.width == Approx(align_dpt_intr.width));
            REQUIRE(ref_intr.height == Approx(align_dpt_intr.height));

            // Extrinsic tests: Aligned_depth_extrinsic == Target frame extrinsic
            rs2_extrinsics actual_extrinsics = ref_video_profile.get_extrinsics_to(aligned_dpt_profile);
            rs2_extrinsics expected_extrinsics = { {1,0,0, 0,1,0, 0,0,1}, {0,0,0} };
            CAPTURE(actual_extrinsics.rotation);
            CAPTURE(actual_extrinsics.translation);
            for (auto i = 0; i < 9; i++)
            {
                REQUIRE(actual_extrinsics.rotation[i] == Approx(expected_extrinsics.rotation[i]));
            }
            for (auto i = 0; i < 3; i++)
            {
                REQUIRE(actual_extrinsics.translation[i] == Approx(expected_extrinsics.translation[i]));
            }
        }

        WARN("Testing 2D Video stream aligned to Depth sensor");
        // Check {uvc}->depth alignment.
        for (auto& tgt_stream : streams_under_test)
        {
            WARN("Testing " << rs2_stream_to_string(tgt_stream) << " aligned to Depth");
            rs2::align align_pb(RS2_STREAM_DEPTH);
            auto aligned_fs = align_pb.process(fs);
            auto origin_2D_frame = fs.first(tgt_stream);
            auto aligned_2D_frame = aligned_fs.first(tgt_stream);
            auto depth_frame = aligned_fs.get_depth_frame();

            auto orig_2D_profile = origin_2D_frame.get_profile();
            auto aligned_2D_profile = aligned_2D_frame.get_profile().as<rs2::video_stream_profile>();
            auto ref_video_profile = depth_frame.get_profile().as<rs2::video_stream_profile>();

            // Test: the 2D frame retains the core attributes of the original stream
            // Stream type/format/fps
            REQUIRE(orig_2D_profile == aligned_2D_profile);
            REQUIRE(orig_2D_profile.unique_id() != aligned_2D_profile.unique_id());

            // Test: the resulted 2D frame properties correspond to the target depth
            // Resolution, Intrinsic
            REQUIRE(aligned_2D_profile.width() == ref_video_profile.width());
            REQUIRE(aligned_2D_profile.height() == ref_video_profile.height());
            const auto ref_intr = ref_video_profile.get_intrinsics();
            const auto align_2D_intr = aligned_2D_profile.get_intrinsics();
            for (auto i = 0; i < 5; i++)
            {
                REQUIRE(ref_intr.coeffs[i] == Approx(align_2D_intr.coeffs[i]));
            }
            REQUIRE(ref_intr.fx == Approx(align_2D_intr.fx));
            REQUIRE(ref_intr.fy == Approx(align_2D_intr.fy));
            REQUIRE(ref_intr.ppx == Approx(align_2D_intr.ppx));
            REQUIRE(ref_intr.ppy == Approx(align_2D_intr.ppy));
            REQUIRE(ref_intr.model == Approx(align_2D_intr.model));
            REQUIRE(ref_intr.width == Approx(align_2D_intr.width));
            REQUIRE(ref_intr.height == Approx(align_2D_intr.height));

            // Extrinsic tests: Aligned_depth_extrinsic == Target frame extrinsic
            rs2_extrinsics actual_extrinsics = aligned_2D_profile.get_extrinsics_to(ref_video_profile);
            rs2_extrinsics expected_extrinsics = { {1,0,0, 0,1,0, 0,0,1}, {0,0,0} };
            CAPTURE(actual_extrinsics.rotation);
            CAPTURE(actual_extrinsics.translation);
            for (auto i = 0; i < 9; i++)
            {
                REQUIRE(actual_extrinsics.rotation[i] == Approx(expected_extrinsics.rotation[i]));
            }
            for (auto i = 0; i < 3; i++)
            {
                REQUIRE(actual_extrinsics.translation[i] == Approx(expected_extrinsics.translation[i]));
            }
        }
    }
}
