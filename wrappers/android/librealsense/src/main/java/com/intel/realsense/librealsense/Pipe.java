package com.intel.realsense.librealsense;

import java.nio.ByteBuffer;

public class Pipe extends LrsClass {
    private native void nStart(int width, int height);
    private native void nStop();
    private native void nWaitForFrames(ByteBuffer colorBuffer, ByteBuffer depthBuffer);

    public Pipe() {}

    public void start(Integer width, Integer height) {
        nStart(width, height);
    }

    public void stop() {
        nStop();
    }

    public void waitForFrames(ByteBuffer colorBuffer, ByteBuffer depthBuffer) {
        nWaitForFrames(colorBuffer, depthBuffer);
    }

    @Override
    public void close() throws Exception {

    }
}
