package com.intel.realsense.librealsense;


public class Pipeline extends LrsClass{
    public Pipeline(){
        RsContext ctx = new RsContext();
        mHandle = nCreate(ctx.getHandle());
    }

    public void start(){
        nStart(mHandle);
    }

    public void start(Config config){
        nStartWithConfig(mHandle, config.getHandle());
    }
    public void stop(){
        nStop(mHandle);
    }

    public FrameSet waitForFrames() {
        return waitForFrames(5000);
    }

    public FrameSet waitForFrames(int timeoutMilliseconds) {
        long frameHandle = nWaitForFrames(mHandle, timeoutMilliseconds);
        return new FrameSet(frameHandle);
    }

    @Override
    public void close() throws Exception {
        nDelete(mHandle);
        mHandle = 0;
    }

    private native long nCreate(long context);
    private native void nDelete(long handle);
    private native void nStart(long handle);
    private native void nStartWithConfig(long handle, long configHandle);
    private native void nStop(long handle);
    private native long nWaitForFrames(long handle, int timeout);
}
