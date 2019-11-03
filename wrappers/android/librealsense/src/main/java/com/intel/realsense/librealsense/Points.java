package com.intel.realsense.librealsense;

public class Points extends Frame {
    private float[] mData;
    private float[] mTextureCoordinates;

    protected Points(long handle) {
        super(handle);
        mOwner = false;
    }

    public float[] getVertices(){
        if(mData == null){
            mData = new float[getCount() * 3];
            getData(mData);
        }
        return mData;
    }

    public float[] getTextureCoordinates(){
        if(mTextureCoordinates == null){
            mTextureCoordinates = new float[getCount() * 2];
            getTextureCoordinates(mTextureCoordinates);
        }
        return mTextureCoordinates;
    }

    public void getTextureCoordinates(float[] data) {
        nGetTextureCoordinates(mHandle, data);
    }

    public void getData(float[] data) {
        nGetData(mHandle, data);
    }

    public int getCount(){
        return nGetCount(mHandle);
    }

    public void exportToPly(String filePath, VideoFrame texture) {
        nExportToPly(mHandle, filePath, texture.getHandle());
    }

    private static native int nGetCount(long handle);
    private static native void nGetData(long handle, float[] data);
    private static native void nGetTextureCoordinates(long handle, float[] data);
    private static native void nExportToPly(long handle, String filePath, long textureHandle);
}
