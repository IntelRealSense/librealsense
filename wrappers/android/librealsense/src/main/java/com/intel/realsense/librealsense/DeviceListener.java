package com.intel.realsense.librealsense;

public interface DeviceListener {
    void onDeviceAttach() throws Exception;
    void onDeviceDetach() throws Exception;
}
