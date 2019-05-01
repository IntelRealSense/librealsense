package com.intel.realsense.librealsense;

public class Points extends Frame {
    protected Points(long handle) {
        super(handle);
    }

    public int getCount(){
        return nGetCount(mHandle);
    }
    public void exportToPly(String filePath, VideoFrame texture) {
        nExportToPly(mHandle, filePath, texture.getHandle());
    }

    private static native int nGetCount(long handle);
    private static native void nExportToPly(long handle, String filePath, long textureHandle);
}
