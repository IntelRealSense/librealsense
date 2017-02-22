// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "option.h"
#include "ms-xu-option.h"

namespace rsimpl
{

option_range ms_xu_control_option::get_range() const
{
    auto uvc_range = _ep.invoke_powered(
        [this](uvc::uvc_device& dev)
        {
            return dev.get_xu_range(_xu, _id, _xu_lenght);
        });

    // Rectify data obtained from FW.
    // The control step is provided as enumerated value, but is rectified to [0/1] range for compatibility with regular XU
    if (uvc_range.max>1)         uvc_range.max = 1;
    if (0==uvc_range.step)       uvc_range.step = 1;

    return {static_cast<float>(uvc_range.min), static_cast<float>(uvc_range.max),
            static_cast<float>(uvc_range.def), static_cast<float>(uvc_range.step)};
}

void ms_xu_control_option::encode_data(const float& val, std::vector<uint8_t>& _transmit_buf) const
{
    auto state = static_cast<int>(val);

    switch (state) // The state is encoded in the first 8 bits
    {
    case 0:
        _transmit_buf[msxu_mode]= MSXU_MODE_D1_MANUAL;
        break;
    case 1:
        _transmit_buf[msxu_mode]= MSXU_MODE_D0_AUTO;
        break;
    default:
        throw invalid_value_exception(msxu_map.at((msxu_ctrl)_id)._desc + " mode is invalid: "
                                        + std::to_string((uint8_t)_transmit_buf[msxu_mode]));
    }
}

float ms_xu_control_option::decode_data(const std::vector<uint8_t>& _transmit_buf) const
{
    switch (static_cast<msxu_ctrl_mode>(_transmit_buf[msxu_mode])) // The state is encoded in the first 8 bits
    {
    case MSXU_MODE_D0_AUTO:
    case MSXU_MODE_D2_LOCK:
    case MSXU_MODE_D0_AUTO | MSXU_MODE_D2_LOCK:
        return 1.f;
    case MSXU_MODE_D1_MANUAL:
        return 0.f;
    default:
        throw invalid_value_exception(msxu_map.at((msxu_ctrl)_id)._desc + " mode is invalid: "
                                        + std::to_string((uint8_t)_transmit_buf[msxu_mode]));
    }
}

option_range ms_xu_data_option::get_range() const
{
    // MS XU range shall be hard-coded till a proper data parsing is provided for cross-platform
    option_range result {-1,-1,-1,-1};
    switch (this->_id)
    {                                       //  min     max     step    default
        case MSXU_EXPOSURE:     result =    {   0,    160000,    1,      0};  break;
        case MSXU_WHITEBALANCE: result =    { 2800,   6500,      1,    2800};  break;
        default:  throw invalid_value_exception(msxu_map.at((msxu_ctrl)_id)._desc + " get range property is not supported");
    }

    return result;
}

void ms_xu_data_option::encode_data(const float& val, std::vector<uint8_t>& _transmit_buf) const
{
    auto set_val = static_cast<int>(val);

    // Prepare the control data for transmit
    std::memcpy(&_transmit_buf[msxu_value],&set_val,sizeof(set_val));   // Set requested value
    _transmit_buf[msxu_mode]= MSXU_MODE_D1_MANUAL;                      // Also explicitely set mode flag to Manual (by spec)
}

float ms_xu_data_option::decode_data(const std::vector<uint8_t>& _transmit_buf) const
{
    // Actual data is provided only when the control is set in manual mode.
    switch (static_cast<msxu_ctrl_mode>(_transmit_buf[msxu_mode])) // The state is encoded in the first 8 bits
    {
    case MSXU_MODE_D1_MANUAL:
    {
        auto val = reinterpret_cast<const uint64_t*>(&_transmit_buf[msxu_value]);
        return static_cast<float>(*val);
    }
    case MSXU_MODE_D0_AUTO:
    case MSXU_MODE_D2_LOCK:
    case MSXU_MODE_D0_AUTO | MSXU_MODE_D2_LOCK:
        throw wrong_api_call_sequence_exception(msxu_map.at((msxu_ctrl)_id)._desc +
            " data query is available in Manual mode only. Actual: "  + std::to_string((uint8_t)_transmit_buf[msxu_mode]));
    default:
        throw invalid_value_exception(msxu_map.at((msxu_ctrl)_id)._desc + " is in unsupported mode, value: "
                                                + std::to_string((uint8_t)_transmit_buf[msxu_mode]));
    }
}
}
