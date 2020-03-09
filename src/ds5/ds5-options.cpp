// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "ds5-options.h"

namespace librealsense
{
    const char* emitter_option::get_value_description(float val) const
    {
        switch (static_cast<int>(val))
        {
            case 0:
            {
                return "Off";
            }
            case 1:
            {
                return "Laser";
            }
            case 2:
            {
                return "Laser Auto";
            }
            case 3:
            {
                return "LED";
            }
            default:
                throw invalid_value_exception("value not found");
        }
    }

    emitter_option::emitter_option(uvc_sensor& ep)
        : uvc_xu_option(ep, ds::depth_xu, ds::DS5_DEPTH_EMITTER_ENABLED,
                        "Emitter select, 0-disable all emitters, 1-enable laser, 2-enable laser auto (opt), 3-enable LED (opt)")
    {}

    float asic_and_projector_temperature_options::query() const
    {
        if (!is_enabled())
            throw wrong_api_call_sequence_exception("query option is allow only in streaming!");

        #pragma pack(push, 1)
        struct temperature
        {
            uint8_t is_projector_valid;
            uint8_t is_asic_valid;
            int8_t projector_temperature;
            int8_t asic_temperature;
        };
        #pragma pack(pop)

        auto temperature_data = static_cast<temperature>(_ep.invoke_powered(
            [this](platform::uvc_device& dev)
            {
                temperature temp{};
                if (!dev.get_xu(ds::depth_xu,
                                ds::DS5_ASIC_AND_PROJECTOR_TEMPERATURES,
                                reinterpret_cast<uint8_t*>(&temp),
                                sizeof(temperature)))
                 {
                        throw invalid_value_exception(to_string() << "get_xu(ctrl=DS5_ASIC_AND_PROJECTOR_TEMPERATURES) failed!" << " Last Error: " << strerror(errno));
                 }

                return temp;
            }));

        int8_t temperature::* field;
        uint8_t temperature::* is_valid_field;

        switch (_option)
        {
        case RS2_OPTION_ASIC_TEMPERATURE:
            field = &temperature::asic_temperature;
            is_valid_field = &temperature::is_asic_valid;
            break;
        case RS2_OPTION_PROJECTOR_TEMPERATURE:
            field = &temperature::projector_temperature;
            is_valid_field = &temperature::is_projector_valid;
            break;
        default:
            throw invalid_value_exception(to_string() << _ep.get_option_name(_option) << " is not temperature option!");
        }

        if (0 == temperature_data.*is_valid_field)
            LOG_ERROR(_ep.get_option_name(_option) << " value is not valid!");

        return temperature_data.*field;
    }

    option_range asic_and_projector_temperature_options::get_range() const
    {
        return option_range { -40, 125, 0, 0 };
    }

    bool asic_and_projector_temperature_options::is_enabled() const
    {
        return _ep.is_streaming();
    }

    const char* asic_and_projector_temperature_options::get_description() const
    {
        switch (_option)
        {
        case RS2_OPTION_ASIC_TEMPERATURE:
            return "Current Asic Temperature (degree celsius)";
        case RS2_OPTION_PROJECTOR_TEMPERATURE:
            return "Current Projector Temperature (degree celsius)";
        default:
            throw invalid_value_exception(to_string() << _ep.get_option_name(_option) << " is not temperature option!");
        }
    }

    asic_and_projector_temperature_options::asic_and_projector_temperature_options(uvc_sensor& ep, rs2_option opt)
        : _option(opt), _ep(ep)
        {}

    float motion_module_temperature_option::query() const
    {
        if (!is_enabled())
            throw wrong_api_call_sequence_exception("query option is allow only in streaming!");

        static const auto report_field = platform::custom_sensor_report_field::value;
        auto data = _ep.get_custom_report_data(custom_sensor_name, report_name, report_field);
        if (data.empty())
            throw invalid_value_exception("query() motion_module_temperature_option failed! Empty buffer arrived.");

        auto data_str = std::string(reinterpret_cast<char const*>(data.data()));
        return std::stof(data_str);
    }

