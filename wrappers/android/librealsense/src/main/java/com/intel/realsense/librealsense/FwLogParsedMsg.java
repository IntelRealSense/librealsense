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
    public long getTimestamp(){return nGetTimestamp(mHandle);}
    public int getSequenceId() {return nGetSequenceId(mHandle);}


    private native static void nRelease(long handle);
    private native static String nGetMessage(long handle);
    private native static String nGetFileName(long handle);
    private native static String nGetThreadName(long handle);
    private native static String nGetSeverity(long handle);
    private native static int nGetLine(long handle);
    private native static long nGetTimestamp(long handle);
    private native static int nGetSequenceId(long handle);
}
