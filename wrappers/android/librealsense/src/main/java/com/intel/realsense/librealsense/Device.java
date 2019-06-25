package com.intel.realsense.librealsense;

import java.util.ArrayList;
import java.util.List;

public class Device extends LrsClass {
    private List<Sensor> _sensors = new ArrayList<>();

    public Device(long handle){
        mHandle = handle;
        long[] sensorsHandles = nQuerySensors(mHandle);
        for(long h : sensorsHandles){
            _sensors.add(new Sensor(h));
        }
    }

    public List<Sensor> querySensors(){
        return _sensors;
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

    public <T extends Device> T as(Class<T> type) {
        return (T) this;
    }

    public void hardwareReset(){
        nHardwareReset(mHandle);
    }

    @Override
    public void close() {
        for (Sensor s : _sensors)
            s.close();
        nRelease(mHandle);
    }

    private static native boolean nSupportsInfo(long handle, int info);
    private static native String nGetInfo(long handle, int info);
    private static native void nToggleAdvancedMode(long handle, boolean enable);
    private static native boolean nIsInAdvancedMode(long handle);
    private static native void nLoadPresetFromJson(long handle, byte[] data);
    private static native byte[] nSerializePresetToJson(long handle);
    private static native long[] nQuerySensors(long handle);
    private static native void nHardwareReset(long handle);
    private static native void nRelease(long handle);
}
