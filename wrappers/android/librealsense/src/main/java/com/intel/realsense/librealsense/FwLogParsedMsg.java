package com.intel.realsense.librealsense;

public class FwLogParsedMsg extends LrsClass {

    FwLogParsedMsg(long handle){
        mHandle = handle;
    }

    @Override
    public void close() {
        nRelease(mHandle);
    }

    public String getMessage() { return nGetMessage(mHandle); }
    public String getFileName() {return nGetFileName(mHandle);}
    public String getThreadName() {return nGetThreadName(mHandle);}
    public String getSeverity() {return nGetSeverity(mHandle);}
    public int getLine() {return nGetLine(mHandle);}
    public int getTimestamp(){return nGetTimestamp(mHandle);}


    private static native void nRelease(long handle);
    private static native String nGetMessage(long handle);
    private static native String nGetFileName(long handle);
    private static native String nGetThreadName(long handle);
    private static native String nGetSeverity(long handle);
    private static native int nGetLine(long handle);
    private static native int nGetTimestamp(long handle);
}
