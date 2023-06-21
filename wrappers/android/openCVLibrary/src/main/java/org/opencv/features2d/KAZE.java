
//
// This file is auto-generated. Please don't modify it!
//
package org.opencv.features2d;

import org.opencv.features2d.Feature2D;

// C++: class KAZE
//javadoc: KAZE
public class KAZE extends Feature2D {

    protected KAZE(long addr) { super(addr); }


    public static final int
            DIFF_PM_G1 = 0,
            DIFF_PM_G2 = 1,
            DIFF_WEICKERT = 2,
            DIFF_CHARBONNIER = 3;


    //
    // C++: static Ptr_KAZE create(bool extended = false, bool upright = false, float threshold = 0.001f, int nOctaves = 4, int nOctaveLayers = 4, int diffusivity = KAZE::DIFF_PM_G2)
    //

    //javadoc: KAZE::create(extended, upright, threshold, nOctaves, nOctaveLayers, diffusivity)
    public static KAZE create(boolean extended, boolean upright, float threshold, int nOctaves, int nOctaveLayers, int diffusivity)
    {
        
        KAZE retVal = new KAZE(create_0(extended, upright, threshold, nOctaves, nOctaveLayers, diffusivity));
        
        return retVal;
    }

    //javadoc: KAZE::create()
    public static KAZE create()
    {
        
        KAZE retVal = new KAZE(create_1());
        
        return retVal;
    }


    //
    // C++:  bool getExtended()
    //

    //javadoc: KAZE::getExtended()
    public  boolean getExtended()
    {
        
        boolean retVal = getExtended_0(nativeObj);
        
        return retVal;
    }


    //
    // C++:  bool getUpright()
    //

    //javadoc: KAZE::getUpright()
    public  boolean getUpright()
    {
        
        boolean retVal = getUpright_0(nativeObj);
        
        return retVal;
    }


    //
    // C++:  double getThreshold()
    //

    //javadoc: KAZE::getThreshold()
    public  double getThreshold()
    {
        
        double retVal = getThreshold_0(nativeObj);
        
        return retVal;
    }


    //
    // C++:  int getDiffusivity()
    //

    //javadoc: KAZE::getDiffusivity()
    public  int getDiffusivity()
    {
        
        int retVal = getDiffusivity_0(nativeObj);
        
        return retVal;
    }


    //
    // C++:  int getNOctaveLayers()
    //

    //javadoc: KAZE::getNOctaveLayers()
    public  int getNOctaveLayers()
    {
        
        int retVal = getNOctaveLayers_0(nativeObj);
        
        return retVal;
    }


    //
    // C++:  int getNOctaves()
    //

    //javadoc: KAZE::getNOctaves()
    public  int getNOctaves()
    {
        
        int retVal = getNOctaves_0(nativeObj);
        
        return retVal;
    }


    //
    // C++:  void setDiffusivity(int diff)
    //

    //javadoc: KAZE::setDiffusivity(diff)
    public  void setDiffusivity(int diff)
    {
        
        setDiffusivity_0(nativeObj, diff);
        
        return;
    }


    //
    // C++:  void setExtended(bool extended)
    //

    //javadoc: KAZE::setExtended(extended)
    public  void setExtended(boolean extended)
    {
        
        setExtended_0(nativeObj, extended);
        
        return;
    }


    //
    // C++:  void setNOctaveLayers(int octaveLayers)
    //

    //javadoc: KAZE::setNOctaveLayers(octaveLayers)
    public  void setNOctaveLayers(int octaveLayers)
    {
        
        setNOctaveLayers_0(nativeObj, octaveLayers);
        
        return;
    }


    //
    // C++:  void setNOctaves(int octaves)
    //

    //javadoc: KAZE::setNOctaves(octaves)
    public  void setNOctaves(int octaves)
    {
        
        setNOctaves_0(nativeObj, octaves);
        
        return;
    }


    //
    // C++:  void setThreshold(double threshold)
    //

    //javadoc: KAZE::setThreshold(threshold)
    public  void setThreshold(double threshold)
    {
        
        setThreshold_0(nativeObj, threshold);
        
        return;
    }


    //
    // C++:  void setUpright(bool upright)
    //

    //javadoc: KAZE::setUpright(upright)
    public  void setUpright(boolean upright)
    {
        
        setUpright_0(nativeObj, upright);
        
        return;
    }


    @Override
    protected void finalize() throws Throwable {
        delete(nativeObj);
    }



    // C++: static Ptr_KAZE create(bool extended = false, bool upright = false, float threshold = 0.001f, int nOctaves = 4, int nOctaveLayers = 4, int diffusivity = KAZE::DIFF_PM_G2)
    private static native long create_0(boolean extended, boolean upright, float threshold, int nOctaves, int nOctaveLayers, int diffusivity);
    private static native long create_1();

    // C++:  bool getExtended()
    private static native boolean getExtended_0(long nativeObj);

    // C++:  bool getUpright()
    private static native boolean getUpright_0(long nativeObj);

    // C++:  double getThreshold()
    private static native double getThreshold_0(long nativeObj);

    // C++:  int getDiffusivity()
    private static native int getDiffusivity_0(long nativeObj);

    // C++:  int getNOctaveLayers()
    private static native int getNOctaveLayers_0(long nativeObj);

    // C++:  int getNOctaves()
    private static native int getNOctaves_0(long nativeObj);

    // C++:  void setDiffusivity(int diff)
    private static native void setDiffusivity_0(long nativeObj, int diff);

    // C++:  void setExtended(bool extended)
    private static native void setExtended_0(long nativeObj, boolean extended);

    // C++:  void setNOctaveLayers(int octaveLayers)
    private static native void setNOctaveLayers_0(long nativeObj, int octaveLayers);

    // C++:  void setNOctaves(int octaves)
    private static native void setNOctaves_0(long nativeObj, int octaves);

    // C++:  void setThreshold(double threshold)
    private static native void setThreshold_0(long nativeObj, double threshold);

    // C++:  void setUpright(bool upright)
    private static native void setUpright_0(long nativeObj, boolean upright);

    // native support for java finalize()
    private static native void delete(long nativeObj);

}
