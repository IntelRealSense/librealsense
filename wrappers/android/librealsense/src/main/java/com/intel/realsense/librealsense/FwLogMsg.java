package com.intel.realsense.librealsense;

public class FwLogMsg extends LrsClass{

    FwLogMsg(long handle){
        mHandle = handle;
    }

    public enum LogSeverity {
        DEBUG(0),
        INFO(1),
        WARN(2),
        ERROR(4),
        FATAL(5);

        private final int mValue;

        private LogSeverity(int value) { mValue = value; }
        public int value() { return mValue; }
    }

    @Override
    public void close() {
        nRelease(mHandle);
    }

    public LogSeverity getSeverity() { return LogSeverity.values()[nGetSeverity(mHandle)]; }
    public String getSeverityStr(){ return nGetSeverityStr(mHandle); }
    public int getTimestamp(){return nGetTimestamp(mHandle);}
    public byte[] getData(byte[] buffer){ return nGetData(mHandle, buffer);}


    private static native void nRelease(long handle);
    private static native int nGetSeverity(long handle);
    private static native String nGetSeverityStr(long handle);
    private static native int nGetTimestamp(long handle);
    private static native byte[] nGetData(long handle, byte[] buffer);
}
