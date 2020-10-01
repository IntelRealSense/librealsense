// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

/////////////////////////////////////////////////////////////////////////////
// This set of tests is valid for any device that supports the HDR feature //
/////////////////////////////////////////////////////////////////////////////


#include "unit-tests-common.h"

using namespace rs2;

// HDR CONFIGURATION TESTS
TEST_CASE("HDR Config - default config", "[hdr][live]") 
{
    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        rs2::device_list devices_list = ctx.query_devices();
        size_t device_count = devices_list.size();
        REQUIRE(device_count > 0);

        rs2::depth_sensor depth_sensor = restart_first_device_and_return_depth_sensor(ctx, devices_list);

        if (depth_sensor && depth_sensor.supports(RS2_OPTION_SEQUENCE_ID) &&
            depth_sensor.supports(RS2_OPTION_HDR_ENABLED))
        {
            auto exposure_range = depth_sensor.get_option_range(RS2_OPTION_EXPOSURE);
            auto gain_range = depth_sensor.get_option_range(RS2_OPTION_GAIN);

            depth_sensor.set_option(RS2_OPTION_HDR_ENABLED, 1);
            REQUIRE(depth_sensor.get_option(RS2_OPTION_HDR_ENABLED) == 1.f);

            depth_sensor.set_option(RS2_OPTION_SEQUENCE_ID, 1);
            REQUIRE(depth_sensor.get_option(RS2_OPTION_SEQUENCE_ID) == 1.f);
            auto exp = depth_sensor.get_option(RS2_OPTION_EXPOSURE);
            REQUIRE(depth_sensor.get_option(RS2_OPTION_EXPOSURE) == exposure_range.def - 1000); //w/a
            REQUIRE(depth_sensor.get_option(RS2_OPTION_GAIN) == gain_range.def);

            depth_sensor.set_option(RS2_OPTION_SEQUENCE_ID, 2);
            REQUIRE(depth_sensor.get_option(RS2_OPTION_SEQUENCE_ID) == 2.f);
            REQUIRE(depth_sensor.get_option(RS2_OPTION_EXPOSURE) == exposure_range.min);
            REQUIRE(depth_sensor.get_option(RS2_OPTION_GAIN) == gain_range.min);

            depth_sensor.set_option(RS2_OPTION_HDR_ENABLED, 0);
            auto enabled = depth_sensor.get_option(RS2_OPTION_HDR_ENABLED);
            REQUIRE(depth_sensor.get_option(RS2_OPTION_HDR_ENABLED) == 0.f);
        }
    }
}

TEST_CASE("HDR Config - custom config", "[hdr][live]") 
{
    // Require at least one device to be plugged in
    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        rs2::device_list devices_list = ctx.query_devices();
        size_t device_count = devices_list.size();
        REQUIRE(device_count > 0);

        rs2::depth_sensor depth_sensor = restart_first_device_and_return_depth_sensor(ctx, devices_list);

        if (depth_sensor && depth_sensor.supports(RS2_OPTION_SEQUENCE_ID) &&
            depth_sensor.supports(RS2_OPTION_HDR_ENABLED))
        {
            depth_sensor.set_option(RS2_OPTION_SEQUENCE_SIZE, 2);
            REQUIRE(depth_sensor.get_option(RS2_OPTION_SEQUENCE_SIZE) == 2.f);

            depth_sensor.set_option(RS2_OPTION_SEQUENCE_ID, 1);
            REQUIRE(depth_sensor.get_option(RS2_OPTION_SEQUENCE_ID) == 1.f);
            depth_sensor.set_option(RS2_OPTION_EXPOSURE, 120.f);
            REQUIRE(depth_sensor.get_option(RS2_OPTION_EXPOSURE) == 120.f);
            depth_sensor.set_option(RS2_OPTION_GAIN, 90.f);
            REQUIRE(depth_sensor.get_option(RS2_OPTION_GAIN) == 90.f);


            depth_sensor.set_option(RS2_OPTION_SEQUENCE_ID, 2);
            REQUIRE(depth_sensor.get_option(RS2_OPTION_SEQUENCE_ID) == 2.f);
            depth_sensor.set_option(RS2_OPTION_EXPOSURE, 1200.f);
            REQUIRE(depth_sensor.get_option(RS2_OPTION_EXPOSURE) == 1200.f);
            depth_sensor.set_option(RS2_OPTION_GAIN, 20.f);
            REQUIRE(depth_sensor.get_option(RS2_OPTION_GAIN) == 20.f);

            depth_sensor.set_option(RS2_OPTION_HDR_ENABLED, 1);
            REQUIRE(depth_sensor.get_option(RS2_OPTION_HDR_ENABLED) == 1.f);

            depth_sensor.set_option(RS2_OPTION_HDR_ENABLED, 0);
            REQUIRE(depth_sensor.get_option(RS2_OPTION_HDR_ENABLED) == 0.f);
        }
    }
}

