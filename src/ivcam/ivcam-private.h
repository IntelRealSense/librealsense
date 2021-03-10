// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "backend.h"

namespace librealsense {
    namespace ivcam {
        // IVCAM depth XU identifiers
        const uint8_t IVCAM_DEPTH_LASER_POWER = 1;
        const uint8_t IVCAM_DEPTH_ACCURACY = 2;
        const uint8_t IVCAM_DEPTH_MOTION_RANGE = 3;
        const uint8_t IVCAM_DEPTH_ERROR = 4;
        const uint8_t IVCAM_DEPTH_FILTER_OPTION = 5;
        const uint8_t IVCAM_DEPTH_CONFIDENCE_THRESH = 6;
        const uint8_t IVCAM_DEPTH_DYNAMIC_FPS = 7; // Only available on IVCAM 1.0 / F200

        // IVCAM color XU identifiers
        const uint8_t IVCAM_COLOR_EXPOSURE_PRIORITY = 1;
        const uint8_t IVCAM_COLOR_AUTO_FLICKER = 2;
        const uint8_t IVCAM_COLOR_ERROR = 3;
        const uint8_t IVCAM_COLOR_EXPOSURE_GRANULAR = 4;

        const platform::extension_unit depth_xu { 1, 6, 1,
            { 0xA55751A1, 0xF3C5, 0x4A5E, { 0x8D, 0x5A, 0x68, 0x54, 0xB8, 0xFA, 0x27, 0x16 } } };

        struct camera_calib_params
        {
            float Rmax;
            float Kc[3][3];     // [3x3]: intrinsic calibration matrix of the IR camera
            float Distc[5];     // [1x5]: forward distortion parameters of the IR camera
            float Invdistc[5];  // [1x5]: the inverse distortion parameters of the IR camera
            float Pp[3][4];     // [3x4]: projection matrix
            float Kp[3][3];     // [3x3]: intrinsic calibration matrix of the projector
            float Rp[3][3];     // [3x3]: extrinsic calibration matrix of the projector
            float Tp[3];        // [1x3]: translation vector of the projector
            float Distp[5];     // [1x5]: forward distortion parameters of the projector
            float Invdistp[5];  // [1x5]: inverse distortion parameters of the projector
            float Pt[3][4];     // [3x4]: IR to RGB (texture mapping) image transformation matrix
            float Kt[3][3];
            float Rt[3][3];
            float Tt[3];
            float Distt[5];     // [1x5]: The inverse distortion parameters of the RGB camera
            float Invdistt[5];
            float QV[6];
        };

        struct cam_calibration
        {
            int     uniqueNumber;           //Should be 0xCAFECAFE in Calibration version 1 or later. In calibration version 0 this is zero.
            int16_t TableValidation;
            int16_t TableVersion;
            camera_calib_params CalibrationParameters;
        };

        struct cam_auto_range_request
        {
            int enableMvR;
            int enableLaser;
            int minMvR;
            int maxMvR;
            int startMvR;
            int minLaser;
            int maxLaser;
            int startLaser;
            int ARUpperTh;
            int ARLowerTh;
        };

        enum fw_cmd : uint8_t
        {
            GetMEMSTemp = 0x0A,
            DebugFormat = 0x0B,
            TimeStampEnable = 0x0C,
            GetFWLastError = 0x0E,
            HWReset = 0x28,
            GVD = 0x3B,
            GetCalibrationTable = 0x3D,
            CheckI2cConnect = 0x4A,
            CheckRGBConnect = 0x4B,
            CheckDPTConnect = 0x4C,
            GetIRTemp = 0x52,
            GoToDFU = 0x80,
            OnSuspendResume = 0x91,
            GetWakeReason = 0x93,
            GetWakeConfidence = 0x92,
            SetAutoRange = 0xA6,
            SetDefaultControls = 0xA6,
            GetDefaultControls = 0xA7,
            AutoRangeSetParamsforDebug = 0xb3,
            UpdateCalib = 0xBC,
            BIST = 0xFF,
            GetPowerGearState = 0xFF,
            GLD = 0x35,
            FlashRead = 0x23,
            SetRgbAeRoi = 0xdb,
            GetRgbAeRoi = 0xdc,
        };

        enum gvd_fields
        {
            fw_version_offset    = 0,
            module_serial_offset = 132
        };

        enum gvd_fields_size
        {
            // Keep sorted
            module_serial_size = 6
        };
        bool try_fetch_usb_device(std::vector<platform::usb_device_info>& devices,
            const platform::uvc_device_info& info, platform::usb_device_info& result);

    } // librealsense::ivcam
} // namespace librealsense
