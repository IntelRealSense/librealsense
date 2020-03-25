package com.intel.realsense.librealsense;

import android.graphics.Rect;
import android.opengl.GLES10;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.IntBuffer;

public class GLVideoFrame extends GLFrame {
    private IntBuffer mGlTexture;

    public GLVideoFrame(){
        mGlTexture = IntBuffer.allocate(1);
        GLES10.glGenTextures(1, mGlTexture);
    }

    public int getTexture() { return mGlTexture.array()[0]; }

    private Rect adjustRatio(Rect in){
        if (mFrame == null || !(mFrame.is(Extension.VIDEO_FRAME)))
            return null;

        float ratio = 0;
        try(VideoFrame vf = mFrame.as(Extension.VIDEO_FRAME)) {
            ratio = (float)vf.getWidth() / (float)vf.getHeight();
        }
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

    @Override
    public synchronized void draw(Rect rect)
    {
        if (mFrame == null || !(mFrame.is(Extension.VIDEO_FRAME)))
            return;

        try(VideoFrame vf = mFrame.as(Extension.VIDEO_FRAME)) {
            int size = vf.getStride() * vf.getHeight();
            if(mBuffer == null || mBuffer.array().length != size){
                mBuffer = ByteBuffer.allocate(size);
                mBuffer.order(ByteOrder.LITTLE_ENDIAN);
            }
            mFrame.getData(mBuffer.array());
            mBuffer.rewind();

            upload(vf, mBuffer, mGlTexture.get(0));
            Rect r = adjustRatio(rect);
            draw(r, mGlTexture.get(0));
        }
    }

    @Override
    public synchronized void close() {
        if(mFrame != null)
            mFrame.close();
        GLES10.glDeleteTextures(1, mGlTexture);
    }

    public static void upload(VideoFrame vf, ByteBuffer buffer, int texture)
    {
        GLES10.glBindTexture(GLES10.GL_TEXTURE_2D, texture);
        try(VideoStreamProfile profile = vf.getProfile()){
        switch (profile.getFormat())
            {
                case RGB8:
                case BGR8:
                    GLES10.glTexImage2D(GLES10.GL_TEXTURE_2D, 0, GLES10.GL_RGB, vf.getWidth(), vf.getHeight(), 0, GLES10.GL_RGB, GLES10.GL_UNSIGNED_BYTE, buffer);
                    break;
                case RGBA8:
                    GLES10.glTexImage2D(GLES10.GL_TEXTURE_2D, 0, GLES10.GL_RGBA, vf.getWidth(), vf.getHeight(), 0, GLES10.GL_RGBA, GLES10.GL_UNSIGNED_BYTE, buffer);
                    break;
                case Y8:
                    GLES10.glTexImage2D(GLES10.GL_TEXTURE_2D, 0, GLES10.GL_LUMINANCE, vf.getWidth(), vf.getHeight(), 0, GLES10.GL_LUMINANCE, GLES10.GL_UNSIGNED_BYTE, buffer);
                    break;
                default:
                    throw new RuntimeException("The requested format is not supported by the viewer");
            }
        }


        GLES10.glTexParameterx(GLES10.GL_TEXTURE_2D, GLES10.GL_TEXTURE_MAG_FILTER, GLES10.GL_LINEAR);
        GLES10.glTexParameterx(GLES10.GL_TEXTURE_2D, GLES10.GL_TEXTURE_MIN_FILTER, GLES10.GL_LINEAR);
        GLES10.glTexParameterx(GLES10.GL_TEXTURE_2D, GLES10.GL_TEXTURE_WRAP_S, 0x2900);
        GLES10.glTexParameterx(GLES10.GL_TEXTURE_2D, GLES10.GL_TEXTURE_WRAP_T, 0x2900);
        GLES10.glPixelStorei(0x0CF2, 0);
        GLES10.glBindTexture(GLES10.GL_TEXTURE_2D, 0);
    }

    public static void draw(Rect r, int texture)
    {
        setViewport(r);

        GLES10.glBindTexture(GLES10.GL_TEXTURE_2D, texture);
        GLES10.glEnable(GLES10.GL_TEXTURE_2D);

        float[] verArray = {
                0,          0,
                0,          r.height(),
                r.width(),  r.height(),
                r.width(),   0
        };
        ByteBuffer ver = ByteBuffer.allocateDirect(verArray.length * 4);
        ver.order(ByteOrder.nativeOrder());

        float[] texArray = {
                0,0,
                0,1,
                1,1,
                1,0
        };
        ByteBuffer tex = ByteBuffer.allocateDirect(texArray.length * 4);
        tex.order(ByteOrder.nativeOrder());

        ver.asFloatBuffer().put(verArray);
        tex.asFloatBuffer().put(texArray);

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
