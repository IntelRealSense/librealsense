package com.intel.rs;

public class Context
{
    static { System.loadLibrary("realsense"); }
    private long handle;
    public Context() { handle = create(); }
    private native long create();
    public native void close();

    public native int getDeviceCount();
    public native Device getDevice(int index);
}
