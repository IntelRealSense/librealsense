// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "ds5-private.h"

#include "algo.h"
#include "error-handling.h"
#include "core/debug.h"
#include "core/advanced_mode.h"
#include "device.h"

namespace librealsense
{
    class ds5_device : public virtual device, public debug_interface
    {
    public:
        std::shared_ptr<uvc_sensor> create_depth_device(std::shared_ptr<context> ctx,
                                                        const std::vector<platform::uvc_device_info>& all_device_infos);

        uvc_sensor& get_depth_sensor()
        {
            return dynamic_cast<uvc_sensor&>(get_sensor(_depth_device_idx));
        }

        ds5_device(std::shared_ptr<context> ctx,
                   const platform::backend_device_group& group);

        std::vector<uint8_t> send_receive_raw_data(const std::vector<uint8_t>& input) override;

        void hardware_reset() override;
        void create_snapshot(std::shared_ptr<debug_interface>& snapshot) const override;
        void enable_recording(std::function<void(const debug_interface&)> record_action) override;
        platform::usb_spec get_usb_spec() const;

    protected:

        std::vector<uint8_t> get_raw_calibration_table(ds::calibration_table_id table_id) const;

        bool is_camera_in_advanced_mode() const;

        float get_stereo_baseline_mm() const;

        void init(std::shared_ptr<context> ctx,
            const platform::backend_device_group& group);

        friend class ds5_depth_sensor;

        std::shared_ptr<hw_monitor> _hw_monitor;
        firmware_version _fw_version;
        firmware_version recommended_fw_version;

        std::shared_ptr<stream_interface> _depth_stream;
        std::shared_ptr<stream_interface> _left_ir_stream;
        std::shared_ptr<stream_interface> _right_ir_stream;

        uint8_t _depth_device_idx;

        lazy<std::vector<uint8_t>> _coefficients_table_raw;

        std::unique_ptr<polling_error_handler> _polling_error_handler;
        std::shared_ptr<lazy<rs2_extrinsics>> _left_right_extrinsics;
    };

    class ds5u_device : public ds5_device
    {
    public:
        ds5u_device(std::shared_ptr<context> ctx,
            const platform::backend_device_group& group);

        std::shared_ptr<uvc_sensor> create_ds5u_depth_device(std::shared_ptr<context> ctx,
            const std::vector<platform::uvc_device_info>& all_device_infos);

    protected:
        friend class ds5u_depth_sensor;
    };

    class ds5_notification_decoder : public notification_decoder
    {
    public:
        notification decode(int value) override;
    };
}
