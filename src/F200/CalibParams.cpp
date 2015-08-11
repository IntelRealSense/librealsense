#include <librealsense/Common.h>
#include <librealsense/F200/F200.h>
#include <librealsense/F200/F200Types.h>
#include <librealsense/F200/Calibration.h>
#include <librealsense/F200/Projection.h>
#include <librealsense/F200/CalibParams.h>

#include "libuvc/libuvc.h"

#include <thread>
#include <atomic>
#include <mutex>

using namespace rs;

#ifndef WIN32

namespace f200
{
    
    #define CM_TO_MM(_d)          float((_d)*10)
    #define MM_TO_CM(_d)          float((_d)/10)
    #define METERS_TO_CM(_d)      float((_d)*10
    
    #define IVCAM_VID                       0x8086
    #define IVCAM_PID                       0x0A66
    #define IVCAM_MONITOR_INTERFACE         0x4
    #define IVCAM_MONITOR_ENDPOINT_OUT      0x1
    #define IVCAM_MONITOR_ENDPOINT_IN       0x81
    #define IVCAM_MONITOR_MAGIC_NUMBER      0xcdab
    #define IVCAM_MIN_SUPPORTED_VERSION     13
    #define IVCAM_MONITOR_MAX_BUFFER_SIZE   1024
    #define IVCAM_MONITOR_HEADER_SIZE       (sizeof(uint32_t)*6)
    #define IVCAM_MONITOR_MUTEX_TIMEOUT     3000
    
    #define NUM_OF_CALIBRATION_PARAMS       (100)
    #define NUM_OF_CALIBRATION_COEFFS       (100)
    #define HW_MONITOR_COMMAND_SIZE         (1000)
    #define HW_MONITOR_BUFFER_SIZE          (1000)
    #define PARAMETERS_BUFFER_SIZE          (50)
    
    #define MAX_SIZE_OF_CALIB_PARAM_BYTES   (800)
    #define SIZE_OF_CALIB_PARAM_BYTES       (512)
    #define SIZE_OF_CALIB_HEADER_BYTES      (4)
    
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
    
    int bcdtoint(uint8_t * buf, int bufsize)
    {
        int r = 0;
        for(int i = 0; i < bufsize; i++)
            r = r * 10 + *buf++;
        return r;
    }
    
    int getVersionOfCalibration(uint8_t * validation, uint8_t * version)
    {
        uint8_t valid[2] = {0X14, 0x0A};
        if (memcmp(valid, validation, 2) != 0) return 0;
        else return bcdtoint(version, 2);
    }
    
    //////////////////////////
    // Private Hardware I/O //
    //////////////////////////
    
    class IVCAMHardwareIOInternal
    {
        libusb_device_handle * usbDeviceHandle = nullptr;
        std::timed_mutex usbMutex;
        
        CameraCalibrationParameters parameters;
        
        std::thread temperatureThread;
        std::atomic<bool> isTemperatureThreadRunning;
        
        int PrepareUSBCommand(uint8_t * request, size_t & requestSize, uint32_t op,
                              uint32_t p1 = 0, uint32_t p2 = 0, uint32_t p3 = 0, uint32_t p4 = 0,
                              uint8_t * data = 0, size_t dataLength = 0)
        {
            
            if (requestSize < IVCAM_MONITOR_HEADER_SIZE)
                return 0;
            
            int index = sizeof(uint16_t);
            *(uint16_t *)(request + index) = IVCAM_MONITOR_MAGIC_NUMBER;
            index += sizeof(uint16_t);
            *(uint32_t *)(request + index) = op;
            index += sizeof(uint32_t);
            *(uint32_t *)(request + index) = p1;
            index += sizeof(uint32_t);
            *(uint32_t *)(request + index) = p2;
            index += sizeof(uint32_t);
            *(uint32_t *)(request + index) = p3;
            index += sizeof(uint32_t);
            *(uint32_t *)(request + index) = p4;
            index += sizeof(uint32_t);
            
            if (dataLength)
            {
                memcpy(request + index , data, dataLength);
                index += dataLength;
            }
            
            // Length doesn't include header size (sizeof(uint32_t))
            *(uint16_t *)request = (uint16_t)(index - sizeof(uint32_t));
            requestSize = index;
            return index;
        }
        
