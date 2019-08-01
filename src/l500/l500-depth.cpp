// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#include <vector>
#include "device.h"
#include "context.h"
#include "stream.h"
#include "l500-depth.h"
#include "l500-private.h"
#include "proc/decimation-filter.h"
#include "proc/threshold.h" 
#include "proc/spatial-filter.h"
#include "proc/temporal-filter.h"
#include "proc/hole-filling-filter.h"
#include "proc/zero-order.h"
#include <cstddef>
#include "metadata-parser.h"

#define MM_TO_METER 1/1000
#define MIN_ALGO_VERSION 115

namespace librealsense
{
    using namespace ivcam2;

    std::vector<uint8_t> l500_depth::get_raw_calibration_table() const
    {
        static const char* fw_ver = "1.2.11.0";

        if(_fw_version >= firmware_version(fw_ver))
            return _hw_monitor->send(command{ DPT_INTRINSICS_FULL_GET });
        else
        {
            //WA untill fw will fix DPT_INTRINSICS_GET command
            command cmd_fx(0x01, 0xa00e0804, 0xa00e0808);
            command cmd_fy(0x01, 0xa00e080c, 0xa00e0810);
            command cmd_cx(0x01, 0xa00e0814, 0xa00e0818);
            command cmd_cy(0x01, 0xa00e0818, 0xa00e081c);
            auto fx = _hw_monitor->send(cmd_fx); // CBUFspare_000
            auto fy = _hw_monitor->send(cmd_fy); // CBUFspare_002
            auto cx = _hw_monitor->send(cmd_cx); // CBUFspare_004
            auto cy = _hw_monitor->send(cmd_cy); // CBUFspare_005

            std::vector<uint8_t> vec;
            vec.insert(vec.end(), fx.begin(), fx.end());
            vec.insert(vec.end(), cx.begin(), cx.end());
            vec.insert(vec.end(), fy.begin(), fy.end());
            vec.insert(vec.end(), cy.begin(), cy.end());

            return vec;
        }
    }

    l500_depth::l500_depth(std::shared_ptr<context> ctx,
                             const platform::backend_device_group& group)
        :device(ctx, group),
        l500_device(ctx, group)
    {
        _calib_table_raw = [this]() { return get_raw_calibration_table(); };

        get_depth_sensor().register_option(RS2_OPTION_LLD_TEMPERATURE,
            std::make_shared <l500_temperature_options>(_hw_monitor.get(), RS2_OPTION_LLD_TEMPERATURE));

        get_depth_sensor().register_option(RS2_OPTION_MC_TEMPERATURE,
            std::make_shared <l500_temperature_options>(_hw_monitor.get(), RS2_OPTION_MC_TEMPERATURE));

        get_depth_sensor().register_option(RS2_OPTION_MA_TEMPERATURE,
            std::make_shared <l500_temperature_options>(_hw_monitor.get(), RS2_OPTION_MA_TEMPERATURE));

        get_depth_sensor().register_option(RS2_OPTION_APD_TEMPERATURE,
            std::make_shared <l500_temperature_options>(_hw_monitor.get(), RS2_OPTION_APD_TEMPERATURE));

        environment::get_instance().get_extrinsics_graph().register_same_extrinsics(*_depth_stream, *_ir_stream);
        environment::get_instance().get_extrinsics_graph().register_same_extrinsics(*_depth_stream, *_confidence_stream);

        register_stream_to_extrinsic_group(*_depth_stream, 0);
        register_stream_to_extrinsic_group(*_ir_stream, 0);

        auto error_control = std::unique_ptr<uvc_xu_option<int>>(new uvc_xu_option<int>(get_depth_sensor(), ivcam2::depth_xu, L500_ERROR_REPORTING, "Error reporting"));

        _polling_error_handler = std::unique_ptr<polling_error_handler>(
            new polling_error_handler(1000,
                std::move(error_control),
                get_depth_sensor().get_notifications_processor(),
                std::unique_ptr<notification_decoder>(new l500_notification_decoder())));

        get_depth_sensor().register_option(RS2_OPTION_ERROR_POLLING_ENABLED, std::make_shared<polling_errors_disable>(_polling_error_handler.get()));

        // attributes of md_capture_timing
        auto md_prop_offset = offsetof(metadata_raw, mode) +
            offsetof(md_l500_depth, intel_capture_timing);

        get_depth_sensor().register_metadata(RS2_FRAME_METADATA_FRAME_TIMESTAMP, make_uvc_header_parser(&platform::uvc_header::timestamp));
        get_depth_sensor().register_metadata(RS2_FRAME_METADATA_FRAME_COUNTER, make_attribute_parser(&l500_md_capture_timing::frame_counter, md_capture_timing_attributes::frame_counter_attribute, md_prop_offset));
        get_depth_sensor().register_metadata(RS2_FRAME_METADATA_SENSOR_TIMESTAMP, make_attribute_parser(&l500_md_capture_timing::sensor_timestamp, md_capture_timing_attributes::sensor_timestamp_attribute, md_prop_offset));
        get_depth_sensor().register_metadata(RS2_FRAME_METADATA_ACTUAL_FPS, make_attribute_parser(&l500_md_capture_timing::exposure_time, md_capture_timing_attributes::sensor_timestamp_attribute, md_prop_offset));

        // attributes of md_depth_control
        md_prop_offset = offsetof(metadata_raw, mode) +
            offsetof(md_l500_depth, intel_depth_control);

        get_depth_sensor().register_metadata(RS2_FRAME_METADATA_FRAME_LASER_POWER, make_attribute_parser(&md_l500_depth_control::laser_power, md_l500_depth_control_attributes::laser_power, md_prop_offset));
        get_depth_sensor().register_metadata(RS2_FRAME_METADATA_FRAME_LASER_POWER_MODE, make_attribute_parser(&md_l500_depth_control::laser_power_mode, md_rgb_control_attributes::manual_exp_attribute, md_prop_offset));
    }

