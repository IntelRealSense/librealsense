package com.intel.realsense.librealsense;

public class DeviceList extends LrsClass {

    public DeviceList(long handle){
        mHandle = handle;
    }

    public int getDeviceCount(){
        return nGetDeviceCount(mHandle);
    }

    public Device createDevice(int index){
        return new Device(nCreateDevice(mHandle, index));
    }

    public boolean containesDevice(Device device){
        return nContainsDevice(mHandle, device.getHandle());
    }

    public void foreach(DeviceCallback callback) {
        int size = getDeviceCount();
        for(int i = 0; i < size; i++) {
            try(Device d = createDevice(i)){
                callback.onDevice(d);
            }
        }
    }

    @Override
    public void close() {
        nRelease(mHandle);
    }

    private static native int nGetDeviceCount(long handle);
    private static native long nCreateDevice(long handle, int index);
    private static native boolean nContainsDevice(long handle, long deviceHandle);
    private static native void nRelease(long handle);
}
