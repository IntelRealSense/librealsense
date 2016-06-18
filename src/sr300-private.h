// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef LIBREALSENSE_SR300_PRIVATE_H
#define LIBREALSENSE_SR300_PRIVATE_H

#include "uvc.h"
#include "iv-common.h"

namespace rsimpl {
    namespace sr300
    {
        // Read calibration or device state
        iv::camera_calib_params read_sr300_calibration(uvc::device & device, std::timed_mutex & mutex);

        enum class wakeonusb_reason : unsigned char
        {
            eNone_provided  = 0,    // Wake-up performed, but FW doesn't provide a comprehensive resason
            eUser_input     = 1,    // Firmware recognized user input and performed system wake up accordingly
            eUninitialized  = 2,    // Querrying the interface before the FW had a chance to perform  ACPI wake on USB
            eMaxWakeOnReason
        };

        enum class e_suspend_fps : uint32_t
        {
            eFPS_2 = 0,
            eFPS_1,
            eFPS_05,
            eFPS_025,
            eFPS_MAX
        };

        struct wakeup_dev_params
        {
            wakeup_dev_params(void) : phase1Period(UINT32_MAX), phase1FPS(e_suspend_fps::eFPS_MAX), phase2Period(UINT32_MAX), phase2FPS(e_suspend_fps::eFPS_MAX) {};
            wakeup_dev_params(uint32_t  p1, e_suspend_fps p2, uint32_t  p3, e_suspend_fps p4) : phase1Period(p1), phase1FPS(p2), phase2Period(p3), phase2FPS(p4) {};
            uint32_t        phase1Period;
            e_suspend_fps   phase1FPS;
            uint32_t        phase2Period;
            e_suspend_fps   phase2FPS;
            bool isValid() { return ((phase1FPS < e_suspend_fps::eFPS_MAX) && (phase2FPS < e_suspend_fps::eFPS_MAX) && (phase1Period < UINT32_MAX) && (phase2Period < UINT32_MAX)); };
        };

        // Wakeup device interfaces
        void set_wakeup_device(uvc::device & device, std::timed_mutex & mutex, const uint32_t&phase1Period, const uint32_t& phase1FPS, const uint32_t&phase2Period, const uint32_t& phase2FPS);
        void reset_wakeup_device(uvc::device & device, std::timed_mutex & mutex);
        void get_wakeup_reason(uvc::device & device, std::timed_mutex & mutex, unsigned char &cReason);
        void get_wakeup_confidence(uvc::device & device, std::timed_mutex & mutex, unsigned char &cConfidence);

    } // rsimpl::sr300
} // namespace rsimpl

#endif
