/*
    INTEL CORPORATION PROPRIETARY INFORMATION This software is supplied under the
    terms of a license agreement or nondisclosure agreement with Intel Corporation
    and may not be copied or disclosed except in accordance with the terms of that
    agreement.
    Copyright(c) 2015 Intel Corporation. All Rights Reserved.
*/

package com.intel.rs;

public enum Stream
{
    DEPTH(0), // Native stream of depth data produced by RealSense device
    COLOR(1), // Native stream of color data captured by RealSense device
    INFRARED(2), // Native stream of infrared data captured by RealSense device
    INFRARED2(3), // Native stream of infrared data captured from a second viewpoint by RealSense device
    RECTIFIED_COLOR(4), // Synthetic stream containing undistorted color data with no extrinsic rotation from the depth stream
    COLOR_ALIGNED_TO_DEPTH(5), // Synthetic stream containing color data but sharing intrinsics of depth stream
    DEPTH_ALIGNED_TO_COLOR(6), // Synthetic stream containing depth data but sharing intrinsics of color stream
    DEPTH_ALIGNED_TO_RECTIFIED_COLOR(7); // Synthetic stream containing depth data but sharing intrinsics of rectified color stream
    public final int code;
    private Stream(int code) { this.code = code; }
}
