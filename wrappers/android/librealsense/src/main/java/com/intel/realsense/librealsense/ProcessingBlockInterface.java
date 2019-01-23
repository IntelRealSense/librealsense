package com.intel.realsense.librealsense;

public interface ProcessingBlockInterface {
    void invoke(Frame original);
    void invoke(FrameSet original);
}
