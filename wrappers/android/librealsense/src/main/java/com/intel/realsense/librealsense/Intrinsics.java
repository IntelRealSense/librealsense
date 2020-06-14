package com.intel.realsense.librealsense;

public class Intrinsics {
    public int width;
    public int height;
    public float ppx;
    public float ppy;
    public float fx;
    public float fy;
    public int model;
    //public DistortionType model;
    public float[] coeffs;


    public Intrinsics(){
        coeffs = new float[5];
    }

    public Intrinsics(int width,int height,
                      float ppx, float ppy,
                      float fx, float fy,
                      int model,
                      //DistortionType model,
                      float[] coeffs){
        this.width = width;
        this.height = height;
        this.ppx = ppx;
        this.ppy = ppy;
        this.fx = fx;
        this.fy = fy;
        this.model = model;
        this.coeffs = coeffs;
    }
}
