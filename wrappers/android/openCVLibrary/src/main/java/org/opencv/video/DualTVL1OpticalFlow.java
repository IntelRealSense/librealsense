
//
// This file is auto-generated. Please don't modify it!
//
package org.opencv.video;



// C++: class DualTVL1OpticalFlow
//javadoc: DualTVL1OpticalFlow
public class DualTVL1OpticalFlow extends DenseOpticalFlow {

    protected DualTVL1OpticalFlow(long addr) { super(addr); }


    //
    // C++: static Ptr_DualTVL1OpticalFlow create(double tau = 0.25, double lambda = 0.15, double theta = 0.3, int nscales = 5, int warps = 5, double epsilon = 0.01, int innnerIterations = 30, int outerIterations = 10, double scaleStep = 0.8, double gamma = 0.0, int medianFiltering = 5, bool useInitialFlow = false)
    //

    //javadoc: DualTVL1OpticalFlow::create(tau, lambda, theta, nscales, warps, epsilon, innnerIterations, outerIterations, scaleStep, gamma, medianFiltering, useInitialFlow)
    public static DualTVL1OpticalFlow create(double tau, double lambda, double theta, int nscales, int warps, double epsilon, int innnerIterations, int outerIterations, double scaleStep, double gamma, int medianFiltering, boolean useInitialFlow)
    {
        
        DualTVL1OpticalFlow retVal = new DualTVL1OpticalFlow(create_0(tau, lambda, theta, nscales, warps, epsilon, innnerIterations, outerIterations, scaleStep, gamma, medianFiltering, useInitialFlow));
        
        return retVal;
    }

    //javadoc: DualTVL1OpticalFlow::create()
    public static DualTVL1OpticalFlow create()
    {
        
        DualTVL1OpticalFlow retVal = new DualTVL1OpticalFlow(create_1());
        
        return retVal;
    }


    //
    // C++:  bool getUseInitialFlow()
    //

    //javadoc: DualTVL1OpticalFlow::getUseInitialFlow()
    public  boolean getUseInitialFlow()
    {
        
        boolean retVal = getUseInitialFlow_0(nativeObj);
        
        return retVal;
    }


    //
    // C++:  double getEpsilon()
    //

    //javadoc: DualTVL1OpticalFlow::getEpsilon()
    public  double getEpsilon()
    {
        
        double retVal = getEpsilon_0(nativeObj);
        
        return retVal;
    }


    //
    // C++:  double getGamma()
    //

    //javadoc: DualTVL1OpticalFlow::getGamma()
    public  double getGamma()
    {
        
        double retVal = getGamma_0(nativeObj);
        
        return retVal;
    }


    //
    // C++:  double getLambda()
    //

    //javadoc: DualTVL1OpticalFlow::getLambda()
    public  double getLambda()
    {
        
        double retVal = getLambda_0(nativeObj);
        
        return retVal;
    }


    //
    // C++:  double getScaleStep()
    //

    //javadoc: DualTVL1OpticalFlow::getScaleStep()
    public  double getScaleStep()
    {
        
        double retVal = getScaleStep_0(nativeObj);
        
        return retVal;
    }


    //
    // C++:  double getTau()
    //

    //javadoc: DualTVL1OpticalFlow::getTau()
    public  double getTau()
    {
        
        double retVal = getTau_0(nativeObj);
        
        return retVal;
    }


    //
    // C++:  double getTheta()
    //

    //javadoc: DualTVL1OpticalFlow::getTheta()
    public  double getTheta()
    {
        
        double retVal = getTheta_0(nativeObj);
        
        return retVal;
    }


    //
    // C++:  int getInnerIterations()
    //

    //javadoc: DualTVL1OpticalFlow::getInnerIterations()
    public  int getInnerIterations()
    {
        
        int retVal = getInnerIterations_0(nativeObj);
        
        return retVal;
    }


    //
    // C++:  int getMedianFiltering()
    //

    //javadoc: DualTVL1OpticalFlow::getMedianFiltering()
    public  int getMedianFiltering()
    {
        
        int retVal = getMedianFiltering_0(nativeObj);
        
        return retVal;
    }


    //
    // C++:  int getOuterIterations()
    //

    //javadoc: DualTVL1OpticalFlow::getOuterIterations()
    public  int getOuterIterations()
    {
        
        int retVal = getOuterIterations_0(nativeObj);
        
        return retVal;
    }


    //
    // C++:  int getScalesNumber()
    //

    //javadoc: DualTVL1OpticalFlow::getScalesNumber()
    public  int getScalesNumber()
    {
        
        int retVal = getScalesNumber_0(nativeObj);
        
        return retVal;
    }


    //
    // C++:  int getWarpingsNumber()
    //

    //javadoc: DualTVL1OpticalFlow::getWarpingsNumber()
    public  int getWarpingsNumber()
    {
        
        int retVal = getWarpingsNumber_0(nativeObj);
        
        return retVal;
    }


    //
    // C++:  void setEpsilon(double val)
    //

    //javadoc: DualTVL1OpticalFlow::setEpsilon(val)
    public  void setEpsilon(double val)
    {
        
        setEpsilon_0(nativeObj, val);
        
        return;
    }


    //
    // C++:  void setGamma(double val)
    //

    //javadoc: DualTVL1OpticalFlow::setGamma(val)
    public  void setGamma(double val)
    {
        
        setGamma_0(nativeObj, val);
        
        return;
    }


    //
    // C++:  void setInnerIterations(int val)
    //

    //javadoc: DualTVL1OpticalFlow::setInnerIterations(val)
    public  void setInnerIterations(int val)
    {
        
        setInnerIterations_0(nativeObj, val);
        
        return;
    }


