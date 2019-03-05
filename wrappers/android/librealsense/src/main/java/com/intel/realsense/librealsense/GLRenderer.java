package com.intel.realsense.librealsense;

import android.graphics.Point;
import android.graphics.Rect;
import android.opengl.GLES10;
import android.opengl.GLSurfaceView;

import java.util.HashMap;
import java.util.Map;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class GLRenderer implements GLSurfaceView.Renderer{

    private final Map<Integer,GLFrame> mFrames = new HashMap<>();
    private int mWindowHeight = 0;
    private int mWindowWidth = 0;
    private boolean mIsDirty = true;

    public void upload(FrameSet frameSet) {
        frameSet.foreach(new FrameCallback() {
            @Override
            public void onFrame(Frame f) {
                upload(f);
            }
        });
    }

    public void upload(Frame f) {
        if(f == null)
            return;

        if(!isFormatSupported(f.getProfile().getFormat()))
            return;

        int uid = f.getProfile().getUniqueId();
        if(!mFrames.containsKey(uid)){
            synchronized (mFrames) {
                mFrames.put(uid, new GLVideoFrame());
            }
            mIsDirty = true;
        }
        mFrames.get(uid).setFrame(f);
    }

    public void clear() {
        synchronized (mFrames) {
            mFrames.clear();
        }
        mIsDirty = true;
    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {

    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        mWindowWidth = width;
        mWindowHeight = height;
        mIsDirty = true;
    }

    @Override
    public void onDrawFrame(GL10 gl) {
        synchronized (mFrames) {
            if (mIsDirty) {
                GLES10.glViewport(0, 0, mWindowWidth, mWindowHeight);
                GLES10.glClearColor(0, 0, 0, 1);
                mIsDirty = false;
            }

            if (mFrames.size() == 0)
                return;

            int i = 0;
            for (Map.Entry<Integer, GLFrame> entry : mFrames.entrySet()) {
                Point size = mWindowWidth > mWindowHeight ?
                        new Point(mWindowWidth / mFrames.size(), mWindowHeight) :
                        new Point(mWindowWidth, mWindowHeight / mFrames.size());
                Point pos = mWindowWidth > mWindowHeight ?
                        new Point(i++ * size.x, 0) :
                        new Point(0, i++ * size.y);
                Rect r = new Rect(pos.x, pos.y, pos.x + size.x, pos.y + size.y);
                GLFrame fl = entry.getValue();
                fl.draw(r);
            }
        }
    }

    private boolean isFormatSupported(StreamFormat format) {
        switch (format){
            case RGB8:
            case RGBA8:
            case Y8: return true;
            default: return false;
        }
    }
}
