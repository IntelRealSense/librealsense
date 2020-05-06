package com.intel.realsense.librealsense;

public class FrameQueue extends LrsClass {

    public FrameQueue(){
        mHandle = nCreate(1);
    }

    public FrameQueue(int capacity){
        mHandle = nCreate(capacity);
    }

    public Frame pollForFrame() {
        long rv = nPollForFrame(mHandle);
        if(rv != 0) {
            return new Frame(rv);
        }
        return null;
    }

    public FrameSet pollForFrames() {
        long rv = nPollForFrame(mHandle);
        if(rv != 0) {
            return new FrameSet(rv);
        }
        return null;
    }

    public Frame waitForFrame() {
        return waitForFrame(5000);
    }

    public Frame waitForFrame(int timeout) {
        long rv = nWaitForFrames(mHandle, timeout);
        return new Frame(rv);
    }

    public FrameSet waitForFrames() {
        return waitForFrames(5000);
    }

    public FrameSet waitForFrames(int timeout) {
        long rv = nWaitForFrames(mHandle, timeout);
        return new FrameSet(rv);
    }

    @Override
    public void close() throws Exception {
        nDelete(mHandle);
    }

    public void Enqueue(Frame f)
    {
        nEnqueue(mHandle, f.getHandle());
    }

    private static native long nCreate(int capacity);
    private static native void nDelete(long handle);
    private static native long nPollForFrame(long handle);
    private static native long nWaitForFrames(long handle, int timeout);
    private static native void nEnqueue(long handle, long frameHandle);
}
