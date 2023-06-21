
//
// This file is auto-generated. Please don't modify it!
//
package org.opencv.features2d;

import org.opencv.features2d.Feature2D;

// C++: class GFTTDetector
//javadoc: GFTTDetector
public class GFTTDetector extends Feature2D {

    protected GFTTDetector(long addr) { super(addr); }


    //
    // C++: static Ptr_GFTTDetector create(int maxCorners = 1000, double qualityLevel = 0.01, double minDistance = 1, int blockSize = 3, bool useHarrisDetector = false, double k = 0.04)
    //

    //javadoc: GFTTDetector::create(maxCorners, qualityLevel, minDistance, blockSize, useHarrisDetector, k)
    public static GFTTDetector create(int maxCorners, double qualityLevel, double minDistance, int blockSize, boolean useHarrisDetector, double k)
    {
        
        GFTTDetector retVal = new GFTTDetector(create_0(maxCorners, qualityLevel, minDistance, blockSize, useHarrisDetector, k));
        
        return retVal;
    }

    //javadoc: GFTTDetector::create()
    public static GFTTDetector create()
    {
        
        GFTTDetector retVal = new GFTTDetector(create_1());
        
        return retVal;
    }


    //
    // C++:  bool getHarrisDetector()
    //

    //javadoc: GFTTDetector::getHarrisDetector()
    public  boolean getHarrisDetector()
    {
        
        boolean retVal = getHarrisDetector_0(nativeObj);
        
        return retVal;
    }


    //
    // C++:  double getK()
    //

    //javadoc: GFTTDetector::getK()
    public  double getK()
    {
        
        double retVal = getK_0(nativeObj);
        
        return retVal;
    }


    //
    // C++:  double getMinDistance()
    //

    //javadoc: GFTTDetector::getMinDistance()
    public  double getMinDistance()
    {
        
        double retVal = getMinDistance_0(nativeObj);
        
        return retVal;
    }


    //
    // C++:  double getQualityLevel()
    //

    //javadoc: GFTTDetector::getQualityLevel()
    public  double getQualityLevel()
    {
        
        double retVal = getQualityLevel_0(nativeObj);
        
        return retVal;
    }


    //
    // C++:  int getBlockSize()
    //

    //javadoc: GFTTDetector::getBlockSize()
    public  int getBlockSize()
    {
        
        int retVal = getBlockSize_0(nativeObj);
        
        return retVal;
    }


    //
    // C++:  int getMaxFeatures()
    //

    //javadoc: GFTTDetector::getMaxFeatures()
    public  int getMaxFeatures()
    {
        
        int retVal = getMaxFeatures_0(nativeObj);
        
        return retVal;
    }


    //
    // C++:  void setBlockSize(int blockSize)
    //

    //javadoc: GFTTDetector::setBlockSize(blockSize)
    public  void setBlockSize(int blockSize)
    {
        
        setBlockSize_0(nativeObj, blockSize);
        
        return;
    }


    //
    // C++:  void setHarrisDetector(bool val)
    //

    //javadoc: GFTTDetector::setHarrisDetector(val)
    public  void setHarrisDetector(boolean val)
    {
        
        setHarrisDetector_0(nativeObj, val);
        
        return;
    }


    //
    // C++:  void setK(double k)
    //

    //javadoc: GFTTDetector::setK(k)
    public  void setK(double k)
    {
        
        setK_0(nativeObj, k);
        
        return;
    }


    //
    // C++:  void setMaxFeatures(int maxFeatures)
    //

    //javadoc: GFTTDetector::setMaxFeatures(maxFeatures)
    public  void setMaxFeatures(int maxFeatures)
    {
        
        setMaxFeatures_0(nativeObj, maxFeatures);
        
        return;
    }


    //
    // C++:  void setMinDistance(double minDistance)
    //

    //javadoc: GFTTDetector::setMinDistance(minDistance)
    public  void setMinDistance(double minDistance)
    {
        
        setMinDistance_0(nativeObj, minDistance);
        
        return;
    }


    //
    // C++:  void setQualityLevel(double qlevel)
    //

    //javadoc: GFTTDetector::setQualityLevel(qlevel)
    public  void setQualityLevel(double qlevel)
    {
        
        setQualityLevel_0(nativeObj, qlevel);
        
        return;
    }


    @Override
    protected void finalize() throws Throwable {
        delete(nativeObj);
    }



    // C++: static Ptr_GFTTDetector create(int maxCorners = 1000, double qualityLevel = 0.01, double minDistance = 1, int blockSize = 3, bool useHarrisDetector = false, double k = 0.04)
    private static native long create_0(int maxCorners, double qualityLevel, double minDistance, int blockSize, boolean useHarrisDetector, double k);
    private static native long create_1();

    // C++:  bool getHarrisDetector()
    private static native boolean getHarrisDetector_0(long nativeObj);

    // C++:  double getK()
    private static native double getK_0(long nativeObj);

    // C++:  double getMinDistance()
    private static native double getMinDistance_0(long nativeObj);

    // C++:  double getQualityLevel()
    private static native double getQualityLevel_0(long nativeObj);

    // C++:  int getBlockSize()
    private static native int getBlockSize_0(long nativeObj);

    // C++:  int getMaxFeatures()
    private static native int getMaxFeatures_0(long nativeObj);

    // C++:  void setBlockSize(int blockSize)
    private static native void setBlockSize_0(long nativeObj, int blockSize);

    // C++:  void setHarrisDetector(bool val)
    private static native void setHarrisDetector_0(long nativeObj, boolean val);

    // C++:  void setK(double k)
    private static native void setK_0(long nativeObj, double k);

    // C++:  void setMaxFeatures(int maxFeatures)
    private static native void setMaxFeatures_0(long nativeObj, int maxFeatures);

    // C++:  void setMinDistance(double minDistance)
    private static native void setMinDistance_0(long nativeObj, double minDistance);

    // C++:  void setQualityLevel(double qlevel)
    private static native void setQualityLevel_0(long nativeObj, double qlevel);

    // native support for java finalize()
    private static native void delete(long nativeObj);

}
