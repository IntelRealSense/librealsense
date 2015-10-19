package com.intel.rs;

public class Device
{
    static { System.loadLibrary("realsense"); }
    private long handle;
    public Device(long handle) { this.handle = handle; }

    public native String getName();
    public native String getSerial();
    public native String getFirmwareVersion();
    public native void getExtrinsics(Stream fromStream, Stream toStream, Extrinsics extrin);
    public native float getDepthScale();
    public native boolean supportsOption(Option option);
    public native int getStreamModeCount(Stream stream);
    public native void getStreamMode(Stream stream, int index, Out<Integer> width, Out<Integer> height, Out<Format> format, Out<Integer> framerate);

    public native void enableStream(Stream stream, int width, int height, Format format, int framerate);
    public native void enableStreamPreset(Stream stream, Preset preset);
    public native void disableStream(Stream stream);
    public native boolean isStreamEnabled(Stream stream);
    public native void getStreamIntrinsics(Stream stream, Intrinsics intrin);
    public native Format getStreamFormat(Stream stream);
    public native int getStreamFramerate(Stream stream);

    public native void start();
    public native void stop();
    public native boolean isStreaming();

    public native void setOption(Option option, int value);
    public native int getOption(Option option);

    public native void waitForFrames();
    public native int getFrameTimestamp(Stream stream);
    public native long getFrameData(Stream stream);
}
