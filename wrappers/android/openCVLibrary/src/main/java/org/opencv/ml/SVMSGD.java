
//
// This file is auto-generated. Please don't modify it!
//
package org.opencv.ml;

import org.opencv.core.Mat;
import org.opencv.core.TermCriteria;

// C++: class SVMSGD
//javadoc: SVMSGD
public class SVMSGD extends StatModel {

    protected SVMSGD(long addr) { super(addr); }


    public static final int
            SGD = 0,
            ASGD = 1,
            SOFT_MARGIN = 0,
            HARD_MARGIN = 1;


    //
    // C++:  Mat getWeights()
    //

    //javadoc: SVMSGD::getWeights()
    public  Mat getWeights()
    {
        
        Mat retVal = new Mat(getWeights_0(nativeObj));
        
        return retVal;
    }


    //
    // C++: static Ptr_SVMSGD create()
    //

    //javadoc: SVMSGD::create()
    public static SVMSGD create()
    {
        
        SVMSGD retVal = new SVMSGD(create_0());
        
        return retVal;
    }


    //
    // C++:  TermCriteria getTermCriteria()
    //

    //javadoc: SVMSGD::getTermCriteria()
    public  TermCriteria getTermCriteria()
    {
        
        TermCriteria retVal = new TermCriteria(getTermCriteria_0(nativeObj));
        
        return retVal;
    }


    //
    // C++:  float getInitialStepSize()
    //

    //javadoc: SVMSGD::getInitialStepSize()
    public  float getInitialStepSize()
    {
        
        float retVal = getInitialStepSize_0(nativeObj);
        
        return retVal;
    }


    //
    // C++:  float getMarginRegularization()
    //

    //javadoc: SVMSGD::getMarginRegularization()
    public  float getMarginRegularization()
    {
        
        float retVal = getMarginRegularization_0(nativeObj);
        
        return retVal;
    }


    //
    // C++:  float getShift()
    //

    //javadoc: SVMSGD::getShift()
    public  float getShift()
    {
        
        float retVal = getShift_0(nativeObj);
        
        return retVal;
    }


    //
    // C++:  float getStepDecreasingPower()
    //

    //javadoc: SVMSGD::getStepDecreasingPower()
    public  float getStepDecreasingPower()
    {
        
        float retVal = getStepDecreasingPower_0(nativeObj);
        
        return retVal;
    }


    //
    // C++:  int getMarginType()
    //

    //javadoc: SVMSGD::getMarginType()
    public  int getMarginType()
    {
        
        int retVal = getMarginType_0(nativeObj);
        
        return retVal;
    }


    //
    // C++:  int getSvmsgdType()
    //

    //javadoc: SVMSGD::getSvmsgdType()
    public  int getSvmsgdType()
    {
        
        int retVal = getSvmsgdType_0(nativeObj);
        
        return retVal;
    }


    //
    // C++:  void setInitialStepSize(float InitialStepSize)
    //

    //javadoc: SVMSGD::setInitialStepSize(InitialStepSize)
    public  void setInitialStepSize(float InitialStepSize)
    {
        
        setInitialStepSize_0(nativeObj, InitialStepSize);
        
        return;
    }


    //
    // C++:  void setMarginRegularization(float marginRegularization)
    //

    //javadoc: SVMSGD::setMarginRegularization(marginRegularization)
    public  void setMarginRegularization(float marginRegularization)
    {
        
        setMarginRegularization_0(nativeObj, marginRegularization);
        
        return;
    }


    //
    // C++:  void setMarginType(int marginType)
    //

    //javadoc: SVMSGD::setMarginType(marginType)
    public  void setMarginType(int marginType)
    {
        
        setMarginType_0(nativeObj, marginType);
        
        return;
    }


    //
    // C++:  void setOptimalParameters(int svmsgdType = SVMSGD::ASGD, int marginType = SVMSGD::SOFT_MARGIN)
    //

    //javadoc: SVMSGD::setOptimalParameters(svmsgdType, marginType)
    public  void setOptimalParameters(int svmsgdType, int marginType)
    {
        
        setOptimalParameters_0(nativeObj, svmsgdType, marginType);
        
        return;
    }

    //javadoc: SVMSGD::setOptimalParameters()
    public  void setOptimalParameters()
    {
        
        setOptimalParameters_1(nativeObj);
        
        return;
    }


    //
    // C++:  void setStepDecreasingPower(float stepDecreasingPower)
    //

    //javadoc: SVMSGD::setStepDecreasingPower(stepDecreasingPower)
    public  void setStepDecreasingPower(float stepDecreasingPower)
    {
        
        setStepDecreasingPower_0(nativeObj, stepDecreasingPower);
        
        return;
    }


    //
    // C++:  void setSvmsgdType(int svmsgdType)
    //

    //javadoc: SVMSGD::setSvmsgdType(svmsgdType)
    public  void setSvmsgdType(int svmsgdType)
    {
        
        setSvmsgdType_0(nativeObj, svmsgdType);
        
        return;
    }


    //
    // C++:  void setTermCriteria(TermCriteria val)
    //

    //javadoc: SVMSGD::setTermCriteria(val)
    public  void setTermCriteria(TermCriteria val)
    {
        
        setTermCriteria_0(nativeObj, val.type, val.maxCount, val.epsilon);
        
        return;
    }


    @Override
    protected void finalize() throws Throwable {
        delete(nativeObj);
    }



    // C++:  Mat getWeights()
    private static native long getWeights_0(long nativeObj);

    // C++: static Ptr_SVMSGD create()
    private static native long create_0();

    // C++:  TermCriteria getTermCriteria()
    private static native double[] getTermCriteria_0(long nativeObj);

    // C++:  float getInitialStepSize()
    private static native float getInitialStepSize_0(long nativeObj);

    // C++:  float getMarginRegularization()
    private static native float getMarginRegularization_0(long nativeObj);

    // C++:  float getShift()
    private static native float getShift_0(long nativeObj);

    // C++:  float getStepDecreasingPower()
    private static native float getStepDecreasingPower_0(long nativeObj);

    // C++:  int getMarginType()
    private static native int getMarginType_0(long nativeObj);

    // C++:  int getSvmsgdType()
    private static native int getSvmsgdType_0(long nativeObj);

    // C++:  void setInitialStepSize(float InitialStepSize)
    private static native void setInitialStepSize_0(long nativeObj, float InitialStepSize);

    // C++:  void setMarginRegularization(float marginRegularization)
    private static native void setMarginRegularization_0(long nativeObj, float marginRegularization);

    // C++:  void setMarginType(int marginType)
    private static native void setMarginType_0(long nativeObj, int marginType);

    // C++:  void setOptimalParameters(int svmsgdType = SVMSGD::ASGD, int marginType = SVMSGD::SOFT_MARGIN)
    private static native void setOptimalParameters_0(long nativeObj, int svmsgdType, int marginType);
    private static native void setOptimalParameters_1(long nativeObj);

    // C++:  void setStepDecreasingPower(float stepDecreasingPower)
    private static native void setStepDecreasingPower_0(long nativeObj, float stepDecreasingPower);

    // C++:  void setSvmsgdType(int svmsgdType)
    private static native void setSvmsgdType_0(long nativeObj, int svmsgdType);

    // C++:  void setTermCriteria(TermCriteria val)
    private static native void setTermCriteria_0(long nativeObj, int val_type, int val_maxCount, double val_epsilon);

    // native support for java finalize()
    private static native void delete(long nativeObj);

}
