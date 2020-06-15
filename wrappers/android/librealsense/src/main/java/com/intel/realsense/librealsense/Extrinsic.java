package com.intel.realsense.librealsense;

public class Extrinsic {

    private float[] mRotation; // Column-major 3x3 rotation matrix
    private float[] mTranslation;  // Three-element translation vector, in meters

    public Extrinsic(){
        mRotation = new float[9];
        mTranslation = new float[3];
    }

    public Extrinsic(float[] rotation, float[] translation){
        this.mRotation = rotation;
        this.mTranslation = translation;
    }
}
