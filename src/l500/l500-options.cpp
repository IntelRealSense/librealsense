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
        return query(int(_resolution->query()));
    }

    void l500_hw_options::set(float value)
    {
        // Block activating alternate IR when IR reflectivity is on [RS5-8358]
        auto &ds = _l500_dev->get_depth_sensor();
        if( ( alternate_ir == _type ) && ( 1.0f == value ) )
        {
            if( ds.supports_option( RS2_OPTION_ENABLE_IR_REFLECTIVITY )
                && ds.get_option( RS2_OPTION_ENABLE_IR_REFLECTIVITY ).query() == 1.0f )
                throw librealsense::wrong_api_call_sequence_exception(
                    "Alternate IR cannot be enabled with IR Reflectivity" );
        }

        _hw_monitor->send(command{ AMCSET, _type, (int)value });
    }

    option_range l500_hw_options::get_range() const
    {
        return _range;
    }

    l500_hw_options::l500_hw_options( l500_device* l500_dev,
                                      hw_monitor * hw_monitor,
                                      l500_control type,
                                      option * resolution,
                                      const std::string & description )
        : _l500_dev( l500_dev )
        , _hw_monitor( hw_monitor )
        , _type( type )
        , _resolution( resolution )
        , _description( description )
    {
        auto min = _hw_monitor->send(command{ AMCGET, _type, get_min });
        auto max = _hw_monitor->send(command{ AMCGET, _type, get_max });
        auto step = _hw_monitor->send(command{ AMCGET, _type, get_step });

        auto def = query(int(_resolution->query()));

        if (min.size() < sizeof(int32_t) || max.size() < sizeof(int32_t) || step.size() < sizeof(int32_t))
        {
            std::stringstream s;
            s << "Size of data returned is not valid min size = " << min.size() << ", max size = " << max.size() << ", step size = " << step.size();
            throw std::runtime_error(s.str());
        }

        auto max_value = float(*(reinterpret_cast<int32_t*>(max.data())));
        auto min_value = float(*(reinterpret_cast<int32_t*>(min.data())));

        _range = option_range{ min_value,
            max_value,
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
        return float(val);
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
            (RS2_OPTION_VISUAL_PRESET, std::make_shared<uvc_xu_option<int >>(raw_depth_sensor, ivcam2::depth_xu, ivcam2::L500_DIGITAL_GAIN,
                "Change the depth digital gain to: 1 for high gain and 2 for low gain",
                std::map<float, std::string>{ { float(RS2_DIGITAL_GAIN_HIGH), "High Gain"},
                { float(RS2_DIGITAL_GAIN_LOW), "Low Gain" }}));
        }
        else
        {
            // On USB2 we have only QVGA sensor mode
            bool usb3mode = (_usb_mode >= platform::usb3_type || _usb_mode == platform::usb_undefined);

            auto default_sensor_mode = static_cast<float>(usb3mode ? RS2_SENSOR_MODE_VGA : RS2_SENSOR_MODE_QVGA);

            auto resolution_option = std::make_shared<sensor_mode_option>(this, option_range{ RS2_SENSOR_MODE_VGA,RS2_SENSOR_MODE_COUNT - 1,1, default_sensor_mode }, "Notify the sensor about the intended streaming mode. Required for preset ");

            depth_sensor.register_option(RS2_OPTION_SENSOR_MODE, resolution_option);

            if( _fw_version >= firmware_version( "1.5.2.0" ) )
            {
                auto alt_ir = std::make_shared< l500_hw_options >( this,
                                                                   _hw_monitor.get(),
                                                                   alternate_ir,
                                                                   resolution_option.get(),
                                                                   "Enable/Disable alternate IR" );

                depth_sensor.register_option( RS2_OPTION_ALTERNATE_IR, alt_ir );
            }

            _hw_options[RS2_OPTION_POST_PROCESSING_SHARPENING] = register_option< l500_hw_options,
                                                                                  l500_device *,
                                                                                  hw_monitor *,
                                                                                  l500_control,
                                                                                  option *,
                                                                                  std::string >(
                RS2_OPTION_POST_PROCESSING_SHARPENING,
                this,
                _hw_monitor.get(),
                post_processing_sharpness,
                resolution_option.get(),
                "Changes the amount of sharpening in the post-processed image" );

            _hw_options[RS2_OPTION_PRE_PROCESSING_SHARPENING] = register_option< l500_hw_options,
                                                                                 l500_device *,
                                                                                 hw_monitor *,
                                                                                 l500_control,
                                                                                 option *,
                                                                                 std::string >(
                RS2_OPTION_PRE_PROCESSING_SHARPENING,
                this,
                _hw_monitor.get(),
                pre_processing_sharpness,
                resolution_option.get(),
                "Changes the amount of sharpening in the pre-processed image" );

            _hw_options[RS2_OPTION_NOISE_FILTERING]
                = register_option< l500_hw_options,
                                   l500_device *,
                                   hw_monitor *,
                                   l500_control,
                                   option *,
                                   std::string >( RS2_OPTION_NOISE_FILTERING,
                                                  this,
                                                  _hw_monitor.get(),
                                                  noise_filtering,
                                                  resolution_option.get(),
                                                  "Control edges and background noise" );

            _hw_options[RS2_OPTION_AVALANCHE_PHOTO_DIODE] = register_option< l500_hw_options,
                                                                             l500_device *,
                                                                             hw_monitor *,
                                                                             l500_control,
                                                                             option *,
                                                                             std::string >(
                RS2_OPTION_AVALANCHE_PHOTO_DIODE,
                this,
                _hw_monitor.get(),
                apd,
                resolution_option.get(),
                "Changes the exposure time of Avalanche Photo Diode in the receiver" );

            _hw_options[RS2_OPTION_CONFIDENCE_THRESHOLD]
                = register_option< l500_hw_options,
                                   l500_device *,
                                   hw_monitor *,
                                   l500_control,
                                   option *,
                                   std::string >( RS2_OPTION_CONFIDENCE_THRESHOLD,
                                                  this,
                                                  _hw_monitor.get(),
                                                  confidence,
                                                  resolution_option.get(),
                                                  "The confidence level threshold to use to mark a "
                                                  "pixel as valid by the depth algorithm" );

            _hw_options[RS2_OPTION_LASER_POWER] = register_option< l500_hw_options,
                                                                   l500_device *,
                                                                   hw_monitor *,
                                                                   l500_control,
                                                                   option *,
                                                                   std::string >(
                RS2_OPTION_LASER_POWER,
                this,
                _hw_monitor.get(),
                laser_gain,
                resolution_option.get(),
                "Power of the laser emitter, with 0 meaning projector off" );

            _hw_options[RS2_OPTION_MIN_DISTANCE]
                = register_option< l500_hw_options,
                                   l500_device *,
                                   hw_monitor *,
                                   l500_control,
                                   option *,
                                   std::string >( RS2_OPTION_MIN_DISTANCE,
                                                  this,
                                                  _hw_monitor.get(),
                                                  min_distance,
                                                  resolution_option.get(),
                                                  "Minimal distance to the target (in mm)" );

            _hw_options[RS2_OPTION_INVALIDATION_BYPASS]
                = register_option< l500_hw_options,
                                   l500_device *,
                                   hw_monitor *,
                                   l500_control,
                                   option *,
                                   std::string >( RS2_OPTION_INVALIDATION_BYPASS,
                                                  this,
                                                  _hw_monitor.get(),
                                                  invalidation_bypass,
                                                  resolution_option.get(),
                                                  "Enable/disable pixel invalidation" );

            _digital_gain = register_option<uvc_xu_option<int>, uvc_sensor&, platform::extension_unit, uint8_t, std::string, const std::map<float, std::string>& >
                (RS2_OPTION_DIGITAL_GAIN, raw_depth_sensor, ivcam2::depth_xu, ivcam2::L500_DIGITAL_GAIN,
                    "Change the depth digital gain to: 1 for high gain and 2 for low gain",
                    std::map<float, std::string>{ { RS2_DIGITAL_GAIN_HIGH, "High Gain"},
                    { RS2_DIGITAL_GAIN_LOW, "Low Gain" }});


            _preset = register_option <float_option_with_description<rs2_l500_visual_preset>, option_range>
                (RS2_OPTION_VISUAL_PRESET, option_range{ RS2_L500_VISUAL_PRESET_CUSTOM , RS2_L500_VISUAL_PRESET_SHORT_RANGE, 1, RS2_L500_VISUAL_PRESET_DEFAULT },
                    "Preset to calibrate the camera to environment ambient, no ambient or low ambient.");

            _advanced_options = get_advanced_controls();
        }
    }

    std::vector<rs2_option> l500_options::get_advanced_controls()
    {
        std::vector<rs2_option> res;

        res.push_back(RS2_OPTION_DIGITAL_GAIN);
        for (auto&& o : _hw_options)
            res.push_back(o.first);

        return res;
    }

    void l500_options::on_set_option(rs2_option opt, float value)
    {
        if (opt == RS2_OPTION_VISUAL_PRESET)
        {
            verify_max_usable_range_restrictions( opt, value );
            change_preset(static_cast<rs2_l500_visual_preset>(int(value)));
        }
        else
        {
            auto advanced_controls = get_advanced_controls();
            if (std::find(advanced_controls.begin(), advanced_controls.end(), opt) != advanced_controls.end())
            {
                verify_max_usable_range_restrictions( opt, value );
                move_to_custom();
            }
            else
                throw  wrong_api_call_sequence_exception(to_string() << "on_set_option support advanced controls only "<< opt<<" injected");
        }
    }

    void l500_options::change_preset(rs2_l500_visual_preset preset)
    {
        if ((preset != RS2_L500_VISUAL_PRESET_CUSTOM) &&
            (preset != RS2_L500_VISUAL_PRESET_DEFAULT))
            reset_hw_controls();

        switch (preset)
        {
        case RS2_L500_VISUAL_PRESET_NO_AMBIENT:
            _digital_gain->set_with_no_signal(RS2_DIGITAL_GAIN_HIGH);
            break;
        case RS2_L500_VISUAL_PRESET_LOW_AMBIENT:
            _digital_gain->set_with_no_signal(RS2_DIGITAL_GAIN_LOW);
            set_max_laser();
            break;
        case RS2_L500_VISUAL_PRESET_MAX_RANGE:
            _digital_gain->set_with_no_signal(RS2_DIGITAL_GAIN_HIGH);
            set_max_laser();
            break;
        case RS2_L500_VISUAL_PRESET_SHORT_RANGE:
            _digital_gain->set_with_no_signal(RS2_DIGITAL_GAIN_LOW);
            break;
        case RS2_L500_VISUAL_PRESET_CUSTOM:
            move_to_custom();
            break;
        case RS2_L500_VISUAL_PRESET_DEFAULT:
            LOG_ERROR("L515 Visual Preset option cannot be changed to Default");
            throw  invalid_value_exception(to_string() << "The Default preset signifies that the controls have not been changed "
                                                           "since initialization, the API does not support changing back to this state, "
                                                           "Please choose one of the other presets");
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

    void l500_options::verify_max_usable_range_restrictions( rs2_option opt, float value )
    {
        // Block changing visual preset while Max Usable Range is on [RS5-8358]
        if (get_depth_sensor().supports_option(RS2_OPTION_ENABLE_MAX_USABLE_RANGE)
            && (get_depth_sensor().get_option(RS2_OPTION_ENABLE_MAX_USABLE_RANGE).query() == 1.0))
        {
            if ((RS2_OPTION_VISUAL_PRESET == opt) && (value == RS2_L500_VISUAL_PRESET_MAX_RANGE))
                return;

            throw wrong_api_call_sequence_exception(
                to_string() << "Visual Preset cannot be changed while Max Usable Range is enabled");
        }
    }  

    void max_usable_range_option::set(float value)
    {
        // Restrictions for Max Usable Range option as required on [RS5-8358]
        auto& ds = _l500_depth_dev->get_depth_sensor();
        if (value == 1.0f)
        {
            auto &sensor_mode_option = ds.get_option(RS2_OPTION_SENSOR_MODE);
            auto sensor_mode = sensor_mode_option.query();
            bool sensor_mode_is_vga = (sensor_mode == rs2_sensor_mode::RS2_SENSOR_MODE_VGA);

            bool visual_preset_is_max_range = ds.is_max_range_preset();

            if (ds.is_streaming())
            {
                if (!sensor_mode_is_vga || !visual_preset_is_max_range)
                    throw wrong_api_call_sequence_exception("Please set 'VGA' resolution and 'Max Range' preset before enabling Max Usable Range");
            }
            else
            {
                if (!visual_preset_is_max_range)
                {
                    auto &visual_preset_option = ds.get_option(RS2_OPTION_VISUAL_PRESET);
                    visual_preset_option.set(rs2_l500_visual_preset::RS2_L500_VISUAL_PRESET_MAX_RANGE);
                    LOG_INFO("Visual Preset was changed to: " << visual_preset_option.get_value_description(rs2_l500_visual_preset::RS2_L500_VISUAL_PRESET_MAX_RANGE));
                }

                if (!sensor_mode_is_vga)
                {
                    sensor_mode_option.set(rs2_sensor_mode::RS2_SENSOR_MODE_VGA);
                    LOG_INFO("Sensor Mode was changed to: " << sensor_mode_option.get_value_description(rs2_sensor_mode::RS2_SENSOR_MODE_VGA));
                }
            }
        }
        else
        {
            if (ds.supports_option(RS2_OPTION_ENABLE_IR_REFLECTIVITY) && ds.get_option(RS2_OPTION_ENABLE_IR_REFLECTIVITY).query() == 1.0f)
            {
                ds.get_option(RS2_OPTION_ENABLE_IR_REFLECTIVITY).set(0.0f);
                LOG_INFO("IR Reflectivity was on - turning it off");
            }
        }

        bool_option::set(value);
    }


    void sensor_mode_option::set(float value)
    {
        // Restrictions for sensor mode option as required on [RS5-8358]
        auto &ds = _l500_depth_dev->get_depth_sensor();
        if (ds.supports_option(RS2_OPTION_ENABLE_IR_REFLECTIVITY) && ds.get_option(RS2_OPTION_ENABLE_IR_REFLECTIVITY).query() == 1.0f)
        {
            ds.get_option(RS2_OPTION_ENABLE_IR_REFLECTIVITY).set(0.0f);
            LOG_INFO("IR Reflectivity was on - turning it off");
        }

        if (ds.supports_option(RS2_OPTION_ENABLE_MAX_USABLE_RANGE) &&
            (ds.get_option(RS2_OPTION_ENABLE_MAX_USABLE_RANGE).query() == 1.0) &&
            (value != rs2_sensor_mode::RS2_SENSOR_MODE_VGA))
        {
            ds.get_option(RS2_OPTION_ENABLE_MAX_USABLE_RANGE).set(0.0f);
            LOG_INFO("Max Usable Range was on - turning it off");
        }

        float_option_with_description::set(value);
    }

    const char * max_usable_range_option::get_description() const
    {

        return "Max Usable Range calculates the maximum range of the camera given the amount of "
               "ambient light in the scene.\n"
               "For example, if Max Usable Range returns 5m, this means that the ambient light in "
               "the scene is reducing the maximum range from 9m down to 5m.\n"
               "Values are rounded to whole meter values and are between 3 meters and 9 meters. "
               "Max range refers to the center 10% of the frame.";
    }

    void ir_reflectivity_option::set(float value)
    {
        // Restrictions for IR Reflectivity option as required on [RS5-8358]
        auto &ds = _l500_depth_dev->get_depth_sensor();
        if (value == 1.0f)
        {
            // Verify option Alternate IR is off
            if( ds.supports_option( RS2_OPTION_ALTERNATE_IR )
                && ds.get_option( RS2_OPTION_ALTERNATE_IR ).query() == 1.0f )
            {
                throw wrong_api_call_sequence_exception("Cannot enable IR Reflectivity when Alternate IR option is on");
            }

            auto &sensor_mode_option = ds.get_option(RS2_OPTION_SENSOR_MODE);
            auto sensor_mode = sensor_mode_option.query();
            bool sensor_mode_is_vga = (sensor_mode == rs2_sensor_mode::RS2_SENSOR_MODE_VGA);

            bool visual_preset_is_max_range = ds.is_max_range_preset();

            if( ds.is_streaming() )
            {
                // While streaming we use active stream resolution and not sensor mode due as a
                // patch for the DQT sensor mode wrong handling. 
                // This should be changed for using sensor mode after DQT fix
                auto active_streams = ds.get_active_streams();
                auto dp = std::find_if( active_streams.begin(),
                                        active_streams.end(),
                                        []( std::shared_ptr< stream_profile_interface > sp ) {
                                            return sp->get_stream_type() == RS2_STREAM_DEPTH;
                                        } );

                bool vga_sensor_mode = false;
                if( dp != active_streams.end() )
                {
                    auto vs = dynamic_cast< video_stream_profile * >( ( *dp ).get() );
                    vga_sensor_mode
                        = ( get_resolution_from_width_height( vs->get_width(), vs->get_height() )
                            == RS2_SENSOR_MODE_VGA );
                }

                if( ( ! vga_sensor_mode ) || ! visual_preset_is_max_range )
                    throw wrong_api_call_sequence_exception(
                        "Please set 'VGA' resolution, 'Max Range' preset and 20% ROI before enabling IR Reflectivity" );
            }
            else
            {
                if (!visual_preset_is_max_range)
                {
                    auto &visual_preset_option = ds.get_option(RS2_OPTION_VISUAL_PRESET);
                    visual_preset_option.set(rs2_l500_visual_preset::RS2_L500_VISUAL_PRESET_MAX_RANGE);
                    LOG_INFO("Visual Preset was changed to: " << visual_preset_option.get_value_description(rs2_l500_visual_preset::RS2_L500_VISUAL_PRESET_MAX_RANGE));
                }

                if (!sensor_mode_is_vga)
                {
                    sensor_mode_option.set(rs2_sensor_mode::RS2_SENSOR_MODE_VGA);
                    LOG_INFO("Sensor Mode was changed to: " << sensor_mode_option.get_value_description(rs2_sensor_mode::RS2_SENSOR_MODE_VGA));
                }
            }

            auto &max_usable_range_option = ds.get_option(RS2_OPTION_ENABLE_MAX_USABLE_RANGE);
            if (max_usable_range_option.query() != 1.0f)
            {
                max_usable_range_option.set(1.0f);
                _max_usable_range_forced_on = true;
                LOG_INFO("Max Usable Range was off - turning it on");
            }
        }
        else
        {
            if (_max_usable_range_forced_on)
            {
                _max_usable_range_forced_on = false;
                bool_option::set( value );  // We set the value here to prevent a loop between Max Usable Range &
                                            // Reflectivity options trying to turn off each other

                ds.get_option(RS2_OPTION_ENABLE_MAX_USABLE_RANGE).set(0.0f);
                LOG_INFO("Max Usable Range was on - turning it off");
                return;
            }
        }

        bool_option::set(value);
    }



    const char * ir_reflectivity_option::get_description() const
    {

        return "IR Reflectivity Tool calculates the percentage of IR light reflected by the "
               "object and returns to the camera for processing.\nFor example, a value of 60% means "
               "that 60% of the IR light projected by the camera is reflected by the object and returns "
               "to the camera.\n\n"
               "Note: only active on 2D view, Visual Preset:Max Range, Resolution:VGA, ROI:20%";
    }
}  // namespace librealsense
