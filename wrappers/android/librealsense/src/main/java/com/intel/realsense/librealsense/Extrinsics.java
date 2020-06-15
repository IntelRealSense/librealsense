package com.intel.realsense.librealsense;

public class Extrinsics {

    private float[] mRotation; // Column-major 3x3 rotation matrix
    private float[] mTranslation;  // Three-element translation vector, in meters

    public Extrinsics(){
        mRotation = new float[9];
        mTranslation = new float[3];
    }

    public Extrinsics(float[] rotation, float[] translation){
        this.mRotation = rotation;
        this.mTranslation = translation;
    }
}
