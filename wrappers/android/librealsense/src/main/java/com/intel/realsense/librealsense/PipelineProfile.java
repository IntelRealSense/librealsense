package com.intel.realsense.librealsense;

class PipelineProfile extends LrsClass {

    PipelineProfile(long handle){
        mHandle = handle;
    }

    @Override
    public void close() {
        nDelete(mHandle);
    }

    private static native void nDelete(long handle);
}
