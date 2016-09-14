// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef LIBREALSENSE_ds_device_H
#define LIBREALSENSE_ds_device_H

#include "device.h"
#include "ds-private.h"

#define R200_PRODUCT_ID  0x0a80
#define LR200_PRODUCT_ID 0x0abf
#define ZR300_PRODUCT_ID 0x0acb
#define FISHEYE_PRODUCT_ID 0x0ad0

namespace rsimpl
{
    namespace ds
    {
        enum subdevices
        {
            SUB_DEVICE_INFRARED = 0,
            SUB_DEVICE_DEPTH = 1,
            SUB_DEVICE_COLOR = 2,
            SUB_DEVICE_FISHEYE = 3,
        };
        /*
        ds_device class is interface that provides partial implementation for ds cameras line functionalities and properties
        */
        class ds_device : public rs_device_base
        {
        protected:
            // This single function interface enforces implementation by subclasses
            //virtual std::shared_ptr<rs_device> make_device(std::shared_ptr<uvc::device> device, static_device_info& info, ds::ds_calibration& c) abstract;
            bool is_disparity_mode_enabled() const;
            void on_update_depth_units(uint32_t units);
            void on_update_disparity_multiplier(double multiplier);
            uint32_t get_lr_framerate() const;
            std::vector<supported_option> get_ae_range_vec();
            time_pad start_stop_pad; // R200 line-up needs minimum 500ms delay between consecutive start-stop commands

        public:
            ds_device(std::shared_ptr<uvc::device> device, const static_device_info & info, calibration_validator validator);
            ~ds_device();

            bool supports_option(rs_option option) const override;
            void get_option_range(rs_option option, double & min, double & max, double & step, double & def) override;
            void set_options(const rs_option options[], size_t count, const double values[]) override;
            void get_options(const rs_option options[], size_t count, double values[]) override;

            void on_before_start(const std::vector<subdevice_mode_selection> & selected_modes) override;
            rs_stream select_key_stream(const std::vector<rsimpl::subdevice_mode_selection> & selected_modes) override;
            std::shared_ptr<frame_timestamp_reader> create_frame_timestamp_reader(int subdevice) const;
            std::vector<std::shared_ptr<frame_timestamp_reader>> create_frame_timestamp_readers() const override;

            static void set_common_ds_config(std::shared_ptr<uvc::device> device, static_device_info& info, const ds::ds_info& cam_info);

            virtual void stop(rs_source source) override;
            virtual void start(rs_source source) override;

            virtual void start_fw_logger(char fw_log_op_code, int grab_rate_in_ms, std::timed_mutex& mutex) override;
            virtual void stop_fw_logger() override;
        };
    }
}

#endif // ds_device_H