    option_range motion_module_temperature_option::get_range() const
    {
        if (!is_enabled())
            throw wrong_api_call_sequence_exception("get option range is allow only in streaming!");

        static const auto min_report_field = platform::custom_sensor_report_field::minimum;
        static const auto max_report_field = platform::custom_sensor_report_field::maximum;
        auto min_data = _ep.get_custom_report_data(custom_sensor_name, report_name, min_report_field);
        auto max_data = _ep.get_custom_report_data(custom_sensor_name, report_name, max_report_field);
        if (min_data.empty() || max_data.empty())
            throw invalid_value_exception("get_range() motion_module_temperature_option failed! Empty buffer arrived.");

        auto min_str = std::string(reinterpret_cast<char const*>(min_data.data()));
        auto max_str = std::string(reinterpret_cast<char const*>(max_data.data()));

        return option_range{std::stof(min_str),
                            std::stof(max_str),
                            0, 0};
    }

    bool motion_module_temperature_option::is_enabled() const
    {
        return _ep.is_streaming();
    }

    const char* motion_module_temperature_option::get_description() const
    {
        return "Current Motion-Module Temperature (degree celsius)";
    }

    motion_module_temperature_option::motion_module_temperature_option(hid_sensor& ep)
        : _ep(ep)
    {}

    void enable_motion_correction::set(float value)
    {
        if (!is_valid(value))
            throw invalid_value_exception(to_string() << "set(enable_motion_correction) failed! Given value " << value << " is out of range.");

        _is_active = value > _opt_range.min;
        _recording_function(*this);
    }

    float enable_motion_correction::query() const
    {
        auto is_active = _is_active.load();
        return is_active ? _opt_range.max : _opt_range.min;
    }

    enable_motion_correction::enable_motion_correction(sensor_base* mm_ep,
                                                       const option_range& opt_range)
        : option_base(opt_range), _is_active(true)
    {}

    void enable_auto_exposure_option::set(float value)
    {
        if (!is_valid(value))
            throw invalid_value_exception("set(enable_auto_exposure) failed! Invalid Auto-Exposure mode request " + std::to_string(value));

        auto auto_exposure_prev_state = _auto_exposure_state->get_enable_auto_exposure();
        _auto_exposure_state->set_enable_auto_exposure(0.f < std::fabs(value));

        if (_auto_exposure_state->get_enable_auto_exposure()) // auto_exposure current value
        {
            if (!auto_exposure_prev_state) // auto_exposure previous value
            {
                _to_add_frames = true; // auto_exposure moved from disable to enable
            }
        }
        else
        {
            if (auto_exposure_prev_state)
            {
                _to_add_frames = false; // auto_exposure moved from enable to disable
            }
        }
        _recording_function(*this);
    }

    float enable_auto_exposure_option::query() const
    {
        return _auto_exposure_state->get_enable_auto_exposure();
    }

    enable_auto_exposure_option::enable_auto_exposure_option(synthetic_sensor* fisheye_ep,
                                                             std::shared_ptr<auto_exposure_mechanism> auto_exposure,
                                                             std::shared_ptr<auto_exposure_state> auto_exposure_state,
                                                             const option_range& opt_range)
        : option_base(opt_range),
          _auto_exposure_state(auto_exposure_state),
          _to_add_frames((_auto_exposure_state->get_enable_auto_exposure())),
          _auto_exposure(auto_exposure)
    {}

    auto_exposure_mode_option::auto_exposure_mode_option(std::shared_ptr<auto_exposure_mechanism> auto_exposure,
                                                         std::shared_ptr<auto_exposure_state> auto_exposure_state,
                                                         const option_range& opt_range,
                                                         const std::map<float, std::string>& description_per_value)
        : option_base(opt_range),
          _auto_exposure_state(auto_exposure_state),
          _auto_exposure(auto_exposure),
          _description_per_value(description_per_value)
    {}

    void auto_exposure_mode_option::set(float value)
    {
        if (!is_valid(value))
            throw invalid_value_exception(to_string() << "set(auto_exposure_mode_option) failed! Given value " << value << " is out of range.");

        _auto_exposure_state->set_auto_exposure_mode(static_cast<auto_exposure_modes>((int)value));
        _auto_exposure->update_auto_exposure_state(*_auto_exposure_state);
        _recording_function(*this);
    }

    float auto_exposure_mode_option::query() const
    {
        return static_cast<float>(_auto_exposure_state->get_auto_exposure_mode());
    }

