package com.intel.rs;

public enum Distortion
{
    NONE(0), // Rectilinear images, no distortion compensation required
    MODIFIED_BROWN_CONRADY(1), // Equivalent to Brown-Conrady distortion, except that tangential distortion is applied to radially distorted points
    INVERSE_BROWN_CONRADY(2); // Equivalent to Brown-Conrady distortion, except undistorts image instead of distorting it
    public final int code;
    private Distortion(int code) { this.code = code; }
}
