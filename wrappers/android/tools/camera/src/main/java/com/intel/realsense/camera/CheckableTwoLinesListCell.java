package com.intel.realsense.camera;

public class CheckableTwoLinesListCell {
    private final String mMainString;
    private final String mSecondaryString;
    protected boolean mIsEnabled;

    public String getMainString() { return mMainString; }
    public String getSecondaryString() { return mSecondaryString; }

    public CheckableTwoLinesListCell(String mainString, String secondaryString, boolean isEnabled) {
        mMainString = mainString;
        mSecondaryString = secondaryString;
        mIsEnabled = isEnabled;
    }

    public boolean isEnabled() { return mIsEnabled; }
    public void setEnabled(boolean enabled) { mIsEnabled = enabled; }
}
