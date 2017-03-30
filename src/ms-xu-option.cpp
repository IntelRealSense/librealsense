// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "option.h"
#include "ms-xu-option.h"

namespace rsimpl2
{

void ms_xu_control_option::parse_data(std::vector<uint8_t>& data) const
{
    static const uint8_t offset_value = msxu_fields::msxu_mode;
    static const uint8_t size_value = sizeof(int32_t);
    std::copy(data.begin() + offset_value, data.begin() + offset_value + size_value, data.begin());
    data.resize(size_value);
}

option_range ms_xu_control_option::get_range() const
{
    auto uvc_range = _ep.invoke_powered(
        [this](uvc::uvc_device& dev)
        {
            return dev.get_xu_range(_xu, _id, _xu_length);
        });

    parse_data(uvc_range.min);
    parse_data(uvc_range.max);
    parse_data(uvc_range.step);
    parse_data(uvc_range.def);

    auto max = *(reinterpret_cast<int32_t*>(uvc_range.max.data()));
    if (max > 1)
    {
        max = 1;
    }

    auto step = *(reinterpret_cast<int32_t*>(uvc_range.step.data()));
    if (0 == step)
    {
        step = 1;
    }

    auto min = *(reinterpret_cast<int32_t*>(uvc_range.min.data()));
    auto def = *(reinterpret_cast<int32_t*>(uvc_range.def.data()));
    return option_range{static_cast<float>(min),
                        static_cast<float>(max),
                        static_cast<float>(step),
                        static_cast<float>(def)};
}

void ms_xu_control_option::encode_data(const float& val, std::vector<uint8_t>& _transmit_buf) const
{
    auto state = static_cast<int>(val);
    auto def = static_cast<int>(_def_value);
    switch (state) // The state is encoded in the first 8 bits
    {
    case 0:
        // Work-around: current spec does not define how to find the correct value to set
        // when moving from auto to manual mode, so we choose to set default
        std::memcpy(&_transmit_buf[msxu_value], &def, sizeof(def));
        _transmit_buf[msxu_mode]= MSXU_MODE_D1_MANUAL;
        break;
    case 1:
        std::memcpy(&_transmit_buf[msxu_value], &def, sizeof(def));
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

void ms_xu_data_option::encode_data(const float& val, std::vector<uint8_t>& _transmit_buf) const
{
    auto set_val = static_cast<int>(val);

    // Prepare the control data for transmit
    std::memcpy(&_transmit_buf[msxu_value],&set_val,sizeof(set_val));   // Set requested value
    _transmit_buf[msxu_mode]= MSXU_MODE_D1_MANUAL;                      // Also explicitely set mode flag to Manual (by spec)
}

float ms_xu_data_option::decode_data(const std::vector<uint8_t>& _transmit_buf) const
{
      auto mode = static_cast<msxu_ctrl_mode>(_transmit_buf[msxu_mode]);
      // Actual data is provided only when the control is set in manual mode.
      switch (mode) // The state is encoded in the first 8 bits
      {
      case MSXU_MODE_D1_MANUAL:
      {
          auto val = reinterpret_cast<const uint64_t*>(&_transmit_buf[msxu_value]);
          return static_cast<float>(*val);
      }
      case MSXU_MODE_D0_AUTO:
      case MSXU_MODE_D2_LOCK:
      case MSXU_MODE_D0_AUTO | MSXU_MODE_D2_LOCK:
          return _def_value;
      default:
          throw invalid_value_exception(msxu_map.at((msxu_ctrl)_id)._desc +
               " is in unsupported Mode " + std::to_string(mode));
      }
}
}
