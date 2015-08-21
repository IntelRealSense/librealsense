#pragma once

#ifndef LIBREALSENSE_F200_HARDWARE_IO_H
#define LIBREALSENSE_F200_HARDWARE_IO_H

#include "F200Types.h"

#include <libuvc/libuvc.h>

#include <cmath>    // for M_PI, tan
#include <cstring>  // for memcpy, memcmp
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>

namespace f200
{
    class Projection;

    class IVCAMHardwareIO
    {
        libusb_device_handle * usbDeviceHandle = nullptr;
        std::timed_mutex usbMutex;

        CameraCalibrationParameters parameters;

        std::thread temperatureThread;
        std::atomic<bool> isTemperatureThreadRunning;

        std::unique_ptr<Projection> projection;

        int PrepareUSBCommand(uint8_t * request, size_t & requestSize, uint32_t op,
                              uint32_t p1 = 0, uint32_t p2 = 0, uint32_t p3 = 0, uint32_t p4 = 0,
                              uint8_t * data = 0, size_t dataLength = 0);
        int ExecuteUSBCommand(uint8_t *out, size_t outSize, uint32_t & op, uint8_t * in, size_t & inSize);
        void FillUSBBuffer(int opCodeNumber, int p1, int p2, int p3, int p4, char * data, int dataLength, char * bufferToSend, int & length);
        void GetCalibrationRawData(uint8_t * data, size_t & bytesReturned);
        void ProjectionCalibrate(uint8_t * rawCalibData, int len, CameraCalibrationParameters * calprms);
        void ReadTemperatures(IVCAMTemperatureData & data);
        bool GetMEMStemp(float & MEMStemp);
        bool GetIRtemp(int & IRtemp);
        void TemperatureControlLoop();

    public:

        IVCAMHardwareIO(uvc_context_t * ctx);
        ~IVCAMHardwareIO();

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

        void PrintParameters();

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

    class Projection
    {
    public:

        Projection(int indexOfCamera, bool RunThermalLoop = false);

        ~Projection(void);

        bool Init();

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

        int GetIndexOfCamera() { return m_IndexOfCamera; }

        IVCAMCalibrator * GetCalibrationObject() { return & m_calibration; }

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

        IVCAMCalibrator m_calibration;
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
