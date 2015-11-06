package com.intel.rs;

public enum Option
{
    COLOR_BACKLIGHT_COMPENSATION(0),
    COLOR_BRIGHTNESS(1),
    COLOR_CONTRAST(2),
    COLOR_EXPOSURE(3),
    COLOR_GAIN(4),
    COLOR_GAMMA(5),
    COLOR_HUE(6),
    COLOR_SATURATION(7),
    COLOR_SHARPNESS(8),
    COLOR_WHITE_BALANCE(9),
    F200_LASER_POWER(10), // 0 - 15
    F200_ACCURACY(11), // 0 - 3
    F200_MOTION_RANGE(12), // 0 - 100
    F200_FILTER_OPTION(13), // 0 - 7
    F200_CONFIDENCE_THRESHOLD(14), // 0 - 15
    F200_DYNAMIC_FPS(15), // {2, 5, 15, 30, 60}
    R200_LR_AUTO_EXPOSURE_ENABLED(16), // {0, 1}
    R200_LR_GAIN(17), // 100 - 1600 (Units of 0.01)
    R200_LR_EXPOSURE(18), // > 0 (Units of 0.1 ms)
    R200_EMITTER_ENABLED(19), // {0, 1}
    R200_DEPTH_CONTROL_PRESET(20), // {0, 5}, 0 is default, 1-5 is low to high outlier rejection
    R200_DEPTH_UNITS(21), // micrometers per increment in integer depth values, 1000 is default (mm scale)
    R200_DEPTH_CLAMP_MIN(22), // 0 - USHORT_MAX
    R200_DEPTH_CLAMP_MAX(23), // 0 - USHORT_MAX
    R200_DISPARITY_MODE_ENABLED(24), // {0, 1}
    R200_DISPARITY_MULTIPLIER(25),
    R200_DISPARITY_SHIFT(26);
    public final int code;
    private Option(int code) { this.code = code; }
}
