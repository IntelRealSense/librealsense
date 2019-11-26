package com.intel.realsense.librealsense;

public class Config extends LrsClass {

    public Config(){
        mHandle = nCreate();
    }

    public void enableStream(StreamType type) {
        enableStream(type, -1, 0, 0, StreamFormat.ANY, 0);
    }

    public void enableStream(StreamType type, StreamFormat format){
        enableStream(type, -1, 0, 0, format, 0);
    }

    public void enableStream(StreamType type, int width, int height) {
        enableStream(type, -1, width, height, StreamFormat.ANY, 0);
    }

    public void enableStream(StreamType type, int width, int height, StreamFormat format){
        enableStream(type, -1, width, height, format, 0);
    }

    public void enableStream(StreamType type, int index, int width, int height, StreamFormat format, int framerate){
        nEnableStream(mHandle, type.value(), index, width, height, format.value(), framerate);
    }

    public void disableStream(StreamType type) { nDisableStream(mHandle, type.value()); }

    public void enableAllStreams() { nEnableAllStreams(mHandle); }

    public void disableAllStreams() { nDisableAllStreams(mHandle); }

    public void enableRecordToFile(String filePath) {
        nEnableRecordToFile(mHandle, filePath);
    }

    public void enableDeviceFromFile(String filePath) {
        nEnableDeviceFromFile(mHandle, filePath);
    }

    public void enableDevice(String serial) { nEnableDevice(mHandle, serial); }

    public boolean canResolve(Pipeline pipeline){
        return nCanResolve(mHandle, pipeline.mHandle);
    }

    public void resolve(Pipeline pipeline) {
        long pipeHandle = nResolve(mHandle, pipeline.mHandle);
        PipelineProfile rv = new PipelineProfile(pipeHandle);
        rv.close();//TODO: enable when PipelineProfile is implemented
    }

    @Override
    public void close() {
        nDelete(mHandle);
    }

    private static native long nCreate();
    private static native void nDelete(long handle);
    private static native void nEnableStream(long handle, int type, int index, int width, int height, int format, int framerate);
    private static native void nDisableStream(long handle, int type);
    private static native void nEnableAllStreams(long handle);
    private static native void nDisableAllStreams(long handle);
    private static native void nEnableDeviceFromFile(long handle, String filePath);
    private static native void nEnableDevice(long handle, String serial);
    private static native void nEnableRecordToFile(long handle, String filePath);
    private static native boolean nCanResolve(long handle, long pipelineHandle);
    private static native long nResolve(long handle, long pipelineHandle);
}
