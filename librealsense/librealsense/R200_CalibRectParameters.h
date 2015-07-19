#pragma once

#include <stdint.h>

const uint16_t DS_MAX_NUM_INTRINSICS_RIGHT = 2;         ///< Max number right cameras supported (e.g. one or two, two would support a multi-baseline unit)
const uint16_t DS_MAX_NUM_INTRINSICS_THIRD = 3;         ///< Max number native resolutions the third camera can have (e.g. 1920x1080 and 640x480)
const uint16_t DS_MAX_NUM_INTRINSICS_PLATFORM = 4;      ///< Max number native resolutions the platform camera can have
const uint16_t DS_MAX_NUM_RECTIFIED_MODES_LR = 4;       ///< Max number rectified LR resolution modes the structure supports (e.g. 640x480, 492x372 and 332x252)
const uint16_t DS_MAX_NUM_RECTIFIED_MODES_THIRD = 3;    ///< Max number rectified Third resolution modes the structure supports (e.g. 1920x1080, 1280x720, 640x480 and 320x240)
const uint16_t DS_MAX_NUM_RECTIFIED_MODES_PLATFORM = 1; ///< Max number rectified Platform resolution modes the structure supports

struct DSCalibIntrinsicsNonRectified
{
    float fx;
    float fy;
    float px;
    float py;
    float k[5];
    uint32_t w;
    uint32_t h;
};

struct DSCalibIntrinsicsRectified
{
    float rfx;
    float rfy;
    float rpx;
    float rpy;
    uint32_t rw;
    uint32_t rh;
};

struct DSCalibRectParameters
{
    uint32_t versionNumber;                 ///< Version of this format
    
    uint16_t numIntrinsicsRight;            ///< Number of right cameras < DS_MAX_NUM_INTRINSICS_RIGHT
    uint16_t numIntrinsicsThird;            ///< Number of native resolutions of third camera < DS_MAX_NUM_INTRINSICS_THIRD
    uint16_t numIntrinsicsPlatform;         ///< Number of native resolutions of platform camera < DS_MAX_NUM_INTRINSICS_PLATFORM
    uint16_t numRectifiedModesLR;           ///< Number of rectified LR resolution modes < DS_MAX_NUM_RECTIFIED_MODES_LR
    uint16_t numRectifiedModesThird;        ///< Number of rectified Third resolution modes < DS_MAX_NUM_RECTIFIED_MODES_THIRD
    uint16_t numRectifiedModesPlatform;     ///< Number of rectified Platform resolution modes < DS_MAX_NUM_RECTIFIED_MODES_PLATFORM
    
    DSCalibIntrinsicsNonRectified intrinsicsLeft;
    DSCalibIntrinsicsNonRectified intrinsicsRight[DS_MAX_NUM_INTRINSICS_RIGHT];
    DSCalibIntrinsicsNonRectified intrinsicsThird[DS_MAX_NUM_INTRINSICS_THIRD];
    DSCalibIntrinsicsNonRectified intrinsicsPlatform[DS_MAX_NUM_INTRINSICS_PLATFORM];
    
    DSCalibIntrinsicsRectified modesLR[DS_MAX_NUM_INTRINSICS_RIGHT][DS_MAX_NUM_RECTIFIED_MODES_LR];
    DSCalibIntrinsicsRectified modesThird[DS_MAX_NUM_INTRINSICS_RIGHT][DS_MAX_NUM_INTRINSICS_THIRD][DS_MAX_NUM_RECTIFIED_MODES_THIRD];
    DSCalibIntrinsicsRectified modesPlatform[DS_MAX_NUM_INTRINSICS_RIGHT][DS_MAX_NUM_INTRINSICS_PLATFORM][DS_MAX_NUM_RECTIFIED_MODES_PLATFORM];
    
    float Rleft[DS_MAX_NUM_INTRINSICS_RIGHT][9];
    float Rright[DS_MAX_NUM_INTRINSICS_RIGHT][9];
    float Rthird[DS_MAX_NUM_INTRINSICS_RIGHT][9];
    float Rplatform[DS_MAX_NUM_INTRINSICS_RIGHT][9];
    
    float B[DS_MAX_NUM_INTRINSICS_RIGHT];
    float T[DS_MAX_NUM_INTRINSICS_RIGHT][3];
    float Tplatform[DS_MAX_NUM_INTRINSICS_RIGHT][3];
    
    float Rworld[9];
    float Tworld[3];
};
