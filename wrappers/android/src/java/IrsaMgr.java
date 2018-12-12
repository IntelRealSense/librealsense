// License: Apache 2.0. See LICENSE file in root directory.
//
package android.irsa;

import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.view.Surface;
import android.view.SurfaceView;
import java.util.Map;
import java.util.HashMap;
import java.util.Iterator;
import java.lang.ref.WeakReference;
import java.lang.reflect.Method;

import java.io.IOException;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.InputStream;
import java.io.FileInputStream;
import java.io.*;

import android.content.IntentFilter;
import android.content.Intent;
import android.content.BroadcastReceiver;
import android.app.PendingIntent;
import android.app.Activity;
import android.content.Context;
import android.hardware.usb.UsbConstants;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbInterface;
import android.hardware.usb.UsbManager;
import android.hardware.usb.*;

final public class IrsaMgr
{
    private final static String TAG = "librs_IrsaMgr";

    private final static int MAX_VERSION_SIZE = 256;
    private static byte[] mVersion = new byte[MAX_VERSION_SIZE];

    private EventHandler mEventHandler;
    private IrsaEventListener mEventListener;
    private EventCallback mCallback = new EventCallback();

    private int mNativeContext;

    private static Activity mMe;
    private static UsbManager mUsbManager;
    private static UsbDevice  mUsbDevice;
    private static UsbDeviceConnection mUsbDeviceConnection;
    private static final String ACTION_DEVICE_PERMISSION = "com.irsa.USB_PERMISSION";
    private static PendingIntent mPermissionIntent;
    private static int mUVCFD = -1;

    private static UsbEndpoint mUsbEndpointIn;
    private static UsbEndpoint mUsbEndpointOut;
    private static UsbInterface mUsbInterfaceInt;
    private static UsbInterface musbInterfaceOut;
    protected static final int STD_USB_REQUEST_GET_DESCRIPTOR = 0x06;
    // http://libusb.sourceforge.net/api-1.0/group__desc.html
    protected static final int LIBUSB_DT_STRING = 0x03;


    private IrsaMgr() throws IrsaException
    {
        throw new IrsaException("private constructor");
    }


    @Override
    protected void finalize()
    {
        IrsaLog.w(TAG, "[Java] finalize");
        native_release();
    }


    public void release()
    {
        IrsaLog.w(TAG, "[Java] release");
        native_release();
    }


    private boolean versionCheck()
    {
        String javaVersion = IrsaVersion.IRSA_VERSION;
        String nativeVersion = getVersion();
        IrsaLog.w(TAG, "[Java] irsajar    Version " + javaVersion);
        IrsaLog.w(TAG, "[Java] libirsajni Version " + nativeVersion);

        if (!nativeVersion.startsWith(javaVersion))
        {
            IrsaLog.e(TAG, "irsa.jar's version does not match libirsajni.so's version -- native version: " + nativeVersion);
            return false;
        }
        return true;
    }


    public IrsaMgr(IrsaEventListener eventListener) throws IrsaException
    {
        if (!IrsaLoader.hasLoaded())
        {
            throw new IrsaException("irsajni library not loaded");
        }

        if (!versionCheck())
        {
            String javaVersion = IrsaVersion.IRSA_VERSION;
            String nativeVersion = getVersion();
            String errorMsg = "irsa.jar's version " + javaVersion + " does not match libirsajni.so's version -- native version: " + nativeVersion;
            IrsaLog.e(TAG, errorMsg);
            throw new IrsaException(errorMsg);
        }

        mEventListener = eventListener;

        Looper looper;
        if ((looper = Looper.myLooper()) != null)
        {
            mEventHandler = new EventHandler(this, looper);
        }
        else if ((looper = Looper.getMainLooper()) != null)
        {
            mEventHandler = new EventHandler(this, looper);
        }
        else
        {
            mEventHandler = null;
        }

        native_setup(new WeakReference<IrsaMgr>(this));
    }


