
//
// This file is auto-generated. Please don't modify it!
//
package org.opencv.video;

import org.opencv.core.Size;
import org.opencv.core.TermCriteria;

// C++: class SparsePyrLKOpticalFlow
//javadoc: SparsePyrLKOpticalFlow
public class SparsePyrLKOpticalFlow extends SparseOpticalFlow {

    protected SparsePyrLKOpticalFlow(long addr) { super(addr); }


    //
    // C++: static Ptr_SparsePyrLKOpticalFlow create(Size winSize = Size(21, 21), int maxLevel = 3, TermCriteria crit = TermCriteria(TermCriteria::COUNT+TermCriteria::EPS, 30, 0.01), int flags = 0, double minEigThreshold = 1e-4)
    //

    //javadoc: SparsePyrLKOpticalFlow::create(winSize, maxLevel, crit, flags, minEigThreshold)
    public static SparsePyrLKOpticalFlow create(Size winSize, int maxLevel, TermCriteria crit, int flags, double minEigThreshold)
    {
        
        SparsePyrLKOpticalFlow retVal = new SparsePyrLKOpticalFlow(create_0(winSize.width, winSize.height, maxLevel, crit.type, crit.maxCount, crit.epsilon, flags, minEigThreshold));
        
        return retVal;
    }

    //javadoc: SparsePyrLKOpticalFlow::create()
    public static SparsePyrLKOpticalFlow create()
    {
        
        SparsePyrLKOpticalFlow retVal = new SparsePyrLKOpticalFlow(create_1());
        
        return retVal;
    }


    //
    // C++:  Size getWinSize()
    //

    //javadoc: SparsePyrLKOpticalFlow::getWinSize()
    public  Size getWinSize()
    {
        
        Size retVal = new Size(getWinSize_0(nativeObj));
        
        return retVal;
    }


    //
    // C++:  TermCriteria getTermCriteria()
    //

    //javadoc: SparsePyrLKOpticalFlow::getTermCriteria()
    public  TermCriteria getTermCriteria()
    {
        
        TermCriteria retVal = new TermCriteria(getTermCriteria_0(nativeObj));
        
        return retVal;
    }


    //
    // C++:  double getMinEigThreshold()
    //

    //javadoc: SparsePyrLKOpticalFlow::getMinEigThreshold()
    public  double getMinEigThreshold()
    {
        
        double retVal = getMinEigThreshold_0(nativeObj);
        
        return retVal;
    }


    //
    // C++:  int getFlags()
    //

    //javadoc: SparsePyrLKOpticalFlow::getFlags()
    public  int getFlags()
    {
        
        int retVal = getFlags_0(nativeObj);
        
        return retVal;
    }


    //
    // C++:  int getMaxLevel()
    //

    //javadoc: SparsePyrLKOpticalFlow::getMaxLevel()
    public  int getMaxLevel()
    {
        
        int retVal = getMaxLevel_0(nativeObj);
        
        return retVal;
    }


    //
    // C++:  void setFlags(int flags)
    //

    //javadoc: SparsePyrLKOpticalFlow::setFlags(flags)
    public  void setFlags(int flags)
    {
        
        setFlags_0(nativeObj, flags);
        
        return;
    }


    //
    // C++:  void setMaxLevel(int maxLevel)
    //

    //javadoc: SparsePyrLKOpticalFlow::setMaxLevel(maxLevel)
    public  void setMaxLevel(int maxLevel)
    {
        
        setMaxLevel_0(nativeObj, maxLevel);
        
        return;
    }


    //
    // C++:  void setMinEigThreshold(double minEigThreshold)
    //

    //javadoc: SparsePyrLKOpticalFlow::setMinEigThreshold(minEigThreshold)
    public  void setMinEigThreshold(double minEigThreshold)
    {
        
        setMinEigThreshold_0(nativeObj, minEigThreshold);
        
        return;
    }


    //
    // C++:  void setTermCriteria(TermCriteria crit)
    //

    //javadoc: SparsePyrLKOpticalFlow::setTermCriteria(crit)
    public  void setTermCriteria(TermCriteria crit)
    {
        
        setTermCriteria_0(nativeObj, crit.type, crit.maxCount, crit.epsilon);
        
        return;
    }


    //
    // C++:  void setWinSize(Size winSize)
    //

    //javadoc: SparsePyrLKOpticalFlow::setWinSize(winSize)
    public  void setWinSize(Size winSize)
    {
        
        setWinSize_0(nativeObj, winSize.width, winSize.height);
        
        return;
    }


    @Override
    protected void finalize() throws Throwable {
        delete(nativeObj);
    }



    // C++: static Ptr_SparsePyrLKOpticalFlow create(Size winSize = Size(21, 21), int maxLevel = 3, TermCriteria crit = TermCriteria(TermCriteria::COUNT+TermCriteria::EPS, 30, 0.01), int flags = 0, double minEigThreshold = 1e-4)
    private static native long create_0(double winSize_width, double winSize_height, int maxLevel, int crit_type, int crit_maxCount, double crit_epsilon, int flags, double minEigThreshold);
    private static native long create_1();

    // C++:  Size getWinSize()
    private static native double[] getWinSize_0(long nativeObj);

    // C++:  TermCriteria getTermCriteria()
    private static native double[] getTermCriteria_0(long nativeObj);

    // C++:  double getMinEigThreshold()
    private static native double getMinEigThreshold_0(long nativeObj);

    // C++:  int getFlags()
    private static native int getFlags_0(long nativeObj);

    // C++:  int getMaxLevel()
    private static native int getMaxLevel_0(long nativeObj);

    // C++:  void setFlags(int flags)
    private static native void setFlags_0(long nativeObj, int flags);

    // C++:  void setMaxLevel(int maxLevel)
    private static native void setMaxLevel_0(long nativeObj, int maxLevel);

    // C++:  void setMinEigThreshold(double minEigThreshold)
    private static native void setMinEigThreshold_0(long nativeObj, double minEigThreshold);

    // C++:  void setTermCriteria(TermCriteria crit)
    private static native void setTermCriteria_0(long nativeObj, int crit_type, int crit_maxCount, double crit_epsilon);

    // C++:  void setWinSize(Size winSize)
    private static native void setWinSize_0(long nativeObj, double winSize_width, double winSize_height);

    // native support for java finalize()
    private static native void delete(long nativeObj);

}
