// License: Apache 2.0. See LICENSE file in root directory.
//
package android.irsa;

import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import java.util.Map;
import java.util.HashMap;
import java.util.Iterator;
import java.lang.ref.WeakReference;
import java.lang.reflect.Method;
import java.io.IOException;

final public class IrsaMgr {
    private final static String TAG = "IrsaMgr";

    private final static int MAX_VERSION_SIZE = 256;
    private static byte[] mVersion = new byte[MAX_VERSION_SIZE];

    private EventHandler mEventHandler;
    private IrsaEventListener mEventListener;
    private EventCallback mCallback = new EventCallback();

    private int mNativeContext;


    private IrsaMgr() throws IrsaException {
        throw new IrsaException("private constructor");
    }


    @Override
    protected void finalize() {
        IrsaLog.w(TAG, "[Java] finalize");
        native_release();
    }


    public void release() {
        IrsaLog.w(TAG, "[Java] destructor");
        native_release();
    }

    private boolean versionCheck() {
        String javaVersion = IrsaVersion.IRSA_VERSION;
        String nativeVersion = getVersion();
        IrsaLog.w(TAG, "[Java] irsajar    Version " + javaVersion);
        IrsaLog.w(TAG, "[Java] libirsajni Version " + nativeVersion);

        if (!nativeVersion.startsWith(javaVersion)) {
            IrsaLog.e(TAG, "irsa.jar's version does not match libirsajni.so's version -- native version: " + nativeVersion);
            return false;
        }
        return true;
    }


    public IrsaMgr(IrsaEventListener eventListener) throws IrsaException {
        if (!IrsaLoader.hasLoaded()) {
           throw new IrsaException("irsajni library not loaded");
        }

        if (!versionCheck()) {
            String javaVersion = IrsaVersion.IRSA_VERSION;
            String nativeVersion = getVersion();
            String errorMsg = "irsa.jar's version " + javaVersion + " does not match libirsajni.so's version -- native version: " + nativeVersion;
            IrsaLog.e(TAG, errorMsg);
            throw new IrsaException(errorMsg);
        }

        mEventListener = eventListener;

        Looper looper;
        if ((looper = Looper.myLooper()) != null) {
            mEventHandler = new EventHandler(this, looper);
        } else if ((looper = Looper.getMainLooper()) != null) {
            mEventHandler = new EventHandler(this, looper);
        } else {
            mEventHandler = null;
        }

        native_setup(new WeakReference<IrsaMgr>(this));
    }


    private class EventCallback implements IrsaEventInterface.IrsaEventCallback {
        public void onEventInfo(IrsaEventType event, int what, int arg1, int arg2, Object obj) {
            if (mEventListener != null) {
                mEventListener.onEvent(event, what, arg1, arg2, obj);
            }
        }
    }


    private class EventHandler extends Handler {
        private IrsaMgr mIrsaMgr;

        public EventHandler(IrsaMgr mgr, Looper looper) {
            super(looper);
            mIrsaMgr = mgr;
        }

        @Override
        public void handleMessage(Message msg) {
            try {
                if (0 == mIrsaMgr.mNativeContext) {
                    IrsaLog.w(TAG, "[Java] pls check JNI initialization");
                    return;
                }

                switch (msg.what) {
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
            } catch (Exception e) {
                IrsaLog.e(TAG, "exception occured in event handler");
            }
        }
    }


    public static String getVersion() {
        int versionLen = getVersionDescription(mVersion, MAX_VERSION_SIZE);
        if (versionLen <= 0) {
            return "";
        }

        return new String(mVersion, 0, versionLen);
    }

    private static void postEventFromNative(Object irsamgr_ref,
                                           int what, int arg1, int arg2, Object obj) {
        IrsaMgr mgr = (IrsaMgr) ((WeakReference) irsamgr_ref).get();
        if (mgr.mEventHandler != null) {
            Message msg = mgr.mEventHandler.obtainMessage(what, arg1, arg2, obj);
            mgr.mEventHandler.sendMessage(msg);
        }
    }

    public void open() {
        native_open();
    }

    public void close() {
        native_close();
    }

    public native void native_open();
    public native void native_close();
    public native int  getDeviceCounts();
    public native void setStreamFormat(int stream, int width, int height, int fps, int format);
    public native void setRenderID(int renderID);
    public native void setPreviewDisplay(Map surfaceMap);
    public native void startPreview();
    public native void stopPreview();

    private native static int getVersionDescription(byte[] byteSecurityToken, int bufSize);
    private native static final void native_init();
    private native final void native_setup(Object irsamgr_this);
    private native final void native_release();

    //for FA(Face Authentication)
    public native void initIRFA(String dataDir);
    public native void switchToRegister();
    public native void switchLight();

    static {
        try {
            IrsaLoader.load();
            native_init();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