    //
    // C++:  void setLambda(double val)
    //

    //javadoc: DualTVL1OpticalFlow::setLambda(val)
    public  void setLambda(double val)
    {
        
        setLambda_0(nativeObj, val);
        
        return;
    }


    //
    // C++:  void setMedianFiltering(int val)
    //

    //javadoc: DualTVL1OpticalFlow::setMedianFiltering(val)
    public  void setMedianFiltering(int val)
    {
        
        setMedianFiltering_0(nativeObj, val);
        
        return;
    }


    //
    // C++:  void setOuterIterations(int val)
    //

    //javadoc: DualTVL1OpticalFlow::setOuterIterations(val)
    public  void setOuterIterations(int val)
    {
        
        setOuterIterations_0(nativeObj, val);
        
        return;
    }


    //
    // C++:  void setScaleStep(double val)
    //

    //javadoc: DualTVL1OpticalFlow::setScaleStep(val)
    public  void setScaleStep(double val)
    {
        
        setScaleStep_0(nativeObj, val);
        
        return;
    }


    //
    // C++:  void setScalesNumber(int val)
    //

    //javadoc: DualTVL1OpticalFlow::setScalesNumber(val)
    public  void setScalesNumber(int val)
    {
        
        setScalesNumber_0(nativeObj, val);
        
        return;
    }


    //
    // C++:  void setTau(double val)
    //

    //javadoc: DualTVL1OpticalFlow::setTau(val)
    public  void setTau(double val)
    {
        
        setTau_0(nativeObj, val);
        
        return;
    }


    //
    // C++:  void setTheta(double val)
    //

    //javadoc: DualTVL1OpticalFlow::setTheta(val)
    public  void setTheta(double val)
    {
        
        setTheta_0(nativeObj, val);
        
        return;
    }


    //
    // C++:  void setUseInitialFlow(bool val)
    //

    //javadoc: DualTVL1OpticalFlow::setUseInitialFlow(val)
    public  void setUseInitialFlow(boolean val)
    {
        
        setUseInitialFlow_0(nativeObj, val);
        
        return;
    }


    //
    // C++:  void setWarpingsNumber(int val)
    //

    //javadoc: DualTVL1OpticalFlow::setWarpingsNumber(val)
    public  void setWarpingsNumber(int val)
    {
        
        setWarpingsNumber_0(nativeObj, val);
        
        return;
    }


    @Override
    protected void finalize() throws Throwable {
        delete(nativeObj);
    }



    // C++: static Ptr_DualTVL1OpticalFlow create(double tau = 0.25, double lambda = 0.15, double theta = 0.3, int nscales = 5, int warps = 5, double epsilon = 0.01, int innnerIterations = 30, int outerIterations = 10, double scaleStep = 0.8, double gamma = 0.0, int medianFiltering = 5, bool useInitialFlow = false)
    private static native long create_0(double tau, double lambda, double theta, int nscales, int warps, double epsilon, int innnerIterations, int outerIterations, double scaleStep, double gamma, int medianFiltering, boolean useInitialFlow);
    private static native long create_1();

    // C++:  bool getUseInitialFlow()
    private static native boolean getUseInitialFlow_0(long nativeObj);

    // C++:  double getEpsilon()
    private static native double getEpsilon_0(long nativeObj);

    // C++:  double getGamma()
    private static native double getGamma_0(long nativeObj);

    // C++:  double getLambda()
    private static native double getLambda_0(long nativeObj);

    // C++:  double getScaleStep()
    private static native double getScaleStep_0(long nativeObj);

    // C++:  double getTau()
    private static native double getTau_0(long nativeObj);

    // C++:  double getTheta()
    private static native double getTheta_0(long nativeObj);

    // C++:  int getInnerIterations()
    private static native int getInnerIterations_0(long nativeObj);

    // C++:  int getMedianFiltering()
    private static native int getMedianFiltering_0(long nativeObj);

    // C++:  int getOuterIterations()
    private static native int getOuterIterations_0(long nativeObj);

    // C++:  int getScalesNumber()
    private static native int getScalesNumber_0(long nativeObj);

    // C++:  int getWarpingsNumber()
    private static native int getWarpingsNumber_0(long nativeObj);

    // C++:  void setEpsilon(double val)
    private static native void setEpsilon_0(long nativeObj, double val);

    // C++:  void setGamma(double val)
    private static native void setGamma_0(long nativeObj, double val);

    // C++:  void setInnerIterations(int val)
    private static native void setInnerIterations_0(long nativeObj, int val);

    // C++:  void setLambda(double val)
    private static native void setLambda_0(long nativeObj, double val);

    // C++:  void setMedianFiltering(int val)
    private static native void setMedianFiltering_0(long nativeObj, int val);

    // C++:  void setOuterIterations(int val)
    private static native void setOuterIterations_0(long nativeObj, int val);

    // C++:  void setScaleStep(double val)
    private static native void setScaleStep_0(long nativeObj, double val);

    // C++:  void setScalesNumber(int val)
    private static native void setScalesNumber_0(long nativeObj, int val);

    // C++:  void setTau(double val)
    private static native void setTau_0(long nativeObj, double val);

    // C++:  void setTheta(double val)
    private static native void setTheta_0(long nativeObj, double val);

    // C++:  void setUseInitialFlow(bool val)
    private static native void setUseInitialFlow_0(long nativeObj, boolean val);

    // C++:  void setWarpingsNumber(int val)
    private static native void setWarpingsNumber_0(long nativeObj, int val);

    // native support for java finalize()
    private static native void delete(long nativeObj);

}
