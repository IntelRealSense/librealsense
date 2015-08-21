#pragma once

#ifndef LIBREALSENSE_F200_PROJECTION_H
#define LIBREALSENSE_F200_PROJECTION_H

#include <libuvc/libuvc.h>

#include "F200Types.h"
#include "Calibration.h"
#include "Projection.h"

namespace f200
{

class Projection
{
public:
    
    Projection(int indexOfCamera, bool RunThermalLoop = false);
    
    ~Projection(void);

    bool Init();
    
    void MapDepthToColorCoordinates(unsigned int npoints, Point3DF32 *pos2d, Point2DF32 *posc, bool isUVunitsRelative = true, CoordinateSystem dir = LEFT_HANDED);
    
    void MapDepthToColorCoordinates(unsigned int width, unsigned int height, uint16_t* pSrcDepth, float* pDestUV, bool isUVunitsRelative = true, CoordinateSystem dir = LEFT_HANDED);
    
    void ProjectImageToRealWorld(unsigned int npoints, Point3DF32 *pos2d, Point3DF32 *pos3d, CoordinateSystem dir = LEFT_HANDED);
    
    void ProjectImageToRealWorld(unsigned int width, unsigned int height, uint16_t * pSrcDepth, float* pDestXYZ, CoordinateSystem dir = LEFT_HANDED);
    
    float ConvertDepth_Uint16ToMM(uint16_t d);
    
    bool IsInitialized() { return m_isInitialized; }
    void SetDepthResolution(int width, int height) { m_currentDepthWidth = width; m_currentDepthHeight = height; }
    void SetColorResolution(int width, int height) { m_currentColorWidth =width; m_currentColorHeight = height; }
    
    int GetColorWidth() { return m_currentColorWidth; }
    int GetColorHeight() { return m_currentColorHeight; }
    int GetDepthWidth() { return m_currentDepthWidth; }
    int GetDepthHeight() { return m_currentDepthHeight; }
    
    // Start function of thermal loop thread.Thread will poll temperature each X seconds and make required changes to
    // Calibration table. Also inform users that calib table has changed and they need to redraw it.
    void CallThermalLoopThread();
    
    int GetIndexOfCamera() { return m_IndexOfCamera; };
    
    IVCAMCalibrator<float> * GetCalibrationObject() { return & m_calibration; };
    
    void InitializeThermalData(IVCAMTemperatureData TemperatureData, IVCAMThermalLoopParams ThermalLoopParams);
    
    void GetThermalData(IVCAMTemperatureData & TemperatureData, IVCAMThermalLoopParams & ThermalLoopParams);
    
    struct ProjectionParams
    {
        uint32_t depthWidth;
        uint32_t depthHeight;
        uint32_t colorWidth;
        uint32_t colorHeight;
        uint32_t nParams;
        CameraCalibrationParametersVersion calibrationParams;
    };
    
    void ThermalLoopKilled();
    
    int m_IndexOfCamera;	
    bool m_isCalibOld;					
    
    IVCAMCalibrator<float> m_calibration;
    CameraCalibrationParametersVersion m_calibrationData;
    
    bool m_isInitialized;
    int m_currentDepthWidth;
    int m_currentDepthHeight;
    int m_currentColorWidth;
    int m_currentColorHeight;
    
    bool m_RunThermalLoop;
    bool m_IsThermalLoopOpen;
};
    
} // end namespace f200

#endif
