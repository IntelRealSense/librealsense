//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#include "l500-private.h"

using namespace std;

namespace librealsense
{
    namespace ivcam2
    {
        pose get_color_stream_extrinsic(const std::vector<uint8_t>& raw_data)
        {
            if (raw_data.size() < sizeof(pose))
                throw invalid_value_exception("size of extrinsic invalid");
            auto res = *((pose*)raw_data.data());
            float trans_scale = 0.001f; // Convert units from mm to meter

            if (res.position.y > 0.f) // Extrinsic of color is referenced to the Depth Sensor CS
                trans_scale *= -1;

            res.position.x *= trans_scale;
            res.position.y *= trans_scale;
            res.position.z *= trans_scale;
            return res;
        }

        bool try_fetch_usb_device(std::vector<platform::usb_device_info>& devices,
            const platform::uvc_device_info& info, platform::usb_device_info& result)
        {
            for (auto it = devices.begin(); it != devices.end(); ++it)
            {
                if (it->unique_id == info.unique_id)
                {

                    result = *it;
                    switch(info.pid)
                    {
                    case L515_PID:
                        if(result.mi == 7)
                        {
                            devices.erase(it);
                            return true;
                        }
                        break;
                    case L500_PID:
                        if(result.mi == 4 || result.mi == 6)
                        {
                            devices.erase(it);
                            return true;
                        }
                        break;
                    default:
                        break;
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
                double APD_temperature;
            };
#pragma pack(pop)

            auto res = _hw_monitor->send(command{ TEMPERATURES_GET });

            if (res.size() < sizeof(temperatures))
            {
                throw std::runtime_error("Invalid result size!");
            }

            auto temperature_data = *(reinterpret_cast<temperatures*>((void*)res.data()));

            switch (_option)
            {
            case RS2_OPTION_LLD_TEMPERATURE:
                return float(temperature_data.LLD_temperature);
            case RS2_OPTION_MC_TEMPERATURE:
                return float(temperature_data.MC_temperature);
            case RS2_OPTION_MA_TEMPERATURE:
                return float(temperature_data.MA_temperature);
            case RS2_OPTION_APD_TEMPERATURE:
                return float(temperature_data.APD_temperature);
            default:
                throw invalid_value_exception(to_string() << _option << " is not temperature option!");
            }
        }

        l500_temperature_options::l500_temperature_options(hw_monitor* hw_monitor, rs2_option opt)
            :_hw_monitor(hw_monitor), _option(opt)
        {
        }
    } // librealsense::ivcam2
} // namespace librealsense
