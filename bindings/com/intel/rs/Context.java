package com.intel.rs;

public class Context
{
    static { System.loadLibrary("realsense"); }
    private long handle;
    public Context() { handle = create(); }
    private native long create();
    public native void close();


    /**
     * determine number of connected devices
     * @return  the count of devices
     */
    public native int getDeviceCount();

    /**
     * retrieve connected device by index
     * @param index  the zero based index of device to retrieve
     * @return       the requested device
     */
    public native Device getDevice(int index);
}
