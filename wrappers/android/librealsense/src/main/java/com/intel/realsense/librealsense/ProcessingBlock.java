package com.intel.realsense.librealsense;

public abstract class ProcessingBlock extends LrsClass implements ProcessingBlockInterface {

    private static native void nDelete(long handle);

    @Override
    public void invoke(Frame original) {
        nInvoke(mHandle, original.getHandle());
    }

    @Override
    public void invoke(FrameSet original) {
        nInvoke(mHandle, original.getHandle());
    }

    @Override
    public void close() throws Exception {
        nDelete(mHandle);
    }

    protected static native void nInvoke(long handle, long frameHandle);
}
