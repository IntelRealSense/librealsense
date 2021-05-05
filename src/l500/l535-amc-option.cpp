//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "l535-amc-option.h"
#include "l500-private.h"
#include "l500-depth.h"

const std::string MIN_CONTROLS_FW_VERSION("1.3.9.0");
const std::string MIN_GET_DEFAULT_FW_VERSION( "1.5.4.0" );

namespace librealsense
{
    namespace ivcam2
    {
        namespace l535
        {
            float amc_option::query() const
            {
                return query_current(RS2_SENSOR_MODE_VGA); //todo: remove sensor mode
            }

            void amc_option::set(float value)
            {
                _hw_monitor->send(command{ AMCSET, _type, (int)value });
            }

            option_range amc_option::get_range() const
            {
                return _range;
            }

            float amc_option::query_default() const
            {
                auto res = _hw_monitor->send(command{ AMCGET,
                    _type,
                    l500_command::get_default,
                    RS2_SENSOR_MODE_VGA } //todo: remove sensor mode
                );

                auto val = *(reinterpret_cast<uint32_t *>(res.data()));
                return float(val);
            }

            amc_option::amc_option(l500_device* l500_dev,
                hw_monitor * hw_monitor,
                amc_control type,
                const std::string & description)
                : _l500_dev(l500_dev)
                , _hw_monitor(hw_monitor)
                , _type(type)
                , _description(description)
            {
                // Keep the USB power on while triggering multiple calls on it.
                ivcam2::group_multiple_fw_calls(_l500_dev->get_depth_sensor(), [&]() {
                    auto min = _hw_monitor->send(command{ AMCGET, _type, get_min });
                    auto max = _hw_monitor->send(command{ AMCGET, _type, get_max });
                    auto step = _hw_monitor->send(command{ AMCGET, _type, get_step });

                    if (min.size() < sizeof(int32_t) || max.size() < sizeof(int32_t)
                        || step.size() < sizeof(int32_t))
                    {
                        std::stringstream s;
                        s << "Size of data returned is not valid min size = " << min.size()
                            << ", max size = " << max.size() << ", step size = " << step.size();
                        throw std::runtime_error(s.str());
                    }

                    auto max_value = float(*(reinterpret_cast<int32_t *>(max.data())));
                    auto min_value = float(*(reinterpret_cast<int32_t *>(min.data())));

                    auto res = query_default();

                    _range = option_range{ min_value,
                                           max_value,
                                           float(*(reinterpret_cast<int32_t *>(step.data()))),
                                           res };
                });
            }

            void amc_option::enable_recording(std::function<void(const option&)> recording_action)
            {}

            float amc_option::query_current(rs2_sensor_mode mode) const //todo: remove sensor mode
            {
                auto res = _hw_monitor->send(command{ AMCGET, _type, get_current, mode });

                if (res.size() < sizeof(int32_t))
                {
                    std::stringstream s;
                    s << "Size of data returned from query(get_current) of " << _type << " is " << res.size()
                        << " while min size = " << sizeof(int32_t);
                    throw std::runtime_error(s.str());
                }
                auto val = *(reinterpret_cast<uint32_t *>(res.data()));
                return float(val);
            }
         } // namespace l535
    } // namespace ivcam2
} // namespace librealsense
