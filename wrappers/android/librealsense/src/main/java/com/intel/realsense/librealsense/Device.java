package com.intel.realsense.librealsense;

import java.util.ArrayList;
import java.util.List;

public class Device extends LrsClass {

    public Device(long handle){
        mHandle = handle;
    }

    public List<Sensor> querySensors(){
        long[] sensorsHandles = nQuerySensors(mHandle);
        List<Sensor> rv = new ArrayList<>();
        for(long h : sensorsHandles){
            rv.add(new Sensor(h));
        }
        return rv;
    }

    public boolean supportsInfo(CameraInfo info){
        return nSupportsInfo(mHandle, info.value());
    }

    public String getInfo(CameraInfo info){
        return nGetInfo(mHandle, info.value());
    }

    public void toggleAdvancedMode(boolean enable){
        nToggleAdvancedMode(mHandle, enable);
    }

    public boolean isInAdvancedMode(){
        return nIsInAdvancedMode(mHandle);
    }

    public void loadPresetFromJson(byte[] data){
        nLoadPresetFromJson(mHandle, data);
    }

    public byte[] serializePresetToJson(){
        return nSerializePresetToJson(mHandle);
    }

    @Override
    public void close() {
        nRelease(mHandle);
    }

    private static native boolean nSupportsInfo(long handle, int info);
    private static native String nGetInfo(long handle, int info);
    private static native void nToggleAdvancedMode(long handle, boolean enable);
    private static native boolean nIsInAdvancedMode(long handle);
    private static native void nLoadPresetFromJson(long handle, byte[] data);
    private static native byte[] nSerializePresetToJson(long handle);
    private static native long[] nQuerySensors(long handle);
    private static native void nRelease(long handle);
}
