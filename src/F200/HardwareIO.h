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

namespace rsimpl { namespace f200
{
    class IVCAMCalibrator;

    class IVCAMHardwareIO
    {
        libusb_device_handle * usbDeviceHandle = nullptr;
        std::timed_mutex usbMutex;

        CameraCalibrationParameters parameters;

        std::thread temperatureThread;
        std::atomic<bool> isTemperatureThreadRunning;

        std::unique_ptr<IVCAMCalibrator> calibration;

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

} } // namespace rsimpl::f200

#endif
