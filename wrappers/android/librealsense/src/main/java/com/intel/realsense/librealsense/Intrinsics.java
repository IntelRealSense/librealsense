package com.intel.realsense.librealsense;

public class Intrinsics {
    public int mWidth;
    public int mHeight;
    public float mPpx;
    public float mPpy;
    public float mFx;
    public float mFy;
    public DistortionType mModel;
    public int mModelValue;
    public float[] mCoeffs;


    public Intrinsics(){
        //mModelValue = new DistortionType(0);
        mCoeffs = new float[5];
    }

    public Intrinsics(int width,int height,
                      float ppx, float ppy,
                      float fx, float fy,
                      int model,
                      float[] coeffs){
        this.mWidth = width;
        this.mHeight = height;
        this.mPpx = ppx;
        this.mPpy = ppy;
        this.mFx = fx;
        this.mFy = fy;
        this.mModel = DistortionType.values()[model];
        this.mModelValue = model;
        this.mCoeffs = coeffs;
    }

    public void SetModel(){
        this.mModel = DistortionType.values()[mModelValue];
    }
}
