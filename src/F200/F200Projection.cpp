#include "Projection.h"
#include "Calibration.h"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cstring>

using namespace std;
using namespace f200;

#define CM_TO_MM(_d)          float((_d)*10)
#define MM_TO_CM(_d)          float((_d)/10)
#define METERS_TO_CM(_d)      float((_d)*100)

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

void Projection::InitializeThermalData(IVCAMTemperatureData temp,IVCAMThermalLoopParams loopParams)
{
    m_calibration.InitializeThermalData(temp, loopParams);
}

void Projection::GetThermalData(IVCAMTemperatureData &temp, IVCAMThermalLoopParams &loopParams)
{
    m_calibration.GetThermalData(temp, loopParams);
}
