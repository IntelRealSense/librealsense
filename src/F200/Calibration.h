#pragma once

#ifndef LIBREALSENSE_F200_CALIBRATION_H
#define LIBREALSENSE_F200_CALIBRATION_H

#include "F200Types.h"

#include <vector>
#include <cmath>
#include <cstring>

namespace f200
{
    
#define NUM_OF_CALIBRATION_COEFFS   (64)
    
template <typename T = float>
class IVCAMCalibrator
{
    
public:
    
    IVCAMCalibrator()
    {
        
    }
    
    IVCAMCalibrator(const CameraCalibrationParameters & p)
    {
        buildParameters(p);
    }
    
    IVCAMCalibrator(const float * newParamData, int nParams)
    {
        buildParameters(newParamData, nParams);
    }
    
    IVCAMCalibrator(const double *  oldParamData, int nParams)
    {
        buildParameters(oldParamData, nParams);
    }
    
    static int width() { return 640; };
    static int height() { return 480; };
    static T defaultZmax() { return T(2047); }
    
    bool buildParameters(const CameraCalibrationParameters & p);
    bool buildParameters(const float * paramData, int nParams);
    bool buildParameters(const double * paramData, int nParams);
    
    void PrintParameters();
    
    void clear() { isInitialized = false; }
    
    //project: (x,y,z) -> (u,v,d)
    void project(T x, T y, T z, T& u, T& v, T& d) const;
    void project(T x, T y, T z, T& u, T& v, uint16_t & d) const;
    
    void unproject(int pixelX, int pixelY, uint16_t depth, T& x, T& y, T& z) const;
    void unproject(int pixelX, int pixelY, T depthInMM, T& x, T& y, T& z) const;
    
    void buildUV(T x, T y, T z, T& u, T& v) const;
    void buildUV_4to3(T x, T y, T z, T& u, T& v) const;
    
    void buildUV(int pixelX, int pixelY, uint16_t depth, T& u, T& v) const;
    void buildUV(int pixelX, int pixelY, T depthInMM, T& u, T& v) const;
    
    T convertU_16to9_to_4to3(T u) const;
    
    T ivcamToMM(uint16_t d) const { return T(d) * uint16_to_mm_ratio; }
    
    T getZMax() const { return params.Rmax; };
    
    operator bool() const { return isInitialized; }
    
    const CameraCalibrationParameters & getParameters()
    {
        if (isInitialized) return params;
        else return std::move(CameraCalibrationParameters());
    }
    
    int nParamters() const { return sizeof (CameraCalibrationParameters) / sizeof (T); }
    
    void InitializeThermalData(IVCAMTemperatureData TemperatureData, IVCAMThermalLoopParams ThermalLoopParams)
    {
        thermalModeData.Initialize(originalParams.Kc, TemperatureData, ThermalLoopParams);
    };
    
    void GetThermalData(IVCAMTemperatureData & TemperatureData, IVCAMThermalLoopParams & ThermalLoopParams)
    {
        TemperatureData = thermalModeData.BaseTemperatureData;
        ThermalLoopParams = thermalModeData.ThermalLoopParams;
    };
    
    OpticalData getOpticalData(const Resolution IRResolution, const Resolution RGBresolution);
    
    void generateCalibrationCoefficients(Resolution IRResolution,bool isZmode, float* ValArray) const;
    
    bool updateParamsAccordingToTemperature(float liguriaTemp,float IRTemp, int* timeout);
    
    struct Properties
    {
        struct CameraIntrinsics
        {
            T HFOV; // horizontal field of view
            T VFOV; // vertical field of view
            T X, Y; // principal point
            T Rad25, Rad75, Tang;
        };
        
        struct ProjectorIntrinsics
        {
            T HFOV; // horizontal field of view
            T X, Curv, Tilt, DTilt;
        };
        
        struct TextureIntrinsics
        {
            T HFOV; // horizontal field of view
            T VFOV; // vertical field of view
            T X, Y; // principal point
        };
        
        struct ProjectorExtrinsics
        {
            T Conv, Pitch, Roll, Disp, xDis, zDis;
        };
        
        struct TextureExtrinsics
        {
            T Conv, Pitch, Roll, Disp, xDis, zDis;
        };
        
        struct Viewpoint
        {
            T xRot, yRot, zRot, xDis, yDis, zDis;
        };
    };
    
    const Properties & GetCalibrationProperties() const { return camProperties; }
    
public:
    
    struct ThermalModelData
    {
        ThermalModelData() {}
        
