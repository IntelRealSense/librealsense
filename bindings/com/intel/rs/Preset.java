/*
    INTEL CORPORATION PROPRIETARY INFORMATION This software is supplied under the
    terms of a license agreement or nondisclosure agreement with Intel Corporation
    and may not be copied or disclosed except in accordance with the terms of that
    agreement.
    Copyright(c) 2015 Intel Corporation. All Rights Reserved.
*/

package com.intel.rs;

public enum Preset
{
    BEST_QUALITY(0),
    LARGEST_IMAGE(1),
    HIGHEST_FRAMERATE(2);
    public final int code;
    private Preset(int code) { this.code = code; }
}
