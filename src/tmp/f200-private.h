// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef LIBREALSENSE_F200_PRIVATE_H
#define LIBREALSENSE_F200_PRIVATE_H

#include "uvc.h"
#include "iv-common.h"

namespace rsimpl {
namespace f200 {

    struct cam_temperature_data
    {
        float LiguriaTemp;
        float IRTemp;
        float AmbientTemp;
    };

    struct thermal_loop_params
    {
        float IRThermalLoopEnable = 1;      // enable the mechanism
        float TimeOutA = 10000;             // default time out
        float TimeOutB = 0;                 // reserved
        float TimeOutC = 0;                 // reserved
        float TransitionTemp = 3;           // celcius degrees, the transition temperatures to ignore and use offset;
        float TempThreshold = 2;            // celcius degrees, the temperatures delta that above should be fixed;
        float HFOVsensitivity = 0.025f;
        float FcxSlopeA = -0.003696988f;    // the temperature model fc slope a from slope_hfcx = ref_fcx*a + b
        float FcxSlopeB = 0.005809239f;     // the temperature model fc slope b from slope_hfcx = ref_fcx*a + b
        float FcxSlopeC = 0;                // reserved
        float FcxOffset = 0;                // the temperature model fc offset
        float UxSlopeA = -0.000210918f;     // the temperature model ux slope a from slope_ux = ref_ux*a + ref_fcx*b
        float UxSlopeB = 0.000034253955f;   // the temperature model ux slope b from slope_ux = ref_ux*a + ref_fcx*b
        float UxSlopeC = 0;                 // reserved
        float UxOffset = 0;                 // the temperature model ux offset
        float LiguriaTempWeight = 1;        // the liguria temperature weight in the temperature delta calculations
        float IrTempWeight = 0;             // the Ir temperature weight in the temperature delta calculations
        float AmbientTempWeight = 0;        // reserved
        float Param1 = 0;                   // reserved
        float Param2 = 0;                   // reserved
        float Param3 = 0;                   // reserved
        float Param4 = 0;                   // reserved
        float Param5 = 0;                   // reserved
    };

    // Read calibration or device state
    std::tuple<iv::camera_calib_params, f200::cam_temperature_data, thermal_loop_params> read_f200_calibration(uvc::device & device, std::timed_mutex & mutex);
    float read_mems_temp(uvc::device & device, std::timed_mutex & mutex);
    int read_ir_temp(uvc::device & device, std::timed_mutex & mutex);

    // Modify device state
    void update_asic_coefficients(uvc::device & device, std::timed_mutex & mutex, const iv::camera_calib_params & compensated_params); // todo - Allow you to specify resolution

} // rsimpl::f200
} // namespace rsimpl

#endif
