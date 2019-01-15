package com.intel.realsense.librealsense;

public class RsContext extends LrsClass{
    private native long nCreate();

    public RsContext() {
        mHandle = nCreate();
    }

    @Override
    public void close() throws Exception {

    }
}
