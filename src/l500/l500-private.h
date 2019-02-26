// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#pragma once

#include "backend.h"
#include "types.h"
#include "option.h"

namespace librealsense
{
    namespace ivcam2
    {
        const uint16_t L500_PID = 0x0b0d;

        // L500 depth XU identifiers
        const uint8_t L500_HWMONITOR = 1;

        const platform::extension_unit depth_xu = { 0, 3, 2,
        { 0xC9606CCB, 0x594C, 0x4D25,{ 0xaf, 0x47, 0xcc, 0xc4, 0x96, 0x43, 0x59, 0x95 } } };

        const uint8_t IVCAM2_DEPTH_LASER_POWER = 2;

        enum fw_cmd : uint8_t
        {
            HWReset = 0x20,
            GVD = 0x10,
            GLD = 0x0f,
            DPT_INTRINSICS_GET = 0x5A,
            MRD = 0x01,
            TEMPERATURES_GET = 0x6A
        };

        enum gvd_fields
        {
            fw_version_offset = 12,
            module_serial_offset = 56,
            module_serial_size = 8
        };

        bool try_fetch_usb_device(std::vector<platform::usb_device_info>& devices,
                                         const platform::uvc_device_info& info, platform::usb_device_info& result);


        class l500_temperature_options : public readonly_option
        {
        public:
            float query() const override;

            option_range get_range() const override { return option_range{ 0, 100, 0, 0 }; };

            bool is_enabled() const override { return true; }

            const char* get_description() const override { return ""; }

            explicit l500_temperature_options(hw_monitor* hw_monitor, rs2_option opt);

        private:
            rs2_option _option;
            hw_monitor* _hw_monitor;
        };
    } // librealsense::ivcam2
} // namespace librealsense
