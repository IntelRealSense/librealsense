// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "ds5-private.h"

#include "algo.h"
#include "error-handling.h"
#include "core/debug.h"
#include "core/advanced_mode.h"
#include "device.h"
#include "global_timestamp_reader.h"
#include "fw-update/fw-update-device-interface.h"
#include "ds5-auto-calibration.h"

namespace librealsense
{
    class ds5_auto_exposure_roi_method : public region_of_interest_method
    {
    public:
        explicit ds5_auto_exposure_roi_method(const hw_monitor& hwm,
            ds::fw_cmd cmd = ds::fw_cmd::SETAEROI);

        void set(const region_of_interest& roi) override;
        region_of_interest get() const override;
    private:
        const ds::fw_cmd _cmd;
        const hw_monitor& _hw_monitor;
    };

    class ds5_device : public virtual device, public debug_interface, public global_time_interface, public updatable, public auto_calibrated
    {
    public:
        std::shared_ptr<synthetic_sensor> create_depth_device(std::shared_ptr<context> ctx,
                                                        const std::vector<platform::uvc_device_info>& all_device_infos);

        synthetic_sensor& get_depth_sensor()
        {
            return dynamic_cast<synthetic_sensor&>(get_sensor(_depth_device_idx));
        }

        uvc_sensor& get_raw_depth_sensor()
        {
            synthetic_sensor& depth_sensor = get_depth_sensor();
            return dynamic_cast<uvc_sensor&>(*depth_sensor.get_raw_sensor());
        }

        ds5_device(std::shared_ptr<context> ctx,
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

        std::vector<uint8_t> get_raw_calibration_table(ds::calibration_table_id table_id) const;
        std::vector<uint8_t> get_new_calibration_table() const;

        bool is_camera_in_advanced_mode() const;

        float get_stereo_baseline_mm() const;

        ds::d400_caps  parse_device_capabilities(const uint16_t pid) const;

        void init(std::shared_ptr<context> ctx,
            const platform::backend_device_group& group);

        friend class ds5_depth_sensor;

        std::shared_ptr<hw_monitor> _hw_monitor;
        firmware_version            _fw_version;
        firmware_version            _recommended_fw_version;
        ds::d400_caps               _device_capabilities;

        std::shared_ptr<stream_interface> _depth_stream;
        std::shared_ptr<stream_interface> _left_ir_stream;
        std::shared_ptr<stream_interface> _right_ir_stream;

        uint8_t _depth_device_idx;

        lazy<std::vector<uint8_t>> _coefficients_table_raw;
        lazy<std::vector<uint8_t>> _new_calib_table_raw;

        std::unique_ptr<polling_error_handler> _polling_error_handler;
        std::shared_ptr<lazy<rs2_extrinsics>> _left_right_extrinsics;
        bool _is_locked = true;
    };

    class ds5u_device : public ds5_device
    {
    public:
        ds5u_device(std::shared_ptr<context> ctx,
            const platform::backend_device_group& group);

        std::shared_ptr<synthetic_sensor> create_ds5u_depth_device(std::shared_ptr<context> ctx,
            const std::vector<platform::uvc_device_info>& all_device_infos);

    protected:
        friend class ds5u_depth_sensor;
    };

    class ds5_notification_decoder : public notification_decoder
    {
    public:
        notification decode(int value) override;
    };

    processing_blocks get_ds5_depth_recommended_proccesing_blocks();
}
