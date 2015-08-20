#pragma once

#ifndef LIBREALSENSE_R200_TYPES_H
#define LIBREALSENSE_R200_TYPES_H

#include <cstdint>
#include <vector>
#include <array>

namespace r200
{
    
// Max number right cameras supported (e.g. one or two, two would support a multi-baseline unit)
const uint16_t MAX_NUM_INTRINSICS_RIGHT = 2;

// Max number native resolutions the third camera can have (e.g. 1920x1080 and 640x480)
const uint16_t MAX_NUM_INTRINSICS_THIRD = 3;

// Max number native resolutions the platform camera can have
const uint16_t MAX_NUM_INTRINSICS_PLATFORM = 4;

// Max number rectified LR resolution modes the structure supports (e.g. 640x480, 492x372 and 332x252)
const uint16_t MAX_NUM_RECTIFIED_MODES_LR = 4;

// Max number rectified Third resolution modes the structure supports (e.g. 1920x1080, 1280x720, etc)
const uint16_t MAX_NUM_RECTIFIED_MODES_THIRD = 3;

// Max number rectified Platform resolution modes the structure supports
const uint16_t MAX_NUM_RECTIFIED_MODES_PLATFORM = 1;

struct CalibrationMetadata
{
    uint32_t versionNumber;
    uint16_t numIntrinsicsRight;
    uint16_t numIntrinsicsThird;
    uint16_t numIntrinsicsPlatform;
    uint16_t numRectifiedModesLR;
    uint16_t numRectifiedModesThird;
    uint16_t numRectifiedModesPlatform;
};

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
    CalibrationMetadata metadata;
    
    UnrectifiedIntrinsics intrinsicsLeft;
    std::vector<UnrectifiedIntrinsics> intrinsicsRight;
    std::vector<UnrectifiedIntrinsics> intrinsicsThird;
    std::vector<UnrectifiedIntrinsics> intrinsicsPlatform;
    
    std::vector<RectifiedIntrinsics> modesLR;
    std::vector<RectifiedIntrinsics> modesThird;
    std::vector<RectifiedIntrinsics> modesPlatform;
    
    std::vector<std::array<float, 9>> Rleft;
    std::vector<std::array<float, 9>> Rright;
    std::vector<std::array<float, 9>> Rthird;
    std::vector<std::array<float, 9>> Rplatform;
    
    std::vector<float> B;
    std::vector<std::array<float, 3>> T;
    std::vector<std::array<float, 3>> Tplatform;
    
    float Rworld[9];
    float Tworld[3];
};
    
} // end namespace r200

#endif
