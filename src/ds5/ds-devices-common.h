// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include "hw-monitor.h"
#include "ds5-private.h"


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

    enum ds_device_type
    {
        ds5,
        ds6
    };

    class ds_devices_common
    {
    public:
        ds_devices_common(device* ds_device, ds_device_type dev_type, std::shared_ptr<hw_monitor> hwm) :
            _owner(ds_device),
            _ds_device_type(dev_type), 
            _hw_monitor(hwm)
        {
            init_fourcc_maps();
        }

        void enter_update_state() const;
        std::vector<uint8_t> backup_flash(update_progress_callback_ptr callback);
        void update_flash(const std::vector<uint8_t>& image, update_progress_callback_ptr callback, int update_mode);
        bool check_fw_compatibility(const std::vector<uint8_t>& image) const;

        bool is_camera_in_advanced_mode() const;
        bool is_locked(uint8_t gvd_cmd, uint32_t offset);

    private:
        void init_fourcc_maps();

        uvc_sensor& get_raw_depth_sensor();

        friend class ds5_depth_sensor;
        friend class ds6_depth_sensor;

        device* _owner;
        ds_device_type _ds_device_type;
        std::shared_ptr<hw_monitor> _hw_monitor;
        std::map<uint32_t, rs2_format> ds_depth_fourcc_to_rs2_format;
        std::map<uint32_t, rs2_stream> ds_depth_fourcc_to_rs2_stream;
        bool _is_locked;
    };

    class ds_notification_decoder : public notification_decoder
    {
    public:
        notification decode(int value) override;
    };

    processing_blocks get_ds_depth_recommended_proccesing_blocks();
}