// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once
#include "hw-monitor.h"
#include "l500-device.h"

namespace librealsense
{
    enum l500_control
    {
        confidence = 0,
        post_processing_sharpness = 1,
        pre_processing_sharpness = 2,
        noise_filtering = 3,
        apd = 4,
        laser_gain = 5,
        min_distance = 6,
        invalidation_bypass = 7
    };

    enum l500_command
    {
        get_current = 0,
        get_min = 1,
        get_max = 2,
        get_step = 3
    };

    class l500_hw_options : public option
    {
    public:
        float query() const override;

        void set(float value) override;

        option_range get_range() const override;

        bool is_enabled() const override { return true; }

        const char* get_description() const override { return ""; }

        l500_hw_options(hw_monitor* hw_monitor, l500_control type, option* resolution);

        void enable_recording(std::function<void(const option&)> recording_action) override;

    private:
        float query(int width) const;

        l500_control _type;
        hw_monitor* _hw_monitor;
        option_range _range;
        uint32_t _width;
        uint32_t _height;
        option* _resolution;
    };

    class l500_options: public virtual l500_device
    {
    public:
        l500_options(std::shared_ptr<context> ctx,
            const platform::backend_device_group& group);

        std::vector<rs2_option> get_advanced_controls();

    private:
        void on_set_option(rs2_option opt, float value);
        void change_preset(rs2_l500_visual_preset preset);
        void move_to_custom ();
        void reset_hw_controls();
        void set_max_laser();

        std::map<rs2_option, std::shared_ptr<cascade_option<l500_hw_options>>> _hw_options;
        std::shared_ptr< cascade_option<uvc_xu_option<int>>> _ambient_light;
        std::shared_ptr< cascade_option<float_option_with_description<rs2_l500_visual_preset>>> _preset;

        template<typename T, class ... Args>
        std::shared_ptr<cascade_option<T>> register_option(rs2_option opt, Args... args)
        {
            auto& depth_sensor = get_depth_sensor();

            auto signaled_opt = std::make_shared <cascade_option<T>>(std::forward<Args>(args)...);
            signaled_opt->add_observer([opt, this](float val) {on_set_option(opt, val);});
            depth_sensor.register_option(opt, std::dynamic_pointer_cast<option>(signaled_opt));

            return signaled_opt;
        }
    };

} // namespace librealsense