// HDR STREAMING TESTS
TEST_CASE("HDR Streaming - default config", "[hdr][live][using_pipeline]") 
{
    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        rs2::device_list devices_list = ctx.query_devices();
        size_t device_count = devices_list.size();
        REQUIRE(device_count > 0);

        rs2::depth_sensor depth_sensor = restart_first_device_and_return_depth_sensor(ctx, devices_list);

        if (depth_sensor && depth_sensor.supports(RS2_OPTION_SEQUENCE_ID) &&
            depth_sensor.supports(RS2_OPTION_HDR_ENABLED))
        {
            auto exposure_range = depth_sensor.get_option_range(RS2_OPTION_EXPOSURE);
            auto gain_range = depth_sensor.get_option_range(RS2_OPTION_GAIN);

            depth_sensor.set_option(RS2_OPTION_HDR_ENABLED, 1);
            REQUIRE(depth_sensor.get_option(RS2_OPTION_HDR_ENABLED) == 1.f);

            rs2::config cfg;
            cfg.enable_stream(RS2_STREAM_DEPTH);
            cfg.enable_stream(RS2_STREAM_INFRARED, 1);
            rs2::pipeline pipe;
            pipe.start(cfg);

            int iteration = 0;
            while (++iteration < 100)
            {
                rs2::frameset data = pipe.wait_for_frames();
                rs2::depth_frame out_depth_frame = data.get_depth_frame();

                if (iteration < 3)
                    continue;

                if (out_depth_frame.supports_frame_metadata(RS2_FRAME_METADATA_SEQUENCE_ID))
                {
                    long long frame_counter = out_depth_frame.get_frame_metadata(RS2_FRAME_METADATA_FRAME_COUNTER);
                    long long frame_exposure = out_depth_frame.get_frame_metadata(RS2_FRAME_METADATA_ACTUAL_EXPOSURE);
                    long long frame_gain = out_depth_frame.get_frame_metadata(RS2_FRAME_METADATA_GAIN_LEVEL);
                    auto seq_id = out_depth_frame.get_frame_metadata(RS2_FRAME_METADATA_SEQUENCE_ID);

                    if (seq_id == 0)
                    {
                        REQUIRE(frame_exposure == exposure_range.def - 1000.f); // w/a
                        REQUIRE(frame_gain == gain_range.def);
                    }
                    else
                    {
                        REQUIRE(frame_exposure == exposure_range.min);
                        REQUIRE(frame_gain == gain_range.min);
                    }
                }
            }
        }
    }
}