    private class EventCallback implements IrsaEventInterface.IrsaEventCallback
    {
        public void onEventInfo(IrsaEventType event, int what, int arg1, int arg2, Object obj)
        {
            if (mEventListener != null)
            {
                mEventListener.onEvent(event, what, arg1, arg2, obj);
            }
        }
    }


    private class EventHandler extends Handler
    {
        private IrsaMgr mIrsaMgr;

        public EventHandler(IrsaMgr mgr, Looper looper)
        {
            super(looper);
            mIrsaMgr = mgr;
        }

        @Override
        public void handleMessage(Message msg)
        {
            try
            {
                if (0 == mIrsaMgr.mNativeContext)
                {
                    IrsaLog.w(TAG, "[Java] pls check JNI initialization");
                    return;
                }

                switch (msg.what)
                {
                case IrsaEvent.IRSA_INFO:
                    mCallback.onEventInfo(IrsaEventType.IRSA_EVENT_INFO, msg.what, msg.arg1, msg.arg2, msg.obj);
                    break;

                case IrsaEvent.IRSA_ERROR:
                    mCallback.onEventInfo(IrsaEventType.IRSA_EVENT_ERROR, msg.what, msg.arg1, msg.arg2, msg.obj);
                    break;

                default:
                    IrsaLog.e(TAG, "Unknown message type " + msg.what);
                    break;
                }
            }
            catch (Exception e)
            {
                IrsaLog.e(TAG, "exception occured in event handler");
            }
        }
    }


    public static String getVersion()
    {
        int versionLen = getVersionDescription(mVersion, MAX_VERSION_SIZE);
        if (versionLen <= 0)
        {
            return "";
        }

        return new String(mVersion, 0, versionLen);
    }


    private static void postEventFromNative(Object irsamgr_ref,
                                            int what, int arg1, int arg2, Object obj)
    {
        IrsaMgr mgr = (IrsaMgr) ((WeakReference) irsamgr_ref).get();
        if (mgr.mEventHandler != null)
        {
            Message msg = mgr.mEventHandler.obtainMessage(what, arg1, arg2, obj);
            mgr.mEventHandler.sendMessage(msg);
        }
    }


    //=========== begin USB experiment ========================== {
    public void initUVCDevice(Activity activity)
    {
        mMe = activity;

        IntentFilter usbFilter = new IntentFilter();
        //usbFilter.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED);
        //usbFilter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);
        usbFilter.addAction(ACTION_DEVICE_PERMISSION);
        mMe.registerReceiver(mUsbReceiver, usbFilter);

        mPermissionIntent = PendingIntent.getBroadcast(mMe, 0, new Intent(ACTION_DEVICE_PERMISSION), 0);
        mUsbManager = (UsbManager)mMe.getSystemService(Context.USB_SERVICE);

        _initUVCDevice();
    }


    public void unInitUVCDevice()
    {
        mMe.unregisterReceiver(mUsbReceiver);
    }


    private static int openUVCDevice()
    {
        IrsaLog.d(TAG, "open device");

        if ((mUsbDevice == null) || (mUsbManager == null))
        {
            IrsaLog.d(TAG, "can't open device");
            return -1;
        }

        _initUVCDevice();

        UsbDevice  device = mUsbDevice;
        IrsaLog.d(TAG, "dev name " + device.getDeviceName());
        mUsbDeviceConnection = mUsbManager.openDevice(mUsbDevice);
        if (mUsbDeviceConnection != null)
        {
            mUVCFD = mUsbDeviceConnection.getFileDescriptor();
            IrsaLog.d(TAG, "uvc fd: " + mUVCFD);
        }
        else
        {
            IrsaLog.d(TAG, "can't open uvc device");
            return -1;
        }

        int interfaceCount = device.getInterfaceCount();
        IrsaLog.d(TAG, "interfaceCount " + interfaceCount + "\n");
        for (int interfaceIndex = 0; interfaceIndex < interfaceCount; interfaceIndex++)
        {
            boolean claimed = mUsbDeviceConnection.claimInterface(device.getInterface(interfaceIndex), true);

            IrsaLog.d(TAG, "claimed " + interfaceIndex + " success: " + claimed);
        }
        return mUVCFD;
    }


