// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <mutex>
#include <chrono>
#include <vector>
#include <iterator>
#include <cstddef>

#include "device.h"
#include "context.h"
#include "image.h"
#include "metadata-parser.h"

#include "ds5-active.h"
#include "ds5-private.h"
#include "ds5-options.h"
#include "ds5-timestamp.h"

namespace librealsense
{
    ds5_active::ds5_active(std::shared_ptr<context> ctx,
                           const platform::backend_device_group& group)
        : device(ctx, group), ds5_device(ctx, group)
    {
        using namespace ds;

        //Projector's capacity is established based on actual HW capabilities
        auto pid = group.uvc_devices.front().pid;
        if ((pid != RS_USB2_PID) && ((_device_capabilities & d400_caps::CAP_ACTIVE_PROJECTOR) == d400_caps::CAP_ACTIVE_PROJECTOR))
        {
            auto&& depth_ep = get_depth_sensor();
            auto&& raw_depth_ep = get_raw_depth_sensor();

            //EMITTER ENABLED OPTION
            auto emitter_enabled = std::make_shared<emitter_option>(raw_depth_ep);
            
            //LASER POWER OPTION
            auto laser_power = std::make_shared<uvc_xu_option<uint16_t>>(raw_depth_ep,
                                                                         depth_xu,
                                                                         DS5_LASER_POWER,
                                                                         "Manual laser power in mw. applicable only when laser power mode is set to Manual");

            auto laser_power_auto_disabling = std::make_shared<auto_disabling_control>(
                                     laser_power,
                                     emitter_enabled,
                                     std::vector<float>{0.f, 2.f}, 1.f);

            if (auto hdr_enabled_option = depth_ep.get_option_handler(RS2_OPTION_HDR_ENABLED))
            {
                std::vector<std::pair<std::shared_ptr<option>, std::string>> emitter_options_and_reasons = { std::make_pair(hdr_enabled_option,
                    "Emitter status cannot be set while HDR is enabled")};
                depth_ep.register_option(RS2_OPTION_EMITTER_ENABLED,
                    std::make_shared<gated_option>(
                        emitter_enabled,
                        emitter_options_and_reasons));
                
                std::vector<std::pair<std::shared_ptr<option>, std::string>> laser_options_and_reasons = { std::make_pair(hdr_enabled_option,
                    "Laser Power status cannot be set while HDR is enabled") };
                depth_ep.register_option(RS2_OPTION_LASER_POWER,
                    std::make_shared<gated_option>(
                        laser_power_auto_disabling,
                        laser_options_and_reasons));
            }
            else
            {
                depth_ep.register_option(RS2_OPTION_EMITTER_ENABLED, emitter_enabled);
                depth_ep.register_option(RS2_OPTION_LASER_POWER, laser_power_auto_disabling);
            }
            
            //PROJECTOR TEMPERATURE OPTION
            depth_ep.register_option(RS2_OPTION_PROJECTOR_TEMPERATURE,
                std::make_shared<asic_and_projector_temperature_options>(raw_depth_ep,
                    RS2_OPTION_PROJECTOR_TEMPERATURE));
        }
        else
        {
            LOG_WARNING("Projector capacity is overrided and disabled by FW\nDevice PID = 0x" << std::hex << pid
                << std::dec << ", Capabilities Vector = [" << _device_capabilities << "]");
        }
    }
}
