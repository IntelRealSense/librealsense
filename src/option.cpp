// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "option.h"
#include "sensor.h"
#include "error-handling.h"

bool librealsense::option_base::is_valid(float value) const
{
    if (!std::isnormal(_opt_range.step) && _opt_range.step != 0)
        throw invalid_value_exception(to_string() << "is_valid(...) failed! step is not properly defined. (" << _opt_range.step << ")");

    if ((value < _opt_range.min) || (value > _opt_range.max))
        return false;

    if (_opt_range.step == 0)
        return true;

    auto n = (value - _opt_range.min) / _opt_range.step;
    return (fabs(fmod(n, 1)) < std::numeric_limits<float>::min());
}

librealsense::option_range librealsense::option_base::get_range() const
{
    return _opt_range;
}
void librealsense::option_base::enable_recording(std::function<void(const option&)> recording_action)
{
    _recording_function = recording_action;
}

void librealsense::option::create_snapshot(std::shared_ptr<option>& snapshot) const
{
    snapshot = std::make_shared<const_value_option>(get_description(), query());
}

void librealsense::float_option::set(float value)
{
    if (!is_valid(value))
        throw invalid_value_exception(to_string() << "set(...) failed! " << value << " is not a valid value");
    _value = value;
}

void librealsense::uvc_pu_option::set(float value)
{
    _ep.invoke_powered(
        [this, value](platform::uvc_device& dev)
        {
            if (!dev.set_pu(_id, static_cast<int32_t>(value)))
                throw invalid_value_exception(to_string() << "set_pu(id=" << std::to_string(_id) << ") failed!" << " Last Error: " << strerror(errno));
            _record(*this);
        });
}

float librealsense::uvc_pu_option::query() const
{
    return static_cast<float>(_ep.invoke_powered(
        [this](platform::uvc_device& dev)
        {
            int32_t value = 0;
            if (!dev.get_pu(_id, value))
                throw invalid_value_exception(to_string() << "get_pu(id=" << std::to_string(_id) << ") failed!" << " Last Error: " << strerror(errno));

            return static_cast<float>(value);
        }));
}

librealsense::option_range librealsense::uvc_pu_option::get_range() const
{
    auto uvc_range = _ep.invoke_powered(
        [this](platform::uvc_device& dev)
        {
            return dev.get_pu_range(_id);
        });

    if (uvc_range.min.size() < sizeof(int32_t)) return option_range{0,0,1,0};

    auto min = *(reinterpret_cast<int32_t*>(uvc_range.min.data()));
    auto max = *(reinterpret_cast<int32_t*>(uvc_range.max.data()));
    auto step = *(reinterpret_cast<int32_t*>(uvc_range.step.data()));
    auto def = *(reinterpret_cast<int32_t*>(uvc_range.def.data()));
    return option_range{static_cast<float>(min),
                        static_cast<float>(max),
                        static_cast<float>(step),
                        static_cast<float>(def)};
}

const char* librealsense::uvc_pu_option::get_description() const
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
    case RS2_OPTION_POWER_LINE_FREQUENCY: return "Power Line Frequency";
    case RS2_OPTION_AUTO_EXPOSURE_PRIORITY: return "Limit exposure time when auto-exposure is ON to preserve constant fps rate";
    default: return _ep.get_option_name(_id);
    }
}

std::vector<uint8_t> librealsense::command_transfer_over_xu::send_receive(const std::vector<uint8_t>& data, int, bool require_response)
{
    return _uvc.invoke_powered([this, &data, require_response]
        (platform::uvc_device& dev)
        {
            std::vector<uint8_t> result;
            std::lock_guard<platform::uvc_device> lock(dev);

            if (data.size() > HW_MONITOR_BUFFER_SIZE)
            {
                LOG_ERROR("XU command size is invalid");
                throw invalid_value_exception(to_string() << "Requested XU command size " <<
                    std::dec << data.size() << " exceeds permitted limit " << HW_MONITOR_BUFFER_SIZE);
            }

            std::vector<uint8_t> transmit_buf(HW_MONITOR_BUFFER_SIZE, 0);
            std::copy(data.begin(), data.end(), transmit_buf.begin());

            if (!dev.set_xu(_xu, _ctrl, transmit_buf.data(), static_cast<int>(transmit_buf.size())))
                throw invalid_value_exception(to_string() << "set_xu(ctrl=" << unsigned(_ctrl) << ") failed!" << " Last Error: " << strerror(errno));

            if (require_response)
            {
                result.resize(HW_MONITOR_BUFFER_SIZE);
                if (!dev.get_xu(_xu, _ctrl, result.data(), static_cast<int>(result.size())))
                    throw invalid_value_exception(to_string() << "get_xu(ctrl=" << unsigned(_ctrl) << ") failed!" << " Last Error: " << strerror(errno));

                // Returned data size located in the last 4 bytes
                auto data_size = *(reinterpret_cast<uint32_t*>(result.data() + HW_MONITOR_DATA_SIZE_OFFSET)) + SIZE_OF_HW_MONITOR_HEADER;
                result.resize(data_size);
            }
            return result;
        });
}

void librealsense::polling_errors_disable::set(float value)
{
    if (value < 0)
        throw invalid_value_exception("Invalid polling errors disable request " + std::to_string(value));

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
    _recording_function(*this);
}

float librealsense::polling_errors_disable::query() const
{
    return _value;
}

librealsense::option_range librealsense::polling_errors_disable::get_range() const
{
    return option_range{0, 1, 1, 0};
}

bool librealsense::polling_errors_disable::is_enabled() const
{
    return true;
}

const char * librealsense::polling_errors_disable::get_description() const
{
    return "Enable / disable polling of camera internal errors";
}

const char * librealsense::polling_errors_disable::get_value_description(float value) const
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

std::vector<rs2_option> librealsense::options_container::get_supported_options() const
{
    std::vector<rs2_option> options;
    for (auto option : _options)
        options.push_back(option.first);

    return options;
}