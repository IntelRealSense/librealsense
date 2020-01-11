package com.intel.realsense.librealsense;

import android.hardware.usb.UsbDeviceConnection;

class UsbDesc {
    public UsbDesc(String n, int d, UsbDeviceConnection c) {
        name = n;
        descriptor = d;
        connection = c;
    }
    String name;
    int descriptor;
    UsbDeviceConnection connection;
}
