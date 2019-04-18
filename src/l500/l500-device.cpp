// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#include <vector>
#include "l500-device.h"
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

namespace librealsense
{
    using namespace ivcam2;

    l500_device::l500_device(std::shared_ptr<context> ctx,
        const platform::backend_device_group& group)
        :device(ctx, group),
        _depth_device_idx(add_sensor(create_depth_device(ctx, group.uvc_devices))),
        _depth_stream(new stream(RS2_STREAM_DEPTH)),
        _ir_stream(new stream(RS2_STREAM_INFRARED)),
        _confidence_stream(new stream(RS2_STREAM_CONFIDENCE))
    {
        auto pid = group.uvc_devices.front().pid;
        std::string device_name = (rs500_sku_names.end() != rs500_sku_names.find(pid)) ? rs500_sku_names.at(pid) : "RS5xx";

        using namespace ivcam2;

        auto&& backend = ctx->get_backend();

        if (group.usb_devices.size() > 0)
        {
            _hw_monitor = std::make_shared<hw_monitor>(
                std::make_shared<locked_transfer>(backend.create_usb_device(group.usb_devices.front()),
                    get_depth_sensor()));
        }
        else
        {
            _hw_monitor = std::make_shared<hw_monitor>(
                std::make_shared<locked_transfer>(std::make_shared<command_transfer_over_xu>(
                    get_depth_sensor(), depth_xu, L500_HWMONITOR),
                    get_depth_sensor()));
        }

#ifdef HWM_OVER_XU
        if (group.usb_devices.size() > 0)
        {
            _hw_monitor = std::make_shared<hw_monitor>(
                std::make_shared<locked_transfer>(std::make_shared<command_transfer_over_xu>(
                    get_depth_sensor(), depth_xu, L500_HWMONITOR),
                    get_depth_sensor()));
        }
#endif


        auto fw_version = _hw_monitor->get_firmware_version_string(GVD, fw_version_offset);
        auto serial = _hw_monitor->get_module_serial_string(GVD, module_serial_offset, module_serial_size);

        _fw_version = firmware_version(fw_version);

        auto pid_hex_str = hexify(group.uvc_devices.front().pid);

        register_info(RS2_CAMERA_INFO_NAME, device_name);
        register_info(RS2_CAMERA_INFO_SERIAL_NUMBER, serial);
        register_info(RS2_CAMERA_INFO_FIRMWARE_VERSION, fw_version);
        register_info(RS2_CAMERA_INFO_DEBUG_OP_CODE, std::to_string(static_cast<int>(fw_cmd::GLD)));
        register_info(RS2_CAMERA_INFO_PHYSICAL_PORT, group.uvc_devices.front().device_path);
        register_info(RS2_CAMERA_INFO_PRODUCT_ID, pid_hex_str);
    }

    std::shared_ptr<uvc_sensor> l500_device::create_depth_device(std::shared_ptr<context> ctx,
        const std::vector<platform::uvc_device_info>& all_device_infos)
    {
        auto&& backend = ctx->get_backend();

        std::vector<std::shared_ptr<platform::uvc_device>> depth_devices;
        for (auto&& info : filter_by_mi(all_device_infos, 0)) // Filter just mi=0, DEPTH
            depth_devices.push_back(backend.create_uvc_device(info));

        auto depth_ep = std::make_shared<l500_depth_sensor>(this, std::make_shared<platform::multi_pins_uvc_device>(depth_devices),
            std::unique_ptr<frame_timestamp_reader>(new l500_timestamp_reader_from_metadata(backend.create_time_service())));

        depth_ep->register_xu(depth_xu);
        depth_ep->register_pixel_format(pf_z16_l500);
        depth_ep->register_pixel_format(pf_confidence_l500);
        depth_ep->register_pixel_format(pf_y8_l500);

        depth_ep->register_option(RS2_OPTION_LASER_POWER,
            std::make_shared<uvc_xu_option<int>>(
                *depth_ep,
                ivcam2::depth_xu,
                ivcam2::L500_DEPTH_LASER_POWER, "Power of the l500 projector, with 0 meaning projector off"));

        return depth_ep;
    }

    void l500_device::force_hardware_reset() const
    {
        command cmd(ivcam2::fw_cmd::HW_RESET);
        cmd.require_response = false;
        _hw_monitor->send(cmd);
    }

    void l500_device::create_snapshot(std::shared_ptr<debug_interface>& snapshot) const
    {
        throw not_implemented_exception("create_snapshot(...) not implemented!");
    }

    void l500_device::enable_recording(std::function<void(const debug_interface&)> record_action)
    {
        throw not_implemented_exception("enable_recording(...) not implemented!");
    }

    notification l500_notification_decoder::decode(int value)
    {
        if (l500_fw_error_report.find(static_cast<uint8_t>(value)) != l500_fw_error_report.end())
            return{ RS2_NOTIFICATION_CATEGORY_HARDWARE_ERROR, value, RS2_LOG_SEVERITY_ERROR, l500_fw_error_report.at(static_cast<uint8_t>(value)) };

        return{ RS2_NOTIFICATION_CATEGORY_HARDWARE_ERROR, value, RS2_LOG_SEVERITY_WARN, (to_string() << "L500 HW report - unresolved type " << value) };
    }
}
