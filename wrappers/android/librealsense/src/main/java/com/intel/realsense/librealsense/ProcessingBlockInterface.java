package com.intel.realsense.librealsense;

public interface ProcessingBlockInterface extends OptionsInterface{
    void invoke(Frame original);
    void invoke(FrameSet original);
}
