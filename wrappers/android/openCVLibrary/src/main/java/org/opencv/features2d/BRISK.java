
//
// This file is auto-generated. Please don't modify it!
//
package org.opencv.features2d;

import java.util.ArrayList;
import org.opencv.core.Mat;
import org.opencv.core.MatOfFloat;
import org.opencv.core.MatOfInt;
import org.opencv.features2d.Feature2D;

// C++: class BRISK
//javadoc: BRISK
public class BRISK extends Feature2D {

    protected BRISK(long addr) { super(addr); }


    //
    // C++: static Ptr_BRISK create(int thresh = 30, int octaves = 3, float patternScale = 1.0f)
    //

    //javadoc: BRISK::create(thresh, octaves, patternScale)
    public static BRISK create(int thresh, int octaves, float patternScale)
    {
        
        BRISK retVal = new BRISK(create_0(thresh, octaves, patternScale));
        
        return retVal;
    }

    //javadoc: BRISK::create()
    public static BRISK create()
    {
        
        BRISK retVal = new BRISK(create_1());
        
        return retVal;
    }


    //
    // C++: static Ptr_BRISK create(vector_float radiusList, vector_int numberList, float dMax = 5.85f, float dMin = 8.2f, vector_int indexChange = std::vector<int>())
    //

    //javadoc: BRISK::create(radiusList, numberList, dMax, dMin, indexChange)
    public static BRISK create(MatOfFloat radiusList, MatOfInt numberList, float dMax, float dMin, MatOfInt indexChange)
    {
        Mat radiusList_mat = radiusList;
        Mat numberList_mat = numberList;
        Mat indexChange_mat = indexChange;
        BRISK retVal = new BRISK(create_2(radiusList_mat.nativeObj, numberList_mat.nativeObj, dMax, dMin, indexChange_mat.nativeObj));
        
        return retVal;
    }

    //javadoc: BRISK::create(radiusList, numberList)
    public static BRISK create(MatOfFloat radiusList, MatOfInt numberList)
    {
        Mat radiusList_mat = radiusList;
        Mat numberList_mat = numberList;
        BRISK retVal = new BRISK(create_3(radiusList_mat.nativeObj, numberList_mat.nativeObj));
        
        return retVal;
    }


    @Override
    protected void finalize() throws Throwable {
        delete(nativeObj);
    }



    // C++: static Ptr_BRISK create(int thresh = 30, int octaves = 3, float patternScale = 1.0f)
    private static native long create_0(int thresh, int octaves, float patternScale);
    private static native long create_1();

    // C++: static Ptr_BRISK create(vector_float radiusList, vector_int numberList, float dMax = 5.85f, float dMin = 8.2f, vector_int indexChange = std::vector<int>())
    private static native long create_2(long radiusList_mat_nativeObj, long numberList_mat_nativeObj, float dMax, float dMin, long indexChange_mat_nativeObj);
    private static native long create_3(long radiusList_mat_nativeObj, long numberList_mat_nativeObj);

    // native support for java finalize()
    private static native void delete(long nativeObj);

}
