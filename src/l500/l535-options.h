// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once
#include "hw-monitor.h"
#include "l500-device.h"

namespace librealsense
{
    enum l535_control
    {
        l535_confidence = 0,
        l535_post_processing_sharpness = 1,
        l535_pre_processing_sharpness = 2,
        l535_noise_filtering = 3,
        l535_apd = 4,
        l535_laser_gain = 5,
        l535_min_distance = 6,
        l535_invalidation_bypass = 7,
        l535_alternate_ir = 8,
        l535_rx_sensitivity = 9,
    };

    enum l535_command
    {
        l535_get_current = 0,
        l535_get_min = 1,
        l535_get_max = 2,
        l535_get_step = 3,
        l535_get_default = 4
    };

    typedef uvc_xu_option< int > super;

    class l535_hw_options : public option
    {
    public:
        float query() const override;

        void set(float value) override;

        option_range get_range() const override;

        bool is_enabled() const override { return true; }

        const char * get_description() const override { return _description.c_str(); }

        void enable_recording(std::function<void(const option&)> recording_action) override;

        l535_hw_options( l500_device* l500_dev,
                         hw_monitor* hw_monitor,
                         l535_control type,
                         const std::string & description);

        float query_current( rs2_sensor_mode mode ) const;

    private:
        float query_default() const;

        l535_control _type;
        l500_device* _l500_dev;
        hw_monitor* _hw_monitor;
        option_range _range;
        std::string _description;
    };

    class l535_preset_option : public float_option_with_description< rs2_l500_visual_preset >
    {
    public:
        l535_preset_option( option_range range, std::string description);
        void set( float value ) override;
        void set_value( float value );
    };

    class l535_options : public virtual l500_device
    {
    public:
        l535_options(std::shared_ptr<context> ctx,
            const platform::backend_device_group& group);

    };

} // namespace librealsense
