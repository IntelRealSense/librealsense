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

        rs2::device dev = devices_list[0];
        std::string serial;
        REQUIRE_NOTHROW(serial = dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));
        //forcing hardware reset to simulate device disconnection
        auto shared_dev = std::make_shared<device>(devices_list.front());
        shared_dev = do_with_waiting_for_camera_connection(ctx, shared_dev, serial, [&]()
            {
                shared_dev->hardware_reset();
            });
        rs2::depth_sensor depth_sensor = dev.query_sensors().front();

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

        rs2::device dev = devices_list[0];
        std::string serial;
        REQUIRE_NOTHROW(serial = dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));
        //forcing hardware reset to simulate device disconnection
        auto shared_dev = std::make_shared<device>(devices_list.front());
        shared_dev = do_with_waiting_for_camera_connection(ctx, shared_dev, serial, [&]()
            {
                shared_dev->hardware_reset();
            });
        rs2::depth_sensor depth_sensor = dev.query_sensors().front();

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

        rs2::device dev = devices_list[0];
        std::string serial;
        REQUIRE_NOTHROW(serial = dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));
        //forcing hardware reset to simulate device disconnection
        auto shared_dev = std::make_shared<device>(devices_list.front());
        shared_dev = do_with_waiting_for_camera_connection(ctx, shared_dev, serial, [&]()
        {
            shared_dev->hardware_reset();
        });
        rs2::depth_sensor depth_sensor = dev.query_sensors().front();

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
            while (dev && ++iteration < 100)
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

        rs2::device dev = devices_list[0];
        std::string serial;
        REQUIRE_NOTHROW(serial = dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));
        //forcing hardware reset to simulate device disconnection
        auto shared_dev = std::make_shared<device>(devices_list.front());
        shared_dev = do_with_waiting_for_camera_connection(ctx, shared_dev, serial, [&]()
            {
                shared_dev->hardware_reset();
            });
        rs2::depth_sensor depth_sensor = dev.query_sensors().front();

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
            while (dev && ++iteration < 100) 
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

        rs2::device dev = devices_list[0];
        std::string serial;
        REQUIRE_NOTHROW(serial = dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));
        //forcing hardware reset to simulate device disconnection
        auto shared_dev = std::make_shared<device>(devices_list.front());
        shared_dev = do_with_waiting_for_camera_connection(ctx, shared_dev, serial, [&]()
            {
                shared_dev->hardware_reset();
            });
        rs2::depth_sensor depth_sensor = dev.query_sensors().front();

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
            while (dev && ++iteration < 100)
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