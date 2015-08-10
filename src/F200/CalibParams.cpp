#include <librealsense/Common.h>
#include <librealsense/F200/F200.h>

#include "libuvc/libuvc.h"

using namespace rs;

#ifndef WIN32

namespace f200
{
    
    #define IVCAM_VID                       0x8086
    #define IVCAM_PID                       0x0A66
    #define IVCAM_MONITOR_INTERFACE         0x4
    #define IVCAM_MONITOR_ENDPOINT_OUT      0x1
    #define IVCAM_MONITOR_ENDPOINT_IN       0x81
    #define IVCAM_MONITOR_MAGIC_NUMBER      0xcdab
    
    enum IvcamMonitorCommand
    {
        UpdateCalib         = 0xBC,
        GetIRTemp           = 0x52,
        GetMEMSTemp         = 0x0A,
        HWReset             = 0x28,
        GVD                 = 0x3B,
        BIST                = 0xFF,
        GoToDFU             = 0x80,
        GetCalibrationTable = 0x3D,
        DebugFormat         = 0x0B,
        TimeStempEnable     = 0x0C,
        GetPowerGearState   = 0xFF,
        SetDefaultControls  = 0xA6,
        GetDefaultControls  = 0xA7,
        GetFWLastError      = 0x0E,
        CheckI2cConnect     = 0x4A,
        CheckRGBConnect     = 0x4B,
        CheckDPTConnect     = 0x4C
    };
    
    class IVCAMHardwareIOInternal
    {
        CameraCalibrationParameters parameters;
        
        std::atomic<bool> tempThreadStatus;
        
        int PrepareUSBCommand(uint8_t *request, size_t & requestSize, uint32_t op,
                              uint32_t p1 = 0, uint32_t p2 = 0, uint32_t p3 = 0, uint32_t p4 = 0,
                              uint8_t *data = 0, size_t dataLength = 0)
        {
            
        }
        
        int ExecuteUSBCommand(uint8_t *out, size_t outSize, uint32_t & op, uint8_t *in, size_t & inSize)
        {
            
        }
        
    public:
        
        IVCAMHardwareIOInternal()
        {
            
        }
        
        ~IVCAMHardwareIOInternal()
        {
            
        }
        
        // Reach out to hardwarw
        bool Initialize()
        {
            
        }
        
        bool StartTempCompensationLoop()
        {
            
        }
        
        void StopTempCompensationLoop()
        {
            
        }
        
        CameraCalibrationParameters & GetParameters()
        {
            return parameters;
        }
        
    };

}

#endif
