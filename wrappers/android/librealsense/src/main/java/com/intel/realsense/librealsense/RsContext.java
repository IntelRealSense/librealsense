package com.intel.realsense.librealsense;

import android.content.Context;

public class RsContext extends LrsClass{
    private static DeviceWatcher mDeviceWatcher;
    private DeviceListener mListener;

    public static void init(Context context){
        if(mDeviceWatcher == null)
            mDeviceWatcher = new DeviceWatcher(context);
    }

    public static String getVersion(){
        return nGetVersion();
    }

    /**
     * @deprecated use {@link #queryDevices()} instead.
     */
    @Deprecated
    public int getDeviceCount() {
        return mDeviceWatcher.getDeviceCount();
    }

    public DeviceList queryDevices() {
        return queryDevices(ProductClass.ANY_INTEL);
    }

    public DeviceList queryDevices(ProductClass productClass) {
        return new DeviceList(nQueryDevices(mHandle, productClass.value()));
    }

    public synchronized void setDevicesChangedCallback(DeviceListener listener) {
        removeDevicesChangedCallback();
        mListener = listener;
        mDeviceWatcher.addListener(mListener);
    }

    public synchronized void removeDevicesChangedCallback() {
        if(mListener != null)
            mDeviceWatcher.removeListener(mListener);
    }

    public RsContext() {
        mHandle = nCreate();
    }

    @Override
    public void close() {
        removeDevicesChangedCallback();
        nDelete(mHandle);
    }

    private static native long nCreate();
    private static native String nGetVersion();
    private static native long nQueryDevices(long handle, int mask);
    private static native void nDelete(long handle);
}