TEST_CASE("HDR Streaming - custom config", "[hdr][live][using_pipeline]")
{
    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        rs2::device_list devices_list = ctx.query_devices();
        size_t device_count = devices_list.size();
        REQUIRE(device_count > 0);

        rs2::depth_sensor depth_sensor = restart_first_device_and_return_depth_sensor(ctx, devices_list);

        if (depth_sensor && depth_sensor.supports(RS2_OPTION_SEQUENCE_ID) &&
            depth_sensor.supports(RS2_OPTION_HDR_ENABLED))
        {
            depth_sensor.set_option(RS2_OPTION_SEQUENCE_SIZE, 2);
            REQUIRE(depth_sensor.get_option(RS2_OPTION_SEQUENCE_SIZE) == 2.f);

            auto first_exposure = 120.f;
            auto first_gain = 90.f;
            depth_sensor.set_option(RS2_OPTION_SEQUENCE_ID, 1);
            REQUIRE(depth_sensor.get_option(RS2_OPTION_SEQUENCE_ID) == 1.f);
            depth_sensor.set_option(RS2_OPTION_EXPOSURE, first_exposure);
            REQUIRE(depth_sensor.get_option(RS2_OPTION_EXPOSURE) == first_exposure);
            depth_sensor.set_option(RS2_OPTION_GAIN, first_gain);
            REQUIRE(depth_sensor.get_option(RS2_OPTION_GAIN) == first_gain);


            auto second_exposure = 1200.f;
            auto second_gain = 20.f;
            depth_sensor.set_option(RS2_OPTION_SEQUENCE_ID, 2);
            REQUIRE(depth_sensor.get_option(RS2_OPTION_SEQUENCE_ID) == 2.f);
            depth_sensor.set_option(RS2_OPTION_EXPOSURE, second_exposure);
            REQUIRE(depth_sensor.get_option(RS2_OPTION_EXPOSURE) == second_exposure);
            depth_sensor.set_option(RS2_OPTION_GAIN, second_gain);
            REQUIRE(depth_sensor.get_option(RS2_OPTION_GAIN) == second_gain);

            depth_sensor.set_option(RS2_OPTION_HDR_ENABLED, 1);
            REQUIRE(depth_sensor.get_option(RS2_OPTION_HDR_ENABLED) == 1.f);

            rs2::config cfg;
            cfg.enable_stream(RS2_STREAM_DEPTH);
            cfg.enable_stream(RS2_STREAM_INFRARED, 1);
            rs2::pipeline pipe;
            pipe.start(cfg);

            int iteration = 0;
            while (++iteration < 100) 
            {
                rs2::frameset data = pipe.wait_for_frames();
                rs2::depth_frame out_depth_frame = data.get_depth_frame();

                if (iteration < 3)
                    continue;

                if (out_depth_frame.supports_frame_metadata(RS2_FRAME_METADATA_SEQUENCE_ID))
                {
                    long long frame_counter = out_depth_frame.get_frame_metadata(RS2_FRAME_METADATA_FRAME_COUNTER);
                    long long frame_exposure = out_depth_frame.get_frame_metadata(RS2_FRAME_METADATA_ACTUAL_EXPOSURE);
                    long long frame_gain = out_depth_frame.get_frame_metadata(RS2_FRAME_METADATA_GAIN_LEVEL);
                    auto seq_id = out_depth_frame.get_frame_metadata(RS2_FRAME_METADATA_SEQUENCE_ID);

                    if (seq_id == 0)
                    {
                        REQUIRE(frame_exposure == first_exposure);
                        REQUIRE(frame_gain == first_gain);
                    }
                    else
                    {
                        REQUIRE(frame_exposure == second_exposure);
                        REQUIRE(frame_gain == second_gain);
                    }
                }
            }
        }
    }
}

// HDR CHANGING CONFIG WHILE STREAMING TESTS
TEST_CASE("HDR Config while Streaming", "[hdr][live][using_pipeline]")
{
    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        rs2::device_list devices_list = ctx.query_devices();
        size_t device_count = devices_list.size();
        REQUIRE(device_count > 0);

        rs2::depth_sensor depth_sensor = restart_first_device_and_return_depth_sensor(ctx, devices_list);

        if (depth_sensor && depth_sensor.supports(RS2_OPTION_SEQUENCE_ID) &&
            depth_sensor.supports(RS2_OPTION_HDR_ENABLED))
        {
            auto exposure_range = depth_sensor.get_option_range(RS2_OPTION_EXPOSURE);
            auto gain_range = depth_sensor.get_option_range(RS2_OPTION_GAIN);

            depth_sensor.set_option(RS2_OPTION_HDR_ENABLED, 1);
            REQUIRE(depth_sensor.get_option(RS2_OPTION_HDR_ENABLED) == 1.f);

            rs2::config cfg;
            cfg.enable_stream(RS2_STREAM_DEPTH);
            cfg.enable_stream(RS2_STREAM_INFRARED, 1);
            rs2::pipeline pipe;
            pipe.start(cfg);

            auto required_exposure = exposure_range.def - 1000; // w/a
            auto required_gain = gain_range.def;
            auto changed_exposure = 33000.f;
            auto changed_gain = 150.f;

            int iteration = 0;
            while (++iteration < 100)
            {
                rs2::frameset data = pipe.wait_for_frames();
                rs2::depth_frame out_depth_frame = data.get_depth_frame();

                if (iteration < 3 || (iteration > 30 && iteration < 36))
                    continue;

                if (iteration == 30)
                {
                    depth_sensor.set_option(RS2_OPTION_SEQUENCE_ID, 1);
                    REQUIRE(depth_sensor.get_option(RS2_OPTION_SEQUENCE_ID) == 1.f);
                    depth_sensor.set_option(RS2_OPTION_EXPOSURE, changed_exposure);
                    REQUIRE(depth_sensor.get_option(RS2_OPTION_EXPOSURE) == changed_exposure);
                    depth_sensor.set_option(RS2_OPTION_GAIN, changed_gain);
                    REQUIRE(depth_sensor.get_option(RS2_OPTION_GAIN) == changed_gain);
                    required_exposure = changed_exposure;
                    required_gain = changed_gain;
                    continue;
                }

                if (out_depth_frame.supports_frame_metadata(RS2_FRAME_METADATA_SEQUENCE_ID))
                {
                    long long frame_counter = out_depth_frame.get_frame_metadata(RS2_FRAME_METADATA_FRAME_COUNTER);
                    long long frame_exposure = out_depth_frame.get_frame_metadata(RS2_FRAME_METADATA_ACTUAL_EXPOSURE);
                    long long frame_gain = out_depth_frame.get_frame_metadata(RS2_FRAME_METADATA_GAIN_LEVEL);
                    auto seq_id = out_depth_frame.get_frame_metadata(RS2_FRAME_METADATA_SEQUENCE_ID);


                    if (seq_id == 0)
                    {
                        REQUIRE(frame_exposure == required_exposure);
                        REQUIRE(frame_gain == required_gain);
                    }
                    else
                    {
                        REQUIRE(frame_exposure == exposure_range.min);
                        REQUIRE(frame_gain == gain_range.min);
                    }
                }
            }
        }
    }
}

