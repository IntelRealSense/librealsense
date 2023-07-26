// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "d400-motion.h"

#include <mutex>
#include <chrono>
#include <vector>
#include <map>
#include <iterator>
#include <cstddef>

#include "ds/ds-timestamp.h"
#include "d400-options.h"
#include "stream.h"
#include "proc/motion-transform.h"
#include "proc/auto-exposure-processor.h"

using namespace librealsense;
namespace librealsense
{
    // D457 development
    const std::map<uint32_t, rs2_format> motion_fourcc_to_rs2_format = {
        {rs_fourcc('G','R','E','Y'), RS2_FORMAT_MOTION_XYZ32F},
    };
    const std::map<uint32_t, rs2_stream> motion_fourcc_to_rs2_stream = {
        {rs_fourcc('G','R','E','Y'), RS2_STREAM_ACCEL},
    };

    rs2_motion_device_intrinsic d400_motion_base::get_motion_intrinsics(rs2_stream stream) const
    {
        return _ds_motion_common->get_motion_intrinsics(stream);
    }

    std::shared_ptr<synthetic_sensor> d400_motion_uvc::create_uvc_device(std::shared_ptr<context> ctx,
                                                  const std::vector<platform::uvc_device_info>& all_uvc_infos,
                                                  const firmware_version& camera_fw_version)
    {
        if (all_uvc_infos.empty())
        {
            LOG_WARNING("No UVC info provided, IMU is disabled");
            return nullptr;
        }

        auto&& backend = ctx->get_backend();

        std::vector<std::shared_ptr<platform::uvc_device>> imu_devices;
        for (auto&& info : filter_by_mi(all_uvc_infos, 4)) // Filter just mi=4, IMU
            imu_devices.push_back(backend.create_uvc_device(info));

        std::unique_ptr<frame_timestamp_reader> timestamp_reader_backup(new ds_timestamp_reader(backend.create_time_service()));
        std::unique_ptr<frame_timestamp_reader> timestamp_reader_metadata(new ds_timestamp_reader_from_metadata_mipi_motion(std::move(timestamp_reader_backup)));

        auto enable_global_time_option = std::shared_ptr<global_time_option>(new global_time_option());

        auto raw_motion_ep = std::make_shared<uvc_sensor>("Raw IMU Sensor", std::make_shared<platform::multi_pins_uvc_device>(imu_devices),
             std::unique_ptr<frame_timestamp_reader>(new global_timestamp_reader(std::move(timestamp_reader_metadata), _tf_keeper, enable_global_time_option)), this);

        auto motion_ep = std::make_shared<ds_motion_sensor>("Motion Module", raw_motion_ep, this,
                                                            motion_fourcc_to_rs2_format, motion_fourcc_to_rs2_stream);

        motion_ep->register_option(RS2_OPTION_GLOBAL_TIME_ENABLED, enable_global_time_option);

        // register pre-processing
        std::shared_ptr<enable_motion_correction> mm_correct_opt = nullptr;

        //  Motion intrinsic calibration presents is a prerequisite for motion correction.
        try
        {
            if (_mm_calib)
            {
                mm_correct_opt = std::make_shared<enable_motion_correction>(motion_ep.get(),
                    option_range{ 0, 1, 1, 1 });
                motion_ep->register_option(RS2_OPTION_ENABLE_MOTION_CORRECTION, mm_correct_opt);
            }
        }
        catch (...) {}

        motion_ep->register_processing_block(
            { {RS2_FORMAT_MOTION_XYZ32F} },
            { {RS2_FORMAT_MOTION_XYZ32F, RS2_STREAM_ACCEL}, {RS2_FORMAT_MOTION_XYZ32F, RS2_STREAM_GYRO} },
            [&, mm_correct_opt]() { return std::make_shared<motion_to_accel_gyro>(_mm_calib, mm_correct_opt);
        });

        return motion_ep;
    }


    std::shared_ptr<synthetic_sensor> d400_motion::create_hid_device(std::shared_ptr<context> ctx,
                                                                const std::vector<platform::hid_device_info>& all_hid_infos,
                                                                const firmware_version& camera_fw_version)
    {
        return _ds_motion_common->create_hid_device(ctx, all_hid_infos, camera_fw_version, _tf_keeper);
    }

