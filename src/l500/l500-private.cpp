//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#include "l500-private.h"

using namespace std;

namespace librealsense
{
    namespace ivcam2
    {
        bool try_fetch_usb_device(std::vector<platform::usb_device_info>& devices,
            const platform::uvc_device_info& info, platform::usb_device_info& result)
        {
            for (auto it = devices.begin(); it != devices.end(); ++it)
            {
                if (it->unique_id == info.unique_id)
                {

                    result = *it;
                    if (result.mi == 4 || result.mi == 6)
                    {
                        devices.erase(it);
                        return true;
                    }
                }
            }
            return false;
        }
        float l500_temperature_options::query() const
        {
            if (!is_enabled())
                throw wrong_api_call_sequence_exception("query option is allow only in streaming!");

#pragma pack(push, 1)
            struct temperatures
            {
                double LLD_temperature;
                double MC_temperature;
                double MA_temperature;
            };
#pragma pack(pop)

            auto res = _hw_monitor->send(command{ 0x6A });
            auto temperature_data = *(reinterpret_cast<temperatures*>((void*)res.data()));

            /*switch (_option)
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
*/
            return temperature_data.LLD_temperature;
        }
        l500_temperature_options::l500_temperature_options(hw_monitor* hw_monitor, rs2_option opt)
            :_hw_monitor(hw_monitor), _option(opt)
        {
        }
    } // librealsense::ivcam2
} // namespace librealsense