// CHECKING HDR AFTER PIPE RESTART
TEST_CASE("HDR Running - restart hdr at restream", "[hdr][live][using_pipeline]")
{
    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        rs2::device_list devices_list = ctx.query_devices();
        size_t device_count = devices_list.size();
        REQUIRE(device_count > 0);

        rs2::depth_sensor depth_sensor = restart_first_device_and_return_depth_sensor(ctx, devices_list);

        rs2::config cfg;
        cfg.enable_stream(RS2_STREAM_DEPTH);
        rs2::pipeline pipe;
        pipe.start(cfg);

        depth_sensor.set_option(RS2_OPTION_HDR_ENABLED, 1);
        REQUIRE(depth_sensor.get_option(RS2_OPTION_HDR_ENABLED) == 1.f);

        for (int i = 0; i < 10; ++i)
        {
            rs2::frameset data = pipe.wait_for_frames();
        }
        REQUIRE(depth_sensor.get_option(RS2_OPTION_HDR_ENABLED) == 1.f);
        pipe.stop();
        //std::cout << "------------------stop - start again ---------------" << std::endl;
        pipe.start(cfg);
        REQUIRE(depth_sensor.get_option(RS2_OPTION_HDR_ENABLED) == 1.f);
        pipe.stop();
    }
}

// CHECKING SEQUENCE ID WHILE STREAMING
TEST_CASE("HDR Streaming - checking sequence id", "[hdr][live][using_pipeline]")
{
    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        rs2::device_list devices_list = ctx.query_devices();
        size_t device_count = devices_list.size();
        REQUIRE(device_count > 0);

        rs2::depth_sensor depth_sensor = restart_first_device_and_return_depth_sensor(ctx, devices_list);

        rs2::config cfg;
        cfg.enable_stream(RS2_STREAM_DEPTH);
        cfg.enable_stream(RS2_STREAM_INFRARED, 1);
        rs2::pipeline pipe;
        pipe.start(cfg);

        depth_sensor.set_option(RS2_OPTION_HDR_ENABLED, 1.f);
        REQUIRE(depth_sensor.get_option(RS2_OPTION_HDR_ENABLED) == 1.f);

        int iteration = 0;
        int sequence_id = -1;
        int iterations_for_preparation = 6;
        while (++iteration < 50) // Application still alive?
        {
            rs2::frameset data = pipe.wait_for_frames();

            if (iteration < iterations_for_preparation)
                continue;

            auto depth_frame = data.get_depth_frame();
            auto depth_seq_id = depth_frame.get_frame_metadata(RS2_FRAME_METADATA_SEQUENCE_ID);
            auto ir_frame = data.get_infrared_frame(1);
            auto ir_seq_id = ir_frame.get_frame_metadata(RS2_FRAME_METADATA_SEQUENCE_ID);
            if (iteration == iterations_for_preparation)
            {
                REQUIRE(depth_seq_id == ir_seq_id);
                sequence_id = depth_seq_id;
            }
            else
            {
                sequence_id = (sequence_id == 0) ? 1 : 0;
                REQUIRE(sequence_id == depth_seq_id);
                REQUIRE(sequence_id == ir_seq_id);
            }
        }
        pipe.stop();
    }
}

