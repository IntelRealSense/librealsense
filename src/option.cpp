// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "option.h"
#include "subdevice.h"
#include "error-handling.h"

void rsimpl2::uvc_pu_option::set(float value)
{
    _ep.invoke_powered(
        [this, value](uvc::uvc_device& dev)
        {
            dev.set_pu(_id, static_cast<int>(value));
        });
}

float rsimpl2::uvc_pu_option::query() const
{
    return static_cast<float>(_ep.invoke_powered(
        [this](uvc::uvc_device& dev)
        {
            return dev.get_pu(_id);
        }));
}

rsimpl2::option_range rsimpl2::uvc_pu_option::get_range() const
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

const char* rsimpl2::uvc_pu_option::get_description() const
{
    switch(_id)
    {
    case RS2_OPTION_BACKLIGHT_COMPENSATION: return "Enable / disable backlight compensation";
    case RS2_OPTION_BRIGHTNESS: return "UVC image brightness";
    case RS2_OPTION_CONTRAST: return "UVC image contrast";
    case RS2_OPTION_EXPOSURE: return "Controls exposure time of color camera. Setting any value will disable auto exposure";
    case RS2_OPTION_GAIN: return "UVC image gain";
    case RS2_OPTION_GAMMA: return "UVC image gamma setting";
    case RS2_OPTION_HUE: return "UVC image hue";
    case RS2_OPTION_SATURATION: return "UVC image saturation setting";
    case RS2_OPTION_SHARPNESS: return "UVC image sharpness setting";
    case RS2_OPTION_WHITE_BALANCE: return "Controls white balance of color image. Setting any value will disable auto white balance";
    case RS2_OPTION_ENABLE_AUTO_EXPOSURE: return "Enable / disable auto-exposure";
    case RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE: return "Enable / disable auto-white-balance";
    default: return rs2_option_to_string(_id);
    }
}

std::vector<uint8_t> rsimpl2::command_transfer_over_xu::send_receive(const std::vector<uint8_t>& data, int, bool require_response)
{
    return _uvc.invoke_powered([this, &data, require_response]
        (uvc::uvc_device& dev)
        {
            std::vector<uint8_t> result;
            std::lock_guard<uvc::uvc_device> lock(dev);

            if (data.size() > HW_MONITOR_BUFFER_SIZE)
            {
                LOG_ERROR("XU command size is invalid");
                throw invalid_value_exception(to_string() << "Requested XU command size " <<
                    std::dec << data.size() << " exceeds permitted limit " << HW_MONITOR_BUFFER_SIZE);
            }

            std::vector<uint8_t> transmit_buf(HW_MONITOR_BUFFER_SIZE, 0);
            std::copy(data.begin(), data.end(), transmit_buf.begin());
            dev.set_xu(_xu, _ctrl, transmit_buf.data(), static_cast<int>(transmit_buf.size()));

            if (require_response)
            {
                result.resize(HW_MONITOR_BUFFER_SIZE);
                dev.get_xu(_xu, _ctrl, result.data(), static_cast<int>(result.size()));
            }
            return result;
        });
}

void rsimpl2::polling_errors_disable::set(float value)
{
    if (value < 0) throw invalid_value_exception("Invalid polling errors disable request " + std::to_string(value));

    if (value == 0)
    {
        _polling_error_handler->stop();
        _value = 0;
    }
    else
    {
        _polling_error_handler->start();
        _value = 1;
    }
}

float rsimpl2::polling_errors_disable::query() const
{
    return _value;
}

rsimpl2::option_range rsimpl2::polling_errors_disable::get_range() const
{
    return{ 0, 1, 1, 1 };
}

bool rsimpl2::polling_errors_disable::is_enabled() const
{
    return true;
}

const char * rsimpl2::polling_errors_disable::get_description() const
{
    return "Enable / disable polling of camera internal errors";
}

const char * rsimpl2::polling_errors_disable::get_value_description(float value)
{
    if (value == 0)
    {
        return "Disable error polling";
    }
    else
    {
        return "Enable error polling";
    }
}
