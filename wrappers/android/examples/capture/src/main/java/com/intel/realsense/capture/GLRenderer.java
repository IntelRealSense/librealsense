package com.intel.realsense.capture;

import android.graphics.Point;
import android.graphics.Rect;
import android.opengl.GLES10;
import android.opengl.GLSurfaceView;

import com.intel.realsense.librealsense.Frame;
import com.intel.realsense.librealsense.FrameSet;
import com.intel.realsense.librealsense.StreamFormat;
import com.intel.realsense.librealsense.StreamType;
import com.intel.realsense.librealsense.VideoFrame;

import java.util.HashMap;
import java.util.Map;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class GLRenderer implements GLSurfaceView.Renderer{

    private final Map<StreamType,GLFrame> mFrameLoaders = new HashMap<>();
    private final int[] mTextures = new int[1];
    private int mWindowHeight = 0;
    private int mWindowWidth = 0;
    private boolean mIsDirty = true;

    public void show(FrameSet frameSet) throws Exception {
        frameSet.foreach(new FrameSet.FrameCallback() {
            @Override
            public void onFrame(Frame f) throws Exception {
                show(f.as(VideoFrame.class));
            }
        });
    }

    public void show(VideoFrame f) throws Exception {
        if(f == null)
            return;

        if(!isFormatSupported(f.getProfile().getFormat()))
            return;

        StreamType stream = f.getProfile().getType();
        if(!mFrameLoaders.containsKey(stream)){
            synchronized (mFrameLoaders) {
                mFrameLoaders.put(stream, new GLFrame(f));
            }
            mIsDirty = true;
        }
        mFrameLoaders.get(stream).setFrame(f);
    }

    private boolean isFormatSupported(StreamFormat format) {
        switch (format){
            case RGB8:
            case RGBA8:
            case Y8: return true;
                default: return false;
        }
    }

    public void clean() { mIsDirty = true; }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        GLES10.glGenTextures(1, mTextures, 0);
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        mWindowWidth = width;
        mWindowHeight = height;
        mIsDirty = true;
    }

    @Override
    public void onDrawFrame(GL10 gl) {
        synchronized (mFrameLoaders) {
            if (mIsDirty) {
                GLES10.glViewport(0, 0, mWindowWidth, mWindowHeight);
                GLES10.glClearColor(0, 0, 0, 1);
                mIsDirty = false;
            }

            if (mFrameLoaders.size() == 0)
                return;

            int i = 0;
            for (Map.Entry<StreamType, GLFrame> entry : mFrameLoaders.entrySet()) {
                Point size = mWindowWidth > mWindowHeight ?
                        new Point(mWindowWidth / mFrameLoaders.size(), mWindowHeight) :
                        new Point(mWindowWidth, mWindowHeight / mFrameLoaders.size());
                Point pos = mWindowWidth > mWindowHeight ?
                        new Point(i++ * size.x, 0) :
                        new Point(0, i++ * size.y);
                Rect r = new Rect(pos.x, pos.y, pos.x + size.x, pos.y + size.y);
                GLFrame fl = entry.getValue();
                fl.upload(mTextures[0]);
                fl.show(r, mTextures[0]);
            }
        }
    }
}
