// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <mutex>
#include <chrono>
#include <vector>
#include <iterator>
#include <cstddef>

#include "device.h"
#include "context.h"
#include "image.h"
#include "metadata-parser.h"

#include "d500-factory.h"
#include "d500-private.h"
#include "ds/ds-options.h"
#include "ds/ds-timestamp.h"
#include "d500-active.h"
#include "d500-color.h"
#include "d500-motion.h"
#include "d500-safety.h"
#include "d500-depth-mapping.h"
#include "sync.h"

#include "firmware_logger_device.h"
#include "device-calibration.h"

namespace librealsense
{
    class rs_d585_device : public d500_active,
        public d500_color,
        public d500_motion,
        public ds_advanced_mode_base,
        public firmware_logger_device
    {
    public:
        rs_d585_device(std::shared_ptr<context> ctx,
            const platform::backend_device_group& group,
            bool register_device_notifications)
            : device(ctx, group, register_device_notifications),
            d500_device(ctx, group),
            d500_active(ctx, group),
            d500_color(ctx, group),
            d500_motion(ctx, group),
            ds_advanced_mode_base(d500_device::_hw_monitor, get_depth_sensor()),
            firmware_logger_device(ctx, group, d500_device::_hw_monitor,
                get_firmware_logs_command(),
                get_flash_logs_command()) {}

        std::shared_ptr<matcher> create_matcher(const frame_holder& frame) const override;

        std::vector<tagged_profile> get_profiles_tags() const override
        {
            std::vector<tagged_profile> tags;

            tags.push_back({ RS2_STREAM_COLOR, -1, 1280, 720, RS2_FORMAT_RGB8, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
            tags.push_back({ RS2_STREAM_DEPTH, -1, 1280, 960, RS2_FORMAT_Z16, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
            tags.push_back({ RS2_STREAM_INFRARED, -1, 1280, 960, RS2_FORMAT_Y8, 30, profile_tag::PROFILE_TAG_SUPERSET });
            tags.push_back({ RS2_STREAM_GYRO, -1, 0, 0, RS2_FORMAT_MOTION_XYZ32F, (int)odr::IMU_FPS_200, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
            tags.push_back({ RS2_STREAM_ACCEL, -1, 0, 0, RS2_FORMAT_MOTION_XYZ32F, (int)odr::IMU_FPS_100, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });

            return tags;
        };
    };
    
    class rs_d585s_device : public d500_active,
        public d500_color,
        public d500_safety,
        public d500_depth_mapping,
        public d500_motion,
        public ds_advanced_mode_base,
        public firmware_logger_device
    {
    public:
        rs_d585s_device(std::shared_ptr<context> ctx,
            const platform::backend_device_group& group,
            bool register_device_notifications)
            : device(ctx, group, register_device_notifications),
            d500_device(ctx, group),
            d500_active(ctx, group),
            d500_color(ctx, group),
            d500_safety(ctx, group),
            d500_depth_mapping(ctx, group),
            d500_motion(ctx, group),
            ds_advanced_mode_base(d500_device::_hw_monitor, get_depth_sensor()),
            firmware_logger_device(ctx, group, d500_device::_hw_monitor,
                get_firmware_logs_command(),
                get_flash_logs_command()) {}

        std::shared_ptr<matcher> create_matcher(const frame_holder& frame) const override;

        std::vector<tagged_profile> get_profiles_tags() const override
        {
            std::vector<tagged_profile> tags;

            tags.push_back({ RS2_STREAM_COLOR, -1, 1280, 720, RS2_FORMAT_RGB8, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
            tags.push_back({ RS2_STREAM_DEPTH, -1, 1280, 960, RS2_FORMAT_Z16, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
            tags.push_back({ RS2_STREAM_INFRARED, -1, 1280, 960, RS2_FORMAT_Y8, 30, profile_tag::PROFILE_TAG_SUPERSET });
            tags.push_back({ RS2_STREAM_GYRO, -1, 0, 0, RS2_FORMAT_MOTION_XYZ32F, (int)odr::IMU_FPS_200, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
            tags.push_back({ RS2_STREAM_ACCEL, -1, 0, 0, RS2_FORMAT_MOTION_XYZ32F, (int)odr::IMU_FPS_100, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });

            return tags;
        };
    };

    std::shared_ptr<device_interface> d500_info::create(std::shared_ptr<context> ctx,
                                                       bool register_device_notifications) const
    {
        using namespace ds;

        if (_depth.size() == 0) throw std::runtime_error("Depth Camera not found!");
        auto pid = _depth.front().pid;
        platform::backend_device_group group{_depth, _hwm, _hid};

        switch(pid)
        {
        case ds::RS_D585_PID:
            return std::make_shared<rs_d585_device>(ctx, group, register_device_notifications);
        case ds::RS_D585S_PID:
            return std::make_shared<rs_d585s_device>(ctx, group, register_device_notifications);
        default:
            throw std::runtime_error(rsutils::string::from() << "Unsupported RS400 model! 0x"
                << std::hex << std::setw(4) << std::setfill('0') <<(int)pid);
        }
    }

    std::vector<std::shared_ptr<device_info>> d500_info::pick_d500_devices(
        std::shared_ptr<context> ctx,
        platform::backend_device_group& group)
    {
        std::vector<platform::uvc_device_info> chosen;
        std::vector<std::shared_ptr<device_info>> results;

        auto valid_pid = filter_by_product(group.uvc_devices, ds::rs500_sku_pid);
        auto group_devices = group_devices_and_hids_by_unique_id(group_devices_by_unique_id(valid_pid), group.hid_devices);

        for (auto& g : group_devices)
        {
            auto& devices = g.first;
            auto& hids = g.second;

            bool all_sensors_present = mi_present(devices, 0);

            // Device with multi sensors can be enabled only if all sensors (RGB + Depth) are present
            auto is_pid_of_multisensor_device = [](int pid) { return std::find(std::begin(ds::d500_multi_sensors_pid), 
                std::end(ds::d500_multi_sensors_pid), pid) != std::end(ds::d500_multi_sensors_pid); };
            bool is_device_multisensor = false;
            for (auto&& uvc : devices)
            {
                if (is_pid_of_multisensor_device(uvc.pid))
                    is_device_multisensor = true;
            }

            if(is_device_multisensor)
            {
                all_sensors_present = all_sensors_present && mi_present(devices, 3);
            }


#if !defined(__APPLE__) // Not supported by macos
            auto is_pid_of_hid_sensor_device = [](int pid) { return std::find(std::begin(ds::d500_hid_sensors_pid), 
                std::end(ds::d500_hid_sensors_pid), pid) != std::end(ds::d500_hid_sensors_pid); };
            bool is_device_hid_sensor = false;
            for (auto&& uvc : devices)
            {
                if (is_pid_of_hid_sensor_device(uvc.pid))
                    is_device_hid_sensor = true;
            }

            // Device with hids can be enabled only if both hids (gyro and accelerometer) are present
            // Assuming that hids amount of 2 and above guarantee that gyro and accelerometer are present
            if (is_device_hid_sensor)
            {
                all_sensors_present &= (hids.size() >= 2);
            }
#endif

            if (!devices.empty() && all_sensors_present)
            {
                platform::usb_device_info hwm;

                std::vector<platform::usb_device_info> hwm_devices;
                if (ds::d500_try_fetch_usb_device(group.usb_devices, devices.front(), hwm))
                {
                    hwm_devices.push_back(hwm);
                }
                else
                {
                    LOG_DEBUG("d500_try_fetch_usb_device(...) failed.");
                }

                auto info = std::make_shared<d500_info>(ctx, devices, hwm_devices, hids);
                chosen.insert(chosen.end(), devices.begin(), devices.end());
                results.push_back(info);

            }
            else
            {
                LOG_WARNING("DS5 group_devices is empty.");
            }
        }

        trim_device_list(group.uvc_devices, chosen);

        return results;
    }

    inline std::shared_ptr<matcher> create_composite_matcher(std::vector<std::shared_ptr<matcher>> matchers)
    {
        return std::make_shared<timestamp_composite_matcher>(matchers);
    }

    std::shared_ptr<matcher> rs_d585_device::create_matcher(const frame_holder& frame) const
    {
        std::vector<stream_interface*> streams = { _depth_stream.get() , _left_ir_stream.get() , _right_ir_stream.get(), _color_stream.get() };
        std::vector<stream_interface*> mm_streams = { _ds_motion_common->get_accel_stream().get(), 
                                                      _ds_motion_common->get_gyro_stream().get()};
        streams.insert(streams.end(), mm_streams.begin(), mm_streams.end());
        return matcher_factory::create(RS2_MATCHER_DEFAULT, streams);
    }

    std::shared_ptr<matcher> rs_d585s_device::create_matcher(const frame_holder& frame) const
    {
        std::vector<stream_interface*> streams = { _depth_stream.get() , _left_ir_stream.get() , _right_ir_stream.get(), _color_stream.get(), _safety_stream.get()};
        std::vector<stream_interface*> mm_streams = { _ds_motion_common->get_accel_stream().get(),
                                                      _ds_motion_common->get_gyro_stream().get() };
        streams.insert(streams.end(), mm_streams.begin(), mm_streams.end());
        return matcher_factory::create(RS2_MATCHER_DEFAULT, streams);
    }
}
