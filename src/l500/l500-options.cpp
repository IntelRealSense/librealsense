//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "l500-options.h"
#include "l500-private.h"
#include "l500-depth.h"

const std::string MIN_CONTROLS_FW_VERSION("1.3.9.0");

namespace librealsense
{
    using namespace ivcam2;

    float l500_hw_options::query() const
    {
        return query(_resolution->query());
    }

    void l500_hw_options::set(float value)
    {
        _hw_monitor->send(command{ AMCSET, _type, (int)value });
    }

    option_range l500_hw_options::get_range() const
    {
        return _range;
    }

    l500_hw_options::l500_hw_options(hw_monitor* hw_monitor, l500_control type, option* resolution)
        :_hw_monitor(hw_monitor),
        _type(type),
        _resolution(resolution)
    {
        auto min = _hw_monitor->send(command{ AMCGET, _type, get_min });
        auto max = _hw_monitor->send(command{ AMCGET, _type, get_max });
        auto step = _hw_monitor->send(command{ AMCGET, _type, get_step });

        auto def = query(_resolution->query());

        if (min.size() < sizeof(int32_t) || max.size() < sizeof(int32_t) || step.size() < sizeof(int32_t))
        {
            std::stringstream s;
            s << "Size of data returned is not valid min size = " << min.size() << ", max size = " << max.size() << ", step size = " << step.size();
            throw std::runtime_error(s.str());
        }

        _range = option_range{ float(*(reinterpret_cast<int32_t*>(min.data()))),
            float(*(reinterpret_cast<int32_t*>(max.data()))),
            float(*(reinterpret_cast<int32_t*>(step.data()))),
            def };
    }

    void l500_hw_options::enable_recording(std::function<void(const option&)> recording_action)
    {}

    float l500_hw_options::query(int mode) const
    {
        auto res = _hw_monitor->send(command{ AMCGET, _type, get_current, mode });

        if (res.size() < sizeof(int32_t))
        {
            std::stringstream s;
            s << "Size of data returned is not valid min size = " << res.size();
            throw std::runtime_error(s.str());
        }

        auto val = *(reinterpret_cast<int32_t*>((void*)res.data()));
        return val;
    }

    l500_options::l500_options(std::shared_ptr<context> ctx, const platform::backend_device_group & group) :
        device(ctx, group),
        l500_device(ctx, group)
    {
        auto& raw_depth_sensor = get_raw_depth_sensor();
        auto& depth_sensor = get_depth_sensor();

        if (_fw_version < firmware_version(MIN_CONTROLS_FW_VERSION))
        {
            depth_sensor.register_option
            (RS2_OPTION_VISUAL_PRESET, std::make_shared<uvc_xu_option<int >>(raw_depth_sensor, ivcam2::depth_xu, ivcam2::L500_AMBIENT,
                "Change the depth ambient light to ambient: 1 for no ambient and 2 for low ambient",
                std::map<float, std::string>{ { RS2_AMBIENT_LIGHT_NO_AMBIENT, "No Ambient"},
                { RS2_AMBIENT_LIGHT_LOW_AMBIENT, "Low Ambient" }}));
        }
        else
        {
            auto resolution_option = std::make_shared<float_option_with_description<rs2_sensor_mode>>(option_range{ RS2_SENSOR_MODE_VGA,RS2_SENSOR_MODE_XGA,1, RS2_SENSOR_MODE_XGA }, "Notify the sensor about the intended streaming mode. Required for preset ");

            depth_sensor.register_option(RS2_OPTION_SENSOR_MODE, resolution_option);

            _hw_options[RS2_OPTION_POST_PROCESSING_SHARPENING] = register_option<l500_hw_options, hw_monitor*, l500_control, option*>(RS2_OPTION_POST_PROCESSING_SHARPENING, _hw_monitor.get(), post_processing_sharpness, resolution_option.get());
            _hw_options[RS2_OPTION_PRE_PROCESSING_SHARPENING] = register_option<l500_hw_options, hw_monitor*, l500_control, option*>(RS2_OPTION_PRE_PROCESSING_SHARPENING, _hw_monitor.get(), pre_processing_sharpness, resolution_option.get());
            _hw_options[RS2_OPTION_NOISE_FILTERING] = register_option<l500_hw_options, hw_monitor*, l500_control, option*>(RS2_OPTION_NOISE_FILTERING, _hw_monitor.get(), noise_filtering, resolution_option.get());
            _hw_options[RS2_OPTION_AVALANCHE_PHOTO_DIODE] = register_option<l500_hw_options, hw_monitor*, l500_control, option*>(RS2_OPTION_AVALANCHE_PHOTO_DIODE, _hw_monitor.get(), apd, resolution_option.get());
            _hw_options[RS2_OPTION_CONFIDENCE_THRESHOLD] = register_option<l500_hw_options, hw_monitor*, l500_control, option*>(RS2_OPTION_CONFIDENCE_THRESHOLD, _hw_monitor.get(), confidence, resolution_option.get());
            _hw_options[RS2_OPTION_LASER_POWER] = register_option<l500_hw_options, hw_monitor*, l500_control, option*>(RS2_OPTION_LASER_POWER, _hw_monitor.get(), laser_gain, resolution_option.get());
            _hw_options[RS2_OPTION_MIN_DISTANCE] = register_option<l500_hw_options, hw_monitor*, l500_control, option*>(RS2_OPTION_MIN_DISTANCE, _hw_monitor.get(), min_distance, resolution_option.get());
            _hw_options[RS2_OPTION_INVALIDATION_BYPASS] = register_option<l500_hw_options, hw_monitor*, l500_control, option*>(RS2_OPTION_INVALIDATION_BYPASS, _hw_monitor.get(), invalidation_bypass, resolution_option.get());

            _ambient_light = register_option<uvc_xu_option<int>, uvc_sensor&, platform::extension_unit, uint8_t, std::string, const std::map<float, std::string>& >
                (RS2_OPTION_AMBIENT_LIGHT, raw_depth_sensor, ivcam2::depth_xu, ivcam2::L500_AMBIENT,
                    "Change the depth ambient light to ambient: 1 for no ambient and 2 for low ambient",
                    std::map<float, std::string>{ { RS2_AMBIENT_LIGHT_NO_AMBIENT, "No Ambient"},
                    { RS2_AMBIENT_LIGHT_LOW_AMBIENT, "Low Ambient" }});


            _preset = register_option <float_option_with_description<rs2_l500_visual_preset>, option_range>
                (RS2_OPTION_VISUAL_PRESET, option_range{ RS2_L500_VISUAL_PRESET_CUSTOM , RS2_L500_VISUAL_PRESET_SHORT_RANGE, 1, RS2_L500_VISUAL_PRESET_DEFAULT },
                    "Preset to calibrate the camera to environment ambient, no ambient or low ambient. 1 is no ambient and 2 is low ambient");

            _advanced_options = get_advanced_controls();
        }
    }