        void Initialize(float Kc[3][3], IVCAMTemperatureData temperatureData, IVCAMThermalLoopParams thermalLoopParams)
        {
            
            BaseTemperatureData = temperatureData;
            ThermalLoopParams = thermalLoopParams;
            
            FcxSlope = Kc[0][0] * ThermalLoopParams.FcxSlopeA + ThermalLoopParams.FcxSlopeB;
            UxSlope = Kc[0][2] * ThermalLoopParams.UxSlopeA + Kc[0][0] * ThermalLoopParams.UxSlopeB + ThermalLoopParams.UxSlopeC;
            FcxOffset = ThermalLoopParams.FcxOffset;
            UxOffset = ThermalLoopParams.UxOffset;
            
            float tempFromHFOV = (tan(ThermalLoopParams.HFOVsensitivity*M_PI/360)*(1 + Kc[0][0]*Kc[0][0]))/(FcxSlope * (1 + Kc[0][0] * tan(ThermalLoopParams.HFOVsensitivity * M_PI/360)));
            TempThreshold = ThermalLoopParams.TempThreshold;
            
            if (TempThreshold <= 0)
            {
                TempThreshold = tempFromHFOV;
            }
            
            if(TempThreshold > tempFromHFOV)
            {
                TempThreshold = tempFromHFOV;
            }
        }
        
        IVCAMTemperatureData BaseTemperatureData;
        IVCAMThermalLoopParams ThermalLoopParams;
        
        float FcxSlope; // the temperature model calculated slope for fc
        float UxSlope; // the temperature model calculated slope for ux
        float FcxOffset; // the temperature model fc offset
        float UxOffset; // the temperature model ux offset
        
        float TempThreshold; //celcius degrees, the temperatures delta that above should be fixed;
    };
    
    ThermalModelData thermalModeData;
    
    bool buildParametersFromOldTable(const double * paramData, int nParams);
    void precomputeUnproj();
    void buildCameraProperties();
    
    CameraCalibrationParameters params;
    CameraCalibrationParameters originalParams;
    
    Properties camProperties;
    
    bool isInitialized = false;
    
    float lastTemperatureDelta;
    
    std::vector<Point> unprojCoeffs; // precomputed coefficients for unproject
    std::vector<UVPreComp> buildUVCoeffs; // precomputed coefficients for buildUV
    
    float uint16_to_mm_ratio;
    float mm_to_uint_ratio;
    
