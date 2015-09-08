#pragma once
#ifndef LIBREALSENSE_F200_PRIVATE_H
#define LIBREALSENSE_F200_PRIVATE_H

#include "uvc.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <cstring>  // for memcpy, memcmp
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>

#define DELTA_INF                           (10000000.0)
#define M_EPSILON                           (0.0001)

#define IV_COMMAND_FIRMWARE_UPDATE_MODE     0x01
#define IV_COMMAND_GET_CALIBRATION_DATA     0x02
#define IV_COMMAND_LASER_POWER              0x03
#define IV_COMMAND_DEPTH_ACCURACY           0x04
#define IV_COMMAND_ZUNIT                    0x05
#define IV_COMMAND_LOW_CONFIDENCE_LEVEL     0x06
#define IV_COMMAND_INTENSITY_IMAGE_TYPE     0x07
#define IV_COMMAND_MOTION_VS_RANGE_TRADE    0x08
#define IV_COMMAND_POWER_GEAR               0x09
#define IV_COMMAND_FILTER_OPTION            0x0A
#define IV_COMMAND_VERSION                  0x0B
#define IV_COMMAND_CONFIDENCE_THRESHHOLD    0x0C

#define IVCAM_DEPTH_LASER_POWER         1
#define IVCAM_DEPTH_ACCURACY            2
#define IVCAM_DEPTH_MOTION_RANGE        3
#define IVCAM_DEPTH_ERROR               4
#define IVCAM_DEPTH_FILTER_OPTION       5
#define IVCAM_DEPTH_CONFIDENCE_THRESH   6
#define IVCAM_DEPTH_DYNAMIC_FPS         7 // Only available on IVCAM 1.5 / SR300

#define IVCAM_COLOR_EXPOSURE_PRIORITY   1
#define IVCAM_COLOR_AUTO_FLICKER        2
#define IVCAM_COLOR_ERROR               3
#define IVCAM_COLOR_EXPOSURE_GRANULAR   4

#define HW_MONITOR_COMMAND_SIZE         (1000)
#define HW_MONITOR_BUFFER_SIZE          (1000)

namespace rsimpl { namespace f200
{

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

struct IVCAMCommand
{
	IVCAMMonitorCommand cmd;
	int Param1;
	int Param2;
	int Param3;
	int Param4;
	char data[HW_MONITOR_BUFFER_SIZE];
	int sizeOfSendCommandData;
	long TimeOut;
	bool oneDirection;
	char recivedCommandData[HW_MONITOR_BUFFER_SIZE];
	int sizeOfRecivedCommandData;
	char recievedOPcode[4];