    d400_motion_base::d400_motion_base(std::shared_ptr<context> ctx,
        const platform::backend_device_group& group)
        : device(ctx, group),
        d400_device(ctx, group),
        _accel_stream(new stream(RS2_STREAM_ACCEL)),
        _gyro_stream(new stream(RS2_STREAM_GYRO))
    {
        _ds_motion_common = std::make_shared<ds_motion_common>(this, _fw_version,
            _device_capabilities, _hw_monitor);
    }

    d400_motion::d400_motion(std::shared_ptr<context> ctx,
                           const platform::backend_device_group& group)
        : device(ctx, group), 
        d400_device(ctx, group),
        d400_motion_base(ctx, group)
    {
        using namespace ds;

        std::vector<platform::hid_device_info> hid_infos = group.hid_devices;

        _ds_motion_common->init_motion(hid_infos.empty(), *_depth_stream);
        
        initialize_fisheye_sensor(ctx,group);

        // Try to add HID endpoint
        auto hid_ep = create_hid_device(ctx, group.hid_devices, _fw_version);
        if (hid_ep)
        {
            _motion_module_device_idx = static_cast<uint8_t>(add_sensor(hid_ep));

            // HID metadata attributes
            hid_ep->get_raw_sensor()->register_metadata(RS2_FRAME_METADATA_FRAME_TIMESTAMP, make_hid_header_parser(&platform::hid_header::timestamp));
        }
    }

    d400_motion_uvc::d400_motion_uvc(std::shared_ptr<context> ctx,
        const platform::backend_device_group& group)
        : device(ctx, group),
          d400_device(ctx, group),
          d400_motion_base(ctx, group)
    {
        using namespace ds;

        std::vector<platform::uvc_device_info> uvc_infos = group.uvc_devices;

        _ds_motion_common->init_motion(uvc_infos.empty(), *_depth_stream);

        if (!uvc_infos.empty())
        {
            // product id - D457 dev - check - must not be the front of uvc_infos vector
            _pid = uvc_infos.front().pid;
        }

        // Try to add HID endpoint
        std::shared_ptr<synthetic_sensor> sensor_ep;
        sensor_ep = create_uvc_device(ctx, group.uvc_devices, _fw_version);
        if (sensor_ep)
        {
            _motion_module_device_idx = static_cast<uint8_t>(add_sensor(sensor_ep));

            // HID metadata attributes - D457 dev - check metadata parser
            sensor_ep->get_raw_sensor()->register_metadata(RS2_FRAME_METADATA_FRAME_TIMESTAMP, make_hid_header_parser(&platform::hid_header::timestamp));
        }
    }

    void d400_motion::initialize_fisheye_sensor(std::shared_ptr<context> ctx, const platform::backend_device_group& group)
    {
        using namespace ds;

        bool is_fisheye_avaialable = false;
        auto fisheye_infos = _ds_motion_common->init_fisheye(group, is_fisheye_avaialable);
        if (!is_fisheye_avaialable)
            return;

        std::unique_ptr<frame_timestamp_reader> ds_timestamp_reader_backup(new ds_timestamp_reader(environment::get_instance().get_time_service()));
        auto&& backend = ctx->get_backend();
        std::unique_ptr<frame_timestamp_reader> ds_timestamp_reader_metadata(new ds_timestamp_reader_from_metadata(std::move(ds_timestamp_reader_backup)));
        auto enable_global_time_option = std::shared_ptr<global_time_option>(new global_time_option());
        auto raw_fisheye_ep = std::make_shared<uvc_sensor>("FishEye Sensor", backend.create_uvc_device(fisheye_infos.front()),
                                std::unique_ptr<frame_timestamp_reader>(new global_timestamp_reader(std::move(ds_timestamp_reader_metadata), _tf_keeper, enable_global_time_option)), this);
        auto fisheye_ep = std::make_shared<ds_fisheye_sensor>(raw_fisheye_ep, this);
        
        _ds_motion_common->assign_fisheye_ep(raw_fisheye_ep, fisheye_ep, enable_global_time_option);
        
        register_fisheye_options();

        register_fisheye_metadata();

        // Add fisheye endpoint
        _fisheye_device_idx = add_sensor(fisheye_ep);
    }

    void d400_motion::register_fisheye_options()
    {
        _ds_motion_common->register_fisheye_options();
    }

    void d400_motion::register_fisheye_metadata()
    {
        _ds_motion_common->register_fisheye_metadata();
    }

    void d400_motion::register_stream_to_extrinsic_group(const stream_interface& stream, uint32_t group_index)
    {
        device::register_stream_to_extrinsic_group(stream, group_index);
    }
}
