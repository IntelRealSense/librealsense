// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef LIBREALSENSE_RS4XX_H
#define LIBREALSENSE_RS4XX_H

#include "device.h"

namespace rsimpl
{
    enum class rs4xx_command : uint32_t
    {
        GVD         = 0x10,     // Get Version and Date
        GLD         = 0x0f,     // Get logger data
    };

    // This is a base class for the various SKUs of the RSXX line of devices
    class rs4xx_camera : public rs_device_base
    {

    public:
        rs4xx_camera(std::shared_ptr<uvc::device> device, const static_device_info & info);
        ~rs4xx_camera() {};

        void set_options(const rs_option options[], size_t count, const double values[]) override;
        void get_options(const rs_option options[], size_t count, double values[]) override;

        void transmit_raw_data(rs_raw_buffer & buffer) override;

        void on_before_start(const std::vector<subdevice_mode_selection> & selected_modes) override;
        rs_stream select_key_stream(const std::vector<rsimpl::subdevice_mode_selection> & selected_modes) override;
        std::vector<std::shared_ptr<rsimpl::frame_timestamp_reader>> create_frame_timestamp_readers() const override;

        void set_fw_logger_option(double value);
        unsigned get_fw_logger_option();

    private:
        std::timed_mutex usbMutex;
    };
}

#endif
