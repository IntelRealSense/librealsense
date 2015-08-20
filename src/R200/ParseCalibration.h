#pragma once

#ifndef LIBREALSENSE_R200_PARSE_CALIBRATION_H
#define LIBREALSENSE_R200_PARSE_CALIBRATION_H

#include <stdint.h>

#include "R200Types.h"

namespace r200
{

/*inline void swap_to_little(unsigned char * result, const unsigned char * origin, int numBytes)
{
    for (int i = 0; i < numBytes; i++)
    {
        result[i] = origin[numBytes - 1 - i];
    }
}*/

// Only valid for basic types
template <class T>
inline void read_bytes(const unsigned char *& p, T & x)
{
    memcpy(&x, p, sizeof(T));
    //swap_to_little((unsigned char *)&x, p, sizeof(T));
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

template<class T> void swap_endian_bytewise(T & value)
{
    auto p = (uint8_t *)&value;
    for(size_t i=0; i<sizeof(T)/2; ++i) std::swap(p[i], p[sizeof(T)-i-1]);
}

void swap_endian(uint16_t & value) { swap_endian_bytewise(value); }
void swap_endian(uint32_t & value) { swap_endian_bytewise(value); }
void swap_endian(float & value) { swap_endian_bytewise(value); }
template<class T, int N> void swap_endian(T (& value)[N]) { for(auto & elem : value) swap_endian(elem); }
void swap_endian(CalibrationMetadata & value)
{
    swap_endian(value.versionNumber);
    swap_endian(value.numIntrinsicsRight);
    swap_endian(value.numIntrinsicsThird);
    swap_endian(value.numIntrinsicsPlatform);
    swap_endian(value.numRectifiedModesLR);
    swap_endian(value.numRectifiedModesThird);
    swap_endian(value.numRectifiedModesPlatform);
}
void swap_endian(UnrectifiedIntrinsics & value)
{
    swap_endian(value.fx);
    swap_endian(value.fy);
    swap_endian(value.px);
    swap_endian(value.py);
    swap_endian(value.k);
    swap_endian(value.w);
    swap_endian(value.h);
}
void swap_endian(RectifiedIntrinsics & value)
{
    swap_endian(value.rfx);
    swap_endian(value.rfy);
    swap_endian(value.rpx);
    swap_endian(value.rpy);
    swap_endian(value.rw);
    swap_endian(value.rh);
}
template<class T, unsigned long N> void swap_endian(std::array<T,N> & value) { for(auto & elem : value) swap_endian(elem); }
template<class T> void swap_endian(std::vector<T> & value) { for(auto & elem : value) swap_endian(elem); }
void swap_endian(CameraCalibrationParameters & value)
{
    swap_endian(value.metadata);
    swap_endian(value.intrinsicsLeft);
    swap_endian(value.intrinsicsRight);
    swap_endian(value.intrinsicsThird);
    swap_endian(value.intrinsicsPlatform);
    swap_endian(value.modesLR);
    swap_endian(value.modesThird);
    swap_endian(value.modesPlatform);
    swap_endian(value.Rleft);
    swap_endian(value.Rright);
    swap_endian(value.Rthird);
    swap_endian(value.Rplatform);
    swap_endian(value.B);
    swap_endian(value.T);
    swap_endian(value.Tplatform);
    swap_endian(value.Rworld);
    swap_endian(value.Tworld);
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
    read_bytes(p, cal.Rworld);
    read_bytes(p, cal.Tworld);
    
    swap_endian(cal);

    return cal;
}

} // end namespace r200

#endif
