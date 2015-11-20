/*
    INTEL CORPORATION PROPRIETARY INFORMATION This software is supplied under the
    terms of a license agreement or nondisclosure agreement with Intel Corporation
    and may not be copied or disclosed except in accordance with the terms of that
    agreement.
    Copyright(c) 2015 Intel Corporation. All Rights Reserved.
*/

package com.intel.rs;

public enum Distortion
{
    NONE(0), // Rectilinear images, no distortion compensation required
    MODIFIED_BROWN_CONRADY(1), // Equivalent to Brown-Conrady distortion, except that tangential distortion is applied to radially distorted points
    INVERSE_BROWN_CONRADY(2); // Equivalent to Brown-Conrady distortion, except undistorts image instead of distorting it
    public final int code;
    private Distortion(int code) { this.code = code; }
}
