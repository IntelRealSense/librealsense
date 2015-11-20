/*
    INTEL CORPORATION PROPRIETARY INFORMATION This software is supplied under the
    terms of a license agreement or nondisclosure agreement with Intel Corporation
    and may not be copied or disclosed except in accordance with the terms of that
    agreement.
    Copyright(c) 2015 Intel Corporation. All Rights Reserved.
*/

package com.intel.rs;

public class Intrinsics
{
    public int width; // width of the image in pixels
    public int height; // height of the image in pixels
    public float PPX; // horizontal coordinate of the principal point of the image, as a pixel offset from the left edge
    public float PPY; // vertical coordinate of the principal point of the image, as a pixel offset from the top edge
    public float FX; // focal length of the image plane, as a multiple of pixel width
    public float FY; // focal length of the image plane, as a multiple of pixel height
    public Distortion model; // distortion model of the image
    public float[] coeffs; // distortion coefficients
}
