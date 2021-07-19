// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once
#include "hw-monitor.h"
#include "l500-device.h"

namespace librealsense {
namespace ivcam2 {
namespace l535 {
    enum amc_control
    {
        confidence = 0,
        post_processing_sharpness = 1,
        pre_processing_sharpness = 2,
        noise_filtering = 3,
        receiver_sensitivity = 4,
        laser_gain = 5,
        min_distance = 6,
        invalidation_bypass = 7,
        alternate_ir = 8,
        auto_rx_sensitivity = 9,
        transmitter_frequency = 10,
        vertical_binning = 11
    };

    enum amc_command
    {
        get_current = 0,
        get_min = 1,
        get_max = 2,
        get_step = 3,
        get_default = 4
    };

    class amc_option : public option
    {
    public:
        float query() const override;

        void set( float value ) override;

        option_range get_range() const override;

        bool is_enabled() const override { return true; }

        const char * get_description() const override { return _description.c_str(); }

        void enable_recording( std::function< void( const option & ) > recording_action ) override;

        amc_option( l500_device * l500_dev,
                    hw_monitor * hw_monitor,
                    amc_control type,
                    const std::string & description );

    private:
        float query_default() const;

        amc_control _control;
        l500_device * _device;
        hw_monitor * _hw_monitor;
        option_range _range;
        std::string _description;
    };
}  // namespace l535
}  // namespace ivcam2
}  // namespace librealsense