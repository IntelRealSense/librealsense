package com.intel.realsense.librealsense;

import android.content.Context;

public class DeviceManager {
    private static UsbHub mUsbHub;

    public static void init(Context context){
        if(mUsbHub == null)
            mUsbHub = new UsbHub(context);
    }

    public static UsbHub getUsbHub() {
        if(mUsbHub == null)
            throw new RuntimeException("DeviceManager must be initialized first, call DeviceManager.init(context)");
        return mUsbHub;
    }
}
