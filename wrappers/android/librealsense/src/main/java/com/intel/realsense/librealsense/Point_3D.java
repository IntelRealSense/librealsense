package com.intel.realsense.librealsense;

public class Point_3D {
    public float mX;
    public float mY;
    public float mZ;

    public Point_3D() {
        mX = 0.f;
        mY = 0.f;
        mZ = 0.f;
    }

    public Point_3D(float x, float y, float z) {
        mX = x;
        mY = y;
        mZ = z;
    }
}
