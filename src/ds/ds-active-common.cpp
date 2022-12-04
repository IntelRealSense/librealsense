// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include "ds-active-common.h"
#include "ds5/ds5-color.h"
#include "ds6/ds6-color.h"

namespace librealsense
{
    using namespace ds;

    ds_active_common::ds_active_common(uvc_sensor& raw_depth_ep,
        synthetic_sensor& depth_ep,
        device* owner,
        d400_caps device_capabilities) :
        _raw_depth_ep(raw_depth_ep),
        _depth_ep(depth_ep),
        _owner(owner),
        _device_capabilities(device_capabilities){}

    void ds_active_common::register_options()
    {
        //Projector's capacity is established based on actual HW capabilities
        auto pid = _owner->_pid;
        if ((pid != RS_USB2_PID) && ((_device_capabilities & d400_caps::CAP_ACTIVE_PROJECTOR) == d400_caps::CAP_ACTIVE_PROJECTOR))
        {
            //EMITTER ENABLED OPTION
            auto emitter_enabled = std::make_shared<emitter_option>(_raw_depth_ep);

            //LASER POWER OPTION
            auto laser_power = std::make_shared<uvc_xu_option<uint16_t>>(_raw_depth_ep,
                depth_xu,
                DS5_LASER_POWER,
                "Manual laser power in mw. applicable only when laser power mode is set to Manual");

            auto laser_power_auto_disabling = std::make_shared<auto_disabling_control>(
                laser_power,
                emitter_enabled,
                std::vector<float>{0.f, 2.f}, 1.f);

            if (auto hdr_enabled_option = _depth_ep.get_option_handler(RS2_OPTION_HDR_ENABLED))
            {
                std::vector<std::pair<std::shared_ptr<option>, std::string>> emitter_options_and_reasons = { std::make_pair(hdr_enabled_option,
                    "Emitter status cannot be set while HDR is enabled") };
                _depth_ep.register_option(RS2_OPTION_EMITTER_ENABLED,
                    std::make_shared<gated_option>(
                        emitter_enabled,
                        emitter_options_and_reasons));

                std::vector<std::pair<std::shared_ptr<option>, std::string>> laser_options_and_reasons = { std::make_pair(hdr_enabled_option,
                    "Laser Power status cannot be set while HDR is enabled") };
                _depth_ep.register_option(RS2_OPTION_LASER_POWER,
                    std::make_shared<gated_option>(
                        laser_power_auto_disabling,
                        laser_options_and_reasons));
            }
            else
            {
                _depth_ep.register_option(RS2_OPTION_EMITTER_ENABLED, emitter_enabled);
                _depth_ep.register_option(RS2_OPTION_LASER_POWER, laser_power_auto_disabling);
            }

            //PROJECTOR TEMPERATURE OPTION
            _depth_ep.register_option(RS2_OPTION_PROJECTOR_TEMPERATURE,
                std::make_shared<asic_and_projector_temperature_options>(_raw_depth_ep,
                    RS2_OPTION_PROJECTOR_TEMPERATURE));
        }
        else
        {
            LOG_WARNING("Projector capacity is overrided and disabled by FW\nDevice PID = 0x" << std::hex << pid
                << std::dec << ", Capabilities Vector = [" << _device_capabilities << "]");
        }
    }
}
