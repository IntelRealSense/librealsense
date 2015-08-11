#include "Projection.h"
#include "Calibration.h"

#include <fstream>
#include <sstream>

using namespace std;
using namespace f200;

#define CM_TO_MM(_d)          float((_d)*10)
#define MM_TO_CM(_d)          float((_d)/10)
#define METERS_TO_CM(_d)      float((_d)*100)

typedef f200::IVCAMCalibrator<float>::OpticalData opticalData;
typedef f200::IVCAMCalibrator<float>::Resolution resolution;

Projection * Projection::GetInstance()
{
    static Projection self(0);
    return &self;
}

Projection::Projection(int indexOfCamera, bool RunThermalLoop)
{
    m_RunThermalLoop = RunThermalLoop;
    m_IndexOfCamera = indexOfCamera;
    m_isInitialized = false;
    m_currentDepthWidth = 640;
    m_currentDepthHeight = 480;
    m_currentColorWidth = 640;
    m_currentColorHeight = 480; //temp!!! calib params must be re-stored from IVCAM itself at every resolution change !!!
    m_IsThermalLoopOpen = false;
}

Projection::~Projection(void)
{
    m_currentDepthWidth = 0;
    m_currentDepthHeight = 0;
    m_currentColorWidth = 0;
    m_currentColorHeight = 0;
    m_isInitialized = false;
}

bool Projection::Init()
{
    m_isInitialized = true;
    m_isCalibOld = false;
    return true;
}

void  Projection::ThermalLoopKilled()
{
    m_IsThermalLoopOpen = false;
}

void  Projection::GetProjectionSize(int &size)
{
    return GetSerializedProjectionData(size, 0);
}

void Projection::GetSerializedProjectionData(int & dataSize, uint8_t * data)
{
    if ((bool) m_calibration)
    {
        unsigned int totalSize = sizeof(ProjectionParams);
        dataSize = totalSize;
        
        if (data)
        {
            ProjectionParams* params = new ProjectionParams;//(ProjectionParams*) (new BYTE[structSize+paramsSize]());
            params->depthWidth = m_currentDepthWidth;
            params->depthHeight = m_currentDepthHeight;
            params->colorWidth =  m_currentColorWidth;
            params->colorHeight = m_currentColorHeight;
            params->nParams =  m_calibration.nParamters();
            const auto calibParams = m_calibration.getParameters();
            memcpy(&(params->calibrationParams.CalibrationParameters), &calibParams, sizeof(CameraCalibrationParameters));
            
            params->calibrationParams.TableVarsion = m_calibrationData.TableVarsion;
            params->calibrationParams.TableValidation = m_calibrationData.TableValidation;
            params->calibrationParams.uniqueNumber = m_calibrationData.uniqueNumber;
            memcpy(data, (uint8_t*) params, totalSize);
            
            delete[] params;
        }
    }
    throw std::runtime_error("GetSerializedProjectionData() failed");
}

void Projection::SetSerializedProjectionData(uint8_t * data)
{
    ProjectionParams * params = (ProjectionParams*)data;
    SetDepthResolution(params->depthWidth,params->depthHeight);
    SetColorResolution(params->colorWidth,params->colorHeight);
    
    int uniqueNum = 0; //UNIQUE_NUMBER;
    int uniqueNumData = *(((int*)(data)) + 5);
    
    if (uniqueNumData == uniqueNum)
    {
        if (m_calibration.buildParameters(params->calibrationParams.CalibrationParameters))
        {
            memcpy(&m_calibrationData, &params->calibrationParams, sizeof(CameraCalibrationParametersVersion));
            m_isInitialized = true;
        }
    }
    throw std::runtime_error("SetSerializedProjectionData() failed");
}

