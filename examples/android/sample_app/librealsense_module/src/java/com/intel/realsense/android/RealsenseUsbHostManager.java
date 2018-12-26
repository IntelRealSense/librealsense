package com.intel.realsense.android;

import android.app.Activity;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbManager;
import android.util.Log;

import java.util.HashMap;
import java.util.Iterator;

public class RealsenseUsbHostManager {


    private static final String ACTION_USB_PERMISSION = "com.intel.realsense.action.USB_PERMISSION";
    private static final String TAG = RealsenseUsbHostManager.class.getSimpleName();
    private final Activity mContext;


    private UsbManager mUsbManager;
    private PendingIntent mPermissionIntent;
    private boolean mPermissionRequestPending;
    UsbDevice device = null;

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("usbhost");
    }

    RealsenseUsbHostManager(Activity context){
        mContext=context;
        setup();
    }

    public boolean findDevice(){
        if (device == null) {
            HashMap<String, UsbDevice> deviceList = mUsbManager.getDeviceList();
            Iterator<UsbDevice> deviceIterator = deviceList.values().iterator();

            while (deviceIterator.hasNext()) {
                UsbDevice curr = deviceIterator.next();
                if (curr.getVendorId() == 0x8086) {
                    device = curr;
                    break;
                }
            }
            if (device != null) {
                if (mUsbManager.hasPermission(device)) {
                    openDevice(device);
                } else {
                    synchronized (mUsbReceiver) {
                        if (!mPermissionRequestPending) {
                            mUsbManager.requestPermission(device,
                                    mPermissionIntent);
                            mPermissionRequestPending = true;
                        }
                    }
                }
            } else {
                Log.d(TAG, "device is null");
            }
        }
        return device!=null;
    }

    private final BroadcastReceiver mUsbReceiver = new BroadcastReceiver() {
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (ACTION_USB_PERMISSION.equals(action)) {
                synchronized (this) {
                    UsbDevice device = (UsbDevice) intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);

                    if (intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)) {
                        Log.d(TAG, "permission granted for device " + device.getDeviceName());
                        openDevice(device);
                    } else {
                        Log.d(TAG, "permission denied for device " + device);
                    }
                    mPermissionRequestPending = false;
                }
            } else if (UsbManager.ACTION_USB_ACCESSORY_DETACHED.equals(action)) {
                UsbDevice device = (UsbDevice) intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
                if (device != null) {
                }
            }
        }
    };


    private void setup() {
        mUsbManager = (UsbManager) mContext.getSystemService(Context.USB_SERVICE);
        mPermissionIntent = PendingIntent.getBroadcast(mContext, 0, new Intent(ACTION_USB_PERMISSION), 0);
        IntentFilter filter = new IntentFilter(ACTION_USB_PERMISSION);
        filter.addAction(UsbManager.ACTION_USB_ACCESSORY_DETACHED);
        mContext.registerReceiver(mUsbReceiver, filter);

    }

    private void openDevice(UsbDevice device) {
        if (device != null) {
            UsbDeviceConnection conn = mUsbManager.openDevice(device);
            for (int i = 0; i < device.getInterfaceCount(); i++) {
                boolean claimed = conn.claimInterface(device.getInterface(i), true);
                Log.d(TAG, "Device Claimed " + i + " success: " + claimed);
            }
             nativeAddUsbDevice(device.getDeviceName(), conn.getFileDescriptor());
        }
    }

    private native void nativeAddUsbDevice(String deviceName, int fileDescriptor);




}