    Point computeUnprojectCoeffs(int x, int y) const;
    UVPreComp computeBuildUVCoeffs(const Point& r) const;
};

////////////////////////////////////
// IVCAMCalibrator Implementation //
////////////////////////////////////
    
template <typename T>
inline bool IVCAMCalibrator<T>::updateParamsAccordingToTemperature(float liguriaTemp, float IRTemp, int * timeout)
{
    if (!thermalModeData.ThermalLoopParams.IRThermalLoopEnable)
    {
        // *time = 999999; Dimitri commented out
        return false;
    }
    
    bool isUpdated = false;
    
    //inistialize TO variable to default
    *time= thermalModeData.ThermalLoopParams.TimeOutA;
    
    double IrBaseTemperature = thermalModeData.BaseTemperatureData.IRTemp; //should be taken from the parameters
    double liguriaBaseTemperature = thermalModeData.BaseTemperatureData.LiguriaTemp; //should be taken from the parameters
    
    //calculate deltas from the calibration and last fix
    double IrTempDelta = IRTemp - IrBaseTemperature;
    double liguriaTempDelta = liguriaTemp - liguriaBaseTemperature;
    double weightedTempDelta = liguriaTempDelta * thermalModeData.ThermalLoopParams.LiguriaTempWeight + IrTempDelta * thermalModeData.ThermalLoopParams.IrTempWeight;
    double tempDetaFromLastFix = abs(weightedTempDelta - lastTemperatureDelta);
    
    //read intrinsic from the calibration working point
    double Kc11 = originalParams.Kc[0][0];
    double Kc13 = originalParams.Kc[0][2];
    
    // Apply model
    if (tempDetaFromLastFix >= thermalModeData.TempThreshold)
    {
        //if we are during a transition, fix for after the transition
        double tempDeltaToUse = weightedTempDelta;
        if (tempDeltaToUse > 0 && tempDeltaToUse < thermalModeData.ThermalLoopParams.TransitionTemp)
        {
            tempDeltaToUse =  thermalModeData.ThermalLoopParams.TransitionTemp;
        }
        
        //calculate fixed values
        double fixed_Kc11 = Kc11 + (thermalModeData.FcxSlope * tempDeltaToUse) + thermalModeData.FcxOffset;
        double fixed_Kc13 = Kc13 + (thermalModeData.UxSlope * tempDeltaToUse) + thermalModeData.UxOffset;
        
        //write back to intrinsic hfov and vfov
        params.Kc[0][0] = (float) fixed_Kc11;
        params.Kc[1][1] = originalParams.Kc[1][1] * (float)(fixed_Kc11/Kc11);
        params.Kc[0][2] = (float) fixed_Kc13;
        isUpdated = true;
        lastTemperatureDelta = weightedTempDelta;
    }
    
    return isUpdated;
}

template <typename T>
inline void IVCAMCalibrator<T>::generateCalibrationCoefficients(Resolution IRResolution,bool isZmode, float* ValArray) const
{
    //handle vertical crop at 360p - change to full 640x480 and crop at the end
    bool Res360p = IRResolution.width == 640 && IRResolution.height == 360;
    
    if (Res360p)
    {
        IRResolution.height = 480;
    }
    
    //generate coefficients
    int scale = 5;
    
    float width  = (float)IRResolution.width*scale;
    float height  = (float)IRResolution.height;
    
    int PrecisionBits = 16;
    int CodeBits = 14;
    int TexturePrecisionBits = 12;
    float ypscale = (1<<(CodeBits+1-10));
    float ypoff = 0;
    
    float s1 = (float)(1<<(PrecisionBits)) / params.Rmax;
    float s2 = (float)(1<<(CodeBits))-ypscale*0.5f;
    
    float alpha = 2/(width*params.Kc[0][0]);
    float beta  = -(params.Kc[0][2]+1)/params.Kc[0][0];
    float gamma = 2/(height*params.Kc[1][1]);
    float delta = -(params.Kc[1][2]+1)/params.Kc[1][1];
    
    float a  = alpha/gamma;
    float a1 = 1;
    float b  = 0.5f*scale*a  + beta/gamma;
    float c  = 0.5f*a1 + delta/gamma;
    
    float d0 = 1;
    float d1 = params.Invdistc[0]*pow(gamma,(float)2.0);
    float d2 = params.Invdistc[1]*pow(gamma,(float)4.0);
    float d5 = (float)( (double)(params.Invdistc[4])*pow((double)gamma,6.0) );
    float d3 = params.Invdistc[2]*gamma;
    float d4 = params.Invdistc[3]*gamma;
    
    float q  = 1/pow(gamma,(float)2.0);
    float p1 = params.Pp[2][3]*s1;
    float p2 = -s1*s2*(params.Pp[1][3]+params.Pp[2][3]);
    
    if (isZmode)
    {
        p1 = p1*sqrt(q);
        p2 = p2*sqrt(q);
    }
    
    float p3 = -params.Pp[2][0];
    float p4 = -params.Pp[2][1];
    float p5 = -params.Pp[2][2]/gamma;
    float p6 = s2*(params.Pp[1][0]+params.Pp[2][0]);
    float p7 = s2*(params.Pp[1][1]+params.Pp[2][1]);
    float p8 = s2*(params.Pp[1][2]+params.Pp[2][2])/gamma;
    
    //Reprojection parameters
    float sreproj = 2;
    float ax = -(1+params.Kp[0][2])/params.Kp[0][0];
    float ay = -(1+params.Kp[1][2])/params.Kp[1][1];
    
    float f0 = (params.Pp[0][1]+params.Pp[2][1]) / (params.Pp[0][0]+params.Pp[2][0]) / params.Kp[0][0];
    float f1 = (params.Pp[0][2]+params.Pp[2][2]) / (params.Pp[0][0]+params.Pp[2][0]) / gamma / params.Kp[0][0];
    float f2 = 0; //(Pp(2,1)+Pp(3,1)) / (Pp(1,1)+Pp(3,1)) / Kp(5);
    float f3 = 0; //(Pp(2,2)+Pp(3,2)) / (Pp(1,1)+Pp(3,1)) / Kp(5);
    float f4 = 0; //(Pp(2,3)+Pp(3,3)) / (Pp(1,1)+Pp(3,1)) / gamma / Kp(5);
    float f5 = 2*params.Pp[2][0] / (params.Pp[0][0]+params.Pp[2][0]) / sreproj;
    float f6 = 2*params.Pp[2][1] / (params.Pp[0][0]+params.Pp[2][0]) / sreproj;
    float f7 = 2*params.Pp[2][2] / (params.Pp[0][0]+params.Pp[2][0]) / sreproj / gamma;
    float f8 = (double)(params.Pp[0][3] + params.Pp[2][3]) / (params.Pp[0][0]+params.Pp[2][0]) * s1 / params.Kp[0][0];
    float f9 = (params.Pp[1][3] + params.Pp[2][3]) / (params.Pp[0][0]+params.Pp[2][0]) * s1 / params.Kp[1][1];
    float f10 = 2*params.Pp[2][3] / (params.Pp[0][0]+params.Pp[2][0]) * s1 / sreproj;
    
    if (isZmode)
    {
        f8  = f8  * sqrt(q);
        f9  = 0; //f9  * sqrt(q);
        f10 = f10 * sqrt(q);
    }
    float f11 = 1 / params.Kp[0][0];
    
    // Fix projection center
    f11 = f11 + ax*f5;
    f0  = f0  + ax*f6;
    f1  = f1  + ax*f7;
    f8  = f8  + ax*f10;
    f2  = f2  + ay*f5;
    f3  = f3  + ay*f6;
    f4  = f4  + ay*f7;
    f9  = f9  + ay*f10;
    
    // Texture coeffs
    float suv = (1<<TexturePrecisionBits)-1;
    
    float h0 =  (params.Pt[0][1] +  params.Pt[2][1]) / (params.Pt[0][0]+params.Pt[2][0]);
    float h1 =  (params.Pt[0][2] +  params.Pt[2][2]) / (params.Pt[0][0]+params.Pt[2][0]) / gamma;
    float h2 =  (params.Pt[1][0] +  params.Pt[2][0]) / (params.Pt[0][0]+params.Pt[2][0]);
    float h3 =  (params.Pt[1][1] +  params.Pt[2][1]) / (params.Pt[0][0]+params.Pt[2][0]);
    float h4 =  (params.Pt[1][2] +  params.Pt[2][2]) / (params.Pt[0][0]+params.Pt[2][0]) / gamma;
    float h5 = 2*params.Pt[2][0] / (params.Pt[0][0]  +  params.Pt[2][0]) / suv;
    float h6 = 2*params.Pt[2][1] / (params.Pt[0][0]  +  params.Pt[2][0]) / suv;
    float h7 = 2*params.Pt[2][2] / (params.Pt[0][0]  +  params.Pt[2][0]) / suv / gamma;
    float h8 =  (params.Pt[0][3] +  params.Pt[2][3]) / (params.Pt[0][0]+params.Pt[2][0]) * s1;
    float h9 =  (params.Pt[1][3] +  params.Pt[2][3]) / (params.Pt[0][0]+params.Pt[2][0]) * s1;
    float h10 = 2*params.Pt[2][3] / (params.Pt[0][0]+params.Pt[2][0]) * s1 / suv;
    float h11 = 1;
    
    if (isZmode)
    {
        h8  = h8  * sqrt(q);
        h9  = h9  * sqrt(q);
        h10 = h10 * sqrt(q);
    }
    
    float o1 =  (1+params.Kp[0][2])/params.Kp[0][0];
    float o2 = -(1+params.Kp[1][2])/params.Kp[1][1];
    float o3 = 1/s2/params.Kp[1][1];
    float o4 = 0; //s2*(1+Kp(8));
    
    float dp1 = params.Distp[0];
    float dp2 = params.Distp[1];
    float dp3 = params.Distp[2];
    float dp4 = params.Distp[3];
    float dp5 = params.Distp[4];
    
    float ip0 = params.Kp[1][1]*s2;
    float ip1 = (s2*params.Invdistp[0]*params.Kp[1][1]) + s2*(1+params.Kp[1][2]);
    float ip2 = (s2*params.Invdistp[1]*params.Kp[1][1]);
    float ip3 = (s2*params.Invdistp[2]*params.Kp[1][1]);
    float ip4 = (s2*params.Invdistp[3]*params.Kp[1][1]);
    float ip5 = (s2*params.Invdistp[4]*params.Kp[1][1]);
    
    if (Res360p)
    {
        c *= 0.75;
    }
    
    //to simplify the ordering of the coefficients, initialize it in list syntax and copy to allocated memory.
    float coeffs [NUM_OF_CALIBRATION_COEFFS] = {(float)1.0,(float)3.0,a,a1,b,c,d0,d1,d2,
        d3,d4,d5,q,p1,p2,p3,p4,p5,p6,p7,p8,h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11,f0,f1,
        f2,f3,f4,f5,f6,f7,f8,f9,f10,f11,o1,o2,o3,o4,dp1,dp2,dp3,dp4,dp5,ip0,ip1,ip2,ip3,
        ip4,ip5,ypscale,ypoff,0,0};
    
    memcpy(ValArray,coeffs,NUM_OF_CALIBRATION_COEFFS * sizeof(float));
    return;
}

template <typename T>
inline OpticalData IVCAMCalibrator<T>::getOpticalData(Resolution IRResolution, Resolution RGBResolution)
{
    OpticalData data;
    
    //////////////////////////
    // Infrared Camera (IR) //
    //////////////////////////
    
    // compute undistorted IR FOV
    T tempX = atan(T(1)/params.Kc[0][0])*T(360)/T(M_PI);
    T tempY = atan(T(1)/params.Kc[1][1])*T(360)/T(M_PI);
    data.IRUndistortedFOV = FOV(tempX, tempY);
    
    // compute IR focal length
    tempX = T(IRResolution.width)*params.Kc[0][0]/T(2);
    tempY = T(IRResolution.height)*params.Kc[1][1]/T(2);
    data.IRUndistortedFocalLengthPxl = FocalLength(tempX, tempY);
    
    // compute distorted IR FOV
    T r[2] = {T(0.8), T(0.6)};
    T r2[2] = {r[0]*r[0],r[1]*r[1]};
    T R[2];
    for(int i = 0; i<2;i++)
    {
        R[i] = 1 + r[i]*(r2[i]*params.Invdistc[0] + r2[i]*r2[i]*params.Invdistc[1]+ r2[i]*r2[i]*r2[i]*params.Invdistc[4]);
    }
    tempX = atan(R[0]/params.Kc[0][0])*T(360)/T(M_PI);
    tempY = atan(R[1]/params.Kc[1][1])*T(360)/T(M_PI);
    data.IRDistortedFOV = FOV(tempX, tempY);
    
    // compute IR focal length
    tempX = T(IRResolution.width)*params.Kc[0][0]/(R[0]*T(2));
    tempY = T(IRResolution.height)*params.Kc[1][1]/(R[1]*T(2));
    data.IRDistortedFocalLengthPxl = FocalLength(tempX, tempY);
    
    // compute IR principal point
    tempX = T(IRResolution.width)*params.Kc[0][2]/T(2);
    tempY = T(IRResolution.height)*params.Kc[1][2]/T(2);
    data.IRPrincipalPoint = Point(tempX, tempY);
    
    ////////////////////////
    // Color Camera (RGB) //
    ////////////////////////
    
    T aspectRatioWanted = T(RGBResolution.width)/T(RGBResolution.height);
    T aspectRatioCalib = params.Kt[1][1]/params.Kt[0][0];
    T diff = std::fabs(aspectRatioWanted - aspectRatioCalib);
    T Kt00 = params.Kt[0][0];
    
    T Kt02 = params.Kt[0][2];
    if ( diff > T(M_EPSILON))
    {
        tempY = atan(T(1)/params.Kt[1][1])*T(360)/T(M_PI);
        tempX = tempY*aspectRatioWanted;
        Kt00 = T(1)/(tan(T(M_PI) * tempX / T(360)));
        Kt02 *= Kt00/params.Kt[0][0];
    }
    
    // compute undistorted RGB FOV
    tempX = atan(T(1)/Kt00)*T(360)/T(M_PI);
    tempY = atan(T(1)/params.Kt[1][1])*T(360)/T(M_PI);
    data.RGBUndistortedFOV = FOV(tempX,tempY);
    
    // compute RGB focal length
    tempX = T(RGBResolution.width)*Kt00/T(2);
    tempY = T(RGBResolution.height)*params.Kt[1][1]/T(2);
    data.RGBUndistortedFocalLengthPxl = FocalLength(tempX,tempY);
    
    // compute RGB principal point
    tempX = T(RGBResolution.width)*Kt02/T(2);
    tempY = T(RGBResolution.height)*params.Kt[1][2]/T(2);
    data.RGBPrincipalPoint = Point(tempX,tempY);
    
    return data;
}

template <typename T>
inline void IVCAMCalibrator<T>::PrintParameters()
{
    printf("\nBegin Calibration Parameters ########################\n");
    printf("# %f \n", params.Rmax);
    printf("# %f \n", params.Kc[0][0]);
    printf("# %f \n", params.Distc[0]);
    printf("# %f \n", params.Invdistc[0]);
    printf("# %f \n", params.Pp[0][0]);
    printf("# %f \n", params.Kp[0][0]);
    printf("# %f \n", params.Rp[0][0]);
    printf("# %f \n", params.Tp[0]);
    printf("# %f \n", params.Distp[0]);
    printf("# %f \n", params.Invdistp[0]);
    printf("# %f \n", params.Pt[0][0]);
    printf("# %f \n", params.Kt[0][0]);
    printf("# %f \n", params.Rt[0][0]);
    printf("# %f \n", params.Tt[0]);
    printf("# %f \n", params.Distt[0]);
    printf("# %f \n", params.Invdistt[0]);
    printf("# %f \n", params.QV[0]);
    printf("\nEnd Calibration Parameters ########################\n");
}

template <typename T>
inline bool IVCAMCalibrator<T>::buildParameters(const CameraCalibrationParameters & _params)
{
    params = _params;
    isInitialized = true;
    lastTemperatureDelta = DELTA_INF;
    std::memcpy(&originalParams,&params,sizeof(CameraCalibrationParameters));
    PrintParameters();
    precomputeUnproj();
    return true;
}

template <typename T>
inline bool IVCAMCalibrator<T>::buildParameters(const float* paramData, int nParams)
{
    memcpy(&params, paramData + 1, sizeof(CameraCalibrationParameters)); // skip the first float or 2 uint16
    isInitialized = true;
    lastTemperatureDelta = DELTA_INF;
    memcpy(&originalParams,&params,sizeof(CameraCalibrationParameters));
    precomputeUnproj();
    return true;
}

template <typename T>
inline bool IVCAMCalibrator<T>::buildParameters(const double* paramData, int nParams)
{
    buildParametersFromOldTable(paramData, nParams);
    lastTemperatureDelta = DELTA_INF;
    memcpy(&originalParams,&params,sizeof(CameraCalibrationParameters));
    precomputeUnproj();
    return true;
}

template <typename T>
inline bool IVCAMCalibrator<T>::buildParametersFromOldTable(const double* paramData, int nParams)
{
    const int kParamsCount = 61;
    
    if (nParams < kParamsCount)
        return false;
    
    int paramIdx=0;
    
    for (int i = 0; i != 3; i++)
        for(int j = 0; j != 3; j++)
            params.Kc[j][i] = float(paramData[paramIdx++]);
    
    for (int i=0; i != 5; i++)
        params.Distc[i] = float(paramData[paramIdx++]);
    for (int i=0; i != 5; i++)
        params.Invdistc[i] = float(paramData[paramIdx++]);
    
    paramIdx++; // skip Width
    paramIdx++; // skip Height
    params.Rmax = float(paramData[paramIdx++]);
    
    if (T(100) > params.Rmax || params.Rmax > T(5000)) // 200 or 2048 expected
        return false;
    
    for (int i = 0; i != 4; i++)
        for(int j = 0; j != 3; j++)
            params.Pp[j][i] = float(paramData[paramIdx++]);
    
    for (int i = 0; i != 3; i++)
        for(int j = 0; j != 3; j++)
            params.Kp[j][i] = float(paramData[paramIdx++]);
    
    for (int i = 0; i != 3; i++)
        for(int j = 0; j != 3; j++)
            params.Rp[j][i] = float(paramData[paramIdx++]);
    
    for (int i = 0; i != 3; i++)
        params.Tp[i] = float(paramData[paramIdx++]);
    
    for (int i = 0; i != 5; i++)
        params.Distp[i] = float(paramData[paramIdx++]);
    
    for (int i = 0; i != 5; i++)
        params.Invdistp[i] = float(paramData[paramIdx++]);
    
    for (int i = 0; i != 4; i++)
        for(int j = 0; j != 3; j++)
            params.Pt[j][i] = float(paramData[paramIdx++]);
    
    for (int i = 0; i != 5; i++)
        params.Distt[i] = float(paramData[paramIdx++]);
    
    isInitialized = true;
    
    // precompute pixel unproject coefficients
    
    if (T(100) <= params.Rmax && params.Rmax < T(500))
    {
        // Zmax in cm
        params.Rmax *= T(10); // converted to mm
        params.Pt[0][3] *= T(10); // converted to mm
        params.Pt[1][3] *= T(10); // converted to mm
        params.Pt[2][3] *= T(10); // converted to mm
        params.Pp[0][3] *= T(10); // converted to mm
        params.Pp[1][3] *= T(10); // converted to mm
        params.Pp[2][3] *= T(10); // converted to mm
    }
    else
    {
        // fix bug when Zmax in mm while tp, Pp, Tp are still in cm
        T ppMax = 0;
        for (int i = 0; i != 3; ++i)
            ppMax = max(ppMax, abs(params.Pp[0][i]));
        
        if (ppMax < T(10)) {
            params.Pt[0][3] *= T(10); // converted to mm
            params.Pt[1][3] *= T(10); // converted to mm
            params.Pt[2][3] *= T(10); // converted to mm
            params.Pp[0][3] *= T(10); // converted to mm
            params.Pp[1][3] *= T(10); // converted to mm
            params.Pp[2][3] *= T(10); // converted to mm
        }
    }
    return true;
}

template <typename T>
void IVCAMCalibrator<T>::precomputeUnproj()
{
    uint16_to_mm_ratio = params.Rmax / T(65535);
    mm_to_uint_ratio = T(65535) / params.Rmax;
    
    const int w = width();
    const int h = height();
    const int sz = w * h;
    
    if (unprojCoeffs.size() == 0)
    {
        unprojCoeffs.resize(sz);
        buildUVCoeffs.resize(sz);
    }
    
    for (int y = 0; y != h; ++y)
    {
        for (int x = 0; x != w; ++x)
        {
            const Point & p = computeUnprojectCoeffs(x, y);
            unprojCoeffs[y * w + x] = p;
            buildUVCoeffs[y * w + x] = computeBuildUVCoeffs(p);
        }
    }
}

template <typename T>
inline UVPreComp IVCAMCalibrator<T>::computeBuildUVCoeffs(const Point& r) const
{
    const CameraCalibrationParameters & p = params;
    const float u = p.Pt[0][0]*r.x + p.Pt[0][1]*r.y + p.Pt[0][2];
    const float v = p.Pt[1][0]*r.x + p.Pt[1][1]*r.y + p.Pt[1][2];
    const float d = p.Pt[2][0]*r.x + p.Pt[2][1]*r.y + p.Pt[2][2];
    return UVPreComp(u, v, d);
}

template <typename T>
inline void IVCAMCalibrator<T>::buildUV(int pixelX, int pixelY, uint16_t depth, T& u, T& v) const
{
    buildUV(pixelX, pixelY, ivcamToMM(depth), u, v);
}

template <typename T>
inline void IVCAMCalibrator<T>::buildUV(int pixelX, int pixelY, T depthInMM, T& u, T& v) const
{
    const UVPreComp& puvc = buildUVCoeffs[pixelY*width() + pixelX];
    
    const float z = depthInMM;
    
    const CameraCalibrationParameters & p = params;
    const T D = T(0.5) / (puvc.d*z + p.Pt[2][3]);
    u = (puvc.u*z + p.Pt[0][3]) * D + T(0.5);
    v = (puvc.v*z + p.Pt[1][3]) * D + T(0.5);
}

template <typename T>
inline void IVCAMCalibrator<T>::buildUV(T x, T y, T z, T& u, T& v) const
{
    // x, y, z: must be unprojected coordinates
    // u, v are in the range [0, 1]
    const CameraCalibrationParameters & p = params;
    const T D = T(0.5) / (p.Pt[2][0]*x + p.Pt[2][1]*y + p.Pt[2][2]*z + p.Pt[2][3]); // 0.5 -> 1.0
    u = (p.Pt[0][0]*x + p.Pt[0][1]*y + p.Pt[0][2]*z + p.Pt[0][3]) * D + T(0.5);
    v = (p.Pt[1][0]*x + p.Pt[1][1]*y + p.Pt[1][2]*z + p.Pt[1][3]) * D + T(0.5);
}

template <typename T>
inline T IVCAMCalibrator<T>::convertU_16to9_to_4to3(T u) const
{
    return u * (T(4.0) / T(3.0)) - (T(1.0) / T(6.0));
}

template <typename T>
inline void IVCAMCalibrator<T>::buildUV_4to3(T x, T y, T z, T& u, T& v) const
{
    buildUV(x, y, z, u, v);
    u = convertU_16to9_to_4to3(u);
}

template <typename T>
void IVCAMCalibrator<T>::project(T x, T y, T z, T& u, T& v, T& d) const
{
    T xc = x/z;
    T yc = y/z;
    
    T r2  = xc*xc + yc*yc;
    T r2d = (T(1.0) + params.Distc[0]*r2 + params.Distc[1]*r2*r2 + params.Distc[4]*r2*r2*r2);
    T xcd = xc*r2d + T(2.0)*params.Distc[2]*xc*yc + params.Distc[3]*(r2 + T(2.0)*xc*xc);
    T ycd = yc*r2d + T(2.0)*params.Distc[3]*xc*yc + params.Distc[2]*(r2 + T(2.0)*yc*yc);
    xcd = xcd*params.Kc[0] + params.Kc[6];
    ycd = ycd*params.Kc[4] + params.Kc[7];
    
    u = T((xcd+1)*width()*0.5);
    v = T((ycd+1)*height()*0.5);
    d = z;
}

template <typename T>
Point IVCAMCalibrator<T>::computeUnprojectCoeffs(int _u, int _v) const
{
    const float u = T(_u) / width() * 2  - 1; // -1.0 to 1.0
    const float v = T(_v) / height() * 2 - 1;
    
    // Distort camera coordinates
    T xc   = (u - params.Kc[0][2])/params.Kc[0][0];
    T yc   = (v - params.Kc[1][2])/params.Kc[1][1];
    T r2  = xc*xc + yc*yc;
    T r2c = (T(1) + params.Invdistc[0]*r2 + params.Invdistc[1]*r2*r2 + params.Invdistc[4]*r2*r2*r2);
    T xcd = xc*r2c + T(2.0)*params.Invdistc[2]*xc*yc + params.Invdistc[3]*(r2 + T(2.0)*xc*xc);
    T ycd = yc*r2c + T(2.0)*params.Invdistc[3]*xc*yc + params.Invdistc[2]*(r2 + T(2.0)*yc*yc);
    xcd = xcd*params.Kc[0][0] + params.Kc[0][2];
    ycd = ycd*params.Kc[1][1] + params.Kc[1][2];
    
    // Unnormalized camera rays
    T dx  = params.Kc[1][1]*xcd - params.Kc[1][1]*params.Kc[0][2];
    T dy  = params.Kc[0][0]*ycd - params.Kc[0][0]*params.Kc[1][2]; 
    T dz  = params.Kc[0][0]*params.Kc[1][1];
    
    const T x = dx / dz;
    const T y = dy / dz;
    return Point(x, y);
}

template <typename T>
inline void IVCAMCalibrator<T>::unproject(int pixelX, int pixelY, T depthInMM, T& x, T& y, T& z) const
{
    const Point & p = unprojCoeffs[pixelY * width() + pixelX];
    
    x = p.x * depthInMM;
    y = p.y * depthInMM;
    z = depthInMM;
}

template <typename T>
inline void IVCAMCalibrator<T>::unproject(int pixelX, int pixelY, uint16_t depth, T& x, T& y, T& z) const
{
    unproject(pixelX, pixelY, ivcamToMM(depth), x, y, z);
}

template <typename T>
inline void IVCAMCalibrator<T>::buildCameraProperties()
{
    const T HFOVc = atan(T(1)/params.Kc[0])*T(360)/T(M_PI); // atan(1/Kc(1,1))*2*180/pi;
    const T VFOVc = atan(T(1)/params.Kc[4])*T(360)/T(M_PI); // atan(1/Kc(2,2))*2*180/pi;
    const T xc = params.Kc[6] / T(2) * T(100);              // Kc(1,3) / 2*100;
    const T yc = params.Kc[7] / T(2) * T(100);              // Kc(2,3) / 2*100;
    
    const T HFOVp = atan(T(1)/params.Kp[4]) * T(360)/T(M_PI); // atan(1/Kp(2,2))*2*180/pi;
    
    camProperties.cameraIntrinsics.HFOV = HFOVc;
    camProperties.cameraIntrinsics.VFOV = VFOVc;
    camProperties.cameraIntrinsics.X = xc;
    camProperties.cameraIntrinsics.Y = yc;
    camProperties.cameraIntrinsics.Rad25 = 0;               // rad(1);
    camProperties.cameraIntrinsics.Rad75 = 0;               // rad(2);
    camProperties.cameraIntrinsics.Tang = 0;                // tang;
    camProperties.projectorIntrinsics.HFOV = HFOVp;
    camProperties.projectorIntrinsics.X = 0;                // xp;
    camProperties.projectorIntrinsics.Curv = 0;             // curv;
    camProperties.projectorIntrinsics.Tilt = 0;             // tilt0;
    camProperties.projectorIntrinsics.DTilt = 0;            // dtilt;
    camProperties.textureIntrinsics.HFOV = 0;               // HFOVt;
    camProperties.textureIntrinsics.VFOV = 0;               // VFOVt;
    camProperties.textureIntrinsics.X = 0;                  // xt;
    camProperties.textureIntrinsics.Y = 0;                  // yt;
    camProperties.projectorExtrinsics.Conv = 0;             // qp(1);
    camProperties.projectorExtrinsics.Pitch = 0;            // qp(2);
    camProperties.projectorExtrinsics.Roll = 0;             // qp(3);
    camProperties.projectorExtrinsics.Disp = params.Tp[1];  // tp(1); // disparity
    camProperties.projectorExtrinsics.xDis = params.Tp[0];  // tp(2);
    camProperties.projectorExtrinsics.zDis = params.Tp[2];  // tp(3);
    camProperties.projectorExtrinsics.Conv = 0;             // qt(1);
    camProperties.projectorExtrinsics.Pitch = 0;            // qt(2);
    camProperties.projectorExtrinsics.Roll = 0;             // qt(3);
    camProperties.projectorExtrinsics.Disp = 0;             // tt(1);
    camProperties.projectorExtrinsics.xDis = 0;             // tt(2);
    camProperties.projectorExtrinsics.zDis = 0;             // tt(3);
    camProperties.viewpoint.xRot = 0;                       // qv(1);
    camProperties.viewpoint.yRot = 0;                       // qv(2);
    camProperties.viewpoint.zRot = 0;                       // qv(3);
    camProperties.viewpoint.xDis = 0;                       // qv(4);
    camProperties.viewpoint.yDis = 0;                       // qv(5);
    camProperties.viewpoint.zDis = 0;                       // qv(6);
}
    
} // end namespace f200

#endif
