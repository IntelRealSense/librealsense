package com.intel.rs;

public enum Option
{
    COLOR_BACKLIGHT_COMPENSATION(0),
    COLOR_BRIGHTNESS(1),
    COLOR_CONTRAST(2),
    COLOR_EXPOSURE(3), // Controls exposure time of color camera. Setting any value will disable auto exposure.
    COLOR_GAIN(4),
    COLOR_GAMMA(5),
    COLOR_HUE(6), // Controls hue of color image. Setting any value will disable auto hue.
    COLOR_SATURATION(7),
    COLOR_SHARPNESS(8),
    COLOR_WHITE_BALANCE(9), // Controls white balance of color image. Setting any value will disable auto white balance.
    COLOR_ENABLE_AUTO_EXPOSURE(10), // Set to 1 to enable automatic exposure control, or 0 to return to manual control
    COLOR_ENABLE_AUTO_HUE(11), // Set to 1 to enable automatic hue control, or 0 to return to manual control
    COLOR_ENABLE_AUTO_WHITE_BALANCE(12), // Set to 1 to enable automatic white balance control, or 0 to return to manual control
    F200_LASER_POWER(13), // 0 - 15
    F200_ACCURACY(14), // 0 - 3
    F200_MOTION_RANGE(15), // 0 - 100
    F200_FILTER_OPTION(16), // 0 - 7
    F200_CONFIDENCE_THRESHOLD(17), // 0 - 15
    F200_DYNAMIC_FPS(18), // {2, 5, 15, 30, 60}
    R200_LR_AUTO_EXPOSURE_ENABLED(19), // {0, 1}
    R200_LR_GAIN(20), // 100 - 1600 (Units of 0.01)
    R200_LR_EXPOSURE(21), // > 0 (Units of 0.1 ms)
    R200_EMITTER_ENABLED(22), // {0, 1}
    R200_DEPTH_CONTROL_PRESET(23), // 0 - 5, 0 is default, 1-5 is low to high outlier rejection
    R200_DEPTH_UNITS(24), // micrometers per increment in integer depth values, 1000 is default (mm scale)
    R200_DEPTH_CLAMP_MIN(25), // 0 - USHORT_MAX
    R200_DEPTH_CLAMP_MAX(26), // 0 - USHORT_MAX
    R200_DISPARITY_MULTIPLIER(27), // 0 - 1000, the increments in integer disparity values corresponding to one pixel of disparity
    R200_DISPARITY_SHIFT(28);
    public final int code;
    private Option(int code) { this.code = code; }
}