    const char* auto_exposure_mode_option::get_value_description(float val) const
    {
        try{
            return _description_per_value.at(val).c_str();
        }
        catch(std::out_of_range)
        {
            throw invalid_value_exception(to_string() << "auto_exposure_mode: get_value_description(...) failed! Description of value " << val << " is not found.");
        }
    }

    auto_exposure_step_option::auto_exposure_step_option(std::shared_ptr<auto_exposure_mechanism> auto_exposure,
        std::shared_ptr<auto_exposure_state> auto_exposure_state,
        const option_range& opt_range)
        : option_base(opt_range),
        _auto_exposure_state(auto_exposure_state),
        _auto_exposure(auto_exposure)
    {}

    void auto_exposure_step_option::set(float value)
    {
        if (!std::isnormal(_opt_range.step) || ((value < _opt_range.min) || (value > _opt_range.max)))
            throw invalid_value_exception(to_string() << "set(auto_exposure_step_option) failed! Given value " << value << " is out of range.");

        _auto_exposure_state->set_auto_exposure_step(value);
        _auto_exposure->update_auto_exposure_state(*_auto_exposure_state);
        _recording_function(*this);
    }

    float auto_exposure_step_option::query() const
    {
        return static_cast<float>(_auto_exposure_state->get_auto_exposure_step());
    }

    auto_exposure_antiflicker_rate_option::auto_exposure_antiflicker_rate_option(std::shared_ptr<auto_exposure_mechanism> auto_exposure,
                                                                                 std::shared_ptr<auto_exposure_state> auto_exposure_state,
                                                                                 const option_range& opt_range,
                                                                                 const std::map<float, std::string>& description_per_value)
        : option_base(opt_range),
          _description_per_value(description_per_value),
          _auto_exposure_state(auto_exposure_state),
          _auto_exposure(auto_exposure)
    {}

    void auto_exposure_antiflicker_rate_option::set(float value)
    {
        if (!is_valid(value))
            throw invalid_value_exception(to_string() << "set(auto_exposure_antiflicker_rate_option) failed! Given value " << value << " is out of range.");

        _auto_exposure_state->set_auto_exposure_antiflicker_rate(static_cast<uint32_t>(value));
        _auto_exposure->update_auto_exposure_state(*_auto_exposure_state);
        _recording_function(*this);
    }

    float auto_exposure_antiflicker_rate_option::query() const
    {
        return static_cast<float>(_auto_exposure_state->get_auto_exposure_antiflicker_rate());
    }

    const char* auto_exposure_antiflicker_rate_option::get_value_description(float val) const
    {
        try{
            return _description_per_value.at(val).c_str();
        }
        catch(std::out_of_range)
        {
            throw invalid_value_exception(to_string() << "antiflicker_rate: get_value_description(...) failed! Description of value " << val << " is not found.");
        }
    }

    ds::depth_table_control depth_scale_option::get_depth_table(ds::advanced_query_mode mode) const
    {
        command cmd(ds::GET_ADV);
        cmd.param1 = ds::etDepthTableControl;
        cmd.param2 = mode;
        auto res = _hwm.send(cmd);

        if (res.size() < sizeof(ds::depth_table_control))
            throw std::runtime_error("Not enough bytes returned from the firmware!");

        auto table = (const ds::depth_table_control*)res.data();
        return *table;
    }

    depth_scale_option::depth_scale_option(hw_monitor& hwm)
        : _hwm(hwm)
    {
        _range = [this]()
        {
            auto min = get_depth_table(ds::GET_MIN);
            auto max = get_depth_table(ds::GET_MAX);
            return option_range{ (float)(0.000001 * min.depth_units),
                                 (float)(0.000001 * max.depth_units),
                                 0.000001f, 0.001f };
        };
    }

    void depth_scale_option::set(float value)
    {
        command cmd(ds::SET_ADV);
        cmd.param1 = ds::etDepthTableControl;

        auto depth_table = get_depth_table(ds::GET_VAL);
        depth_table.depth_units = static_cast<uint32_t>(1000000 * value);
        auto ptr = (uint8_t*)(&depth_table);
        cmd.data = std::vector<uint8_t>(ptr, ptr + sizeof(ds::depth_table_control));

        _hwm.send(cmd);
        _record_action(*this);
        notify(value);
    }