void Projection::QueryProperty(Property label, float &value)
{
    opticalData OD;
    resolution colorRes(m_currentColorWidth, m_currentColorHeight);
    resolution depthRes(m_currentDepthWidth, m_currentDepthHeight);
    
    OD = m_calibration.getOpticalData(depthRes,colorRes);
    
    // Dimitri removed "m_isCalibOld"
    switch(label)
    {
        case IVCAM_PROPERTY_COLOR_FIELD_OF_VIEW:
            value = OD.RGBUndistortedFOV.x; break;
        case IVCAM_PROPERTY_COLOR_FIELD_OF_VIEW + 1:
            value = OD.RGBUndistortedFOV.y; break;
        case IVCAM_PROPERTY_COLOR_FOCAL_LENGTH:
            value = OD.RGBUndistortedFocalLengthPxl.x; break;
        case IVCAM_PROPERTY_COLOR_FOCAL_LENGTH + 1:
            value = OD.RGBUndistortedFocalLengthPxl.y; break;
        case IVCAM_PROPERTY_COLOR_PRINCIPAL_POINT:
            value = OD.RGBPrincipalPoint.x; break;
        case IVCAM_PROPERTY_COLOR_PRINCIPAL_POINT+1:
            value = OD.RGBPrincipalPoint.y; break;
        case IVCAM_PROPERTY_DEPTH_FIELD_OF_VIEW:
            value = OD.IRDistortedFOV.x; break;
        case IVCAM_PROPERTY_DEPTH_FIELD_OF_VIEW+1:
            value = OD.IRDistortedFOV.y; break;
        case IVCAM_PROPERTY_DEPTH_UNDISTORTED_FIELD_OF_VIEW:
            value = OD.IRUndistortedFOV.x; break;
        case IVCAM_PROPERTY_DEPTH_UNDISTORTED_FIELD_OF_VIEW+1:
            value = OD.IRUndistortedFOV.y; break;
        case IVCAM_PROPERTY_DEPTH_FOCAL_LENGTH:
            value = OD.IRDistortedFocalLengthPxl.x; break;
        case IVCAM_PROPERTY_DEPTH_FOCAL_LENGTH +1:
            value = OD.IRDistortedFocalLengthPxl.y; break;
        case IVCAM_PROPERTY_DEPTH_UNDISTORTED_FOCAL_LENGTH:
            value = OD.IRUndistortedFocalLengthPxl.x; break;
        case IVCAM_PROPERTY_DEPTH_UNDISTORTED_FOCAL_LENGTH +1:
            value = OD.IRUndistortedFocalLengthPxl.y; break;
        case IVCAM_PROPERTY_DEPTH_PRINCIPAL_POINT:
            value = OD.IRPrincipalPoint.x; break;
        case IVCAM_PROPERTY_DEPTH_PRINCIPAL_POINT +1:
            value = OD.IRPrincipalPoint.y; break;
        default:
            throw std::runtime_error("unsupported property");
    }
}

void Projection::MapDepthToColorCoordinates(unsigned int npoints, Point3DF32 *pos2d, Point2DF32 *posc, bool isUVunitsRelative, CoordinateSystem dir)
{
    if (!m_calibration)
        throw std::runtime_error("not initialized");
    
    const int xShift = (m_currentDepthWidth == 320) ? 1 : 0;
    const int yShift = (m_currentDepthHeight == 240) ? 1 : 0;
    const int yOffset = (m_currentDepthHeight == 360) ? 60 : 0;
    
    const bool aspectRatio43 = (m_currentColorWidth * 3 == m_currentColorHeight * 4);
    
    for (int i=0; i< (int)npoints; i++)
    {
        if (pos2d[i].z > 0)
        {
            int pxX = ((int)pos2d[i].x) << xShift;
            if (dir == RIGHT_HANDED) //not mirrored
                pxX = m_currentDepthWidth - pxX;
            int pxY = (((int)pos2d[i].y) << yShift) + yOffset;
            
            //Call unproject with mm units.
            float u,v;
            m_calibration.buildUV(pxX, pxY, pos2d[i].z, u ,v);
            
            if (aspectRatio43)
                u = m_calibration.convertU_16to9_to_4to3(u);
            
            if (dir == RIGHT_HANDED) //not mirrored
                u = 1 - u;
            
            posc[i].x = u;
            posc[i].y = v;
            
            if (!isUVunitsRelative)
            {
                posc[i].x *= m_currentColorWidth;
                posc[i].y *= m_currentColorHeight;
            }
        }
        else
        {
            posc[i].x = 0;
            posc[i].y = 0;
        }
    }
}

void Projection::MapDepthToColorCoordinates(unsigned int width, unsigned int height, uint16_t* pSrcDepth,  float* pDestUV, bool isUVunitsRelative, CoordinateSystem dir)
{
    if (!m_calibration)
        throw std::runtime_error("MapDepthToColorCoordinates failed, m_calibration not initialized");
    
    const int xShift = (m_currentDepthWidth == 320) ? 1 : 0;
    const int yShift = (m_currentDepthHeight == 240) ? 1 : 0;
    
    float u,v;
    
    float* pTmpDestUV = pDestUV;
    
    const bool aspectRatio43 = (m_currentColorWidth * 3 == m_currentColorHeight * 4);
    uint16_t *pDepth = (uint16_t*)pSrcDepth;
    
    for(int i=0; i<m_currentDepthHeight ; i++)
    {
        float* pUV = pTmpDestUV;
        for (int j=0 ; j< m_currentDepthWidth; j++)
        {
            uint16_t d = *pDepth++;
            if (d)
            {
                float df = ConvertDepth_Uint16ToMM(d);
                
                if (dir == RIGHT_HANDED) //not mirrored
                {
                    int xx = j << xShift;
                    xx = m_currentDepthWidth - xx;
                    
                    m_calibration.buildUV(xx , i<< yShift, (float)df, u ,v);
                }
                else
                {
                    m_calibration.buildUV(j << xShift, i<< yShift, (float)df, u ,v);
                }
                
                if (aspectRatio43)
                    u = m_calibration.convertU_16to9_to_4to3(u);
                
                if (dir == RIGHT_HANDED) //not mirrored
                    u = 1 - u;
                
                if (isUVunitsRelative)
                {
                    pUV[j*2] = (u);
                    pUV[j*2+1] = (v);
                }
                else
                {
                    pUV[j*2] = (u*m_currentColorWidth);
                    pUV[j*2+1] = (v*m_currentColorHeight);
                }
            }
            else
            {
                pUV[j*2] = 0;
                pUV[j*2+1] = 0;
            }
        }
        
        pTmpDestUV = pTmpDestUV + 2 * m_currentDepthWidth;
    }
    
}

