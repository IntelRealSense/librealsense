// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "ds5-private.h"

#include "algo.h"
#include "error-handling.h"
#include "core/debug.h"
#include "core/advanced_mode.h"


namespace librealsense
{
    class ds5_device : public virtual device, public debug_interface
    {
    public:
        rs2_extrinsics get_extrinsics(size_t from_subdevice, rs2_stream, size_t to_subdevice, rs2_stream) const override;

        std::shared_ptr<uvc_sensor> create_depth_device(const std::shared_ptr<context>& ctx,
                                                        const std::vector<platform::uvc_device_info>& all_device_infos);

        uvc_sensor& get_depth_sensor()
        {
            return dynamic_cast<uvc_sensor&>(get_sensor(_depth_device_idx));
        }

        ds5_device(const std::shared_ptr<context>& ctx,
                   const platform::backend_device_group& group);

        std::vector<uint8_t> send_receive_raw_data(const std::vector<uint8_t>& input) override;

        void hardware_reset() override;
        void create_snapshot(std::shared_ptr<debug_interface>& snapshot) override;
        void create_recordable(std::shared_ptr<debug_interface>& recordable,
                               std::function<void(std::shared_ptr<extension_snapshot>)> record_action) override;
    protected:
        std::shared_ptr<hw_monitor> _hw_monitor;
        firmware_version _fw_version;

        pose get_device_position(unsigned int subdevice) const;

    private:
        friend class ds5_depth_sensor;

        bool is_camera_in_advanced_mode() const;

        uint8_t _depth_device_idx;
        std::shared_ptr<uvc_sensor> _depth_sensor;

        lazy<std::vector<uint8_t>> _coefficients_table_raw;

        std::vector<uint8_t> get_raw_calibration_table(ds::calibration_table_id table_id) const;

        std::unique_ptr<polling_error_handler> _polling_error_handler;
    };

    class ds5_notification_decoder : public notification_decoder
    {
    public:
        notification decode(int value) override;
    };
}
