//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#include "l500-controls.h"
#include "l500-private.h"
#include "l500-depth.h"

#define CONTROLS_FIRMWARE_VERSION "1.3.8.0"

namespace librealsense
{
    using namespace ivcam2;

    float l500_hw_controls::query() const
    {
        return query(_resolution->query());
    }

    void l500_hw_controls::set(float value)
    {
        _hw_monitor->send(command{ AMCSET, _type, (int)value });
    }

    option_range l500_hw_controls::get_range() const
    {
        return _range;
    }

    l500_hw_controls::l500_hw_controls(hw_monitor* hw_monitor, l500_control type, option* resolution)
        :_hw_monitor(hw_monitor),
        _type(type),
        _resolution(resolution)
    {
        auto min = _hw_monitor->send(command{ AMCGET, _type, get_min });
        auto max = _hw_monitor->send(command{ AMCGET, _type, get_max });
        auto step = _hw_monitor->send(command{ AMCGET, _type, get_step });

        auto def = query(_resolution->query());

        _range = option_range{ float(*(reinterpret_cast<int*>(min.data()))),
            float(*(reinterpret_cast<int*>(max.data()))),
            float(*(reinterpret_cast<int*>(step.data()))),
            def };
    }

    void l500_hw_controls::enable_recording(std::function<void(const option&)> recording_action)
    {}

    float l500_hw_controls::query(int mode) const
    {
        auto res = _hw_monitor->send(command{ AMCGET, _type, get_current, mode });
        auto val = *(reinterpret_cast<int*>((void*)res.data()));
        return val;
    }

    l500_controls::l500_controls(std::shared_ptr<context> ctx, const platform::backend_device_group & group) :
        device(ctx, group),
        l500_device(ctx, group)
    {
        auto& raw_depth_sensor = get_raw_depth_sensor();
        auto& depth_sensor = get_depth_sensor();

        if (_fw_version < firmware_version(CONTROLS_FIRMWARE_VERSION))
        {
            depth_sensor.register_option
            (RS2_OPTION_VISUAL_PRESET, std::make_shared<uvc_xu_option<int >>(raw_depth_sensor, ivcam2::depth_xu, ivcam2::L500_AMBIENT,
                "Canges the depth sensetivity to ambient: 1 for no ambient and 2 for low ambient",
                std::map<float, std::string>{ { sensetivity_long, "No Ambient"},
                { sensetivity_short, "Low Ambient" }}));
        }
        else
        {
            auto resolution_option = std::make_shared<float_option_with_description>(option_range{ XGA,VGA,1, XGA }, "Resolution mode", std::map<float, std::string>{
                { XGA, "XGA"},
                { VGA, "VGA" }});

            depth_sensor.register_option(RS2_OPTION_CAMERA_MODE, resolution_option);

            _hw_options[RS2_OPTION_POST_PROCESSING_SHARPENING] = register_option<l500_hw_controls, hw_monitor*, l500_control, option*>(RS2_OPTION_POST_PROCESSING_SHARPENING, _hw_monitor.get(), post_processing_sharpness, resolution_option.get());
            _hw_options[RS2_OPTION_PRE_PROCESSING_SHARPENING] = register_option<l500_hw_controls, hw_monitor*, l500_control, option*>(RS2_OPTION_PRE_PROCESSING_SHARPENING, _hw_monitor.get(), pre_processing_sharpness, resolution_option.get());
            _hw_options[RS2_OPTION_NOISE_FILTERING] = register_option<l500_hw_controls, hw_monitor*, l500_control, option*>(RS2_OPTION_NOISE_FILTERING, _hw_monitor.get(), noise_filtering, resolution_option.get());
            _hw_options[RS2_OPTION_AVALANCHE_PHOTO_DIODE] = register_option<l500_hw_controls, hw_monitor*, l500_control, option*>(RS2_OPTION_AVALANCHE_PHOTO_DIODE, _hw_monitor.get(), apd, resolution_option.get());
            _hw_options[RS2_OPTION_CONFIDENCE_THRESHOLD] = register_option<l500_hw_controls, hw_monitor*, l500_control, option*>(RS2_OPTION_CONFIDENCE_THRESHOLD, _hw_monitor.get(), confidence, resolution_option.get());
            _hw_options[RS2_OPTION_LASER_POWER] = register_option<l500_hw_controls, hw_monitor*, l500_control, option*>(RS2_OPTION_LASER_POWER, _hw_monitor.get(), laser_gain, resolution_option.get());
            _hw_options[RS2_OPTION_MIN_DISTANCE] = register_option<l500_hw_controls, hw_monitor*, l500_control, option*>(RS2_OPTION_MIN_DISTANCE, _hw_monitor.get(), min_distance, resolution_option.get());
            _hw_options[RS2_OPTION_INVALIDATION_BYPASS] = register_option<l500_hw_controls, hw_monitor*, l500_control, option*>(RS2_OPTION_INVALIDATION_BYPASS, _hw_monitor.get(), invalidation_bypass, resolution_option.get());

            _sensitivity = register_option<uvc_xu_option<int>, uvc_sensor&, platform::extension_unit, uint8_t, std::string, const std::map<float, std::string>& >
                (RS2_OPTION_SENSETIVITY, raw_depth_sensor, ivcam2::depth_xu, ivcam2::L500_AMBIENT,
                    "Canges the depth sensetivity to ambient: 1 for no ambient and 2 for low ambient",
                    std::map<float, std::string>{ { sensetivity_long, "No Ambient"},
                    { sensetivity_short, "Low Ambient" }});


            _preset = register_option <float_option_with_description, option_range, std::string, const std::map<float, std::string >>
                (RS2_OPTION_VISUAL_PRESET, option_range{ custom , short_range, 1, custom  },
                    "Preset to calibrate the camera to environment ambient no ambient or low ambient. 1 is no ambient and 2 is low ambient",
                    std::map<float, std::string>{
                        { custom , "custom " },
                        { no_ambient, "No Ambient" },
                        { low_ambient, "Low Ambient-max range" },
                        { max_range ,"No Ambient max range" },
                        { short_range ,"Low Ambient " }});

            _advanced_option = get_advanced_controlls();
        }
    }

