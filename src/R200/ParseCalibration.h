#pragma once

#ifndef LIBREALSENSE_R200_PARSE_CALIBRATION_H
#define LIBREALSENSE_R200_PARSE_CALIBRATION_H

#include <stdint.h>

#include "R200Types.h"

namespace r200
{

inline void swap_to_little(unsigned char * result, const unsigned char * origin, int numBytes)
{
    for (int i = 0; i < numBytes; i++)
    {
        result[i] = origin[numBytes - 1 - i];
    }
}

// Only valid for basic types
template <class T>
inline void read_bytes(const unsigned char *& p, T & x)
{
    swap_to_little((unsigned char *)&x, p, sizeof(T));
    p += sizeof(T);
}
 
template<unsigned long size>
inline void read_array(const unsigned char *& p, std::array<float, size> & x)
{
    for (int i = 0; i < x.size(); i++)
    {
        read_bytes(p, x[i]);
    }
}

inline void read_unrectified(const unsigned char *& p, UnrectifiedIntrinsics & cri)
{
    read_bytes(p, cri.fx);
    read_bytes(p, cri.fy);
    read_bytes(p, cri.px);
    read_bytes(p, cri.py);
    read_bytes(p, cri.k[0]);
    read_bytes(p, cri.k[1]);
    read_bytes(p, cri.k[2]);
    read_bytes(p, cri.k[3]);
    read_bytes(p, cri.k[4]);
    read_bytes(p, cri.w);
    read_bytes(p, cri.h);
}

inline void read_rectified(const unsigned char *& p, RectifiedIntrinsics & crm)
{
    read_bytes(p, crm.rfx);
    read_bytes(p, crm.rfy);
    read_bytes(p, crm.rpx);
    read_bytes(p, crm.rpy);
    read_bytes(p, crm.rw);
    read_bytes(p, crm.rh);
}

inline CameraCalibrationParameters ParseCalibrationParameters(const uint8_t * buffer)
{
    const unsigned char * p = buffer;
    
    CameraCalibrationParameters cal = {0};
    
    //////////////
    // Metadata //
    //////////////
    
    read_bytes(p, cal.metadata.versionNumber);
    read_bytes(p, cal.metadata.numIntrinsicsRight);
    read_bytes(p, cal.metadata.numIntrinsicsThird);
    read_bytes(p, cal.metadata.numIntrinsicsPlatform);
    read_bytes(p, cal.metadata.numRectifiedModesLR);
    read_bytes(p, cal.metadata.numRectifiedModesThird);
    read_bytes(p, cal.metadata.numRectifiedModesPlatform);
    
    ////////////////////////////
    // Unrectified Parameters //
    ////////////////////////////
    
    read_unrectified(p, cal.intrinsicsLeft);
    
    cal.intrinsicsRight.resize(MAX_NUM_INTRINSICS_RIGHT);
    for (auto & t : cal.intrinsicsRight) { read_unrectified(p, t); }
    
    cal.intrinsicsThird.resize(MAX_NUM_INTRINSICS_THIRD);
    for (auto & t : cal.intrinsicsThird) { read_unrectified(p, t); }
    
    cal.intrinsicsPlatform.resize(MAX_NUM_INTRINSICS_PLATFORM);
    for (auto & t : cal.intrinsicsPlatform) { read_unrectified(p, t); }
    
    //////////////////////////
    // Rectified Parameters //
    //////////////////////////
    
    cal.modesLR.resize(MAX_NUM_INTRINSICS_RIGHT * MAX_NUM_RECTIFIED_MODES_LR);
    for (auto & t : cal.modesLR) { read_rectified(p, t); }
    
    cal.modesThird.resize(MAX_NUM_INTRINSICS_RIGHT * MAX_NUM_INTRINSICS_THIRD * MAX_NUM_RECTIFIED_MODES_THIRD);
    for (auto & t : cal.modesThird) { read_rectified(p, t); }
    
    cal.modesPlatform.resize(MAX_NUM_INTRINSICS_RIGHT * MAX_NUM_INTRINSICS_PLATFORM * MAX_NUM_RECTIFIED_MODES_PLATFORM);
    for (auto & t : cal.modesPlatform) { read_rectified(p, t); }
    
    //////////
    // Mats //
    //////////
    
    cal.Rleft.resize(MAX_NUM_INTRINSICS_RIGHT);
    for (auto & mat : cal.Rleft) { read_array(p, mat); };
    
    cal.Rright.resize(MAX_NUM_INTRINSICS_RIGHT);
    for (auto & mat : cal.Rright) { read_array(p, mat); };
    
    cal.Rthird.resize(MAX_NUM_INTRINSICS_RIGHT);
    for (auto & mat : cal.Rthird) { read_array(p, mat); };
    
    cal.Rplatform.resize(MAX_NUM_INTRINSICS_RIGHT);
    for (auto & mat : cal.Rplatform) { read_array(p, mat); };
    
    // B & T
    cal.B.resize(MAX_NUM_INTRINSICS_RIGHT);
    for (int i = 0; i < cal.B.size(); i++) { read_bytes(p, cal.B[i]); }
    
    cal.T.resize(MAX_NUM_INTRINSICS_RIGHT);
    for (auto & mat : cal.T) { read_array(p, mat); };
    
    cal.Tplatform.resize(MAX_NUM_INTRINSICS_RIGHT);
    for (auto & mat : cal.Tplatform) { read_array(p, mat); };
    
    // RWorld & TWorld
    read_array(p, cal.Rworld);
    read_array(p, cal.Tworld);
    
    return cal;
}

} // end namespace r200

#endif