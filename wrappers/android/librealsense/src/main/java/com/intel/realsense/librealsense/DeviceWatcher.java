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

class DeviceWatcher extends LrsClass {
    private static final String TAG = "librs DeviceWatcher";

    private final List<DeviceListener> mAppDeviceListener;

    public synchronized void addListener(DeviceListener deviceListener){
        if(!mAppDeviceListener.contains(deviceListener)) {
            mAppDeviceListener.add(deviceListener);
        }
    }

    public synchronized void removeListener(DeviceListener deviceListener){
        mAppDeviceListener.remove(deviceListener);
    }

    public int getDeviceCount() {
        return mDescriptors.size();
    }

    private final Context mContext;
    private final Enumerator mEnumerator;

    private HashMap<String, UsbDesc> mDescriptors = new LinkedHashMap<>();


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

    DeviceWatcher(Context context){
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

    private enum UpdateListenersType {
        ATTACH,
        DETACH
    }

    private void updateListeners(UpdateListenersType type){
        for(DeviceListener listener : mAppDeviceListener) {
            try {
                if (type == UpdateListenersType.ATTACH)
                    listener.onDeviceAttach();
                else
                    listener.onDeviceDetach();
            } catch (Exception e) {
                Log.e(TAG, e.getMessage());
            }
        }
    }

    private void removeDevice(UsbDesc desc) {
        Log.d(TAG, "Removing device: " + desc.name);

        nRemoveUsbDevice(desc.descriptor);
        desc.connection.close();
        updateListeners(UpdateListenersType.DETACH);
        Log.d(TAG, "Device: " + desc.name + " removed successfully");
    }

    private void addDevice(UsbDevice device) {
        if (device == null)
            return;

        UsbManager usbManager = (UsbManager) mContext.getSystemService(Context.USB_SERVICE);
        UsbDeviceConnection conn = usbManager.openDevice(device);
        if(conn == null)
            return;
        UsbDesc desc = new UsbDesc(device.getDeviceName(), conn.getFileDescriptor(), conn);
        Log.d(TAG, "Adding device: " + desc.name);
        mDescriptors.put(device.getDeviceName(), desc);
        nAddUsbDevice(desc.name, desc.descriptor);

        updateListeners(UpdateListenersType.ATTACH);
        Log.d(TAG, "Device: " + desc.name + " added successfully");
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
