#pragma once

#ifndef LIBREALSENSE_F200_TYPES_H
#define LIBREALSENSE_F200_TYPES_H

#include "libuvc/libuvc.h"

#include <stdint.h>
#include <string>
#include <stdio.h>
#include <stdint.h>
#include <iostream>


namespace f200
{

#define DELTA_INF	(10000000.0)
#define M_EPSILON	(0.0001)
    
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
    
enum DepthOutputUnit
{
    RAW_IVCAM   = 0,    // uint16_t in units of 1/32 mm
    FLOAT32_MM  = 1     // float in units of 1 mm
};

struct Point2DF32
{
    float x, y;
};

struct Point3DF32
{
    float x, y, z;
};

enum CoordinateSystem
{
    RIGHT_HANDED  = 0,
    LEFT_HANDED = 1
};

struct OACOffsetData
{
    int OACOffset1;
    int OACOffset2;
    int OACOffset3;
    int OACOffset4;
};

struct IVCAMTemperatureData
{
    float LiguriaTemp;
    float IRTemp;
    float AmbientTemp;
};

struct IVCAMThermalLoopParams
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

struct IVCAMTesterData
{
    int16_t TableValidation;
    int16_t TableVarsion;
    OACOffsetData OACOffsetData;
    IVCAMThermalLoopParams ThermalLoopParams;
    IVCAMTemperatureData TemperatureData;
};

struct CameraCalibrationParameters
{
    float Rmax;
    float Kc[3][3];		// [3x3]: intrinsic calibration matrix of the IR camera
    float Distc[5];		// [1x5]: forward distortion parameters of the IR camera
    float Invdistc[5];	// [1x5]: the inverse distortion parameters of the IR camera
    float Pp[3][4];		// [3x4]: projection matrix
    float Kp[3][3];		// [3x3]: intrinsic calibration matrix of the projector
    float Rp[3][3];		// [3x3]: extrinsic calibration matrix of the projector
    float Tp[3];			// [1x3]: translation vector of the projector
    float Distp[5];		// [1x5]: forward distortion parameters of the projector
    float Invdistp[5];	// [1x5]: inverse distortion parameters of the projector
    float Pt[3][4];		// [3x4]: IR to RGB (texture mapping) image transformation matrix
    float Kt[3][3];
    float Rt[3][3];
    float Tt[3];
    float Distt[5];		// [1x5]: The inverse distortion parameters of the RGB camera
    float Invdistt[5];
    float QV[6];
};

struct CameraCalibrationParametersVersion
{
    int uniqueNumber; //Should be 0xCAFECAFE in Calibration version 1 or later. In calibration version 0 this is zero.
    int16_t TableValidation;
    int16_t TableVarsion;
    CameraCalibrationParameters CalibrationParameters;
};

    
} // end namespace f200

#endif
