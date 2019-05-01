package com.intel.realsense.librealsense;

import java.util.ArrayList;
import java.util.List;

public class FrameReleaser implements AutoCloseable {
    private List<Frame> mFrames = new ArrayList<>();
    private List<FrameSet> mFramesets = new ArrayList<>();

    public void addFrame(Frame frame){
        mFrames.add(frame);
    }

    public void addFrame(FrameSet frames){
        mFramesets.add(frames);
    }

    @Override
    public void close() {
        for(Frame f : mFrames){
            f.close();
        }

        for(FrameSet f : mFramesets){
            f.close();
        }
    }
}