TEST_CASE("Emitter on/off - checking sequence id", "[hdr][live][using_pipeline]") 
{
    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        rs2::device_list devices_list = ctx.query_devices();
        size_t device_count = devices_list.size();
        REQUIRE(device_count > 0);

        rs2::depth_sensor depth_sensor = restart_first_device_and_return_depth_sensor(ctx, devices_list);

        rs2::config cfg;
        cfg.enable_stream(RS2_STREAM_DEPTH);
        cfg.enable_stream(RS2_STREAM_INFRARED, 1);
        rs2::pipeline pipe;
        pipe.start(cfg);

        depth_sensor.set_option(RS2_OPTION_EMITTER_ON_OFF, 1);
        REQUIRE(depth_sensor.get_option(RS2_OPTION_EMITTER_ON_OFF) == 1.f);

        int iteration = 0;
        int sequence_id = -1;
        int iterations_for_preparation = 6;
        while (++iteration < 50) // Application still alive?
        {
            rs2::frameset data = pipe.wait_for_frames();

            if (iteration < iterations_for_preparation)
                continue;

            auto depth_frame = data.get_depth_frame();
            auto depth_seq_id = depth_frame.get_frame_metadata(RS2_FRAME_METADATA_SEQUENCE_ID);
            auto ir_frame = data.get_infrared_frame(1);
            auto ir_seq_id = ir_frame.get_frame_metadata(RS2_FRAME_METADATA_SEQUENCE_ID);

            if (iteration == iterations_for_preparation)
            {
                REQUIRE(depth_seq_id == ir_seq_id);
                sequence_id = depth_seq_id;
            }
            else
            {
                sequence_id = (sequence_id == 0) ? 1 : 0;
                REQUIRE(sequence_id == depth_seq_id);
                REQUIRE(sequence_id == ir_seq_id);
            }
        }
        pipe.stop();
    }
}