	IVCAMCommand(IVCAMMonitorCommand cmd)
	{
		this->cmd = cmd;
		Param1 = 0;
		Param2 = 0;
		Param3 = 0;
		Param4 = 0;
		sizeOfSendCommandData = 0;
		TimeOut = 5000;
		oneDirection = false;
	}
};

struct IVCAMCommandDetails
{
	bool oneDirection;
	char sendCommandData[HW_MONITOR_COMMAND_SIZE];
	int sizeOfSendCommandData;
	long TimeOut;
	char recievedOPcode[4];
	char recievedCommandData[HW_MONITOR_BUFFER_SIZE];
	int sizeOfRecievedCommandData;
};

struct OACOffsetData
{
    int OACOffset1;
    int OACOffset2;
    int OACOffset3;
    int OACOffset4;
};

struct IVCAMTemperatureData
{
    float LiguriaTemp;
    float IRTemp;
    float AmbientTemp;
};

struct IVCAMThermalLoopParams
{
    float IRThermalLoopEnable = 1;      // enable the mechanism
    float TimeOutA = 10000;             // default time out
    float TimeOutB = 0;                 // reserved
    float TimeOutC = 0;                 // reserved
    float TransitionTemp = 3;           // celcius degrees, the transition temperatures to ignore and use offset;
    float TempThreshold = 2;            // celcius degrees, the temperatures delta that above should be fixed;
    float HFOVsensitivity = 0.025f;
    float FcxSlopeA = -0.003696988f;    // the temperature model fc slope a from slope_hfcx = ref_fcx*a + b
    float FcxSlopeB = 0.005809239f;     // the temperature model fc slope b from slope_hfcx = ref_fcx*a + b
    float FcxSlopeC = 0;                // reserved
    float FcxOffset = 0;                // the temperature model fc offset
    float UxSlopeA = -0.000210918f;     // the temperature model ux slope a from slope_ux = ref_ux*a + ref_fcx*b
    float UxSlopeB = 0.000034253955f;   // the temperature model ux slope b from slope_ux = ref_ux*a + ref_fcx*b
    float UxSlopeC = 0;                 // reserved
    float UxOffset = 0;                 // the temperature model ux offset
    float LiguriaTempWeight = 1;        // the liguria temperature weight in the temperature delta calculations
    float IrTempWeight = 0;             // the Ir temperature weight in the temperature delta calculations
    float AmbientTempWeight = 0;        // reserved
    float Param1 = 0;                   // reserved
    float Param2 = 0;                   // reserved
    float Param3 = 0;                   // reserved
    float Param4 = 0;                   // reserved
    float Param5 = 0;                   // reserved
};

struct IVCAMTesterData
{
    int16_t TableValidation;
    int16_t TableVarsion;
    OACOffsetData OACOffsetData_;
    IVCAMThermalLoopParams ThermalLoopParams;
    IVCAMTemperatureData TemperatureData;
};

struct CameraCalibrationParameters
{
    float Rmax;
    float Kc[3][3];     // [3x3]: intrinsic calibration matrix of the IR camera
    float Distc[5];     // [1x5]: forward distortion parameters of the IR camera
    float Invdistc[5];  // [1x5]: the inverse distortion parameters of the IR camera
    float Pp[3][4];     // [3x4]: projection matrix
    float Kp[3][3];     // [3x3]: intrinsic calibration matrix of the projector
    float Rp[3][3];     // [3x3]: extrinsic calibration matrix of the projector
    float Tp[3];        // [1x3]: translation vector of the projector
    float Distp[5];     // [1x5]: forward distortion parameters of the projector
    float Invdistp[5];  // [1x5]: inverse distortion parameters of the projector
    float Pt[3][4];     // [3x4]: IR to RGB (texture mapping) image transformation matrix
    float Kt[3][3];
    float Rt[3][3];
    float Tt[3];
    float Distt[5];     // [1x5]: The inverse distortion parameters of the RGB camera
    float Invdistt[5];
    float QV[6];
};

struct CameraCalibrationParametersVersion
{
    int uniqueNumber; //Should be 0xCAFECAFE in Calibration version 1 or later. In calibration version 0 this is zero.
    int16_t TableValidation;
    int16_t TableVarsion;
    CameraCalibrationParameters CalibrationParameters;
};

enum Property
{
    IVCAM_PROPERTY_COLOR_EXPOSURE                   =   1,
    IVCAM_PROPERTY_COLOR_BRIGHTNESS                 =   2,
    IVCAM_PROPERTY_COLOR_CONTRAST                   =   3,
    IVCAM_PROPERTY_COLOR_SATURATION                 =   4,
    IVCAM_PROPERTY_COLOR_HUE                        =   5,
    IVCAM_PROPERTY_COLOR_GAMMA                      =   6,
    IVCAM_PROPERTY_COLOR_WHITE_BALANCE              =   7,
    IVCAM_PROPERTY_COLOR_SHARPNESS                  =   8,
    IVCAM_PROPERTY_COLOR_BACK_LIGHT_COMPENSATION    =   9,
    IVCAM_PROPERTY_COLOR_GAIN                       =   10,
    IVCAM_PROPERTY_COLOR_POWER_LINE_FREQUENCY       =   11,
    IVCAM_PROPERTY_AUDIO_MIX_LEVEL                  =   12,
    IVCAM_PROPERTY_APERTURE                         =   13,
    IVCAM_PROPERTY_DISTORTION_CORRECTION_I          =   202,
    IVCAM_PROPERTY_DISTORTION_CORRECTION_DPTH       =   203,
    IVCAM_PROPERTY_DEPTH_MIRROR                     =   204,    //0 - not mirrored, 1 - mirrored
    IVCAM_PROPERTY_COLOR_MIRROR                     =   205,
    IVCAM_PROPERTY_COLOR_FIELD_OF_VIEW              =   207,
    IVCAM_PROPERTY_COLOR_SENSOR_RANGE               =   209,
    IVCAM_PROPERTY_COLOR_FOCAL_LENGTH               =   211,
    IVCAM_PROPERTY_COLOR_PRINCIPAL_POINT            =   213,
    IVCAM_PROPERTY_DEPTH_FIELD_OF_VIEW              =   215,
    IVCAM_PROPERTY_DEPTH_UNDISTORTED_FIELD_OF_VIEW  =   223,
    IVCAM_PROPERTY_DEPTH_SENSOR_RANGE               =   217,
    IVCAM_PROPERTY_DEPTH_FOCAL_LENGTH               =   219,
    IVCAM_PROPERTY_DEPTH_UNDISTORTED_FOCAL_LENGTH   =   225,
    IVCAM_PROPERTY_DEPTH_PRINCIPAL_POINT            =   221,
    IVCAM_PROPERTY_MF_DEPTH_LOW_CONFIDENCE_VALUE    =   5000,
    IVCAM_PROPERTY_MF_DEPTH_UNIT                    =   5001,   // in micron
    IVCAM_PROPERTY_MF_CALIBRATION_DATA              =   5003,
    IVCAM_PROPERTY_MF_LASER_POWER                   =   5004,
    IVCAM_PROPERTY_MF_ACCURACY                      =   5005,
    IVCAM_PROPERTY_MF_INTENSITY_IMAGE_TYPE          =   5006,   //0 - (I0 - laser off), 1 - (I1 - Laser on), 2 - (I1-I0), default is I1.
    IVCAM_PROPERTY_MF_MOTION_VS_RANGE_TRADE         =   5007,
    IVCAM_PROPERTY_MF_POWER_GEAR                    =   5008,
    IVCAM_PROPERTY_MF_FILTER_OPTION                 =   5009,
    IVCAM_PROPERTY_MF_VERSION                       =   5010,
    IVCAM_PROPERTY_MF_DEPTH_CONFIDENCE_THRESHOLD    =   5013,
    IVCAM_PROPERTY_ACCELEROMETER_READING            =   3000,   // three values
    IVCAM_PROPERTY_PROJECTION_SERIALIZABLE          =   3003,   
    IVCAM_PROPERTY_CUSTOMIZED                       =   0x04000000,
};