    private static void closeUVCDevice()
    {
        IrsaLog.d(TAG, "close device");
        //if (mUsbManager == null)
        //    return;

        int interfaceCount = mUsbDevice.getInterfaceCount();
        for (int interfaceIndex = 0; interfaceIndex < interfaceCount; interfaceIndex++)
        {
            UsbInterface usbInterface = mUsbDevice.getInterface(interfaceIndex);
            mUsbDeviceConnection.claimInterface(usbInterface, false);
        }
        mUsbDeviceConnection.close();
        mUVCFD = -1;
    }


    private static String getUVCName()
    {
        if ((mUsbDevice == null) || (mUsbManager == null))
            return null;

        UsbDevice  device = mUsbDevice;
        IrsaLog.d(TAG, "dev name " + device.getDeviceName());

        return device.getDeviceName();
    }


    private static int getDescriptor(byte[] description)
    {
        if ((mUsbDevice == null) || (mUsbManager == null))
            return 0;

        UsbDevice  device = mUsbDevice;
        IrsaLog.d(TAG, "dev name " + device.getDeviceName());
        IrsaLog.d(TAG, "initTalkWithCamera\n");
        int interfaceCount = device.getInterfaceCount();
        IrsaLog.d(TAG, "interfaceCount " + interfaceCount + "\n");
        for (int interfaceIndex = 0; interfaceIndex < interfaceCount; interfaceIndex++)
        {
            mUsbEndpointIn = null;
            mUsbEndpointOut = null;

            UsbInterface usbInterface = device.getInterface(interfaceIndex);
            IrsaLog.d(TAG, "=====usbInterface " + interfaceIndex + ",id: " + usbInterface.getId() +  ",name " + usbInterface.getName() + " class: " +  usbInterface.getInterfaceClass() + ",subclass:" +  usbInterface.getInterfaceSubclass()  + ",protocol: "+ usbInterface.getInterfaceProtocol() + ",endPointCount: "+ usbInterface.getEndpointCount() + "===================\n");

            if (UsbConstants.USB_CLASS_VENDOR_SPEC == usbInterface.getInterfaceClass())
            {
                IrsaLog.d(TAG, "USB_CLASS_VENDOR_SPEC");
                continue;
            }
            else if (UsbConstants.USB_CLASS_VIDEO == usbInterface.getInterfaceClass())
            {
                IrsaLog.d(TAG, "found uvc device");
            }
            else
            {
                IrsaLog.d(TAG, "not uvc device");
                continue;
            }

            IrsaLog.d(TAG, "=========" + usbInterface.getEndpointCount() + "===================\n");
            for (int i = 0; i < usbInterface.getEndpointCount(); i++)
            {
                UsbEndpoint ep = usbInterface.getEndpoint(i);
                IrsaLog.d(TAG, "usbInterface.getEndpoint(" + i + "),type: " + ep.getType() + ",maxPacketSize: " + ep.getMaxPacketSize() + ",name:" + ep.toString() + ",attr:" + ep.getAttributes() + ",addr " + ep.getAddress() + ",desc:" + ep.describeContents() + ",endpointNumber:" + ep.getEndpointNumber() + ",interval:" + ep.getInterval() +  "\n");
                if (ep.getType() == UsbConstants.USB_ENDPOINT_XFER_BULK)
                {
                    IrsaLog.d(TAG, "usbInterface.getEndpoint(" + i + "),type: " + "USB_ENDPOINT_XFER_BULK" + "\n");
                }
                else if (ep.getType() == UsbConstants.USB_ENDPOINT_XFER_INT)
                {
                    IrsaLog.d(TAG, "usbInterface.getEndpoint(" + i + "),type: " + "USB_ENDPOINT_XFER_INT" + "\n");
                }
                else if (ep.getType() == UsbConstants.USB_ENDPOINT_XFER_CONTROL)
                {
                    IrsaLog.d(TAG, "usbInterface.getEndpoint(" + i + "),type: " + "USB_ENDPOINT_XFER_CONTROL" + "\n");
                }
                else if (ep.getType() == UsbConstants.USB_ENDPOINT_XFER_ISOC)
                {
                    IrsaLog.d(TAG, "usbInterface.getEndpoint(" + i + "),type: " + "USB_ENDPOINT_XFER_ISOC" + "\n");
                }

                if (ep.getType() == UsbConstants.USB_ENDPOINT_XFER_BULK)
                {
                    if (ep.getDirection() == UsbConstants.USB_DIR_IN)
                    {
                        mUsbEndpointIn = ep;
                        mUsbInterfaceInt = usbInterface;
                        IrsaLog.d(TAG, "mUsbEndpointIn \n");
                    }
                    else
                    {
                        mUsbEndpointOut = ep;
                        musbInterfaceOut = usbInterface;
                        IrsaLog.d(TAG, "mUsbEndpointOut \n");
                    }

                }


                if (ep.getType() != UsbConstants.USB_ENDPOINT_XFER_BULK)
                {
                    IrsaLog.d(TAG, "ep is not XFER_BULK");
                    continue;
                }

                if ((null == mUsbEndpointIn) && (null == mUsbEndpointOut))
                {
                    IrsaLog.d(TAG, "endpoint is null\n");
                    mUsbEndpointIn = null;
                    mUsbEndpointOut = null;
                }
                else
                {
                    IrsaLog.d(TAG, "endpoint out: " + mUsbEndpointOut + ",endpoint in: " + mUsbEndpointIn.getAddress()+"\n");
                    IrsaLog.d(TAG, "uvc fd: " + mUVCFD);
                    byte[] rawDescs = mUsbDeviceConnection.getRawDescriptors();
                    IrsaLog.d(TAG, "rawDescs: " + byteToString(rawDescs) + "\n");
                    System.arraycopy(rawDescs, 0, description, 0, rawDescs.length);

                    IrsaLog.d(TAG, "rawDescs length: " + rawDescs.length);
                    return rawDescs.length;
                }
            }
        }

        return 0;
    }