    std::vector<rs2_option> l500_options::get_advanced_controls()
    {
        std::vector<rs2_option> res;

        res.push_back(RS2_OPTION_AMBIENT_LIGHT);
        for (auto&& o : _hw_options)
            res.push_back(o.first);

        return res;
    }

    void l500_options::on_set_option(rs2_option opt, float value)
    {
        if (opt == RS2_OPTION_VISUAL_PRESET)
        {
            change_preset(static_cast<rs2_l500_visual_preset>(int(value)));
        }
        else
        {
            auto advanced_controls = get_advanced_controls();
            if (std::find(advanced_controls.begin(), advanced_controls.end(), opt) != advanced_controls.end())
                move_to_custom ();
            else
                throw  wrong_api_call_sequence_exception(to_string() << "on_set_option support advanced controls only "<< opt<<" injected");
        }
    }

    void l500_options::change_preset(rs2_l500_visual_preset preset)
    {
        if (preset != RS2_L500_VISUAL_PRESET_CUSTOM)
            reset_hw_controls();

        switch (preset)
        {
        case RS2_L500_VISUAL_PRESET_NO_AMBIENT:
            _ambient_light->set_with_no_signal(RS2_AMBIENT_LIGHT_NO_AMBIENT);
            break;
        case RS2_L500_VISUAL_PRESET_LOW_AMBIENT:
            _ambient_light->set_with_no_signal(RS2_AMBIENT_LIGHT_LOW_AMBIENT);
            set_max_laser();
            break;
        case RS2_L500_VISUAL_PRESET_MAX_RANGE:
            _ambient_light->set_with_no_signal(RS2_AMBIENT_LIGHT_NO_AMBIENT);
            set_max_laser();
            break;
        case RS2_L500_VISUAL_PRESET_SHORT_RANGE:
            _ambient_light->set_with_no_signal(RS2_AMBIENT_LIGHT_LOW_AMBIENT);
            break;
        case RS2_L500_VISUAL_PRESET_CUSTOM:
            move_to_custom();
            break;
        default: break;
        };
    }

    void l500_options::move_to_custom ()
    {
        for (auto& o : _hw_options)
        {
            auto  val = o.second->query();
            o.second->set_with_no_signal(val);
        }
        _preset->set_with_no_signal(RS2_L500_VISUAL_PRESET_CUSTOM);
    }

    void l500_options::reset_hw_controls()
    {
        for (auto& o : _hw_options)
            o.second->set_with_no_signal(-1);
    }

    void l500_options::set_max_laser()
    {
        auto range = _hw_options[RS2_OPTION_LASER_POWER]->get_range();
        _hw_options[RS2_OPTION_LASER_POWER]->set_with_no_signal(range.max);
    }
}
