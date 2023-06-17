
//
// This file is auto-generated. Please don't modify it!
//
package org.opencv.features2d;

import org.opencv.features2d.Feature2D;

// C++: class AgastFeatureDetector
//javadoc: AgastFeatureDetector
public class AgastFeatureDetector extends Feature2D {

    protected AgastFeatureDetector(long addr) { super(addr); }


    public static final int
            AGAST_5_8 = 0,
            AGAST_7_12d = 1,
            AGAST_7_12s = 2,
            OAST_9_16 = 3,
            THRESHOLD = 10000,
            NONMAX_SUPPRESSION = 10001;


    //
    // C++: static Ptr_AgastFeatureDetector create(int threshold = 10, bool nonmaxSuppression = true, int type = AgastFeatureDetector::OAST_9_16)
    //

    //javadoc: AgastFeatureDetector::create(threshold, nonmaxSuppression, type)
    public static AgastFeatureDetector create(int threshold, boolean nonmaxSuppression, int type)
    {
        
        AgastFeatureDetector retVal = new AgastFeatureDetector(create_0(threshold, nonmaxSuppression, type));
        
        return retVal;
    }

    //javadoc: AgastFeatureDetector::create()
    public static AgastFeatureDetector create()
    {
        
        AgastFeatureDetector retVal = new AgastFeatureDetector(create_1());
        
        return retVal;
    }


    //
    // C++:  bool getNonmaxSuppression()
    //

    //javadoc: AgastFeatureDetector::getNonmaxSuppression()
    public  boolean getNonmaxSuppression()
    {
        
        boolean retVal = getNonmaxSuppression_0(nativeObj);
        
        return retVal;
    }


    //
    // C++:  int getThreshold()
    //

    //javadoc: AgastFeatureDetector::getThreshold()
    public  int getThreshold()
    {
        
        int retVal = getThreshold_0(nativeObj);
        
        return retVal;
    }


    //
    // C++:  int getType()
    //

    //javadoc: AgastFeatureDetector::getType()
    public  int getType()
    {
        
        int retVal = getType_0(nativeObj);
        
        return retVal;
    }


    //
    // C++:  void setNonmaxSuppression(bool f)
    //

    //javadoc: AgastFeatureDetector::setNonmaxSuppression(f)
    public  void setNonmaxSuppression(boolean f)
    {
        
        setNonmaxSuppression_0(nativeObj, f);
        
        return;
    }


    //
    // C++:  void setThreshold(int threshold)
    //

    //javadoc: AgastFeatureDetector::setThreshold(threshold)
    public  void setThreshold(int threshold)
    {
        
        setThreshold_0(nativeObj, threshold);
        
        return;
    }


    //
    // C++:  void setType(int type)
    //

    //javadoc: AgastFeatureDetector::setType(type)
    public  void setType(int type)
    {
        
        setType_0(nativeObj, type);
        
        return;
    }


    @Override
    protected void finalize() throws Throwable {
        delete(nativeObj);
    }



    // C++: static Ptr_AgastFeatureDetector create(int threshold = 10, bool nonmaxSuppression = true, int type = AgastFeatureDetector::OAST_9_16)
    private static native long create_0(int threshold, boolean nonmaxSuppression, int type);
    private static native long create_1();

    // C++:  bool getNonmaxSuppression()
    private static native boolean getNonmaxSuppression_0(long nativeObj);

    // C++:  int getThreshold()
    private static native int getThreshold_0(long nativeObj);

    // C++:  int getType()
    private static native int getType_0(long nativeObj);

    // C++:  void setNonmaxSuppression(bool f)
    private static native void setNonmaxSuppression_0(long nativeObj, boolean f);

    // C++:  void setThreshold(int threshold)
    private static native void setThreshold_0(long nativeObj, int threshold);

    // C++:  void setType(int type)
    private static native void setType_0(long nativeObj, int type);

    // native support for java finalize()
    private static native void delete(long nativeObj);

}
