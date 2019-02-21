package com.intel.realsense.librealsense;

public interface OptionsInterface {
    boolean supports(Option option);
    float getValue(Option option);
    void setValue(Option option, float value);
    float getMinRange(Option option);
    float getMaxRange(Option option);
    float getStep(Option option);
    float getDefault(Option option);
    boolean isReadOnly(Option option);
    String getDescription(Option option);
    String valueDescription(Option option, float value);
}
