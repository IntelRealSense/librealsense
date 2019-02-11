package com.intel.realsense.librealsense;

import android.content.Context;

public class RsContext extends LrsClass{
    private static DeviceWatcher mDeviceWatcher;
    private DeviceListener mListener;

    public static void init(Context context){
        if(mDeviceWatcher == null)
            mDeviceWatcher = new DeviceWatcher(context);
    }

    public int getDeviceCount() {
        return mDeviceWatcher.getDeviceCount();
    }

    public synchronized void setDevicesChangedCallback(DeviceListener listener) {
        if(mListener != null)
            mDeviceWatcher.removeListener(mListener);
        mListener = listener;
        mDeviceWatcher.addListener(mListener);
    }

    public RsContext() {
        mHandle = nCreate();
    }

    @Override
    public void close() {
        if(mListener != null)
            mDeviceWatcher.removeListener(mListener);
        nDelete(mHandle);
    }

    private static native long nCreate();
    private static native void nDelete(long handle);
}
