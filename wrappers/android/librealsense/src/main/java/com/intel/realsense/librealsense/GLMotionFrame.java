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

    private void  drawCircle(float radius, Float3 x, Float3 y, float axisWidth, Color color, Boolean dashed)
    {
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

        drawLines(verArray, axisWidth, color, dashed);
    }

    private void  drawLines(float[] verArray, float axisWidth, Color color, Boolean dashed)
    {
        ByteBuffer ver = ByteBuffer.allocateDirect(verArray.length * 4);
        ver.order(ByteOrder.nativeOrder());
        ver.asFloatBuffer().put(verArray);

        GLES10.glEnableClientState(GLES10.GL_VERTEX_ARRAY);
        GLES10.glColor4f(color.red, color.green, color.blue, 1f);
        GLES10.glLineWidth(axisWidth);

        GLES10.glVertexPointer(3, GLES10.GL_FLOAT, 0, ver);
        GLES10.glDrawArrays(dashed ? GLES10.GL_LINES: GLES10.GL_LINE_LOOP,0, verArray.length / 3);

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

        drawLines(verAxisX, axisWidth, red, false);
        drawTriangle(verTriangleX, red);
        drawLines(verAxisY, axisWidth, green, false);
        drawTriangle(verTriangleY, green);
        drawLines(verAxisZ, axisWidth, blue, false);
        drawTriangle(verTriangleZ, blue);
    }

    private Float3 normalizeMotionData(Float3 md, float norm){
        return new Float3(
                (md.x / norm),
                (md.y / norm),
                (md.z / norm));
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

        drawCircle(axisSize, X, Y, 1, white, true);
        drawCircle(axisSize, Y, Z, 1, white, true);
        drawCircle(axisSize, X, Z, 1, white, true);
        drawAxes(r, axisSize, axisWidth);

        // draw norm vector
        MotionFrame mf = mFrame.as(Extension.MOTION_FRAME);

        Float3 md = mf.getMotionData();
        float norm = (float) Math.sqrt(md.x * md.x + md.y * md.y + md.z * md.z);

        float vecThreshold = 0.2f;

        // If the absolute value of the motion vector is less than predefined `vecThreshold` meaning zero / noise values, draw a centered dot
        if ( norm < vecThreshold ) {
            drawCircle(0.05f, X, Y,  7, white, false);
        }
        else{
            // Display the motion vector line
            Float3 nmd = normalizeMotionData(mf.getMotionData(), norm);
            float[] verArray = {0, 0, 0, axisSize * nmd.x, axisSize * nmd.y, axisSize * nmd.z};
            drawLines(verArray, axisWidth, white, true);
        }

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
