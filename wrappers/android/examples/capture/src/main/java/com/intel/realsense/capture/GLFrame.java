package com.intel.realsense.capture;

import android.graphics.Point;
import android.graphics.Rect;
import android.opengl.GLES10;

import com.intel.realsense.librealsense.StreamFormat;
import com.intel.realsense.librealsense.StreamType;
import com.intel.realsense.librealsense.VideoFrame;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public class GLFrame {

    private final ByteBuffer mBuffer;
    private final Point mSize = new Point();
    private final StreamType mStreamType;

    private final StreamFormat mStreamFormat;

    public GLFrame(VideoFrame frame){
        mBuffer = ByteBuffer.allocate(frame.getStride() * frame.getHeight());
        mBuffer.order(ByteOrder.LITTLE_ENDIAN);
        mSize.set(frame.getWidth(), frame.getHeight());
        mStreamType = frame.getProfile().getType();
        mStreamFormat = frame.getProfile().getFormat();
    }

    public synchronized void setFrame(VideoFrame frame) throws Exception {
        frame.getData(mBuffer.array());
    }

    private void set_viewport(Rect r)
    {
        GLES10.glViewport(r.left, r.top, r.width(), r.height());
        GLES10.glLoadIdentity();
        GLES10.glMatrixMode(GLES10.GL_PROJECTION);
        GLES10.glOrthof(0, r.width(), r.height(), 0, -1, +1);
    }

    public synchronized void upload(int texture)
    {
        if (mBuffer == null)
            return;

        mBuffer.rewind();

        int width = mSize.x;
        int height = mSize.y;

        GLES10.glBindTexture(GLES10.GL_TEXTURE_2D, texture);

        switch (mStreamFormat)
        {
            case RGB8:
            case BGR8:
                GLES10.glTexImage2D(GLES10.GL_TEXTURE_2D, 0, GLES10.GL_RGB, width, height, 0, GLES10.GL_RGB, GLES10.GL_UNSIGNED_BYTE, mBuffer);
                break;
            case RGBA8:
                GLES10.glTexImage2D(GLES10.GL_TEXTURE_2D, 0, GLES10.GL_RGBA, width, height, 0, GLES10.GL_RGBA, GLES10.GL_UNSIGNED_BYTE, mBuffer);
                break;
            case Y8:
                GLES10.glTexImage2D(GLES10.GL_TEXTURE_2D, 0, GLES10.GL_LUMINANCE, width, height, 0, GLES10.GL_LUMINANCE, GLES10.GL_UNSIGNED_BYTE, mBuffer);
                break;
            default:
                throw new RuntimeException("The requested format is not supported by the viewer");
        }

        GLES10.glTexParameterx(GLES10.GL_TEXTURE_2D, GLES10.GL_TEXTURE_MAG_FILTER, GLES10.GL_LINEAR);
        GLES10.glTexParameterx(GLES10.GL_TEXTURE_2D, GLES10.GL_TEXTURE_MIN_FILTER, GLES10.GL_LINEAR);
        GLES10.glTexParameterx(GLES10.GL_TEXTURE_2D, GLES10.GL_TEXTURE_WRAP_S, 0x2900);
        GLES10.glTexParameterx(GLES10.GL_TEXTURE_2D, GLES10.GL_TEXTURE_WRAP_T, 0x2900);
        GLES10.glPixelStorei(0x0CF2, 0);
        GLES10.glBindTexture(GLES10.GL_TEXTURE_2D, 0);
    }

    private Rect adjustRatio(Rect in){
        float ratio = (float)mSize.x / (float)mSize.y;
        float newHeight = in.height();
        float newWidth = in.height() * ratio;
        if(newWidth > in.width()){
            ratio = in.width() / newWidth;
            newWidth *= ratio;
            newHeight *= ratio;
        }

        float newLeft = in.left + (in.width() - newWidth) / 2f;
        float newTop = in.top + (in.height() - newHeight) / 2f;
        float newRight = newLeft + newWidth;
        float newBottom = newTop + newHeight;
        return new Rect((int)newLeft, (int)newTop, (int)newRight, (int)newBottom);
    }

    public void show(Rect rect, int texture)
    {
        if (texture == 0)
            return;

        Rect r = adjustRatio(rect);

        set_viewport(r);

        GLES10.glBindTexture(GLES10.GL_TEXTURE_2D, texture);
        GLES10.glEnable(GLES10.GL_TEXTURE_2D);

        ByteBuffer ver = ByteBuffer.allocateDirect(8 * 4);
        ver.order(ByteOrder.nativeOrder());
        ByteBuffer tex = ByteBuffer.allocateDirect(8 * 4);
        tex.order(ByteOrder.nativeOrder());

        float[] vtx1 = {
                0,          0,
                0,          r.height(),
                r.width(),  r.height(),
                r.width(),   0
        };

        float[] tex1 = {
                0,0,
                0,1,
                1,1,
                1,0
        };

        ver.asFloatBuffer().put(vtx1);
        tex.asFloatBuffer().put(tex1);

        GLES10.glEnableClientState(GLES10.GL_VERTEX_ARRAY);
        GLES10.glEnableClientState(GLES10.GL_TEXTURE_COORD_ARRAY);

        GLES10.glVertexPointer(2, GLES10.GL_FLOAT, 0, ver);
        GLES10.glTexCoordPointer(2, GLES10.GL_FLOAT, 0, tex);
        GLES10.glDrawArrays(GLES10.GL_TRIANGLE_FAN,0,4);

        GLES10.glDisableClientState(GLES10.GL_VERTEX_ARRAY);
        GLES10.glDisableClientState(GLES10.GL_TEXTURE_COORD_ARRAY);

        GLES10.glDisable(GLES10.GL_TEXTURE_2D);
        GLES10.glBindTexture(GLES10.GL_TEXTURE_2D, 0);
    }
}
