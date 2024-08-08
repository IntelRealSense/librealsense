package com.intel.realsense.librealsense;

import android.util.Log;

public class DeviceList extends LrsClass {
    private static final String TAG = "librs api";

    public DeviceList(long handle){
        mHandle = handle;
    }

    public int getDeviceCount(){
        return nGetDeviceCount(mHandle);
    }

    public Device createDevice(int index){
        try {
            long deviceHandle = nCreateDevice(mHandle, index);
            return new Device(deviceHandle);
        }catch (Exception e){
            Log.e(TAG, e.getMessage());
            return null;
        }
    }

    public boolean containsDevice(Device device){
        return nContainsDevice(mHandle, device.getHandle());
    }

    public void foreach(DeviceCallback callback) {
        int size = getDeviceCount();
        for(int i = 0; i < size; i++) {
            try(Device d = createDevice(i)){
                callback.onDevice(d);
            }
        }
    }

    @Override
    public void close() {
        nRelease(mHandle);
    }

    private static native int nGetDeviceCount(long handle);
    private static native long nCreateDevice(long handle, int index);
    private static native boolean nContainsDevice(long handle, long deviceHandle);
    private static native void nRelease(long handle);
}
