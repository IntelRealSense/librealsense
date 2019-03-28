package com.intel.realsense.librealsense;

public enum CameraInfo {
    NAME(0),
    SERIAL_NUMBER(1),
    FIRMWARE_VERSION(2),
    RECOMMENDED_FIRMWARE_VERSION(3),
    PHYSICAL_PORT(4),
    DEBUG_OP_CODE(5),
    ADVANCED_MODE(6),
    PRODUCT_ID(7),
    CAMERA_LOCKED(8),
    USB_TYPE_DESCRIPTOR(9);

    private final int mValue;

    private CameraInfo(int value) { mValue = value; }
    public int value() { return mValue; }
}