        int ExecuteUSBCommand(uint8_t *out, size_t outSize, uint32_t & op, uint8_t * in, size_t & inSize)
        {
            // write
            errno = 0;
            
            int outXfer;
            
            if (usbMutex.try_lock_for(std::chrono::milliseconds(IVCAM_MONITOR_MUTEX_TIMEOUT)))
            {
                
            }
            else
            {
                throw std::runtime_error("usbMutex timed out");
            }
            
            int ret = libusb_bulk_transfer(usbDeviceHandle, IVCAM_MONITOR_ENDPOINT_OUT, out, (int) outSize, &outXfer, 1000); // timeout in ms
            
            printf("libusb_bulk_transfer OUT returns %d (errno %d:%s)", ret, errno, strerror(errno));
            
            // Debugging only
            //dumpCommand(out, outSize);
            
            if (ret < 0)
            {
                printf("usb_device_bulk_transfer OUT failed %d (errno %d:%s)", ret, errno, strerror(errno));
                return -1;
            }
            
            // read
            if (in && inSize)
            {
                uint8_t buf[IVCAM_MONITOR_MAX_BUFFER_SIZE];
                
                errno = 0;
                
                ret = libusb_bulk_transfer(usbDeviceHandle, IVCAM_MONITOR_ENDPOINT_IN, buf, sizeof(buf), &outXfer, 1000);
                
                printf("usb_device_bulk_transfer IN returns %d (errno %d:%s)", ret, errno, strerror(errno));
                
                if (outXfer < (int)sizeof(uint32_t))
                {
                    printf("usb_device_bulk_transfer IN failed %d (errno %d:%s)", outXfer, errno, strerror(errno));
                    usbMutex.unlock();
                    return -1;
                }
                else
                {
                    // Debuggong only
                    //dumpCommand(buf, outXfer);
                    //outXfer -= sizeof(uint32_t);
                    
                    op = *(uint32_t *)buf;
                    
                    if (outXfer > (int)inSize)
                    {
                        printf("usb_device_bulk_transfer IN failed: user buffer too small (%d:%zu)", outXfer, inSize);
                        usbMutex.unlock();
                        return -1;
                    }
                    else
                        inSize = outXfer;
                    memcpy(in, buf, inSize);
                }
            }
            usbMutex.unlock();
            return ret;
        }
        
        void FillUSBBuffer(int opCodeNumber, int p1, int p2, int p3, int p4, char * data, int dataLength, char * bufferToSend, int & length)
        {
            uint16_t preHeaderData = IVCAM_MONITOR_MAGIC_NUMBER;
            
            char * writePtr = bufferToSend;
            int header_size = 4;
            
            // TBD !!! This may change. Need to define it as part of API
            //typeSize = sizeof(float);
            
            int cur_index = 2;
            *(uint16_t *)(writePtr + cur_index) = preHeaderData;
            cur_index += sizeof(uint16_t);
            *(int *)( writePtr + cur_index) = opCodeNumber;
            cur_index += sizeof(uint32_t);
            *(int *)( writePtr + cur_index) = p1;
            cur_index += sizeof(uint32_t);
            *(int *)( writePtr + cur_index) = p2;
            cur_index += sizeof(uint32_t);
            *(int *)( writePtr + cur_index) = p3;
            cur_index += sizeof(uint32_t);
            *(int *)( writePtr + cur_index) = p4;
            cur_index += sizeof(uint32_t);
            
            if (dataLength)
            {
                memcpy(writePtr + cur_index , data, dataLength);
                cur_index += dataLength;
            }
            
            length = cur_index;
            *(uint16_t *) bufferToSend = (uint16_t)(length - header_size);// Length doesn't include header
            
        }
        
        void GetCalibrationRawData(uint8_t * data, size_t & bytesReturned)
        {
            uint8_t request[IVCAM_MONITOR_HEADER_SIZE];
            size_t requestSize = sizeof(request);
            uint32_t responseOp;
            
            if ( !((PrepareUSBCommand(request, requestSize, GetCalibrationTable) > 0) && (ExecuteUSBCommand(request, requestSize, responseOp, data, bytesReturned) != -1)) )
                throw std::runtime_error("usb transfer to retrieve calibration data failed");
        }
        
