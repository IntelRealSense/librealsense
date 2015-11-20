/*
    INTEL CORPORATION PROPRIETARY INFORMATION This software is supplied under the
    terms of a license agreement or nondisclosure agreement with Intel Corporation
    and may not be copied or disclosed except in accordance with the terms of that
    agreement.
    Copyright(c) 2015 Intel Corporation. All Rights Reserved.
*/

package com.intel.rs;

public class Extrinsics
{
    public float[] rotation; // column-major 3x3 rotation matrix
    public float[] translation; // 3 element translation vector, in meters
}
