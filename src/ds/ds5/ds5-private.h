// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "ds/ds-private.h"

namespace librealsense
{
    namespace ds
    {
        const uint16_t RS400_PID = 0x0ad1; // PSR
        const uint16_t RS410_PID = 0x0ad2; // ASR
        const uint16_t RS415_PID = 0x0ad3; // ASRC
        const uint16_t RS430_PID = 0x0ad4; // AWG
        const uint16_t RS430_MM_PID = 0x0ad5; // AWGT
        const uint16_t RS_USB2_PID = 0x0ad6; // USB2
        const uint16_t RS_RECOVERY_PID = 0x0adb;
        const uint16_t RS_USB2_RECOVERY_PID = 0x0adc;
        const uint16_t RS400_IMU_PID = 0x0af2; // IMU
        const uint16_t RS420_PID = 0x0af6; // PWG
        const uint16_t RS420_MM_PID = 0x0afe; // PWGT
        const uint16_t RS410_MM_PID = 0x0aff; // ASRT
        const uint16_t RS400_MM_PID = 0x0b00; // PSR
        const uint16_t RS430_MM_RGB_PID = 0x0b01; // AWGCT
        const uint16_t RS460_PID = 0x0b03; // DS5U
        const uint16_t RS435_RGB_PID = 0x0b07; // AWGC
        const uint16_t RS405U_PID = 0x0b0c; // DS5U
        const uint16_t RS435I_PID = 0x0b3a; // D435i
        const uint16_t RS416_PID = 0x0b49; // F416
        const uint16_t RS430I_PID = 0x0b4b; // D430i
        const uint16_t RS465_PID = 0x0b4d; // D465
        const uint16_t RS416_RGB_PID = 0x0B52; // F416 RGB
        const uint16_t RS405_PID = 0x0B5B; // D405
        const uint16_t RS455_PID = 0x0B5C; // D455
        const uint16_t RS457_PID = 0xabcd; // D457

        // ds5 Devices supported by the current version
        static const std::set<std::uint16_t> rs400_sku_pid = {
            ds::RS400_PID,
            ds::RS410_PID,
            ds::RS415_PID,
            ds::RS430_PID,
            ds::RS430_MM_PID,
            ds::RS_USB2_PID,
            ds::RS400_IMU_PID,
            ds::RS420_PID,
            ds::RS420_MM_PID,
            ds::RS410_MM_PID,
            ds::RS400_MM_PID,
            ds::RS430_MM_RGB_PID,
            ds::RS460_PID,
            ds::RS435_RGB_PID,
            ds::RS405U_PID,
            ds::RS435I_PID,
            ds::RS416_RGB_PID,
            ds::RS430I_PID,
            ds::RS465_PID,
            ds::RS416_PID,
            ds::RS405_PID,
            ds::RS455_PID,
            ds::RS457_PID
        };

        static const std::set<std::uint16_t> ds5_multi_sensors_pid = {
            ds::RS400_MM_PID,
            ds::RS410_MM_PID,
            ds::RS415_PID,
            ds::RS420_MM_PID,
            ds::RS430_MM_PID,
            ds::RS430_MM_RGB_PID,
            ds::RS435_RGB_PID,
            ds::RS435I_PID,
            ds::RS465_PID,
            ds::RS455_PID,
            ds::RS457_PID
        };

        static const std::set<std::uint16_t> ds5_hid_sensors_pid = {
            ds::RS435I_PID,
            ds::RS430I_PID,
            ds::RS465_PID,
            ds::RS455_PID
        };

        static const std::set<std::uint16_t> ds5_hid_bmi_055_pid = {
            ds::RS435I_PID,
            ds::RS430I_PID,
            ds::RS455_PID
        };

        static const std::set<std::uint16_t> ds5_hid_bmi_085_pid = {
            RS465_PID
        };

