package com.intel.realsense.librealsense;


public class Utils {

     // Given a point in 3D space, compute the corresponding pixel coordinates in an image
    // with no distortion or forward distortion coefficients produced by the same camera
    public static Pixel projectPointToPixel(final Intrinsic intrinsic, final Point_3D point) {
        Pixel rv = new Pixel();
        nProjectPointToPixel(rv, intrinsic, point);
        return rv;
    }

    // Given pixel coordinates and depth in an image with no distortion or inverse distortion coefficients,
    // compute the corresponding point in 3D space relative to the same camera
    public static Point_3D deprojectPixelToPoint(final Intrinsic intrinsic, final Pixel pixel, final float depth) {
        Point_3D rv = new Point_3D();
        nDeprojectPixelToPoint(rv, intrinsic, pixel, depth);
        return rv;
    }

    // Transform 3D coordinates relative to one sensor to 3D coordinates relative to another viewpoint
    public static Point_3D transformPointToPoint(final Extrinsic extrinsic, final Point_3D from_point) {
        Point_3D rv = new Point_3D();
        nTransformPointToPoint(rv, extrinsic, from_point);
        return rv;
    }

    private static class Fov {
        public float mFovX;
        public float mFovY;

        public Fov() {
            mFovX = 0.f;
            mFovY = 0.f;
        }
    }

    public static float[] getFov(final Intrinsic intrinsic) {
        float[] rv = new float[2];
        Fov fov = new Fov();
        nGetFov(fov, intrinsic);
        rv[0] = fov.mFovX;
        rv[1] = fov.mFovY;
        return rv;
    }

    public static Pixel project2dPixelToDepthPixel(final DepthFrame depthFrame, float depthScale,
                                      float depthMin, float depthMax,
                                      final Intrinsic depthIntrinsic, final Intrinsic otherIntrinsic,
                                      final Extrinsic otherToDepth, final Extrinsic depthToOther,
                                      final Pixel fromPixel) {
        Pixel rv = new Pixel();
        nProject2dPixelToDepthPixel(rv, depthFrame.getHandle(), depthScale, depthMin, depthMax,
                depthIntrinsic, otherIntrinsic, otherToDepth, depthToOther, fromPixel);
        return rv;
    }


    private native static void nProjectPointToPixel(Pixel pixel, Intrinsic intrinsic, Point_3D point_3D);
    private native static void nDeprojectPixelToPoint(Point_3D point_3D, Intrinsic intrinsic, Pixel pixel, float depth);
    private native static void nTransformPointToPoint(Point_3D to_point_3D, Extrinsic extrinsic, Point_3D from_point_3D);
    private native static void nGetFov(Fov fov, Intrinsic intrinsic);
    private native static void nProject2dPixelToDepthPixel(Pixel toPixel, long depthFrameHandle, float depthScale,
                                                           float depthMin, float depthMax,
                                                           Intrinsic depthIntrinsic, Intrinsic otherIntrinsic,
                                                           Extrinsic otherToDepth, Extrinsic depthToOther,
                                                           Pixel fromPixel);
}
