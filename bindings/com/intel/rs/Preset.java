package com.intel.rs;

public enum Preset
{
    BEST_QUALITY(0),
    LARGEST_IMAGE(1),
    HIGHEST_FRAMERATE(2);
    public final int code;
    private Preset(int code) { this.code = code; }
}
