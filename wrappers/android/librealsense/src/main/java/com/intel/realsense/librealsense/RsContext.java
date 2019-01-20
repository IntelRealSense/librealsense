package com.intel.realsense.librealsense;

public class RsContext extends LrsClass{

    public RsContext() {
        mHandle = nCreate();
    }

    @Override
    public void close() throws Exception {
        nDelete(mHandle);
    }

    private static native long nCreate();
    private static native void nDelete(long handle);
}
