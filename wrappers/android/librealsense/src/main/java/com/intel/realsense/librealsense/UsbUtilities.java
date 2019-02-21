package com.intel.realsense.librealsense;

import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.util.Log;

import java.util.HashMap;
import java.util.Map;

public class UsbUtilities {
    private static final String TAG = "librs UsbUtilities";
    public static final String ACTION_USB_PERMISSION = "USB_CONTROL_PERMISSION";

    public static boolean isIntel(UsbDevice usbDevice){
        if (usbDevice.getVendorId() == 0x8086)
            return true;
        return false;
    }

    public static UsbDevice getUsbDevice(Context context, Integer vId) {
        return getUsbDevice(context, vId, 0);
    }

    public static UsbDevice getUsbDevice(Context context, Integer vId, Integer pId) {
        UsbManager usbManager = (UsbManager) context.getSystemService(Context.USB_SERVICE);

        HashMap<String, UsbDevice> devicesMap = usbManager.getDeviceList();
        for (Map.Entry<String, UsbDevice> entry : devicesMap.entrySet()) {
            UsbDevice usbDevice = entry.getValue();
            if (usbDevice.getVendorId() == vId && (usbDevice.getProductId() == pId || pId == 0)) {
                return usbDevice;
            }
        }
        Log.w(TAG, "getUsbDevice: failed to locate USB device, " + "VID: " + String.format("0x%04x", vId) + ", PID: " + String.format("0x%04x", pId));
        return null;
    }

    public static boolean hasUsbPermission(Context context, UsbDevice usbDevice){
        Log.d(TAG, "hasUsbPermission");

        if(usbDevice == null){
            Log.w(TAG, "hasUsbPermission: null USB device");
            return false;
        }

        UsbManager usbManager = (UsbManager) context.getSystemService(Context.USB_SERVICE);
        return usbManager.hasPermission(usbDevice);
    }

    public static void grantUsbPermissions(Context context, UsbDevice usbDevice){
        Log.d(TAG, "grantUsbPermissions");

        if(usbDevice == null){
            Log.w(TAG, "grantUsbPermissions: null USB device");
            return;
        }

        UsbManager usbManager = (UsbManager) context.getSystemService(Context.USB_SERVICE);
        boolean permission = usbManager.hasPermission(usbDevice);

        if(!permission) {
            Log.i(TAG, "grantUsbPermissions:\ndevice: " + usbDevice.toString());
            PendingIntent pi = PendingIntent.getBroadcast(context, 0, new Intent(UsbUtilities.ACTION_USB_PERMISSION), 0);
            usbManager.requestPermission(usbDevice, pi);
        }
    }

    public static UsbDevice getDevice(Context context) {
        return getUsbDevice(context, 0x8086);
    }

    public static boolean hasUsbPermission(Context context){
        UsbDevice usbDevice = getDevice(context);
        return hasUsbPermission(context, usbDevice);
    }

    public static void grantUsbPermission(Context context){
        UsbDevice usbDevice = getDevice(context);
        grantUsbPermissions(context, usbDevice);
    }
}
