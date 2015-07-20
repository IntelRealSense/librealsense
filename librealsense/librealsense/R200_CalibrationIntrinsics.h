#pragma once

#ifndef CALIBRATION_INTRINSICS_H
#define CALIBRATION_INTRINSICS_H

#include <stdint.h>

const uint16_t MAX_NUM_INTRINSICS_RIGHT = 1;         // Max number right cameras supported (e.g. one or two, two would support a multi-baseline unit)
const uint16_t MAX_NUM_INTRINSICS_THIRD = 3;         // Max number native resolutions the third camera can have (e.g. 1920x1080 and 640x480)
const uint16_t MAX_NUM_INTRINSICS_PLATFORM = 4;      // Max number native resolutions the platform camera can have
const uint16_t MAX_NUM_RECTIFIED_MODES_LR = 4;       // Max number rectified LR resolution modes the structure supports (e.g. 640x480, 492x372 and 332x252)
const uint16_t MAX_NUM_RECTIFIED_MODES_THIRD = 3;    // Max number rectified Third resolution modes the structure supports (e.g. 1920x1080, 1280x720, 640x480 and 320x240)
const uint16_t MAX_NUM_RECTIFIED_MODES_PLATFORM = 1; // Max number rectified Platform resolution modes the structure supports

struct UnrectifiedIntrinsics
{
    float fx;
    float fy;
    float px;
    float py;
    float k[5];
    uint32_t w;
    uint32_t h;
};

struct RectifiedIntrinsics
{
    float rfx;
    float rfy;
    float rpx;
    float rpy;
    uint32_t rw;
    uint32_t rh;
};

struct CameraCalibrationParameters
{
    uint32_t versionNumber;                 // Version of this format
    
    uint16_t numIntrinsicsRight;            // Number of right cameras < MAX_NUM_INTRINSICS_RIGHT
    uint16_t numIntrinsicsThird;            // Number of native resolutions of third camera < MAX_NUM_INTRINSICS_THIRD
    uint16_t numIntrinsicsPlatform;         // Number of native resolutions of platform camera < MAX_NUM_INTRINSICS_PLATFORM
    uint16_t numRectifiedModesLR;           // Number of rectified LR resolution modes < MAX_NUM_RECTIFIED_MODES_LR
    uint16_t numRectifiedModesThird;        // Number of rectified Third resolution modes < MAX_NUM_RECTIFIED_MODES_THIRD
    uint16_t numRectifiedModesPlatform;     // Number of rectified Platform resolution modes < MAX_NUM_RECTIFIED_MODES_PLATFORM
    
    UnrectifiedIntrinsics intrinsicsLeft;
    UnrectifiedIntrinsics intrinsicsRight[MAX_NUM_INTRINSICS_RIGHT];
    UnrectifiedIntrinsics intrinsicsThird[MAX_NUM_INTRINSICS_THIRD];
    UnrectifiedIntrinsics intrinsicsPlatform[MAX_NUM_INTRINSICS_PLATFORM];
    
    RectifiedIntrinsics modesLR[MAX_NUM_INTRINSICS_RIGHT][MAX_NUM_RECTIFIED_MODES_LR];
    RectifiedIntrinsics modesThird[MAX_NUM_INTRINSICS_RIGHT][MAX_NUM_INTRINSICS_THIRD][MAX_NUM_RECTIFIED_MODES_THIRD];
    RectifiedIntrinsics modesPlatform[MAX_NUM_INTRINSICS_RIGHT][MAX_NUM_INTRINSICS_PLATFORM][MAX_NUM_RECTIFIED_MODES_PLATFORM];
    
    float Rleft[MAX_NUM_INTRINSICS_RIGHT][9];
    float Rright[MAX_NUM_INTRINSICS_RIGHT][9];
    float Rthird[MAX_NUM_INTRINSICS_RIGHT][9];
    float Rplatform[MAX_NUM_INTRINSICS_RIGHT][9];
    
    float B[MAX_NUM_INTRINSICS_RIGHT];
    float T[MAX_NUM_INTRINSICS_RIGHT][3];
    float Tplatform[MAX_NUM_INTRINSICS_RIGHT][3];
    
    float Rworld[9];
    float Tworld[3];
};

#endif
