// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "serializable-interface.h"
#include "hw-monitor.h"
#include "sensor.h"

namespace librealsense
{
    class auto_cal_algo
    {
    public:
      
        bool optimaize(rs2::frame depth, rs2::frame ir, rs2::frame yuy, rs2::frame prev_yuy, const calibration& old_calib, calibration* new_calib);

    private:
     
    };
} // namespace librealsense
