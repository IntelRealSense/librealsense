package com.intel.realsense.librealsense;

import android.content.Context;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbManager;
import android.util.Log;

import java.io.Closeable;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

public class UsbHostManager extends LrsClass {
    private final List<Listener> mAppDeviceListner;

    public static void init(Context context){
        if(mInstance == null)
            mInstance = new UsbHostManager(context);
    }

    public static void addListener(Listener deviceListener){
        if(mInstance == null)
            throw new RuntimeException("UsbHostManager must be initialized before adding a listener");
        if(mInstance.mAppDeviceListner.contains(deviceListener) || deviceListener == null)
            return;
        mInstance.mAppDeviceListner.add(deviceListener);
    }

    public static void removeListener(Listener deviceListener){
        if(mInstance == null)
            throw new RuntimeException("UsbHostManager must be initialized before removing a listener");
        if(!mInstance.mAppDeviceListner.contains(deviceListener) || deviceListener == null)
            return;
        mInstance.mAppDeviceListner.remove(deviceListener);
    }

    private static UsbHostManager mInstance;

    private static final String TAG = UsbHostManager.class.getSimpleName();
    private final Context mContext;
    private final Enumerator mEnumerator;

    private HashMap<String, UsbDesc> mDescriptors = new LinkedHashMap<>();

    public interface Listener {
        void onUsbDeviceAttach();
        void onUsbDeviceDetach();
    }

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

    private Enumerator.Listener mListener = new Enumerator.Listener() {
        @Override
        public void onUsbDeviceAttach() {
            invalidateDevices();
        }

        @Override
        public void onUsbDeviceDetach() {
            invalidateDevices();
        }
    };

    private UsbHostManager(Context context){
        mContext = context;
        mEnumerator = new Enumerator(mContext, mListener);
        mAppDeviceListner = new ArrayList<>();
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
        for(Listener listener : mAppDeviceListner)
            listener.onUsbDeviceDetach();
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
        for (Listener listener : mAppDeviceListner)
            listener.onUsbDeviceAttach();
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

    public native void nAddUsbDevice(String deviceName, int fileDescriptor);
    public native void nRemoveUsbDevice(int fileDescriptor);
}
