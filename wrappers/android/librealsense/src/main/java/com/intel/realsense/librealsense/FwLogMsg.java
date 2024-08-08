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
    public long getTimestamp(){return nGetTimestamp(mHandle);}
    public byte[] getData(byte[] buffer){ return nGetData(mHandle, buffer);}
    public int getSize() { return nGetSize(mHandle);}


    private native static void nRelease(long handle);
    private native static int nGetSeverity(long handle);
    private native static String nGetSeverityStr(long handle);
    private native static long nGetTimestamp(long handle);
    private native static byte[] nGetData(long handle, byte[] buffer);
    private native static int nGetSize(long handle);
}
