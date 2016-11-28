// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "option.h"
#include "subdevice.h"

void rsimpl::uvc_pu_option::set(float value)
{
    _ep.invoke_powered(
        [this, value](uvc::uvc_device& dev)
        {
            dev.set_pu(_id, static_cast<int>(value));
        });
}

float rsimpl::uvc_pu_option::query() const
{
    return static_cast<float>(_ep.invoke_powered(
        [this](uvc::uvc_device& dev)
        {
            return dev.get_pu(_id);
        }));
}

rsimpl::option_range rsimpl::uvc_pu_option::get_range() const
{
    auto uvc_range = _ep.invoke_powered(
        [this](uvc::uvc_device& dev)
        {
            return dev.get_pu_range(_id);
        });
    option_range result;
    result.min  = static_cast<float>(uvc_range.min);
    result.max  = static_cast<float>(uvc_range.max);
    result.def  = static_cast<float>(uvc_range.def);
    result.step = static_cast<float>(uvc_range.step);
    return result;
}

const char* rsimpl::uvc_pu_option::get_description() const
{
    switch(_id)
    {
    case RS_OPTION_BACKLIGHT_COMPENSATION: return "Enable / disable backlight compensation";
    case RS_OPTION_BRIGHTNESS: return "UVC image brightness";
    case RS_OPTION_CONTRAST: return "UVC image contrast";
    case RS_OPTION_EXPOSURE: return "Controls exposure time of color camera. Setting any value will disable auto exposure";
    case RS_OPTION_GAIN: return "UVC image gain";
    case RS_OPTION_GAMMA: return "UVC image gamma setting";
    case RS_OPTION_HUE: return "UVC image hue";
    case RS_OPTION_SATURATION: return "UVC image saturation setting";
    case RS_OPTION_SHARPNESS: return "UVC image sharpness setting";
    case RS_OPTION_WHITE_BALANCE: return "Controls white balance of color image. Setting any value will disable auto white balance";
    case RS_OPTION_ENABLE_AUTO_EXPOSURE: return "Enable / disable auto-exposure";
    case RS_OPTION_ENABLE_AUTO_WHITE_BALANCE: return "Enable / disable auto-white-balance";
    default: return rs_option_to_string(_id);
    }
}

std::vector<uint8_t> rsimpl::command_transfer_over_xu::send_receive(const std::vector<uint8_t>& data, int, bool require_response)
{
    return _uvc.invoke_powered([this, &data, require_response]
        (uvc::uvc_device& dev)
        {
            std::vector<uint8_t> result;
            std::lock_guard<uvc::uvc_device> lock(dev);
            dev.set_xu(_xu, _ctrl, data.data(), static_cast<int>(data.size()));
            if (require_response)
            {
                result.resize(IVCAM_MONITOR_MAX_BUFFER_SIZE);
                dev.get_xu(_xu, _ctrl, result.data(), static_cast<int>(result.size()));
            }
            return result;
        });
}