    private static String byteToString(byte[] b)
    {
        String result = "";
        for (int i = 0; i < b.length; i++)
        {
            result = String.format("%s %02x", result, b[i]);
        }
        return result;
    }


    private static int checkUSBVFS(String path)
    {
        IrsaLog.d(TAG, "checkUSBVFS: " + path);
        try
        {
            if (new File(path).exists())
            {
                File file = new File("/sys/bus/usb/devices/usb1/descriptors");
                InputStream in = new FileInputStream(file);
                if (in != null)
                {
                    int tmpbyte;
                    while ((tmpbyte = in.read()) != -1)
                    {
                        //IrsaLog.d(TAG, "byte: " + tmpbyte);
                    }
                    in.close();
                }
                else
                {
                    IrsaLog.d(TAG, "cann't open");
                }
                IrsaLog.d(TAG, "find " + path);
                return 1;
            }
            else
            {
                IrsaLog.d(TAG, "can't open " + path);
                return 0;
            }
        }
        catch (Exception ex)
        {
            ex.printStackTrace();
        }

        return 1;
    }


    private static void _initUVCDevice()
    {
        IrsaLog.d(TAG, "getUVCDevice");
        if (mMe == null)
        {
            IrsaLog.d(TAG, "pls check initUVCDevice(...) already invoked in application");
            return;
        }
        //mUsbManager = (UsbManager)mMe.getSystemService(Context.USB_SERVICE);

        HashMap<String, UsbDevice> deviceList = mUsbManager.getDeviceList();
        IrsaLog.d(TAG, "deviceList.size " + deviceList.size());
        try
        {
            Iterator<UsbDevice> deviceIterator = deviceList.values().iterator();

            String devInfo = "";
            StringBuilder sb = new StringBuilder();
            int i = 0;
            while (deviceIterator.hasNext())
            {
                UsbDevice device = deviceIterator.next();
                int deviceClass = device.getDeviceClass();

                IrsaLog.e(TAG, "\ndevice class " + device.getDeviceClass() + ", device name: " + device.getDeviceName() + "\ndevice product name:" + device.getProductName() + "\nvendor id:" + Integer.toHexString(device.getVendorId()) + "\n\n");

                if (!(Integer.toHexString(device.getVendorId()).equals(new String("8086"))))
                {
                    IrsaLog.d(TAG, "not Realsense Camera");
                    continue;
                }

                sb.append("\n" +
                          "DeviceID: " + device.getDeviceId() + "\n" +
                          "DeviceName: " + device.getDeviceName() + "\n" +
                          "ProductName: " + device.getProductName() + "\n" +
                          "DeviceClass: " + device.getDeviceClass() + " - "
                          + translateDeviceClass(device.getDeviceClass()) + "\n" +
                          "DeviceSubClass: " + device.getDeviceSubclass() + "\n" +
                          "VendorID: " + Integer.toHexString(device.getVendorId()) + "\n" +
                          "ProductID: " + Integer.toHexString(device.getProductId()) + "\n" +
                          "device serial: " + device.getSerialNumber() + "\n" +
                          "device desc: " + device.toString() + "\n\n");

                IrsaLog.d(TAG, "usb device  " + (i++) +  sb.toString() + "\n\n");

                if (device.getDeviceClass() != 0)
                {
                    mUsbDevice = device;
                    if (mUsbManager.hasPermission(device))
                    {
                        IrsaLog.d(TAG, "dev name " + device.getDeviceName() + " has permission");
                    }
                    else
                    {
                        IrsaLog.d(TAG, "no permission");
                        mUsbManager.requestPermission(device, mPermissionIntent);
                    }
                }

            }

        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
    }


    private static String translateDeviceClass(int deviceClass)
    {
        switch(deviceClass)
        {
        case UsbConstants.USB_CLASS_APP_SPEC:
            return "Application specific USB class";
        case UsbConstants.USB_CLASS_AUDIO:
            return "USB class for audio devices";
        case UsbConstants.USB_CLASS_CDC_DATA:
            return "USB class for CDC devices (communications device class)";
        case UsbConstants.USB_CLASS_COMM:
            return "USB class for communication devices";
        case UsbConstants.USB_CLASS_CONTENT_SEC:
            return "USB class for content security devices";
        case UsbConstants.USB_CLASS_CSCID:
            return "USB class for content smart card devices";
        case UsbConstants.USB_CLASS_HID:
            return "USB class for human interface devices (for example, mice and keyboards)";
        case UsbConstants.USB_CLASS_HUB:
            return "USB class for USB hubs";
        case UsbConstants.USB_CLASS_MASS_STORAGE:
            return "USB class for mass storage devices";
        case UsbConstants.USB_CLASS_MISC:
            //return "USB class for miscellaneous devices";
            return "USB class for Intel(R) RealSense(TM) 415";
        case UsbConstants.USB_CLASS_PER_INTERFACE:
            return "USB class indicating that the class is determined on a per-interface basis";
        case UsbConstants.USB_CLASS_PHYSICA:
            return "USB class for physical devices";
        case UsbConstants.USB_CLASS_PRINTER:
            return "USB class for printers";
        case UsbConstants.USB_CLASS_STILL_IMAGE:
            return "USB class for still image devices (digital cameras)";
        case UsbConstants.USB_CLASS_VENDOR_SPEC:
            return "Vendor specific USB class";
        case UsbConstants.USB_CLASS_VIDEO:
            return "USB class for video devices";
        case UsbConstants.USB_CLASS_WIRELESS_CONTROLLER:
            return "USB class for wireless controller devices";
        default:
            return "Unknown USB class!";
        }
    }

    private final BroadcastReceiver mUsbReceiver = new BroadcastReceiver()
    {
        public void onReceive(Context context, Intent intent)
        {
            String action = intent.getAction();
            IrsaLog.d(TAG, "BroadcastReceiver\n");

            if(UsbManager.ACTION_USB_DEVICE_ATTACHED.equals(action))
            {
                IrsaLog.d(TAG, "ACTION_USB_DEVICE_ATTACHED\n");
                UsbDevice device = (UsbDevice)intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
                //initTalkWithCamera(device);
            }
            else if(UsbManager.ACTION_USB_DEVICE_DETACHED.equals(action))
            {
                IrsaLog.d(TAG, "ACTION_USB_DEVICE_DETACHED\n");
            }
            else if (ACTION_DEVICE_PERMISSION.equals(action))
            {
                synchronized (this)
                {
                    UsbDevice device = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
                    if (intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false))
                    {
                        if (device != null)
                        {
                            IrsaLog.d(TAG, "usb EXTRA_PERMISSION_GRANTED");
                            IrsaLog.d(TAG, "has permission");
                            mMe.unregisterReceiver(mUsbReceiver);
                            /*
                            mUsbDeviceConnection = mUsbManager.openDevice(device);
                            if (mUsbDeviceConnection != null) {
                                mUVCFD = mUsbDeviceConnection.getFileDescriptor();
                                IrsaLog.d(TAG, "uvc fd: " + mUVCFD);
                            } else {
                                IrsaLog.d(TAG, "cann't open usb device");
                            }
                            */
                        }
                    }
                    else
                    {
                        IrsaLog.d(TAG, "usb EXTRA_PERMISSION_GRANTED null!!!");
                    }
                }

            }
        }
    };
    //=========== end USB experiment ========================== }


