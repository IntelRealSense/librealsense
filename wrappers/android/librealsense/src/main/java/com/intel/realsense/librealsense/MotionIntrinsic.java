package com.intel.realsense.librealsense;

public class MotionIntrinsic {

    /* mData matrix description:
     * Scale X       cross axis  cross axis  Bias X \n
     * cross axis    Scale Y     cross axis  Bias Y \n
     * cross axis    cross axis  Scale Z     Bias Z */
    private float[][] mData;
    private float[] mNoiseVariances; // Variance of noise for X, Y, and Z axis
    private float[] mBiasVariances;  // Variance of bias for X, Y, and Z axis


    public MotionIntrinsic(){
        mData = new float[3][4];
        mNoiseVariances = new float[3];
        mBiasVariances = new float[3];
    }

    public MotionIntrinsic(float[][] data, float[] noiseVariances, float[] biasVariances){
        this.mData = data;
        this.mNoiseVariances = noiseVariances;
        this.mBiasVariances = biasVariances;
    }
}
