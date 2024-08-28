// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.


#include "d500-options.h"

namespace librealsense
{
    temperature_option::temperature_option( std::shared_ptr< hw_monitor > hwm,
                                            temperature_component component,
                                            const char * description )
        : _hwm( hwm )
        , _component( component )
        , _description( description )
    {
        _range = [this]()
        {
            return option_range{ -127, 128, 1, 30 };
        };
    }

    float temperature_option::query() const
    {
        if (!is_enabled() || !_hwm)
            throw wrong_api_call_sequence_exception("error occurred in the temperature reading");

        float temperature = -1;
        try {
            command cmd(ds::fw_cmd::GTEMP, static_cast<int>(_component));
            auto res = _hwm->send(cmd);
            auto res_16 = reinterpret_cast<uint16_t*>(res.data());
            auto offset_for_component = static_cast<int>(_component) - 1;
            uint16_t result_for_component = res_16[offset_for_component];
            if (result_for_component == 0xffff)
                temperature = 0.f;
            else
            {
                // parsing the temperature result: 0xABCD:
                // whole number = 0xCD - int8_t, 
                // decimal part = 0xAB - uint8_t
                int8_t whole_number_part = static_cast<int8_t>(result_for_component >> 8 & 0xff);
                uint8_t decimal_part = static_cast<uint8_t>(result_for_component & 0x0ff);
                temperature = static_cast<float>(whole_number_part) + decimal_part / 256.f;
            }
        }
        catch (...)
        {
            throw wrong_api_call_sequence_exception("hw monitor command for reading temperature failed");
        }

        return temperature;
    }
    temperature_xu_option::temperature_xu_option(const std::weak_ptr<uvc_sensor>& ep, 
        platform::extension_unit xu, uint8_t id, 
        std::string description)
        : uvc_xu_option<int16_t>(ep, xu, id, description, false,
            // defining the parsing modifier, to be used on the calls for query and get_range methods
            [](const int16_t read_value) {
                return static_cast<float>(read_value) / 10.f;
            }) {}

    float temperature_xu_option::query() const
    {
        return uvc_xu_option<int16_t>::query();
    }

    void temperature_xu_option::set(float value)
    {
        readonly_option::set(value);
    }

    d500_thermal_compensation_option::d500_thermal_compensation_option( std::shared_ptr< hw_monitor > hwm )
        : _hwm( hwm )
    {
    }

    void d500_thermal_compensation_option::set( float value )
    {
        if( auto hwm = _hwm.lock() )
        {
            int on_off = value > 0 ? 1 : 0; // 1 toggle on, 0 off
            int param1 = 5; // received from HKR team, probably means toggle
            try
            {
                command cmd( ds::fw_cmd::HKR_THERMAL_COMPENSATION, param1, on_off );
                hwm->send( cmd );
                _value = value; // Currently no way to query actual value, save set value.
            }
            catch( ... )
            {
                throw wrong_api_call_sequence_exception( "hw monitor command for setting thermal compensation failed" );
            }
        }
    }

    power_line_freq_option::power_line_freq_option(const std::weak_ptr< uvc_sensor >& ep, rs2_option id,
        const std::map< float, std::string >& description_per_value) :
        uvc_pu_option(ep, id, description_per_value) {}
}