//This tests checks that the previously saved merged frame is discarded after a pipe restart
TEST_CASE("HDR Merge - discard merged frame", "[hdr][live][using_pipeline]") {

    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        rs2::device_list devices_list = ctx.query_devices();
        size_t device_count = devices_list.size();
        REQUIRE(device_count > 0);

        rs2::depth_sensor depth_sensor = restart_first_device_and_return_depth_sensor(ctx, devices_list);

        depth_sensor.set_option(RS2_OPTION_HDR_ENABLED, 1);
        REQUIRE(depth_sensor.get_option(RS2_OPTION_HDR_ENABLED) == 1.f);

        rs2::config cfg;
        cfg.enable_stream(RS2_STREAM_DEPTH);
        rs2::pipeline pipe;
        pipe.start(cfg);

        // initializing the merging filter
        rs2::hdr_merge merging_filter;

        int num_of_iterations_in_serie = 10;
        int first_series_last_merged_ts = -1;
        for (int i = 0; i < num_of_iterations_in_serie; ++i)
        {
            rs2::frameset data = pipe.wait_for_frames();
            rs2::depth_frame out_depth_frame = data.get_depth_frame();

            auto seq_id = out_depth_frame.get_frame_metadata(RS2_FRAME_METADATA_SEQUENCE_ID);
            long long frame_counter = out_depth_frame.get_frame_metadata(RS2_FRAME_METADATA_FRAME_COUNTER);
            long long frame_exposure = out_depth_frame.get_frame_metadata(RS2_FRAME_METADATA_ACTUAL_EXPOSURE);
            long long frame_ts = out_depth_frame.get_frame_metadata(RS2_FRAME_METADATA_FRAME_TIMESTAMP);
            //std::cout << "depth - seq_id = " << seq_id << ", frame counter = " << frame_counter << ", exposure = " << frame_exposure << ", ts = " << frame_ts << std::endl;

            // merging the frames from the different HDR sequence IDs 
            auto merged_frameset = merging_filter.process(data);
            auto merged_depth_frame = merged_frameset.as<rs2::frameset>().get_depth_frame();

            seq_id = merged_depth_frame.get_frame_metadata(RS2_FRAME_METADATA_SEQUENCE_ID);
            frame_counter = merged_depth_frame.get_frame_metadata(RS2_FRAME_METADATA_FRAME_COUNTER);
            frame_exposure = merged_depth_frame.get_frame_metadata(RS2_FRAME_METADATA_ACTUAL_EXPOSURE);
            frame_ts = merged_depth_frame.get_frame_metadata(RS2_FRAME_METADATA_FRAME_TIMESTAMP);
            //std::cout << "merged - seq_id = " << seq_id << ", frame counter = " << frame_counter << ", exposure = " << frame_exposure << ", ts = " << frame_ts << std::endl;

            if (i == (num_of_iterations_in_serie - 1))
                first_series_last_merged_ts = frame_ts;
        }

        pipe.stop();

        //std::cout << "------------------stop - start again ---------------" << std::endl;

        depth_sensor.set_option(RS2_OPTION_HDR_ENABLED, 1);
        REQUIRE(depth_sensor.get_option(RS2_OPTION_HDR_ENABLED) == 1.f);

        pipe.start(cfg);

        for (int i = 0; i < 10; ++i)
        {
            rs2::frameset data = pipe.wait_for_frames();
            rs2::depth_frame out_depth_frame = data.get_depth_frame();

            auto seq_id = out_depth_frame.get_frame_metadata(RS2_FRAME_METADATA_SEQUENCE_ID);
            long long frame_counter = out_depth_frame.get_frame_metadata(RS2_FRAME_METADATA_FRAME_COUNTER);
            long long frame_exposure = out_depth_frame.get_frame_metadata(RS2_FRAME_METADATA_ACTUAL_EXPOSURE);
            long long frame_ts = out_depth_frame.get_frame_metadata(RS2_FRAME_METADATA_FRAME_TIMESTAMP);
            //std::cout << "depth - seq_id = " << seq_id << ", frame_counter = " << frame_counter << ", exposure = " << frame_exposure << ", ts = " << frame_ts << std::endl;

            // merging the frames from the different HDR sequence IDs 
            auto merged_frameset = merging_filter.process(data);
            auto merged_depth_frame = merged_frameset.as<rs2::frameset>().get_depth_frame();

            seq_id = merged_depth_frame.get_frame_metadata(RS2_FRAME_METADATA_SEQUENCE_ID);
            frame_counter = merged_depth_frame.get_frame_metadata(RS2_FRAME_METADATA_FRAME_COUNTER);
            frame_exposure = merged_depth_frame.get_frame_metadata(RS2_FRAME_METADATA_ACTUAL_EXPOSURE);
            frame_ts = merged_depth_frame.get_frame_metadata(RS2_FRAME_METADATA_FRAME_TIMESTAMP);
            //std::cout << "merged - seq_id = " << seq_id << ", frame_counter = " << frame_counter << ", exposure = " << frame_exposure << ", ts = " << frame_ts << std::endl;
            REQUIRE(frame_ts > first_series_last_merged_ts);
        }

        pipe.stop();
    }
}


