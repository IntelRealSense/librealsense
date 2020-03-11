// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "fa-private.h"

#include "algo.h"
#include "error-handling.h"
#include "core/debug.h"
#include "core/advanced_mode.h"
#include "device.h"
#include "global_timestamp_reader.h"
#include "fw-update/fw-update-device-interface.h"

namespace librealsense
{
    class fa_device : public virtual device, public debug_interface, public global_time_interface, public updatable
    {
    public:

        synthetic_sensor& get_ir_sensor()
        {
            return dynamic_cast<synthetic_sensor&>(get_sensor(_ir_device_idx));
        }

        uvc_sensor& get_raw_ir_sensor()
        {
            synthetic_sensor& ir_sensor = get_ir_sensor();
            return dynamic_cast<uvc_sensor&>(*ir_sensor.get_raw_sensor());
        }

        fa_device(std::shared_ptr<context> ctx,
            const platform::backend_device_group& group);

        std::vector<uint8_t> send_receive_raw_data(const std::vector<uint8_t>& input) override;

        void hardware_reset() override;

        void create_snapshot(std::shared_ptr<debug_interface>& snapshot) const override;
        void enable_recording(std::function<void(const debug_interface&)> record_action) override;
        platform::usb_spec get_usb_spec() const;
        virtual double get_device_time_ms() override;

        void enter_update_state() const override;
        std::vector<uint8_t> backup_flash(update_progress_callback_ptr callback) override;
        void update_flash(const std::vector<uint8_t>& image, update_progress_callback_ptr callback, int update_mode) override;
    protected:

        std::vector<uint8_t> get_raw_calibration_table(fa::calibration_table_id table_id) const;
        std::vector<uint8_t> get_new_calibration_table() const;

        bool is_camera_in_advanced_mode() const;

        float get_stereo_baseline_mm() const;

        ds::d400_caps  parse_device_capabilities(const uint16_t pid) const;

        void init(std::shared_ptr<context> ctx,
            const platform::backend_device_group& group);

        std::shared_ptr<hw_monitor> _hw_monitor;
        firmware_version            _fw_version;
        firmware_version            _recommended_fw_version;
        ds::d400_caps               _device_capabilities;

        uint8_t _ir_device_idx;

        std::shared_ptr<stream_interface> _left_ir_stream;
        std::shared_ptr<stream_interface> _right_ir_stream;

        lazy<std::vector<uint8_t>> _coefficients_table_raw;
        lazy<std::vector<uint8_t>> _new_calib_table_raw;

        std::unique_ptr<polling_error_handler> _polling_error_handler;
        std::shared_ptr<lazy<rs2_extrinsics>> _left_right_extrinsics;
        bool _is_locked = true;
    };

    class fa_notification_decoder : public notification_decoder
    {
    public:
        notification decode(int value) override;
    };
}
