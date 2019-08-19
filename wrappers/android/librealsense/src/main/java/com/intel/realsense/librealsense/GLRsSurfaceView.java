package com.intel.realsense.librealsense;

import android.content.Context;
import android.graphics.Rect;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;

import java.util.Map;

public class GLRsSurfaceView extends GLSurfaceView {

    private final GLRenderer mRenderer;

    public GLRsSurfaceView(Context context) {
        super(context);
        mRenderer = new GLRenderer();
        setRenderer(mRenderer);
    }

    public GLRsSurfaceView(Context context, AttributeSet attrs) {
        super(context, attrs);
        mRenderer = new GLRenderer();
        setRenderer(mRenderer);
    }

    public Map<Integer, Rect> getRectangles() {
        return mRenderer.getRectangles();
    }

    public void upload(FrameSet frames) {
        mRenderer.upload(frames);
    }

    public void upload(Frame frame) {
        mRenderer.upload(frame);
    }

    public void clear() {
        mRenderer.clear();
    }
}