    std::vector<rs2_option> l500_controls::get_advanced_controlls()
    {
        std::vector<rs2_option> res;

        res.push_back(RS2_OPTION_SENSETIVITY);
        for (auto o : _hw_options)
            res.push_back(o.first);

        return res;
    }

    void l500_controls::on_set_option(rs2_option opt, float value)
    {
        if (opt == RS2_OPTION_VISUAL_PRESET)
        {
            change_preset(static_cast<preset_values>(int(value)));
        }
        else
        {
            move_to_custom ();
        }
    }

    void l500_controls::change_preset(preset_values preset)
    {
        if (preset != custom )
            reset_hw_controls();

        switch (preset)
        {
        case no_ambient:
            _sensitivity->set_with_no_signal(sensetivity_long);
            break;
        case low_ambient:
            _sensitivity->set_with_no_signal(sensetivity_short);
            set_max_laser();
            break;
        case max_range:
            _sensitivity->set_with_no_signal(sensetivity_long);
            set_max_laser();
            break;
        case short_range:
            _sensitivity->set_with_no_signal(sensetivity_short);
            break;
        case custom :
            move_to_custom ();
            break;
        default: break;
        };
    }

    void l500_controls::move_to_custom ()
    {
        for (auto o : _hw_options)
        {
            auto  val = o.second->query();
            o.second->set_with_no_signal(val);
        }
        _preset->set_with_no_signal(custom );
    }

    void l500_controls::reset_hw_controls()
    {
        for (auto o : _hw_options)
            o.second->set_with_no_signal(-1);
    }

    void l500_controls::set_max_laser()
    {
        auto range = _hw_options[RS2_OPTION_LASER_POWER]->get_range();
        _hw_options[RS2_OPTION_LASER_POWER]->set_with_no_signal(range.max);
    }
}
