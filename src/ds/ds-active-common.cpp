// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include "ds-active-common.h"
#include "d400/d400-color.h"

namespace librealsense
{
    using namespace ds;

    ds_active_common::ds_active_common(uvc_sensor& raw_depth_ep,
        synthetic_sensor& depth_ep,
        device* owner,
        ds_caps device_capabilities,
        std::shared_ptr<hw_monitor> hw_monitor,
        firmware_version fw_version) :
        _raw_depth_ep(raw_depth_ep),
        _depth_ep(depth_ep),
        _owner(owner),
        _device_capabilities(device_capabilities),
        _hw_monitor(hw_monitor),
        _fw_version(fw_version) {}

    void ds_active_common::register_options()
    {
        //Projector's capacity is established based on actual HW capabilities
        auto pid = _owner->_pid;
        if ((pid != RS_USB2_PID) && ((_device_capabilities & ds_caps::CAP_ACTIVE_PROJECTOR) == ds_caps::CAP_ACTIVE_PROJECTOR))
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
            if (pid == ds::RS457_PID)
            {
                _depth_ep.register_option(RS2_OPTION_PROJECTOR_TEMPERATURE,
                    std::make_shared<projector_temperature_option_mipi>(_hw_monitor,
                        RS2_OPTION_PROJECTOR_TEMPERATURE));
            }
            else
            {
                _depth_ep.register_option(RS2_OPTION_PROJECTOR_TEMPERATURE,
                    std::make_shared<asic_and_projector_temperature_options>(_raw_depth_ep,
                        RS2_OPTION_PROJECTOR_TEMPERATURE));
            }

            // EMITTER FREQUENCY OPTION
            if ((pid == ds::RS457_PID || pid == ds::RS455_PID)
                && _fw_version >= firmware_version("5.14.0"))
            {
                auto emitter_freq = std::make_shared<uvc_xu_option<uint16_t>>(
                    _raw_depth_ep,
                    ds::depth_xu,
                    ds::DS5_EMITTER_FREQUENCY,
                    "Controls the emitter frequency, 57 [KHZ] / 91 [KHZ]",
                    std::map<float, std::string>{
                        { (float)RS2_EMITTER_FREQUENCY_57_KHZ, "57 KHZ" },
                    { (float)RS2_EMITTER_FREQUENCY_91_KHZ, "91 KHZ" } }, false);

                _depth_ep.register_option(RS2_OPTION_EMITTER_FREQUENCY, emitter_freq);
            }
        }
        else
        {
            LOG_WARNING("Projector capacity is overrided and disabled by FW\nDevice PID = 0x" << std::hex << pid
                << std::dec << ", Capabilities Vector = [" << _device_capabilities << "]");
        }
    }
}
