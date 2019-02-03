package com.intel.realsense.librealsense;


public class Pipeline extends LrsClass{
    public Pipeline(){
        RsContext ctx = new RsContext();
        mHandle = nCreate(ctx.getHandle());
    }

    public void start() throws Exception{
        nStart(mHandle);
    }

    public void start(Config config) throws Exception {
        nStartWithConfig(mHandle, config.getHandle());
    }
    public void stop() throws Exception{
        nStop(mHandle);
    }

    public FrameSet waitForFrames() throws Exception {
        return waitForFrames(5000);
    }

    public FrameSet waitForFrames (int timeoutMilliseconds) throws Exception{
        long frameHandle = nWaitForFrames(mHandle, timeoutMilliseconds);
        return new FrameSet(frameHandle);
    }

    @Override
    public void close() throws Exception {
        nDelete(mHandle);
        mHandle = 0;
    }

    private static native long nCreate(long context);
    private static native void nDelete(long handle);
    private static native void nStart(long handle);
    private static native void nStartWithConfig(long handle, long configHandle);
    private static native void nStop(long handle);
    private static native long nWaitForFrames(long handle, int timeout);
}