        void ProjectionCalibrate(uint8_t * rawCalibData,int len, CameraCalibrationParameters * calprms)
        {
            uint8_t * bufParams = rawCalibData + 4;
            
            IVCAMCalibrator<float> * calibration = Projection::GetInstance()->GetCalibrationObject();
            
            CameraCalibrationParametersVersion CalibrationData;
            IVCAMTesterData TesterData;
            
            memset(&CalibrationData, 0, sizeof(CameraCalibrationParametersVersion));
            
            int ver = getVersionOfCalibration(bufParams, bufParams + 2);
            
            if (ver == IVCAM_MIN_SUPPORTED_VERSION)
            {
                float *params = (float *)bufParams;
                
                calibration->buildParameters(params, 100);
                
                // Debugging -- optional
                calibration->PrintParameters();
                
                memcpy(calprms, params+1, sizeof(CameraCalibrationParameters));
                memcpy(&TesterData, bufParams, SIZE_OF_CALIB_HEADER_BYTES);
                
                memset((uint8_t*)&TesterData+SIZE_OF_CALIB_HEADER_BYTES,0,sizeof(IVCAMTesterData) - SIZE_OF_CALIB_HEADER_BYTES);
            }
            else if (ver > IVCAM_MIN_SUPPORTED_VERSION)
            {
                rawCalibData = rawCalibData + 4;
                
                int size = (sizeof(CameraCalibrationParametersVersion) > len) ? len : sizeof(CameraCalibrationParametersVersion);
                
                auto fixWithVersionInfo = [&](CameraCalibrationParametersVersion &d, int size, uint8_t * data)
                {
                    memcpy((uint8_t*)&d + sizeof(int), data, size - sizeof(int));
                };
                
                fixWithVersionInfo(CalibrationData, size, rawCalibData);
                
                memcpy(calprms, &CalibrationData.CalibrationParameters, sizeof(CameraCalibrationParameters));
                calibration->buildParameters(CalibrationData.CalibrationParameters);
                
                // Debugging -- optional
                calibration->PrintParameters();
                
                
                memcpy(&TesterData,  rawCalibData, SIZE_OF_CALIB_HEADER_BYTES);  //copy the header: valid + version
                
                //copy the tester data from end of calibration
                int EndOfCalibratioData = SIZE_OF_CALIB_PARAM_BYTES + SIZE_OF_CALIB_HEADER_BYTES;
                memcpy((uint8_t*)&TesterData + SIZE_OF_CALIB_HEADER_BYTES , rawCalibData + EndOfCalibratioData , sizeof(IVCAMTesterData) - SIZE_OF_CALIB_HEADER_BYTES);
                calibration->InitializeThermalData(TesterData.TemperatureData, TesterData.ThermalLoopParams);
            }
        }
        
    public:
        
        IVCAMHardwareIOInternal()
        {
            usbDeviceHandle = libusb_open_device_with_vid_pid(NULL, IVCAM_VID, IVCAM_PID);
            
            if (usbDeviceHandle == NULL)
                throw std::runtime_error("libusb_open_device_with_vid_pid() failed");
            
            int err = libusb_claim_interface(usbDeviceHandle, IVCAM_MONITOR_INTERFACE);
            
            if (err)
            {
                throw std::runtime_error("libusb_claim_interface() failed");
            }
            
            uint8_t calBuf[HW_MONITOR_BUFFER_SIZE];
            size_t len = HW_MONITOR_BUFFER_SIZE;
            
            GetCalibrationRawData(calBuf, len);
            
            CameraCalibrationParameters calParams;
            
            ProjectionCalibrate(calBuf, (int) len, &calParams);
            
            parameters = calParams;
        }
        
        ~IVCAMHardwareIOInternal()
        {
            libusb_release_interface(usbDeviceHandle, IVCAM_MONITOR_INTERFACE);
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
    
    IVCAMHardwareIO::IVCAMHardwareIO()
    {
        internal.reset(new IVCAMHardwareIOInternal());
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
