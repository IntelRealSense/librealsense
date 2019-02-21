package com.intel.realsense.librealsense;

public interface FilterInterface extends ProcessingBlockInterface {
    Frame process(Frame original);
    FrameSet process(FrameSet original);
}
