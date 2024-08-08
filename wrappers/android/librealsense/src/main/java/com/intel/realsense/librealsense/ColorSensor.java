package com.intel.realsense.librealsense;

public class ColorSensor extends Sensor {

    ColorSensor(long handle) {
        super(handle);
        mOwner = false;
    }
}
