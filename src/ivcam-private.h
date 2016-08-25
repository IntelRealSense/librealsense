// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef LIBREALSENSE_IV_PRIVATE_H
#define LIBREALSENSE_IV_PRIVATE_H

#include "uvc.h"
#include <mutex>

namespace rsimpl {
namespace ivcam {

    const uvc::extension_unit depth_xu{ 1, 6, 1, { 0xA55751A1, 0xF3C5, 0x4A5E, { 0x8D, 0x5A, 0x68, 0x54, 0xB8, 0xFA, 0x27, 0x16 } } };

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
        ivcam::camera_calib_params CalibrationParameters;
    };

    struct cam_auto_range_request
    {
        int enableMvR;          // Send as IVCAMCommand::Param1
        int enableLaser;        // Send as IVCAMCommand::Param2
        int16_t minMvR;         // Copy into IVCAMCommand::data
        int16_t maxMvR;         // "
        int16_t startMvR;       // "
        int16_t minLaser;       // "
        int16_t maxLaser;       // "
        int16_t startLaser;     // "
        uint16_t ARUpperTh;     // Copy into IVCAMCommand::data if not -1
        uint16_t ARLowerTh;     // "
    };

    enum class fw_cmd : uint8_t
    {
        GetMEMSTemp                 = 0x0A,
        DebugFormat                 = 0x0B,
        TimeStampEnable             = 0x0C,
        GetFWLastError              = 0x0E,
        HWReset                     = 0x28,
        GVD                         = 0x3B,
        GetCalibrationTable         = 0x3D,
        CheckI2cConnect             = 0x4A,
        CheckRGBConnect             = 0x4B,
        CheckDPTConnect             = 0x4C,
        GetIRTemp                   = 0x52,
        GoToDFU                     = 0x80,
        OnSuspendResume             = 0x91,
        GetWakeReason               = 0x93,
        GetWakeConfidence           = 0x92,
        SetAutoRange                = 0xA6,
        SetDefaultControls          = 0xA6,
        GetDefaultControls          = 0xA7,
        AutoRangeSetParamsforDebug  = 0xb3,
        UpdateCalib                 = 0xBC,
        BIST                        = 0xFF,
        GetPowerGearState           = 0xFF
    };

    enum class FirmwareError : int32_t
    {
        FW_ACTIVE = 0,
        FW_MSAFE_S1_ERR,
        FW_I2C_SAFE_ERR,
        FW_FLASH_SAFE_ERR,
        FW_I2C_CFG_ERR,
        FW_I2C_EV_ERR,
        FW_HUMIDITY_ERR,
        FW_MSAFE_S0_ERR,
        FW_LD_ERR,
        FW_PI_ERR,
        FW_PJCLK_ERR,
        FW_OAC_ERR,
        FW_LIGURIA_TEMPERATURE_ERR,
        FW_CONTINUE_SAFE_ERROR,
        FW_FORZA_HUNG,
        FW_FORZA_CONTINUES_HUNG,
        FW_PJ_EYESAFETY_CHKRHARD,
        FW_MIPI_PCAM_ERR,
        FW_MIPI_TCAM_ERR,
        FW_SYNC_DISABLED,
        FW_MIPI_PCAM_SVR_ERR,
        FW_MIPI_TCAM_SVR_ERR,
        FW_MIPI_PCAM_FRAME_SIZE_ERR,
        FW_MIPI_TCAM_FRAME_SIZE_ERR,
        FW_MIPI_PCAM_FRAME_RESPONSE_ERR,
        FW_MIPI_TCAM_FRAME_RESPONSE_ERR,
        FW_USB_PCAM_THROTTLED_ERR,
        FW_USB_TCAM_THROTTLED_ERR,
        FW_USB_PCAM_QOS_WAR,
        FW_USB_TCAM_QOS_WAR,
        FW_USB_PCAM_OVERFLOW,
        FW_USB_TCAM_OVERFLOW,
        FW_Flash_OEM_SECTOR,
        FW_Flash_CALIBRATION_RW,
        FW_Flash_IR_CALIBRATION,
        FW_Flash_RGB_CALIBRATION,
        FW_Flash_THERMAL_LOOP_CONFIGURATION,
        FW_Flash_REALTEK,
        FW_RGB_ISP_BOOT_FAILED,
        FW_PRIVACY_RGB_OFF,
        FW_PRIVACY_DEPTH_OFF,
        FW_COUNT_ERROR
    };

    // Claim USB interface used for device
    void claim_ivcam_interface(uvc::device & device);

    // Read device state
    size_t prepare_usb_command(uint8_t * request, size_t & requestSize, uint32_t op, uint32_t p1 = 0, uint32_t p2 = 0, uint32_t p3 = 0, uint32_t p4 = 0, uint8_t * data = 0, size_t dataLength = 0);
    void get_gvd(uvc::device & device, std::timed_mutex & mutex, size_t sz, char * gvd, int gvd_cmd = (int)fw_cmd::GVD);
    void get_firmware_version_string(uvc::device & device, std::timed_mutex & mutex, std::string & version, int gvd_cmd = (int)fw_cmd::GVD, int offset = 0);
    void get_module_serial_string(uvc::device & device, std::timed_mutex & mutex, std::string & serial, int offset);

    // Modify device state
    void force_hardware_reset(uvc::device & device, std::timed_mutex & mutex);
    void enable_timestamp(uvc::device & device, std::timed_mutex & mutex, bool colorEnable, bool depthEnable);
    void set_auto_range(uvc::device & device, std::timed_mutex & mutex, int enableMvR, int16_t minMvR, int16_t maxMvR, int16_t startMvR, int enableLaser, int16_t minLaser, int16_t maxLaser, int16_t startLaser, int16_t ARUpperTH, int16_t ARLowerTH);

    // XU read/write
    void get_laser_power(const uvc::device & device, uint8_t & laser_power);
    void set_laser_power(uvc::device & device, uint8_t laser_power);
    void get_accuracy(const uvc::device & device, uint8_t & accuracy);
    void set_accuracy(uvc::device & device, uint8_t accuracy);
    void get_motion_range(const uvc::device & device, uint8_t & motion_range);
    void set_motion_range(uvc::device & device, uint8_t motion_range);
    void get_filter_option(const uvc::device & device, uint8_t & filter_option);
    void set_filter_option(uvc::device & device, uint8_t filter_option);
    void get_confidence_threshold(const uvc::device & device, uint8_t & conf_thresh);
    void set_confidence_threshold(uvc::device & device, uint8_t conf_thresh);

} // rsimpl::ivcam

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
    std::tuple<ivcam::camera_calib_params, f200::cam_temperature_data, thermal_loop_params> read_f200_calibration(uvc::device & device, std::timed_mutex & mutex);
    float read_mems_temp(uvc::device & device, std::timed_mutex & mutex);
    int read_ir_temp(uvc::device & device, std::timed_mutex & mutex);

    // Modify device state
    void update_asic_coefficients(uvc::device & device, std::timed_mutex & mutex, const ivcam::camera_calib_params & compensated_params); // todo - Allow you to specify resolution

    void get_dynamic_fps(const uvc::device & device, uint8_t & dynamic_fps);
    void set_dynamic_fps(uvc::device & device, uint8_t dynamic_fps);

} // rsimpl::f200

namespace sr300
{
    // Read calibration or device state
    ivcam::camera_calib_params read_sr300_calibration(uvc::device & device, std::timed_mutex & mutex);
} // rsimpl::sr300
} // namespace rsimpl

#endif  // IV_PRIVATE_H
