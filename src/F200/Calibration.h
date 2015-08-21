#pragma once

#ifndef LIBREALSENSE_F200_CALIBRATION_H
#define LIBREALSENSE_F200_CALIBRATION_H

#include "F200Types.h"

#include <vector>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace f200
{
    
#define NUM_OF_CALIBRATION_COEFFS   (64)
    
template <typename T = float>
class IVCAMCalibrator
{
public:
    
    IVCAMCalibrator() {}
    
    IVCAMCalibrator(const CameraCalibrationParameters & p) { buildParameters(p); }
    IVCAMCalibrator(const float * newParamData, int nParams) { buildParameters(newParamData, nParams); }
    
    static int width() { return 640; }
    static int height() { return 480; }
    static T defaultZmax() { return T(2047); }
    
    bool buildParameters(const CameraCalibrationParameters & p);
    bool buildParameters(const float * paramData, int nParams);
    
    void PrintParameters();
    
    void clear() { isInitialized = false; }
    
    T getZMax() const { return params.Rmax; }
    
    operator bool() const { return isInitialized; }
    
    const CameraCalibrationParameters & getParameters()
    {
        if (!isInitialized) throw std::runtime_error("not initialized");
        return params;
    }
    
    int nParamters() const { return sizeof (CameraCalibrationParameters) / sizeof (T); }
    
    void InitializeThermalData(IVCAMTemperatureData TemperatureData, IVCAMThermalLoopParams ThermalLoopParams)
    {
        thermalModeData.Initialize(originalParams.Kc, TemperatureData, ThermalLoopParams);
    }
    
    void GetThermalData(IVCAMTemperatureData & TemperatureData, IVCAMThermalLoopParams & ThermalLoopParams)
    {
        TemperatureData = thermalModeData.BaseTemperatureData;
        ThermalLoopParams = thermalModeData.ThermalLoopParams;
    }
    
    bool updateParamsAccordingToTemperature(float liguriaTemp,float IRTemp, int* timeout);

private:
    
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

    CameraCalibrationParameters params;
    CameraCalibrationParameters originalParams;
    
    bool isInitialized = false;
    
    float lastTemperatureDelta;
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
    return true;
}

template <typename T>
inline bool IVCAMCalibrator<T>::buildParameters(const float* paramData, int nParams)
{
    memcpy(&params, paramData + 1, sizeof(CameraCalibrationParameters)); // skip the first float or 2 uint16
    isInitialized = true;
    lastTemperatureDelta = DELTA_INF;
    memcpy(&originalParams,&params,sizeof(CameraCalibrationParameters));
    return true;
}
    
} // end namespace f200

#endif
