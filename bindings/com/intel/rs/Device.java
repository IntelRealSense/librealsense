package com.intel.rs;

public class Device
{
    static { System.loadLibrary("realsense"); }
    private long handle;
    public Device(long handle) { this.handle = handle; }


    /**
     * retrieve a human readable device model string
     * @return  the model string, such as "Intel RealSense F200" or "Intel RealSense R200"
     */
    public native String getName();

    /**
     * retrieve the unique serial number of the device
     * @return  the serial number, in a format specific to the device model
     */
    public native String getSerial();

    /**
     * retrieve the version of the firmware currently installed on the device
     * @return  firmware version string, in a format is specific to device model
     */
    public native String getFirmwareVersion();

    /**
     * retrieve extrinsic transformation between the viewpoints of two different streams
     * @param fromStream  stream whose coordinate space we will transform from
     * @param toStream    stream whose coordinate space we will transform to
     * @param extrin      the transformation between the two streams
     */
    public native void getExtrinsics(Stream fromStream, Stream toStream, Extrinsics extrin);

    /**
     * retrieve mapping between the units of the depth image and meters
     * @return  depth in meters corresponding to a depth value of 1
     */
    public native float getDepthScale();

    /**
     * determine if the device allows a specific option to be queried and set
     * @param option  the option to check for support
     * @return        true if the option can be queried and set
     */
    public native boolean supportsOption(Option option);

    /**
     * determine the number of streaming modes available for a given stream
     * @param stream  the stream whose modes will be enumerated
     * @return        the count of available modes
     */
    public native int getStreamModeCount(Stream stream);

    /**
     * determine the properties of a specific streaming mode
     * @param stream     the stream whose mode will be queried
     * @param index      the zero based index of the streaming mode
     * @param width      the width of a frame image in pixels
     * @param height     the height of a frame image in pixels
     * @param format     the pixel format of a frame image
     * @param framerate  the number of frames which will be streamed per second
     */
    public native void getStreamMode(Stream stream, int index, Out<Integer> width, Out<Integer> height, Out<Format> format, Out<Integer> framerate);

    /**
     * enable a specific stream and request specific properties
     * @param stream     the stream to enable
     * @param width      the desired width of a frame image in pixels, or 0 if any width is acceptable
     * @param height     the desired height of a frame image in pixels, or 0 if any height is acceptable
     * @param format     the pixel format of a frame image, or ANY if any format is acceptable
     * @param framerate  the number of frames which will be streamed per second, or 0 if any framerate is acceptable
     */
    public native void enableStream(Stream stream, int width, int height, Format format, int framerate);

    /**
     * enable a specific stream and request properties using a preset
     * @param stream  the stream to enable
     * @param preset  the preset to use to enable the stream
     */
    public native void enableStreamPreset(Stream stream, Preset preset);

    /**
     * disable a specific stream
     * @param stream  the stream to disable
     */
    public native void disableStream(Stream stream);

    /**
     * determine if a specific stream is enabled
     * @param stream  the stream to check
     * @return        true if the stream is currently enabled
     */
    public native boolean isStreamEnabled(Stream stream);

    /**
     * retrieve intrinsic camera parameters for a specific stream
     * @param stream  the stream whose parameters to retrieve
     * @param intrin  the intrinsic parameters of the stream
     */
    public native void getStreamIntrinsics(Stream stream, Intrinsics intrin);

    /**
     * retrieve the pixel format for a specific stream
     * @param stream  the stream whose format to retrieve
     * @return        the pixel format of the stream
     */
    public native Format getStreamFormat(Stream stream);

    /**
     * retrieve the framerate for a specific stream
     * @param stream  the stream whose framerate to retrieve
     * @return        the framerate of the stream, in frames per second
     */
    public native int getStreamFramerate(Stream stream);

    /**
     * begin streaming on all enabled streams for this device
     */
    public native void start();

    /**
     * end streaming on all streams for this device
     */
    public native void stop();

    /**
     * determine if the device is currently streaming
     * @return  true if the device is currently streaming
     */
    public native boolean isStreaming();

    /**
     * set the value of a specific device option
     * @param option  the option whose value to set
     * @param value   the desired value to set
     */
    public native void setOption(Option option, int value);

    /**
     * query the current value of a specific device option
     * @param option  the option whose value to retrieve
     * @return        the current value of the option
     */
    public native int getOption(Option option);

    /**
     * block until new frames are available
     */
    public native void waitForFrames();

    /**
     * retrieve the time at which the latest frame on a stream was captured
     * @param stream  the stream whose latest frame we are interested in
     * @return        the timestamp of the frame, in milliseconds since the device was started
     */
    public native int getFrameTimestamp(Stream stream);

    /**
     * retrieve the contents of the latest frame on a stream
     * @param stream  the stream whose latest frame we are interested in
     * @return        the pointer to the start of the frame data
     */
    public native long getFrameData(Stream stream);
}
