// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include "ds/ds-private.h"

namespace librealsense
{
    namespace ds
    {
        const uint16_t RS_D585_PID = 0x0B6A; // D585, D for depth
        const uint16_t RS_S585_PID = 0x0B6B; // S585, S for safety

        // Safety depth XU identifiers
        namespace xu_id
        {
            const uint8_t SAFETY_CAMERA_OPER_MODE = 0x1;
            const uint8_t SAFETY_PRESET_ACTIVE_INDEX = 0x2;
        }

        // ds6 Devices supported by the current version
        static const std::set<std::uint16_t> rs500_sku_pid = {
            ds::RS_D585_PID,
            ds::RS_S585_PID
        };

        static const std::set<std::uint16_t> ds6_multi_sensors_pid = {
            ds::RS_D585_PID,
            ds::RS_S585_PID
        };

        static const std::set<std::uint16_t> ds6_hid_sensors_pid = {
            ds::RS_D585_PID,
            ds::RS_S585_PID
        };

        static const std::set<std::uint16_t> ds6_hid_bmi_085_pid = {
            RS_D585_PID,
            RS_S585_PID
        };

        static const std::map<std::uint16_t, std::string> rs500_sku_names = {
            { RS_D585_PID,          "Intel RealSense D585" },
            { RS_S585_PID,          "Intel RealSense S585" }
        };

        //TODO
        //static std::map<uint16_t, std::string> ds6_device_to_fw_min_version = {
        //    {RS_D585_PID, "5.8.15.0"},
        //    {RS_S585_PID, "5.8.15.0"}
        //};

        bool ds6_try_fetch_usb_device(std::vector<platform::usb_device_info>& devices,
            const platform::uvc_device_info& info, platform::usb_device_info& result);
    } // namespace ds
} // namespace librealsense