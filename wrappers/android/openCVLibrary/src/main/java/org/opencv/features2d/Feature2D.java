
//
// This file is auto-generated. Please don't modify it!
//
package org.opencv.features2d;

import java.lang.String;
import java.util.ArrayList;
import java.util.List;
import org.opencv.core.Algorithm;
import org.opencv.core.Mat;
import org.opencv.core.MatOfKeyPoint;
import org.opencv.utils.Converters;

// C++: class Feature2D
//javadoc: Feature2D
public class Feature2D extends Algorithm {

    protected Feature2D(long addr) { super(addr); }


    //
    // C++:  bool empty()
    //

    //javadoc: Feature2D::empty()
    public  boolean empty()
    {
        
        boolean retVal = empty_0(nativeObj);
        
        return retVal;
    }


    //
    // C++:  int defaultNorm()
    //

    //javadoc: Feature2D::defaultNorm()
    public  int defaultNorm()
    {
        
        int retVal = defaultNorm_0(nativeObj);
        
        return retVal;
    }


    //
    // C++:  int descriptorSize()
    //

    //javadoc: Feature2D::descriptorSize()
    public  int descriptorSize()
    {
        
        int retVal = descriptorSize_0(nativeObj);
        
        return retVal;
    }


    //
    // C++:  int descriptorType()
    //

    //javadoc: Feature2D::descriptorType()
    public  int descriptorType()
    {
        
        int retVal = descriptorType_0(nativeObj);
        
        return retVal;
    }


    //
    // C++:  void compute(Mat image, vector_KeyPoint& keypoints, Mat& descriptors)
    //

    //javadoc: Feature2D::compute(image, keypoints, descriptors)
    public  void compute(Mat image, MatOfKeyPoint keypoints, Mat descriptors)
    {
        Mat keypoints_mat = keypoints;
        compute_0(nativeObj, image.nativeObj, keypoints_mat.nativeObj, descriptors.nativeObj);
        
        return;
    }


    //
    // C++:  void compute(vector_Mat images, vector_vector_KeyPoint& keypoints, vector_Mat& descriptors)
    //

    //javadoc: Feature2D::compute(images, keypoints, descriptors)
    public  void compute(List<Mat> images, List<MatOfKeyPoint> keypoints, List<Mat> descriptors)
    {
        Mat images_mat = Converters.vector_Mat_to_Mat(images);
        List<Mat> keypoints_tmplm = new ArrayList<Mat>((keypoints != null) ? keypoints.size() : 0);
        Mat keypoints_mat = Converters.vector_vector_KeyPoint_to_Mat(keypoints, keypoints_tmplm);
        Mat descriptors_mat = new Mat();
        compute_1(nativeObj, images_mat.nativeObj, keypoints_mat.nativeObj, descriptors_mat.nativeObj);
        Converters.Mat_to_vector_vector_KeyPoint(keypoints_mat, keypoints);
        keypoints_mat.release();
        Converters.Mat_to_vector_Mat(descriptors_mat, descriptors);
        descriptors_mat.release();
        return;
    }


    //
    // C++:  void detect(Mat image, vector_KeyPoint& keypoints, Mat mask = Mat())
    //

    //javadoc: Feature2D::detect(image, keypoints, mask)
    public  void detect(Mat image, MatOfKeyPoint keypoints, Mat mask)
    {
        Mat keypoints_mat = keypoints;
        detect_0(nativeObj, image.nativeObj, keypoints_mat.nativeObj, mask.nativeObj);
        
        return;
    }

    //javadoc: Feature2D::detect(image, keypoints)
    public  void detect(Mat image, MatOfKeyPoint keypoints)
    {
        Mat keypoints_mat = keypoints;
        detect_1(nativeObj, image.nativeObj, keypoints_mat.nativeObj);
        
        return;
    }


    //
    // C++:  void detect(vector_Mat images, vector_vector_KeyPoint& keypoints, vector_Mat masks = vector_Mat())
    //

    //javadoc: Feature2D::detect(images, keypoints, masks)
    public  void detect(List<Mat> images, List<MatOfKeyPoint> keypoints, List<Mat> masks)
    {
        Mat images_mat = Converters.vector_Mat_to_Mat(images);
        Mat keypoints_mat = new Mat();
        Mat masks_mat = Converters.vector_Mat_to_Mat(masks);
        detect_2(nativeObj, images_mat.nativeObj, keypoints_mat.nativeObj, masks_mat.nativeObj);
        Converters.Mat_to_vector_vector_KeyPoint(keypoints_mat, keypoints);
        keypoints_mat.release();
        return;
    }

