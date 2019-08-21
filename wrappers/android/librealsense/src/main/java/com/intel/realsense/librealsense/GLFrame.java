package com.intel.realsense.librealsense;

import android.graphics.Rect;
import android.opengl.GLES10;

import java.nio.ByteBuffer;

public abstract class GLFrame implements AutoCloseable {
    protected Frame mFrame;
    protected ByteBuffer mBuffer;

    public abstract void draw(Rect rect);

    public synchronized void setFrame(Frame frame) {
        if(mFrame != null)
            mFrame.close();
        mFrame = frame.clone();
    }

    protected static void setViewport(Rect r) {
        GLES10.glViewport(r.left, r.top, r.width(), r.height());
        GLES10.glLoadIdentity();
        GLES10.glMatrixMode(GLES10.GL_PROJECTION);
        GLES10.glOrthof(0, r.width(), r.height(), 0, -1, +1);
    }

    public String getLabel() {
        if(mFrame == null)
            return "";
        try(StreamProfile sp = mFrame.getProfile()){
            return sp.getType() + " - " + sp.getFormat();
        }
    }

    @Override
    public void close() {
        if(mFrame != null)
            mFrame.close();
    }
}
