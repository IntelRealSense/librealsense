#include <librealsense/Common.h>
#include <librealsense/F200/F200.h>

#include "libuvc/libuvc.h"

#include <thread>
#include <atomic>
#include <mutex>

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
    #define IVCAM_MIN_SUPPORTED_VERSION     13
    #define IVCAM_MONITOR_MAX_BUFFER_SIZE   1024
    
    enum IVCAMMonitorCommand
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
    
    //////////////////////////
    // Private Hardware I/O //
    //////////////////////////
    
    class IVCAMHardwareIOInternal
    {
        uvc_device_handle_t * uvcDeviceHandle = nullptr;
        libusb_device_handle * usbDeviceHandle = nullptr;
        
        CameraCalibrationParameters parameters;
        
        std::thread temperatureThread;
        std::atomic<bool> isTemperatureThreadRunning;
        
        int PrepareUSBCommand(uint8_t *request, size_t & requestSize, uint32_t op,
                              uint32_t p1 = 0, uint32_t p2 = 0, uint32_t p3 = 0, uint32_t p4 = 0,
                              uint8_t *data = 0, size_t dataLength = 0)
        {
            
        }
        
        int ExecuteUSBCommand(uint8_t *out, size_t outSize, uint32_t & op, uint8_t * in, size_t & inSize)
        {
            
        }
        
        int FillUSBBuffer(int opCodeNumber, int p1, int p2, int p3, int p4, char * data, int dataLength, char * bufferToSend, int & length)
        {
            
        }
        
        int GetCalibrationRawData(uint8_t * data, int & bytesReturned)
        {
            
        }
        
        int ParseCalibrationParameters(uint8_t * data)
        {
            
        }
        
    public:
        
        IVCAMHardwareIOInternal(uvc_device_handle_t * handle) : uvcDeviceHandle(handle)
        {
            
            usbDeviceHandle = uvc_get_libusb_handle(handle);
            
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
    
    /////////////////////////
    // Public Hardware I/O //
    /////////////////////////
    
    IVCAMHardwareIO::IVCAMHardwareIO(uvc_device_handle_t * handle)
    {
        internal.reset(new IVCAMHardwareIOInternal(handle));
    }
    
    IVCAMHardwareIO::~IVCAMHardwareIO()
    {
        
    }
    
    bool IVCAMHardwareIO::StartTempCompensationLoop()
    {
        return internal->StartTempCompensationLoop();
    }
    
    void IVCAMHardwareIO::StopTempCompensationLoop()
    {
        internal->StopTempCompensationLoop();
    }
    
    CameraCalibrationParameters & IVCAMHardwareIO::GetParameters()
    {
        return internal->GetParameters();
    }

}

#endif
