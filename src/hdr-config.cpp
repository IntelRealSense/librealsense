// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "hdr-config.h"
#include "ds5/ds5-private.h"

namespace librealsense
{
    hdr_config::hdr_config(hw_monitor& hwm, sensor_base* depth_ep) :
        _sequence_size(1),
        _current_hdr_sequence_index(0),
        _relative_mode(false),
        _is_enabled(false),
        _is_config_in_process(false),
        _hwm(hwm),
        _sensor(depth_ep)
    {
        hdr_params first_id_params;
        _hdr_sequence_params.push_back(first_id_params);
    }

    float hdr_config::get(rs2_option option) const
    {
        float rv = 0.f;
        switch (option)
        {
        case RS2_OPTION_HDR_SEQUENCE_SIZE:
            rv = static_cast<float>(_sequence_size);
            break;
        case RS2_OPTION_HDR_RELATIVE_MODE:
            rv = static_cast<float>(_relative_mode);
            break;
        case RS2_OPTION_HDR_SEQUENCE_ID:
            rv = static_cast<float>(_current_hdr_sequence_index);
            break;
        case RS2_OPTION_HDR_ENABLED:
            rv = static_cast<float>(_is_enabled);
            break;
        case RS2_OPTION_EXPOSURE:
            try {
                rv = _hdr_sequence_params[_current_hdr_sequence_index]._exposure;
            }
            // should never happen
            catch (std::out_of_range)
            {
                throw invalid_value_exception(to_string() << "hdr_config::get(...) failed! Index is above the sequence size.");
            }
            break;
        case RS2_OPTION_GAIN:
            try {
                rv = _hdr_sequence_params[_current_hdr_sequence_index]._gain;
            }
            // should never happen
            catch (std::out_of_range)
            {
                throw invalid_value_exception(to_string() << "hdr_config::get(...) failed! Index is above the sequence size.");
            }
            break;
        default:
            throw invalid_value_exception("option is not an HDR option");
        }
        return rv;
    }


    void hdr_config::set(rs2_option option, float value, option_range range)
    {
        if (value < range.min || value > range.max)
            throw invalid_value_exception(to_string() << "hdr_config::set(...) failed! value is out of the option range.");

        switch (option)
        {
        case RS2_OPTION_HDR_SEQUENCE_SIZE:
            set_sequence_size(value);
            break;
        case RS2_OPTION_HDR_RELATIVE_MODE:
            set_relative_mode(value);
            break;
        case RS2_OPTION_HDR_SEQUENCE_ID:
            set_sequence_index(value);
            break;
        case RS2_OPTION_HDR_ENABLED:
            set_is_active_status(value);
            break;
        case RS2_OPTION_EXPOSURE:
            set_exposure(value, range);
            break;
        case RS2_OPTION_GAIN:
            set_gain(value);
            break;
        default:
            throw invalid_value_exception("option is not an HDR option");
        }
    }

    bool hdr_config::is_config_in_process() const
    {
        return _is_config_in_process;
    }

    void hdr_config::set_is_active_status(float value)
    {
        if (value)
        {
            if (validate_config())
            {
                enable();
                _is_enabled = true;
            }
            else
                // msg to user to be improved later on
                throw invalid_value_exception("config is not valid");
        }
        else
        {
            disable();
            _is_enabled = false;
        }
    }

    void hdr_config::enable()
    {
        // prepare sub-preset command
        command cmd = prepare_hdr_sub_preset_command();
        auto res = _hwm.send(cmd);
    }

    void hdr_config::disable()
    {
        // sending empty sub preset
        std::vector<uint8_t> pattern{};

        // TODO - make it usable not only for ds - use _sensor
        command cmd(ds::SETSUBPRESET, static_cast<int>(pattern.size()));
        cmd.data = pattern;
        auto res = _hwm.send(cmd);
    }

    command hdr_config::prepare_hdr_sub_preset_command()
    {
        std::vector<uint8_t> pattern{};

        // TODO - make it usable not only for ds - use _sensor
        command cmd(ds::SETSUBPRESET, static_cast<int>(pattern.size()));
        cmd.data = pattern;
        return cmd;
    }

    bool hdr_config::validate_config() const
    {
        // to be elaborated or deleted
        return true;
    }

    void hdr_config::set_sequence_size(float value)
    {
        size_t new_size = static_cast<size_t>(value);
        if (new_size > 3 || new_size < 2)
            throw invalid_value_exception(to_string() << "hdr_config::set_sequence_size(...) failed! Only size 2 or 3 are supported.");

        if (new_size != _sequence_size)
        {
            _hdr_sequence_params.resize(new_size);
        }       
    }

    void hdr_config::set_relative_mode(float value)
    {
        _relative_mode = static_cast<bool>(value);
    }

    void hdr_config::set_sequence_index(float value)
    {
        size_t new_index = static_cast<size_t>(value);
        
        if (new_index == (_current_hdr_sequence_index - 1))
            return;

        if (new_index <= _hdr_sequence_params.size())
        {
            _current_hdr_sequence_index = new_index - 1;
            _is_config_in_process == (new_index != 0);  
        }
        else
            throw invalid_value_exception(to_string() << "hdr_config::set_sequence_index(...) failed! Index above sequence size.");
    }

    void hdr_config::set_exposure(float value, option_range range)
    {
        /* TODO - add limitation on max exposure to be below frame interval - is range really needed for this?*/
        _hdr_sequence_params[_current_hdr_sequence_index]._exposure = value;
    }

    void hdr_config::set_gain(float value)
    {
        _hdr_sequence_params[_current_hdr_sequence_index]._gain = value;
    }


    hdr_params::hdr_params() :
        _sequence_id(0),
        _exposure(0.f),
        _gain(0.f)
    {}
    
    hdr_params::hdr_params(int sequence_id, float exposure, float gain) :
        _sequence_id(sequence_id),
        _exposure(exposure),
        _gain(gain)
    {}

    


}