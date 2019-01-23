package com.intel.realsense.librealsense;

import android.content.Context;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbManager;
import android.util.Log;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

public class UsbHub extends LrsClass {
    private final List<DeviceListener> mAppDeviceListener;

    public void addListener(DeviceListener deviceListener){
        mAppDeviceListener.add(deviceListener);
    }

    public void removeListener(DeviceListener deviceListener){
        mAppDeviceListener.remove(deviceListener);
    }

    private static final String TAG = UsbHub.class.getSimpleName();
    private final Context mContext;
    private final Enumerator mEnumerator;

    private HashMap<String, UsbDesc> mDescriptors = new LinkedHashMap<>();

    private class UsbDesc {
        public UsbDesc(UsbDeviceConnection c, String n, int d ) {
            connection = c;
            name = n;
            descriptor = d;
        }
        UsbDeviceConnection connection;
        String name;
        int descriptor;
    }

    private DeviceListener mListener = new DeviceListener() {
        @Override
        public void onDeviceAttach() {
            invalidateDevices();
        }

        @Override
        public void onDeviceDetach() {
            invalidateDevices();
        }
    };

    UsbHub(Context context){
        mContext = context;
        mEnumerator = new Enumerator(mContext, mListener);
        mAppDeviceListener = new ArrayList<>();
    }

    private synchronized void invalidateDevices() {
        UsbManager usbManager = (UsbManager) mContext.getSystemService(Context.USB_SERVICE);
        HashMap<String, UsbDevice> devicesMap = usbManager.getDeviceList();
        List<String> intelDevices = new ArrayList<String>();
        for (Map.Entry<String, UsbDevice> entry : devicesMap.entrySet()) {
            UsbDevice usbDevice = entry.getValue();
            if (UsbUtilities.isIntel(usbDevice))
                intelDevices.add(entry.getKey());
        }
        Iterator<Map.Entry<String, UsbDesc>> iter = mDescriptors.entrySet().iterator();
        while (iter.hasNext()) {
            Map.Entry<String, UsbDesc> entry = iter.next();
            if(!intelDevices.contains(entry.getKey())) {
                removeDevice(entry.getValue());
                iter.remove();
            }
        }
        for(String name : intelDevices) {
            if(!mDescriptors.containsKey(name)) {
                addDevice(devicesMap.get(name));
            }
        }
    }

    private void removeDevice(UsbDesc desc) {
        nRemoveUsbDevice(desc.descriptor);
        desc.connection.close();
        for(DeviceListener listener : mAppDeviceListener)
            listener.onDeviceDetach();
    }

    private  void addDevice(UsbDevice device) {
        if (device == null)
            return;

        UsbManager usbManager = (UsbManager) mContext.getSystemService(Context.USB_SERVICE);
        UsbDeviceConnection conn = usbManager.openDevice(device);
        for (int i = 0; i < device.getInterfaceCount(); i++) {
            boolean claimed = conn.claimInterface(device.getInterface(i), true);
            Log.d(TAG, "Device Claimed " + i + " success: " + claimed);
        }
        UsbDesc desc = new UsbDesc(conn, device.getDeviceName(), conn.getFileDescriptor());
        mDescriptors.put(device.getDeviceName(), desc);
        nAddUsbDevice(desc.name, desc.descriptor);
        for (DeviceListener listener : mAppDeviceListener)
            listener.onDeviceAttach();
    }

    @Override
    public void close() {
        Iterator<Map.Entry<String, UsbDesc>> iter = mDescriptors.entrySet().iterator();
        while (iter.hasNext()) {
            Map.Entry<String, UsbDesc> entry = iter.next();
            removeDevice(entry.getValue());
            iter.remove();
        }
    }

    private static native void nAddUsbDevice(String deviceName, int fileDescriptor);
    private static native void nRemoveUsbDevice(int fileDescriptor);
}