    public void open()
    {
        IrsaLog.d(TAG, "enter open");
        IrsaLog.d(TAG, "fd: " + mUVCFD);
        if ( -1 == mUVCFD)
        {
            _initUVCDevice();
            openUVCDevice();

            IrsaLog.d(TAG, "fd: " + mUVCFD);
        }
        native_open();
    }


    public void close()
    {
        IrsaLog.d(TAG, "enter close");
        native_close();
    }


    public String getIRSAVersion() {
        String javaVersion = IrsaVersion.IRSA_VERSION;

        return javaVersion;
    }

    public native void native_open();
    public native void native_close();
    public native int  getDeviceCounts();
    public native String  getDeviceName();
    public native String  getDeviceSN();
    public native String  getDeviceFW();

    //get librealsense's version
    public native String  getSDKVersion();

    public native Map<Integer, IrsaStreamProfile> getStreamProfiles(int stream);
    public native void setStreamFormat(int stream, int width, int height, int fps, int format);
    public native String formatToString(int format);
    public native int    formatFromString(String format);


    /**
     * @param previewType: IRSA_PREVIEW_FAKE, IRSA_PREVIEW_REALSENSE, IRSA_PREVIEW_FA
     */
    public native void setPreviewType(int previewType);
    public native void setBAGFile(String name);

    public native void enablePreview(int stream, Surface surface);
    //public native void setPreviewDisplay(Map surfaceMap);
    public native void disablePreview(int stream);

    //open/close emitter
    public native void switchEmitter();
    public native void setEmitter(boolean bEnable);
    public native int  getEmitter();
    public native void setLaserPower(int value);

    public native void setROI(int stream, int x_start, int y_start, int x_end, int y_end);
    public native IrsaRect getROI(int stream);

    public native void startPreview();
    public native void stopPreview();

    private native static int getVersionDescription(byte[] byteSecurityToken, int bufSize);
    private native static final void native_init();
    private native final void native_setup(Object irsamgr_this);
    private native final void native_release();


    //for FA(Face Authentication)
    public native boolean initIRFA(String dataDir);
    public native void switchToRegister();
    public native void setFADirection(int dir);

    static
    {
        try
        {
            IrsaLoader.load();
            native_init();
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
    }
}