TEST_CASE("HDR Start Stop - recover manual exposure and gain", "[HDR]") 
{
    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        rs2::device_list devices_list = ctx.query_devices();
        size_t device_count = devices_list.size();
        REQUIRE(device_count > 0);

        rs2::depth_sensor depth_sensor = restart_first_device_and_return_depth_sensor(ctx, devices_list);

        float gain_before_hdr = 50.f;
        depth_sensor.set_option(RS2_OPTION_GAIN, gain_before_hdr);
        REQUIRE(depth_sensor.get_option(RS2_OPTION_GAIN) == gain_before_hdr);

        float exposure_before_hdr = 5000.f;
        depth_sensor.set_option(RS2_OPTION_EXPOSURE, exposure_before_hdr);
        REQUIRE(depth_sensor.get_option(RS2_OPTION_EXPOSURE) == exposure_before_hdr);

        depth_sensor.set_option(RS2_OPTION_HDR_ENABLED, 1);
        REQUIRE(depth_sensor.get_option(RS2_OPTION_HDR_ENABLED) == 1.f);

        rs2::config cfg;
        cfg.enable_stream(RS2_STREAM_DEPTH);
        rs2::pipeline pipe;
        pipe.start(cfg);

        int iteration = 0;
        int iteration_for_disable = 50;
        int iteration_to_check_after_disable = iteration_for_disable + 2;
        while (++iteration < 70)
        {
            rs2::frameset data = pipe.wait_for_frames();

            rs2::depth_frame out_depth_frame = data.get_depth_frame();
            long long frame_counter = out_depth_frame.get_frame_metadata(RS2_FRAME_METADATA_FRAME_COUNTER);
            long long frame_gain = out_depth_frame.get_frame_metadata(RS2_FRAME_METADATA_GAIN_LEVEL);
            long long frame_exposure = out_depth_frame.get_frame_metadata(RS2_FRAME_METADATA_ACTUAL_EXPOSURE);
            //std::cout << "iteration = " << iteration << ", gain = " << frame_gain << ", exposure = " << frame_exposure << std::endl;

            if (iteration > iteration_for_disable && iteration < iteration_to_check_after_disable)
                continue;

            if (iteration == iteration_for_disable)
            {
                depth_sensor.set_option(RS2_OPTION_HDR_ENABLED, 0);
                REQUIRE(depth_sensor.get_option(RS2_OPTION_HDR_ENABLED) == 0.f);
            }
            else if (iteration >= iteration_to_check_after_disable)
            {
                REQUIRE(frame_gain == gain_before_hdr);
                REQUIRE(frame_exposure == exposure_before_hdr);
            }
        }
    }
}

// CONTROLS STABILITY WHILE HDR ACTIVE
TEST_CASE("HDR Active - set locked options", "[hdr][live]") {
    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        std::string last_option_set;
        std::string last_warning_received;
        try {
            auto callback = [&](rs2_log_severity severity, rs2::log_message const& msg)
            {
                last_warning_received = msg.raw();
            };
            rs2::log_to_callback(RS2_LOG_SEVERITY_WARN, callback);
            rs2::device_list devices_list = ctx.query_devices();
            size_t device_count = devices_list.size();
            REQUIRE(device_count > 0);

            rs2::depth_sensor depth_sensor = restart_first_device_and_return_depth_sensor(ctx, devices_list);

            //setting laser ON
            depth_sensor.set_option(RS2_OPTION_EMITTER_ENABLED, 1.f);
            auto laser_power_before_hdr = depth_sensor.get_option(RS2_OPTION_LASER_POWER);

            std::this_thread::sleep_for((std::chrono::milliseconds)(1500));

            depth_sensor.set_option(RS2_OPTION_HDR_ENABLED, 1.f);
            REQUIRE(depth_sensor.get_option(RS2_OPTION_HDR_ENABLED) == 1.f);

            // the following calls should not be performed and should send a LOG_WARNING
            last_option_set = "RS2_OPTION_ENABLE_AUTO_EXPOSURE";
            depth_sensor.set_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE, 1.f);
            REQUIRE(depth_sensor.get_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE) == 0.f);
            REQUIRE(last_warning_received == "Auto Exposure cannot be set while HDR is enabled");

            last_option_set = "RS2_OPTION_EMITTER_ENABLED";
            depth_sensor.set_option(RS2_OPTION_EMITTER_ENABLED, 0.f);
            REQUIRE(depth_sensor.get_option(RS2_OPTION_EMITTER_ENABLED) == 1.f);
            REQUIRE(last_warning_received == "Emitter status cannot be set while HDR is enabled");

            last_option_set = "RS2_OPTION_EMITTER_ON_OFF";
            depth_sensor.set_option(RS2_OPTION_EMITTER_ON_OFF, 1.f);
            REQUIRE(depth_sensor.get_option(RS2_OPTION_EMITTER_ON_OFF) == 0.f);
            REQUIRE(last_warning_received == "Emitter ON/OFF cannot be set while HDR is enabled");

            last_option_set = "RS2_OPTION_LASER_POWER";
            depth_sensor.set_option(RS2_OPTION_LASER_POWER, laser_power_before_hdr - 30.f);
            REQUIRE(depth_sensor.get_option(RS2_OPTION_LASER_POWER) == laser_power_before_hdr);
            REQUIRE(last_warning_received == "Laser Power status cannot be set while HDR is enabled");
        }
        catch (std::exception e)
        {
            REQUIRE(false);
            //std::cout << "Exception :" << last_option_set << ", " << e.what() << std::endl;
        }
    }
}