    std::vector<tagged_profile> l500_depth::get_profiles_tags() const
    {
        std::vector<tagged_profile> tags;

        tags.push_back({ RS2_STREAM_DEPTH, -1, 640, 480, RS2_FORMAT_Z16, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
        tags.push_back({ RS2_STREAM_INFRARED, -1, 640, 480, RS2_FORMAT_Y8, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
        tags.push_back({ RS2_STREAM_CONFIDENCE, -1, 640, 480, RS2_FORMAT_RAW8, 30, profile_tag::PROFILE_TAG_SUPERSET });
        
        return tags;
    }

    std::shared_ptr<matcher> l500_depth::create_matcher(const frame_holder& frame) const
    {
        std::vector<std::shared_ptr<matcher>> depth_matchers;

        std::vector<stream_interface*> streams = { _depth_stream.get(), _ir_stream.get(), _confidence_stream.get() };

        for (auto& s : streams)
        {
            depth_matchers.push_back(std::make_shared<identity_matcher>(s->get_unique_id(), s->get_stream_type()));
        }
        std::vector<std::shared_ptr<matcher>> matchers;
        if (!frame.frame->supports_frame_metadata(RS2_FRAME_METADATA_FRAME_COUNTER))
        {
            matchers.push_back(std::make_shared<timestamp_composite_matcher>(depth_matchers));
        }
        else
        {
            matchers.push_back(std::make_shared<frame_number_composite_matcher>(depth_matchers));
        }

        return std::make_shared<timestamp_composite_matcher>(matchers);
    }

    int l500_depth_sensor::read_algo_version()
    {
        const int algo_version_address = 0xa0020bd8;
        command cmd(ivcam2::fw_cmd::MRD, algo_version_address, algo_version_address + 4);
        auto res = _owner->_hw_monitor->send(cmd);
        if (res.size() < 2)
        {
            throw std::runtime_error("Invalid result size!");
        }
        auto data = (uint8_t*)res.data();
        auto ver = data[0] + 100* data[1];
        return ver;
    }

    float l500_depth_sensor::read_baseline()
    {
        const int baseline_address = 0xa00e0868;
        command cmd(ivcam2::fw_cmd::MRD, baseline_address, baseline_address + 4);
        auto res = _owner->_hw_monitor->send(cmd);
        if (res.size() < 1)
        {
            throw std::runtime_error("Invalid result size!");
        }
        auto data = (float*)res.data();
        return *data;
    }

    float l500_depth_sensor::read_znorm()
    {
        const int baseline_znorm = 0xa00e0b08;
        auto res = _owner->_hw_monitor->send(command(ivcam2::fw_cmd::MRD, baseline_znorm, baseline_znorm + 4));
        if (res.size() < 1)
        {
            throw std::runtime_error("Invalid result size!");
        }
        auto znorm = *(float*)res.data();
        return 1/znorm* MM_TO_METER;
    }

    rs2_time_t l500_timestamp_reader_from_metadata::get_frame_timestamp(const request_mapping& mode, const platform::frame_object& fo)
    {
        std::lock_guard<std::recursive_mutex> lock(_mtx);

        if (has_metadata_ts(fo))
        {
            auto md = (librealsense::metadata_raw*)(fo.metadata);
            return (double)(md->header.timestamp)*TIMESTAMP_USEC_TO_MSEC;
        }
        else
        {
            if (!one_time_note)
            {
                LOG_WARNING("UVC metadata payloads are not available for stream "
                    << std::hex << mode.pf->fourcc << std::dec << (mode.profile.format)
                    << ". Please refer to installation chapter for details.");
                one_time_note = true;
            }
            return _backup_timestamp_reader->get_frame_timestamp(mode, fo);
        }
    }

    unsigned long long l500_timestamp_reader_from_metadata::get_frame_counter(const request_mapping & mode, const platform::frame_object& fo) const
    {
        std::lock_guard<std::recursive_mutex> lock(_mtx);

        if (has_metadata_fc(fo))
        {
            auto md = (byte*)(fo.metadata);
            // WA until we will have the struct of metadata
            return ((int*)md)[7];
        }

        return _backup_timestamp_reader->get_frame_counter(mode, fo);
    }

    void l500_timestamp_reader_from_metadata::reset()
    {
        std::lock_guard<std::recursive_mutex> lock(_mtx);
        one_time_note = false;
        _backup_timestamp_reader->reset();
        ts_wrap.reset();
    }

    rs2_timestamp_domain l500_timestamp_reader_from_metadata::get_frame_timestamp_domain(const request_mapping & mode, const platform::frame_object& fo) const
    {
        std::lock_guard<std::recursive_mutex> lock(_mtx);

        return (has_metadata_ts(fo)) ? RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK : _backup_timestamp_reader->get_frame_timestamp_domain(mode, fo);
    }

    processing_blocks l500_depth_sensor::get_l500_recommended_proccesing_blocks()
    {
        processing_blocks res;
        res.push_back(std::make_shared<zero_order>());
        auto depth_standart = get_depth_recommended_proccesing_blocks();
        res.insert(res.end(), depth_standart.begin(), depth_standart.end());
        res.push_back(std::make_shared<threshold>());
        res.push_back(std::make_shared<spatial_filter>());
        res.push_back(std::make_shared<temporal_filter>());
        res.push_back(std::make_shared<hole_filling_filter>());
        return res;
    }

}
