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
