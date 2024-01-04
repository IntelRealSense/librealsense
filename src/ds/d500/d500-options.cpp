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
}
