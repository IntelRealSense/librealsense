#pragma once

#ifndef LIBREALSENSE_F200_XU_H
#define LIBREALSENSE_F200_XU_H

#include "libuvc/libuvc.h"

#include <librealsense/F200/CalibParams.h>
#include <librealsense/F200/F200Types.h>

#include <stdint.h>
#include <string>
#include <stdio.h>
#include <stdint.h>
#include <iostream>

namespace f200
{
    
    inline CameraCalibrationParameters GetCalibration(uvc_device_handle_t *devh)
    {
        CameraCalibrationParameters params;
        
        uint8_t calib[800];
        
        size_t length = sizeof(calib);
        
        // XU Read
        auto bytesReturned = uvc_get_ctrl(devh, 2, IV_COMMAND_GET_CALIBRATION_DATA, &calib, int(length), UVC_GET_CUR);
        
        std::cout << "[Get Calibration] Actual Status " << bytesReturned << std::endl;
        
        if (bytesReturned > 0)
        {
            int size = sizeof(CameraCalibrationParameters) > bytesReturned ? bytesReturned : sizeof(CameraCalibrationParameters);
            memcpy(&params, calib, size);
            return params;
        }
        
        throw std::runtime_error("XURead failed for F200::GetCalibration()");
        
        return params;
    }

} // end namespace f200

#endif
