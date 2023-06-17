package com.intel.realsense.librealsense;

import android.graphics.Rect;
import android.opengl.GLES10;
import android.renderscript.Float3;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.IntBuffer;

public class GLMotionFrame extends GLFrame {
    private IntBuffer mGlTexture;

    final private Float3 X = new Float3(1,0,0);
    final private Float3 Y = new Float3(0,1,0);
    final private Float3 Z = new Float3(0,0,1);

    public int getTexture() { return mGlTexture.array()[0]; }

    class Color{
        public Color(float r, float g, float b){
            red = r;
            green = g;
            blue = b;
        }
        public float red, green, blue;
    }

    private Rect adjustRatio(Rect in){
        float ratio = 4f/3f;
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

    private void  drawCircle(Float3 x, Float3 y, float axisWidth, Color color)
    {
        float radius = 2f;
        Float3 center = new Float3(0,0,0);
        final int N = 50;
        float[] verArray = new float[N * 3];
        for (int i = 0; i < N; i++)
        {
            final double theta = (2* Math.PI / N) * i;
            final double cost = Math.cos(theta);
            final double sint = Math.sin(theta);
            verArray[i*3] = ((float) (center.x + radius * (x.x * cost + y.x * sint)));
            verArray[i*3 + 1] = ((float) (center.y + radius * (x.y * cost + y.y * sint)));
            verArray[i*3 + 2] = ((float) (center.z + radius * (x.z * cost + y.z * sint)));
        }

        drawLine(verArray, axisWidth, color);
    }

    private void  drawLine(float[] verArray, float axisWidth, Color color)
    {
        ByteBuffer ver = ByteBuffer.allocateDirect(verArray.length * 4);
        ver.order(ByteOrder.nativeOrder());
        ver.asFloatBuffer().put(verArray);

        GLES10.glEnableClientState(GLES10.GL_VERTEX_ARRAY);

        GLES10.glColor4f(color.red, color.green, color.blue, 1f);
        GLES10.glLineWidth(axisWidth);

        GLES10.glVertexPointer(3, GLES10.GL_FLOAT, 0, ver);

        GLES10.glDrawArrays(GLES10.GL_LINES,0,verArray.length / 3);

        GLES10.glDisableClientState(GLES10.GL_VERTEX_ARRAY);
        GLES10.glColor4f(1f, 1f, 1f, 1f);
    }

    private void  drawTriangle(float[] verArray, Color color)
    {
        ByteBuffer ver = ByteBuffer.allocateDirect(verArray.length * 4);
        ver.order(ByteOrder.nativeOrder());
        ver.asFloatBuffer().put(verArray);

        GLES10.glEnableClientState(GLES10.GL_VERTEX_ARRAY);

        GLES10.glColor4f(color.red, color.green, color.blue, 1f);

        GLES10.glVertexPointer(3, GLES10.GL_FLOAT, 0, ver);

        GLES10.glDrawArrays(GLES10.GL_TRIANGLES,0,verArray.length / 3);

        GLES10.glDisableClientState(GLES10.GL_VERTEX_ARRAY);
        GLES10.glColor4f(1f, 1f, 1f, 1f);
    }

    private void  drawAxes(Rect rect, float axisSize, float axisWidth)
    {
        float baseSize = 0.05f * axisSize;
        float basePos = 0.9f * axisSize;

        float[] verAxisX = { 0, 0, 0, axisSize, 0, 0 };
        float[] verTriangleX = {
                axisSize, 0, 0,
                basePos, baseSize, 0,
                basePos, -baseSize, 0
        };
        float[] verAxisY = { 0, 0, 0, 0, axisSize, 0 };
        float[] verTriangleY = {
                0, axisSize, 0,
                baseSize, basePos, 0,
                -baseSize, basePos, 0
        };
        float[] verAxisZ = { 0, 0, 0, 0, 0, axisSize };
        float[] verTriangleZ = {
                0, 0, axisSize,
                0, baseSize, basePos,
                0, -baseSize, basePos
        };
        Color red = new Color(0.5f, 0, 0);
        Color green = new Color(0, 0.5f, 0);
        Color blue = new Color(0, 0, 0.5f);

        drawLine(verAxisX, axisWidth, red);
        drawTriangle(verTriangleX, red);
        drawLine(verAxisY, axisWidth, green);
        drawTriangle(verTriangleY, green);
        drawLine(verAxisZ, axisWidth, blue);
        drawTriangle(verTriangleZ, blue);
    }

    private Float3 calcMotionData(Float3 md, float norm){
        float size = (float) Math.sqrt(md.x * md.x + md.y * md.y + md.z * md.z);
        return new Float3(
                (md.x / size) * norm,
                (md.y / size) * norm,
                (md.z / size) * norm);
    }

    @Override
    public synchronized void draw(Rect rect)
    {
        if (mFrame == null || !(mFrame.is(Extension.MOTION_FRAME)))
            return;

        Rect r = adjustRatio(rect);
        setViewport(r);

        GLES10.glMatrixMode(GLES10.GL_PROJECTION);
        GLES10.glPushMatrix();
        GLES10.glLoadIdentity();

        GLES10.glOrthof(-2.8f, 2.8f, -2.4f, 2.4f, -7f, 7f);

        GLES10.glRotatef(25, 1.0f, 0.0f, 0.0f);

        GLES10.glTranslatef(0, -0.33f, -1.f);

        GLES10.glRotatef(-135, 0.0f, 1.0f, 0.0f);
        GLES10.glRotatef(180, 0.0f, 0.0f, 1.0f);
        GLES10.glRotatef(-90, 0.0f, 1.0f, 0.0f);

        float axisSize = 2;
        float axisWidth = 5;
        Color white = new Color(1, 1, 1);

        drawCircle(X, Y, 1, white);
        drawCircle(Y, Z, 1, white);
        drawCircle(X, Z, 1, white);
        drawAxes(r, axisSize, axisWidth);

        MotionFrame mf = mFrame.as(Extension.MOTION_FRAME);

        float norm = axisSize / 2.f;
        Float3 md = calcMotionData(mf.getMotionData(), norm);

        float[] verArray = { 0, 0, 0, md.x, md.y, md.z};
        Color dir = new Color(Math.abs(md.x/norm), Math.abs(md.y/norm), Math.abs(md.z/norm));

        drawLine(verArray, axisWidth, dir);

        GLES10.glMatrixMode(GLES10.GL_PROJECTION);
        GLES10.glPopMatrix();
    }

    @Override
    public synchronized String getLabel() {
        MotionFrame mf = mFrame.as(Extension.MOTION_FRAME);
        Float3 d = mf.getMotionData();
        try(StreamProfile sp = mFrame.getProfile()){
            return sp.getType().name() +
                    " [ X: " + String.format("%+.2f", d.x) +
                    ", Y: " + String.format("%+.2f", d.y) +
                    ", Z: " + String.format("%+.2f", d.z) + " ]";
        }
    }

    @Override
    public synchronized void close() {
        if(mFrame != null)
            mFrame.close();
        if(mGlTexture != null)
            GLES10.glDeleteTextures(1, mGlTexture);
        mGlTexture = null;
    }
}