    class IVCAMCalibrator;

    class IVCAMHardwareIO
    {
        uvc::device device;
        std::timed_mutex usbMutex;

        CameraCalibrationParameters parameters;

        std::thread temperatureThread;
        std::atomic<bool> isTemperatureThreadRunning;

        std::unique_ptr<IVCAMCalibrator> calibration;

        int PrepareUSBCommand(uint8_t * request, size_t & requestSize, uint32_t op,
                              uint32_t p1 = 0, uint32_t p2 = 0, uint32_t p3 = 0, uint32_t p4 = 0,
                              uint8_t * data = 0, size_t dataLength = 0);
        void ExecuteUSBCommand(uint8_t *out, size_t outSize, uint32_t & op, uint8_t * in, size_t & inSize);
        void FillUSBBuffer(int opCodeNumber, int p1, int p2, int p3, int p4, char * data, int dataLength, char * bufferToSend, int & length);
        void GetCalibrationRawData(uint8_t * data, size_t & bytesReturned);
        void ProjectionCalibrate(uint8_t * rawCalibData, int len, CameraCalibrationParameters * calprms);
        void ReadTemperatures(IVCAMTemperatureData & data);
		bool EnableTimeStamp(bool enableColor, bool enableDepth);
        bool GetMEMStemp(float & MEMStemp);
        bool GetIRtemp(int & IRtemp);
        void TemperatureControlLoop();

		bool PerfomAndSendHWmonitorCommand(IVCAMCommand & newCommand);
		bool IVCAMHardwareIO::SendHWmonitorCommand(IVCAMCommandDetails & details);

    public:

        IVCAMHardwareIO(uvc::device device);

        bool StartTempCompensationLoop()
        {
            // @tofix
            return false;
        }

        void StopTempCompensationLoop()
        {
            // @tofix
        }

        CameraCalibrationParameters & GetParameters() { return parameters; }
    };

    #define NUM_OF_CALIBRATION_COEFFS   (64)

    class IVCAMCalibrator
    {
    public:

        IVCAMCalibrator() {}

        IVCAMCalibrator(const CameraCalibrationParameters & p) { buildParameters(p); }
        IVCAMCalibrator(const float * newParamData, int nParams) { buildParameters(newParamData, nParams); }

        static int width() { return 640; }
        static int height() { return 480; }
        static float defaultZmax() { return 2047; }

        bool buildParameters(const CameraCalibrationParameters & p);
        bool buildParameters(const float * paramData, int nParams);

        void clear() { isInitialized = false; }

        float getZMax() const { return params.Rmax; }

        operator bool() const { return isInitialized; }

        const CameraCalibrationParameters & getParameters()
        {
            if (!isInitialized) throw std::runtime_error("not initialized");
            return params;
        }

        int nParamters() const { return sizeof (CameraCalibrationParameters) / sizeof (float); }

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

            float FcxSlope;			// the temperature model calculated slope for fc
            float UxSlope;			// the temperature model calculated slope for ux
            float FcxOffset;		// the temperature model fc offset
            float UxOffset;			// the temperature model ux offset

            float TempThreshold;	//celcius degrees, the temperatures delta that above should be fixed;
        };

        ThermalModelData thermalModeData;

        CameraCalibrationParameters params;
        CameraCalibrationParameters originalParams;

        bool isInitialized = false;

        float lastTemperatureDelta;
    };

} } // namespace rsimpl::f200

#endif
