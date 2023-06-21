
//
// This file is auto-generated. Please don't modify it!
//
package org.opencv.features2d;

import java.util.ArrayList;
import java.util.List;
import org.opencv.core.Mat;
import org.opencv.core.MatOfPoint;
import org.opencv.core.MatOfRect;
import org.opencv.features2d.Feature2D;
import org.opencv.utils.Converters;

// C++: class MSER
//javadoc: MSER
public class MSER extends Feature2D {

    protected MSER(long addr) { super(addr); }


    //
    // C++: static Ptr_MSER create(int _delta = 5, int _min_area = 60, int _max_area = 14400, double _max_variation = 0.25, double _min_diversity = .2, int _max_evolution = 200, double _area_threshold = 1.01, double _min_margin = 0.003, int _edge_blur_size = 5)
    //

    //javadoc: MSER::create(_delta, _min_area, _max_area, _max_variation, _min_diversity, _max_evolution, _area_threshold, _min_margin, _edge_blur_size)
    public static MSER create(int _delta, int _min_area, int _max_area, double _max_variation, double _min_diversity, int _max_evolution, double _area_threshold, double _min_margin, int _edge_blur_size)
    {
        
        MSER retVal = new MSER(create_0(_delta, _min_area, _max_area, _max_variation, _min_diversity, _max_evolution, _area_threshold, _min_margin, _edge_blur_size));
        
        return retVal;
    }

    //javadoc: MSER::create()
    public static MSER create()
    {
        
        MSER retVal = new MSER(create_1());
        
        return retVal;
    }


    //
    // C++:  bool getPass2Only()
    //

    //javadoc: MSER::getPass2Only()
    public  boolean getPass2Only()
    {
        
        boolean retVal = getPass2Only_0(nativeObj);
        
        return retVal;
    }


    //
    // C++:  int getDelta()
    //

    //javadoc: MSER::getDelta()
    public  int getDelta()
    {
        
        int retVal = getDelta_0(nativeObj);
        
        return retVal;
    }


    //
    // C++:  int getMaxArea()
    //

    //javadoc: MSER::getMaxArea()
    public  int getMaxArea()
    {
        
        int retVal = getMaxArea_0(nativeObj);
        
        return retVal;
    }


    //
    // C++:  int getMinArea()
    //

    //javadoc: MSER::getMinArea()
    public  int getMinArea()
    {
        
        int retVal = getMinArea_0(nativeObj);
        
        return retVal;
    }


    //
    // C++:  void detectRegions(Mat image, vector_vector_Point& msers, vector_Rect& bboxes)
    //

    //javadoc: MSER::detectRegions(image, msers, bboxes)
    public  void detectRegions(Mat image, List<MatOfPoint> msers, MatOfRect bboxes)
    {
        Mat msers_mat = new Mat();
        Mat bboxes_mat = bboxes;
        detectRegions_0(nativeObj, image.nativeObj, msers_mat.nativeObj, bboxes_mat.nativeObj);
        Converters.Mat_to_vector_vector_Point(msers_mat, msers);
        msers_mat.release();
        return;
    }


    //
    // C++:  void setDelta(int delta)
    //

    //javadoc: MSER::setDelta(delta)
    public  void setDelta(int delta)
    {
        
        setDelta_0(nativeObj, delta);
        
        return;
    }


    //
    // C++:  void setMaxArea(int maxArea)
    //

    //javadoc: MSER::setMaxArea(maxArea)
    public  void setMaxArea(int maxArea)
    {
        
        setMaxArea_0(nativeObj, maxArea);
        
        return;
    }


    //
    // C++:  void setMinArea(int minArea)
    //

    //javadoc: MSER::setMinArea(minArea)
    public  void setMinArea(int minArea)
    {
        
        setMinArea_0(nativeObj, minArea);
        
        return;
    }


    //
    // C++:  void setPass2Only(bool f)
    //

    //javadoc: MSER::setPass2Only(f)
    public  void setPass2Only(boolean f)
    {
        
        setPass2Only_0(nativeObj, f);
        
        return;
    }


    @Override
    protected void finalize() throws Throwable {
        delete(nativeObj);
    }



    // C++: static Ptr_MSER create(int _delta = 5, int _min_area = 60, int _max_area = 14400, double _max_variation = 0.25, double _min_diversity = .2, int _max_evolution = 200, double _area_threshold = 1.01, double _min_margin = 0.003, int _edge_blur_size = 5)
    private static native long create_0(int _delta, int _min_area, int _max_area, double _max_variation, double _min_diversity, int _max_evolution, double _area_threshold, double _min_margin, int _edge_blur_size);
    private static native long create_1();

    // C++:  bool getPass2Only()
    private static native boolean getPass2Only_0(long nativeObj);

    // C++:  int getDelta()
    private static native int getDelta_0(long nativeObj);

    // C++:  int getMaxArea()
    private static native int getMaxArea_0(long nativeObj);

    // C++:  int getMinArea()
    private static native int getMinArea_0(long nativeObj);

    // C++:  void detectRegions(Mat image, vector_vector_Point& msers, vector_Rect& bboxes)
    private static native void detectRegions_0(long nativeObj, long image_nativeObj, long msers_mat_nativeObj, long bboxes_mat_nativeObj);

    // C++:  void setDelta(int delta)
    private static native void setDelta_0(long nativeObj, int delta);

    // C++:  void setMaxArea(int maxArea)
    private static native void setMaxArea_0(long nativeObj, int maxArea);

    // C++:  void setMinArea(int minArea)
    private static native void setMinArea_0(long nativeObj, int minArea);

    // C++:  void setPass2Only(bool f)
    private static native void setPass2Only_0(long nativeObj, boolean f);

    // native support for java finalize()
    private static native void delete(long nativeObj);

}