    //javadoc: Feature2D::detect(images, keypoints)
    public  void detect(List<Mat> images, List<MatOfKeyPoint> keypoints)
    {
        Mat images_mat = Converters.vector_Mat_to_Mat(images);
        Mat keypoints_mat = new Mat();
        detect_3(nativeObj, images_mat.nativeObj, keypoints_mat.nativeObj);
        Converters.Mat_to_vector_vector_KeyPoint(keypoints_mat, keypoints);
        keypoints_mat.release();
        return;
    }


    //
    // C++:  void detectAndCompute(Mat image, Mat mask, vector_KeyPoint& keypoints, Mat& descriptors, bool useProvidedKeypoints = false)
    //

    //javadoc: Feature2D::detectAndCompute(image, mask, keypoints, descriptors, useProvidedKeypoints)
    public  void detectAndCompute(Mat image, Mat mask, MatOfKeyPoint keypoints, Mat descriptors, boolean useProvidedKeypoints)
    {
        Mat keypoints_mat = keypoints;
        detectAndCompute_0(nativeObj, image.nativeObj, mask.nativeObj, keypoints_mat.nativeObj, descriptors.nativeObj, useProvidedKeypoints);
        
        return;
    }

    //javadoc: Feature2D::detectAndCompute(image, mask, keypoints, descriptors)
    public  void detectAndCompute(Mat image, Mat mask, MatOfKeyPoint keypoints, Mat descriptors)
    {
        Mat keypoints_mat = keypoints;
        detectAndCompute_1(nativeObj, image.nativeObj, mask.nativeObj, keypoints_mat.nativeObj, descriptors.nativeObj);
        
        return;
    }


    //
    // C++:  void read(String fileName)
    //

    //javadoc: Feature2D::read(fileName)
    public  void read(String fileName)
    {
        
        read_0(nativeObj, fileName);
        
        return;
    }


    //
    // C++:  void write(String fileName)
    //

    //javadoc: Feature2D::write(fileName)
    public  void write(String fileName)
    {
        
        write_0(nativeObj, fileName);
        
        return;
    }


    @Override
    protected void finalize() throws Throwable {
        delete(nativeObj);
    }



    // C++:  bool empty()
    private static native boolean empty_0(long nativeObj);

    // C++:  int defaultNorm()
    private static native int defaultNorm_0(long nativeObj);

    // C++:  int descriptorSize()
    private static native int descriptorSize_0(long nativeObj);

    // C++:  int descriptorType()
    private static native int descriptorType_0(long nativeObj);

    // C++:  void compute(Mat image, vector_KeyPoint& keypoints, Mat& descriptors)
    private static native void compute_0(long nativeObj, long image_nativeObj, long keypoints_mat_nativeObj, long descriptors_nativeObj);

    // C++:  void compute(vector_Mat images, vector_vector_KeyPoint& keypoints, vector_Mat& descriptors)
    private static native void compute_1(long nativeObj, long images_mat_nativeObj, long keypoints_mat_nativeObj, long descriptors_mat_nativeObj);

    // C++:  void detect(Mat image, vector_KeyPoint& keypoints, Mat mask = Mat())
    private static native void detect_0(long nativeObj, long image_nativeObj, long keypoints_mat_nativeObj, long mask_nativeObj);
    private static native void detect_1(long nativeObj, long image_nativeObj, long keypoints_mat_nativeObj);

    // C++:  void detect(vector_Mat images, vector_vector_KeyPoint& keypoints, vector_Mat masks = vector_Mat())
    private static native void detect_2(long nativeObj, long images_mat_nativeObj, long keypoints_mat_nativeObj, long masks_mat_nativeObj);
    private static native void detect_3(long nativeObj, long images_mat_nativeObj, long keypoints_mat_nativeObj);

    // C++:  void detectAndCompute(Mat image, Mat mask, vector_KeyPoint& keypoints, Mat& descriptors, bool useProvidedKeypoints = false)
    private static native void detectAndCompute_0(long nativeObj, long image_nativeObj, long mask_nativeObj, long keypoints_mat_nativeObj, long descriptors_nativeObj, boolean useProvidedKeypoints);
    private static native void detectAndCompute_1(long nativeObj, long image_nativeObj, long mask_nativeObj, long keypoints_mat_nativeObj, long descriptors_nativeObj);

    // C++:  void read(String fileName)
    private static native void read_0(long nativeObj, String fileName);

    // C++:  void write(String fileName)
    private static native void write_0(long nativeObj, String fileName);

    // native support for java finalize()
    private static native void delete(long nativeObj);

}
