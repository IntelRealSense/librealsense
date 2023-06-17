package com.intel.realsense.librealsense;

import android.graphics.Rect;
import android.opengl.GLES10;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.IntBuffer;

public class GLPointsFrame extends GLFrame {
    private Frame mTexture;
    private IntBuffer mGlTexture;
    private float mDeltaX = 0;
    private float mDeltaY = 0;
    private float mRotationFactor = 0.1f;

    public synchronized void setTextureFrame(Frame frame) {
        if(mTexture != null)
            mTexture.close();
        mTexture = frame.clone();
    }
    public int getTexture() { return mGlTexture.array()[0]; }

    private void drawPoints(float[] verArray, byte[] colorArray)
    {
        if(colorArray != null){
            ByteBuffer tex = ByteBuffer.allocateDirect(colorArray.length);
            tex.order(ByteOrder.nativeOrder());
            tex.put(colorArray);
            tex.position(0);

            GLES10.glEnableClientState(GLES10.GL_COLOR_ARRAY);
            GLES10.glColorPointer(4, GLES10.GL_UNSIGNED_BYTE, 0, tex);
        }

        ByteBuffer ver = ByteBuffer.allocateDirect(verArray.length * 4);
        ver.order(ByteOrder.nativeOrder());
        ver.asFloatBuffer().put(verArray);

        GLES10.glEnableClientState(GLES10.GL_VERTEX_ARRAY);
        GLES10.glVertexPointer(3, GLES10.GL_FLOAT, 0, ver);
        GLES10.glDrawArrays(GLES10.GL_POINTS,0,verArray.length / 3);

        GLES10.glDisableClientState(GLES10.GL_VERTEX_ARRAY);
        if(colorArray != null)
            GLES10.glDisableClientState(GLES10.GL_COLOR_ARRAY);
    }

    @Override
    public synchronized void draw(Rect rect)
    {
        if (mFrame == null || !(mFrame.is(Extension.POINTS)))
            return;

        setViewport(rect);

        GLES10.glMatrixMode(GLES10.GL_PROJECTION);
        GLES10.glPushMatrix();
        GLES10.glLoadIdentity();

        GLES10.glOrthof(1f, -1f, -1f, 1f, -7f, 0f);

        GLES10.glRotatef(180, 0.0f, 0.0f, 1.0f);
        GLES10.glRotatef(-mDeltaY * mRotationFactor, 1.0f, 0.0f, 0.0f);
        GLES10.glRotatef(mDeltaX * mRotationFactor, 0.0f, 1.0f, 0.0f);

        Points points = mFrame.as(Extension.POINTS);
        float[] data = points.getVertices();
        byte[] tex = mTexture == null ? createTexture(data, 1.2f) : createTexture(points);
        drawPoints(data, tex);

        GLES10.glMatrixMode(GLES10.GL_PROJECTION);
        GLES10.glPopMatrix();
    }

    //fallback to gray scale if no texture is available
    private byte[] createTexture(float[] data, float maxRange){
        int size = data.length / 3;
        byte[] texture = new byte[size * 4];
        for(int i = 0; i < size; i++){
            float z = data[i * 3 + 2];
            byte val = (byte) (z / maxRange * 255);
            for(int j = 0; j < 3; j++)
                texture[i*4+j] = val;
            texture[i*4+3] = (byte) 255;
        }
        return texture;
    }

    public byte[] createTexture(Points points){
        if(mTexture == null)
            return null;

        VideoFrame vf = mTexture.as(Extension.VIDEO_FRAME);
        int textureSize = vf.getWidth() * vf.getHeight();
        int pointCount = points.getCount();
        float[] pointsData = points.getVertices();
        byte[] textureData = new byte[textureSize * 3];
        byte[] texture = new byte[pointCount * 4];
        float[] textureCoordinates = points.getTextureCoordinates();

        vf.getData(textureData);
        int w = vf.getWidth();
        int h = vf.getHeight();
        for(int i = 0; i < pointCount; i++){
            if(pointsData[i * 3 + 2] == 0)
                continue;
            float x = (float) Math.round(textureCoordinates[2 * i] * w);
            float y = (float) Math.round(textureCoordinates[2 * i + 1] * h);
            if(x <= 0 || y <= 0 || x >= w || y >= h)
                continue;
            long texIndex = (long) (x + y * w);
            for(int j = 0; j < 3; j++)
                texture[i*4+j] = textureData[(int) (texIndex * 3 + j)];
            texture[i*4+3] = (byte) 255;
        }
        return texture;
    }

    @Override
    public synchronized void close() {
        if(mFrame != null)
            mFrame.close();
        if(mGlTexture != null)
            GLES10.glDeleteTextures(1, mGlTexture);
        mGlTexture = null;
    }

    public synchronized void rotate(float deltaX, float deltaY) {
        mDeltaX += deltaX;
        mDeltaY += deltaY;
    }
}
