package com.intel.rs;

public enum Option
{
    COLOR_BACKLIGHT_COMPENSATION(0),
    COLOR_BRIGHTNESS(1),
    COLOR_CONTRAST(2),
    COLOR_EXPOSURE(3), // Controls exposure time of color camera. Setting any value will disable auto exposure.
    COLOR_GAIN(4),
    COLOR_GAMMA(5),
    COLOR_HUE(6),
    COLOR_SATURATION(7),
    COLOR_SHARPNESS(8),
    COLOR_WHITE_BALANCE(9), // Controls white balance of color image. Setting any value will disable auto white balance.
    COLOR_ENABLE_AUTO_EXPOSURE(10), // Set to 1 to enable automatic exposure control, or 0 to return to manual control
    COLOR_ENABLE_AUTO_WHITE_BALANCE(11), // Set to 1 to enable automatic white balance control, or 0 to return to manual control
    F200_LASER_POWER(12), // 0 - 15
    F200_ACCURACY(13), // 0 - 3
    F200_MOTION_RANGE(14), // 0 - 100
    F200_FILTER_OPTION(15), // 0 - 7
    F200_CONFIDENCE_THRESHOLD(16), // 0 - 15
    F200_DYNAMIC_FPS(17), // {2, 5, 15, 30, 60}
    R200_LR_AUTO_EXPOSURE_ENABLED(18), // {0, 1}
    R200_LR_GAIN(19), // 100 - 1600 (Units of 0.01)
    R200_LR_EXPOSURE(20), // > 0 (Units of 0.1 ms)
    R200_EMITTER_ENABLED(21), // {0, 1}
    R200_DEPTH_UNITS(22), // micrometers per increment in integer depth values, 1000 is default (mm scale)
    R200_DEPTH_CLAMP_MIN(23), // 0 - USHORT_MAX
    R200_DEPTH_CLAMP_MAX(24), // 0 - USHORT_MAX
    R200_DISPARITY_MULTIPLIER(25), // 0 - 1000, the increments in integer disparity values corresponding to one pixel of disparity
    R200_DISPARITY_SHIFT(26);
    public final int code;
    private Option(int code) { this.code = code; }
}
