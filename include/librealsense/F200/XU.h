#pragma once

#ifndef LIBREALSENSE_F200_XU_H
#define LIBREALSENSE_F200_XU_H

#include "libuvc/libuvc.h"

#include <librealsense/F200/CalibParams.h>

#include <stdint.h>
#include <string>
#include <stdio.h>
#include <stdint.h>
#include <iostream>

#define IV_COMMAND_FIRMWARE_UPDATE_MODE     0x01
#define IV_COMMAND_GET_CALIBRATION_DATA     0x02
#define IV_COMMAND_LASER_POWER              0x03
#define IV_COMMAND_DEPTH_ACCURACY           0x04
#define IV_COMMAND_ZUNIT                    0x05
#define IV_COMMAND_LOW_CONFIDENCE_LEVEL     0x06
#define IV_COMMAND_INTENSITY_IMAGE_TYPE     0x07
#define IV_COMMAND_MOTION_VS_RANGE_TRADE    0x08
#define IV_COMMAND_POWER_GEAR               0x09
#define IV_COMMAND_FILTER_OPTION            0x0A
#define IV_COMMAND_VERSION                  0x0B
#define IV_COMMAND_CONFIDENCE_THRESHHOLD    0x0C

namespace f200
{
    
    CameraCalibrationParameters GetCalibration(uvc_device_handle_t *devh)
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
