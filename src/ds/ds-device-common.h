// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include "hw-monitor.h"
#include "ds-private.h"


namespace librealsense
{
    class ds_auto_exposure_roi_method : public region_of_interest_method
    {
    public:
        explicit ds_auto_exposure_roi_method(const hw_monitor& hwm,
            ds::fw_cmd cmd = ds::fw_cmd::SETAEROI);

        void set(const region_of_interest& roi) override;
        region_of_interest get() const override;
    private:
        const ds::fw_cmd _cmd;
        const hw_monitor& _hw_monitor;
    };

    class ds_device_common
    {
    public:
        ds_device_common(device* ds_device, std::shared_ptr<hw_monitor> hwm) :
            _owner(ds_device),
            _hw_monitor(hwm),
            _is_locked(false)
        {}

        void enter_update_state() const;
        std::vector<uint8_t> backup_flash(update_progress_callback_ptr callback);
        void update_flash(const std::vector<uint8_t>& image, update_progress_callback_ptr callback, int update_mode);

        bool is_camera_in_advanced_mode() const;
        bool is_locked(uint8_t gvd_cmd, uint32_t offset);
        void get_fw_details( const std::vector<uint8_t> &gvd_buff, std::string& optic_serial, std::string& asic_serial, std::string& fwv ) const;

    private:
        uvc_sensor& get_raw_depth_sensor();

        device* _owner;
        std::shared_ptr<hw_monitor> _hw_monitor;
        bool _is_locked;
    };

    class ds_notification_decoder : public notification_decoder
    {
    public:
        notification decode(int value) override;
    };

    processing_blocks get_ds_depth_recommended_proccesing_blocks();
}