        static const std::set<std::uint16_t> ds5_fisheye_pid = {
            ds::RS400_MM_PID,
            ds::RS410_MM_PID,
            ds::RS420_MM_PID,
            ds::RS430_MM_PID,
            ds::RS430_MM_RGB_PID,
        };

        static const std::map<std::uint16_t, std::string> rs400_sku_names = {
            { RS400_PID,            "Intel RealSense D400"},
            { RS410_PID,            "Intel RealSense D410"},
            { RS415_PID,            "Intel RealSense D415"},
            { RS430_PID,            "Intel RealSense D430"},
            { RS430_MM_PID,         "Intel RealSense D430 with Tracking Module"},
            { RS_USB2_PID,          "Intel RealSense USB2" },
            { RS_RECOVERY_PID,      "Intel RealSense D4XX Recovery"},
            { RS_USB2_RECOVERY_PID, "Intel RealSense D4XX USB2 Recovery"},
            { RS400_IMU_PID,        "Intel RealSense IMU" },
            { RS420_PID,            "Intel RealSense D420"},
            { RS420_MM_PID,         "Intel RealSense D420 with Tracking Module"},
            { RS410_MM_PID,         "Intel RealSense D410 with Tracking Module"},
            { RS400_MM_PID,         "Intel RealSense D400 with Tracking Module"},
            { RS430_MM_RGB_PID,     "Intel RealSense D430 with Tracking and RGB Modules"},
            { RS460_PID,            "Intel RealSense D460" },
            { RS435_RGB_PID,        "Intel RealSense D435"},
            { RS405U_PID,           "Intel RealSense DS5U" },
            { RS435I_PID,           "Intel RealSense D435I" },
            { RS416_PID,            "Intel RealSense F416"},
            { RS430I_PID,           "Intel RealSense D430I"},
            { RS465_PID,            "Intel RealSense D465" },
            { RS416_RGB_PID,        "Intel RealSense F416 with RGB Module"},
            { RS405_PID,            "Intel RealSense D405" },
            { RS455_PID,            "Intel RealSense D455" },
            { RS457_PID,            "Intel RealSense D457" },
        };

        static std::map<uint16_t, std::string> ds5_device_to_fw_min_version = {
            {RS400_PID, "5.8.15.0"},
            {RS410_PID, "5.8.15.0"},
            {RS415_PID, "5.8.15.0"},
            {RS430_PID, "5.8.15.0"},
            {RS430_MM_PID, "5.8.15.0"},
            {RS_USB2_PID, "5.8.15.0"},
            {RS_RECOVERY_PID, "5.8.15.0"},
            {RS_USB2_RECOVERY_PID, "5.8.15.0"},
            {RS400_IMU_PID, "5.8.15.0"},
            {RS420_PID, "5.8.15.0"},
            {RS420_MM_PID, "5.8.15.0"},
            {RS410_MM_PID, "5.8.15.0"},
            {RS400_MM_PID, "5.8.15.0" },
            {RS430_MM_RGB_PID, "5.8.15.0" },
            {RS460_PID, "5.8.15.0" },
            {RS435_RGB_PID, "5.8.15.0" },
            {RS405U_PID, "5.8.15.0" },
            {RS435I_PID, "5.12.7.100" },
            {RS416_PID, "5.8.15.0" },
            {RS430I_PID, "5.8.15.0" },
            {RS465_PID, "5.12.7.100" },
            {RS416_RGB_PID, "5.8.15.0" },
            {RS405_PID, "5.12.11.8" },
            {RS455_PID, "5.12.7.100" },
            {RS457_PID, "5.13.1.1" }
        };

        std::vector<platform::uvc_device_info> filter_ds5_device_by_capability(
            const std::vector<platform::uvc_device_info>& devices, d400_caps caps);
        bool ds5_try_fetch_usb_device(std::vector<platform::usb_device_info>& devices,
            const platform::uvc_device_info& info, platform::usb_device_info& result);

    } // namespace ds
} // namespace librealsense