    float depth_scale_option::query() const
    {
        auto table = get_depth_table(ds::GET_VAL);
        return (float)(0.000001 * (float)table.depth_units);
    }

    option_range depth_scale_option::get_range() const
    {
        return *_range;
    }

    external_sync_mode::external_sync_mode(hw_monitor& hwm)
        : _hwm(hwm)
    {
        _range = [this]()
        {
            return option_range{ ds::inter_cam_sync_mode::INTERCAM_SYNC_DEFAULT,
                                 ds::inter_cam_sync_mode::INTERCAM_SYNC_MAX - 1,
                                 ds::inter_cam_sync_mode::INTERCAM_SYNC_DEFAULT, 1 };
        };
    }

    void external_sync_mode::set(float value)
    {
        command cmd(ds::SET_CAM_SYNC);
        cmd.param1 = static_cast<int>(value);

        _hwm.send(cmd);
        _record_action(*this);
    }

    float external_sync_mode::query() const
    {
        command cmd(ds::GET_CAM_SYNC);
        auto res = _hwm.send(cmd);
        if (res.empty())
            throw invalid_value_exception("external_sync_mode::query result is empty!");

        return (res.front());
    }

    option_range external_sync_mode::get_range() const
    {
        return *_range;
    }

    emitter_on_and_off_option::emitter_on_and_off_option(hw_monitor& hwm, sensor_base* ep)
        : _hwm(hwm), _sensor(ep)
    {
        _range = [this]()
        {
            return option_range{ 0, 1, 1, 0 };
        };
    }

    void emitter_on_and_off_option::set(float value)
    {
        if (_sensor->is_streaming())
            throw std::runtime_error("Cannot change Emitter On/Off option while streaming!");

        command cmd(ds::SET_PWM_ON_OFF);
        cmd.param1 = static_cast<int>(value);

        _hwm.send(cmd);
        _record_action(*this);
    }

    float emitter_on_and_off_option::query() const
    {
        command cmd(ds::GET_PWM_ON_OFF);
        auto res = _hwm.send(cmd);
        if (res.empty())
            throw invalid_value_exception("emitter_on_and_off_option::query result is empty!");

        return (res.front());
    }

    option_range emitter_on_and_off_option::get_range() const
    {
        return *_range;
    }

    alternating_emitter_option::alternating_emitter_option(hw_monitor& hwm, sensor_base* ep)
        : _hwm(hwm), _sensor(ep)
    {
        _range = [this]()
        {
            return option_range{ 0, 1, 1, 0 };
        };
    }

    void alternating_emitter_option::set(float value)
    {
        std::vector<uint8_t> pattern{};
        if (static_cast<int>(value))
            pattern = ds::alternating_emitter_pattern;

        command cmd(ds::SETSUBPRESET, static_cast<int>(pattern.size()));
        cmd.data = pattern;
        auto res = _hwm.send(cmd);
        _record_action(*this);
    }

    float alternating_emitter_option::query() const
    {
        command cmd(ds::GETSUBPRESETNAME);
        auto res = _hwm.send(cmd);
        if (res.size()>20)
            throw invalid_value_exception("HWMON::GETSUBPRESETNAME invalid size");

        static std::vector<uint8_t> alt_emitter_name(ds::alternating_emitter_pattern.begin()+2,ds::alternating_emitter_pattern.begin()+22);
        return (alt_emitter_name == res);
    }

    emitter_always_on_option::emitter_always_on_option(hw_monitor& hwm, sensor_base* ep)
        : _hwm(hwm), _sensor(ep)
    {
        _range = [this]()
        {
            return option_range{ 0, 1, 1, 0 };
        };
    }

    void emitter_always_on_option::set(float value)
    {
        command cmd(ds::LASERONCONST);
        cmd.param1 = static_cast<int>(value);

        _hwm.send(cmd);
        _record_action(*this);
    }

    float emitter_always_on_option::query() const
    {
        command cmd(ds::LASERONCONST);
        cmd.param1 = 2;

        auto res = _hwm.send(cmd);
        if (res.empty())
            throw invalid_value_exception("emitter_always_on_option::query result is empty!");

        return (res.front());
    }

    option_range emitter_always_on_option::get_range() const
    {
        return *_range;
    }
}
