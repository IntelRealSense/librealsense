package com.intel.realsense.librealsense;


public class Pipeline extends LrsClass{
    public Pipeline(){
        RsContext ctx = new RsContext();
        mHandle = nCreate(ctx.getHandle());
    }

    public void start() throws Exception{
        PipelineProfile rv =  new PipelineProfile(nStart(mHandle));
        rv.close();//TODO: enable when PipelineProfile is implemented
    }

    public void start(Config config) throws Exception {
        PipelineProfile rv = new PipelineProfile(nStartWithConfig(mHandle, config.getHandle()));
        rv.close();//TODO: enable when PipelineProfile is implemented
    }
    public void stop() {
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
    public void close(){
        nDelete(mHandle);
    }

    private static native long nCreate(long context);
    private static native void nDelete(long handle);
    private static native long nStart(long handle);
    private static native long nStartWithConfig(long handle, long configHandle);
    private static native void nStop(long handle);
    private static native long nWaitForFrames(long handle, int timeout);
}
