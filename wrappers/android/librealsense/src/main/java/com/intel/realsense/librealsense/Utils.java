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


    private native static void nProjectPointToPixel(Pixel pixel, Intrinsic intrinsic, Point_3D point_3D);
    private native static void nDeprojectPixelToPoint(Point_3D point_3D, Intrinsic intrinsic, Pixel pixel, float depth);
    private native static void nTransformPointToPoint(Point_3D to_point_3D, Extrinsic extrinsic, Point_3D from_point_3D);
}
