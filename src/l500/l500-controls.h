// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#pragma once
#include "hw-monitor.h"
#include "l500-device.h"
//#include "option.h"

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

    enum preset_values
    {
        custom  = 0,
        no_ambient = 1,
        low_ambient = 2,
        max_range = 3,
        short_range = 4
    };

    enum sensetivity
    {
        sensetivity_long = 1,
        sensetivity_short = 2
    };

    enum resulotion
    {
        XGA = 0,
        VGA = 1
    };

    class l500_hw_controls : public option
    {
    public:
        float query() const override;

        void set(float value) override;

        option_range get_range() const override;

        bool is_enabled() const override { return true; }

        const char* get_description() const override { return ""; }

        l500_hw_controls(hw_monitor* hw_monitor, l500_control type, option* resolution);

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

    template<class T>
    class signaled_option : public T, public observable_option
    {
    public:
        template <class... Args>
        signaled_option(rs2_option opt, Args&&... args) :
            T(std::forward<Args>(args)...), _opt(opt){}

        void set(float value) override
        {
            notify(value);
            T::set(value);
        }

        void set_with_no_signal(float value)
        {
            T::set(value);
        }
    private:
        std::function<void(rs2_option, float)> _callback;
        rs2_option _opt;
    };

    class l500_controls: public virtual l500_device
    {
    public:
        l500_controls(std::shared_ptr<context> ctx,
            const platform::backend_device_group& group);

        std::vector<rs2_option> get_advanced_controlls();

    private:
        void on_set_option(rs2_option opt, float value);
        void change_preset(preset_values preset);
        void move_to_custom ();
        void reset_hw_controls();
        void set_max_laser();

        std::map<rs2_option, std::shared_ptr<signaled_option<l500_hw_controls>>> _hw_options;
        std::shared_ptr< signaled_option<uvc_xu_option<int>>> _sensitivity;
        std::shared_ptr< signaled_option<float_option_with_description>> _preset;

        template<typename T, class ... Args>
        std::shared_ptr<signaled_option<T>> register_option(rs2_option opt, Args... args)
        {
            auto& depth_sensor = get_depth_sensor();

            auto signaled_opt = std::make_shared <signaled_option<T>>(opt, std::forward<Args>(args)...);
            signaled_opt->add_observer([opt, this](float val) {on_set_option(opt, val);});
            depth_sensor.register_option(opt, std::dynamic_pointer_cast<option>(signaled_opt));

            return signaled_opt;
        }
    };

} // namespace librealsense