void Projection::ProjectImageToRealWorld(unsigned int npoints, Point3DF32 *pos2d, Point3DF32 *pos3d, CoordinateSystem dir)
{
    if (!m_calibration)
        throw std::runtime_error("not initialized");
    
    const int xShift = (m_currentDepthWidth == 320) ? 1 : 0;
    const int yShift = (m_currentDepthHeight == 240) ? 1 : 0;
    
    for(int i=0; i< (int)npoints; i++)
    {
        int pxX = ((int)pos2d[i].x) << xShift;
        
        if (dir == RIGHT_HANDED) // not mirrored
            pxX = m_currentDepthWidth - pxX;
        
        int pxY = ((int)pos2d[i].y) << yShift;
        //z is 0 - 2000mm (2000mm -> 65536-1)
        float x, y, z;
        //d is (0-65536) = (0-2m)
        
        m_calibration.unproject(pxX, pxY, pos2d[i].z, x,y,z);
        
        if (dir == RIGHT_HANDED) //not mirrored
            x = -x;
        
        pos3d[i].x = x;
        pos3d[i].y = -1*y;
        pos3d[i].z = z;
    }
}

void Projection::ProjectImageToRealWorld(unsigned int width, unsigned int height, uint16_t* pSrcDepth,  float* pDestXYZ, CoordinateSystem dir)
{
    if (!m_calibration)
        throw std::runtime_error("not initialized");
    
    const int xShift = (m_currentDepthWidth == 320) ? 1 : 0;
    const int yShift = (m_currentDepthHeight == 240) ? 1 : 0;
    float fX, fY, fZ;
    
    int DestSizeInBytes;
    int SourceSizeOfPixelInBytes = 2;
    int SourceSizeInBytes = m_currentDepthWidth*SourceSizeOfPixelInBytes;
    
    float *pTmpDestXYZ = pDestXYZ;
    
    DestSizeInBytes = m_currentDepthWidth * 3 * sizeof(uint16_t);
    
    for(int i=0; i<m_currentDepthHeight; i++)
    {
        uint16_t *pDepth = (uint16_t*) pSrcDepth;
        float *pXYZ = pTmpDestXYZ;
        
        for(int j=0 ; j< m_currentDepthWidth; j++)
        {
            uint16_t d = *(uint16_t*)(((int8_t *)pDepth)+ j*SourceSizeOfPixelInBytes);
            if (d)
            {
                if (dir == RIGHT_HANDED) //not mirrored
                    m_calibration.unproject(m_currentDepthWidth-(j << xShift), i<< yShift, (float)d, fX,fY,fZ);
                else
                    m_calibration.unproject(j << xShift, i<< yShift, (float)d, fX,fY,fZ);
                
                if (dir == RIGHT_HANDED) //not mirrored
                    fX = -fX;
                
                pXYZ[3*j + 0] = (float)fX;
                pXYZ[3*j + 1] = (float)(fY * -1); //LHS <-> RHS
                pXYZ[3*j + 2] = (float)(fZ); // = d
            }
            else
            {
                pXYZ[3*j + 0] = 0;
                pXYZ[3*j + 1] = 0;
                pXYZ[3*j + 2] = 0;
            }
        }
        pTmpDestXYZ = pTmpDestXYZ + m_currentDepthWidth * 3;
        pSrcDepth = (uint16_t*)(((uint8_t*)pSrcDepth) + SourceSizeInBytes );
    }
}

float Projection::ConvertDepth_Uint16ToMM(uint16_t d)
{
    if (!m_calibration) return 0.0f;
    return m_calibration.ivcamToMM(d);
}

void Projection::InitializeThermalData(IVCAMTemperatureData temp,IVCAMThermalLoopParams loopParams)
{
    m_calibration.InitializeThermalData(temp, loopParams);
}

void Projection::GetThermalData(IVCAMTemperatureData &temp, IVCAMThermalLoopParams &loopParams)
{
    m_calibration.GetThermalData(temp, loopParams);
}
