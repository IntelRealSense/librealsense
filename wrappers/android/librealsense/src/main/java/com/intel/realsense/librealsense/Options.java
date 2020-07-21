package com.intel.realsense.librealsense;

import java.util.HashMap;
import java.util.Map;

public abstract class Options extends LrsClass implements OptionsInterface {
    private Map<Option,OptionRange> mOptionRange = new HashMap<>();

    private class OptionRange {
        public float min;
        public float max;
        public float step;
        public float def;
    }

    private synchronized OptionRange getRange(Option option){
        if(!mOptionRange.containsKey(option)) {
            OptionRange optionRange = new OptionRange();
            nGetRange(mHandle, option.value(), optionRange);
            mOptionRange.put(option, optionRange);
        }
        return mOptionRange.get(option);
    }

    @Override
    public boolean supports(Option option) {
        return nSupports(mHandle, option.value());
    }

    @Override
    public float getValue(Option option) {
        return nGetValue(mHandle, option.value());
    }

    @Override
    public void setValue(Option option, float value) {
        nSetValue(mHandle, option.value(), value);
    }

    @Override
    public float getMinRange(Option option) {
        return getRange(option).min;
    }

    @Override
    public float getMaxRange(Option option) {
        return getRange(option).max;
    }

    @Override
    public float getStep(Option option) {
        return getRange(option).step;
    }

    @Override
    public float getDefault(Option option) {
        return getRange(option).def;
    }

    @Override
    public boolean isReadOnly(Option option) {
        return nIsReadOnly(mHandle, option.value());
    }

    @Override
    public String getDescription(Option option) {
        return nGetDescription(mHandle, option.value());
    }

    @Override
    public String valueDescription(Option option, float value) {
        return nGetValueDescription(mHandle, option.value(), value);
    }

    private static native boolean nSupports(long handle, int option);
    private static native float nGetValue(long handle, int option);
    private static native void nSetValue(long handle, int option, float value);
    private static native void nGetRange(long handle, int option, OptionRange outParams);
    private static native boolean nIsReadOnly(long handle, int option);
    private static native String nGetDescription(long handle, int option);
    private static native String nGetValueDescription(long handle, int option, float value);

}
