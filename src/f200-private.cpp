// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <cstring>
#include <algorithm>
#include <thread>
#include <cmath>

#include "hw-monitor.h"
#include "f200-private.h"

using namespace rsimpl::hw_monitor;
using namespace rsimpl::iv;

namespace rsimpl
{
    namespace f200
    {
        struct cam_asic_coefficients
        {
            float CoefValueArray[NUM_OF_CALIBRATION_COEFFS];
        };

        struct OACOffsetData
        {
            int OACOffset1;
            int OACOffset2;
            int OACOffset3;
            int OACOffset4;
        };

        struct IVCAMTesterData
        {
            int16_t                 TableValidation;
            int16_t                 TableVarsion;
            cam_temperature_data    TemperatureData;
            OACOffsetData           OACOffsetData_;
            thermal_loop_params  ThermalLoopParams;
        };

        ///////////////////
        // USB functions //
        ///////////////////

        void set_asic_coefficients(uvc::device & device, std::timed_mutex & mutex, const cam_asic_coefficients & coeffs)
        {
            // command.Param1 =
            // 0 - INVZ VGA (640x480)
            // 1 - INVZ QVGA (Possibly 320x240?)
            // 2 - INVZ HVGA (640x240)
            // 3 - INVZ 640x360
            // 4 - INVR VGA (640x480)
            // 5 - INVR QVGA (Possibly 320x240?)
            // 6 - INVR HVGA (640x240)
            // 7 - INVR 640x360

            hwmon_cmd command((uint8_t)fw_cmd::UpdateCalib);

            memcpy(command.data, coeffs.CoefValueArray, NUM_OF_CALIBRATION_COEFFS * sizeof(float));
            command.Param1 = 0; // todo - Send appropriate value at appropriate times, see above
            command.Param2 = 0;
            command.Param3 = 0;
            command.Param4 = 0;
            command.oneDirection = false;
            command.sizeOfSendCommandData = NUM_OF_CALIBRATION_COEFFS * sizeof(float);
            command.TimeOut = 5000;

            perform_and_send_monitor_command(device, mutex, command);
        }

        float read_mems_temp(uvc::device & device, std::timed_mutex & mutex)
        {
            hwmon_cmd command((uint8_t)fw_cmd::GetMEMSTemp);
            command.Param1 = 0;
            command.Param2 = 0;
            command.Param3 = 0;
            command.Param4 = 0;
            command.sizeOfSendCommandData = 0;
            command.TimeOut = 5000;
            command.oneDirection = false;

            perform_and_send_monitor_command(device, mutex, command);
            int32_t t = *reinterpret_cast<int32_t *>(command.receivedCommandData);
            return static_cast<float>(t) / 100;
        }

        int read_ir_temp(uvc::device & device, std::timed_mutex & mutex)
        {
            hwmon_cmd command((uint8_t)fw_cmd::GetIRTemp);
            command.Param1 = 0;
            command.Param2 = 0;
            command.Param3 = 0;
            command.Param4 = 0;
            command.sizeOfSendCommandData = 0;
            command.TimeOut = 5000;
            command.oneDirection = false;

            perform_and_send_monitor_command(device, mutex, command);
            return static_cast<int8_t>(command.receivedCommandData[0]);
        }

        void get_f200_calibration_raw_data(uvc::device & device, std::timed_mutex & usbMutex, uint8_t * data, size_t & bytesReturned)
        {
            uint8_t request[IVCAM_MONITOR_HEADER_SIZE];
            size_t requestSize = sizeof(request);
            uint32_t responseOp;

            if (prepare_usb_command(request, requestSize, (uint32_t)fw_cmd::GetCalibrationTable) <= 0)
                throw std::runtime_error("usb transfer to retrieve calibration data failed");
            execute_usb_command(device, usbMutex, 0, request, requestSize, responseOp, data, bytesReturned);
        }

        int bcdtoint(uint8_t * buf, int bufsize)
        {
            int r = 0;
            for (int i = 0; i < bufsize; i++)
                r = r * 10 + *buf++;
            return r;
        }

        int get_version_of_calibration(uint8_t * validation, uint8_t * version)
        {
            uint8_t valid[2] = { 0X14, 0x0A };
            if (memcmp(valid, validation, 2) != 0) return 0;
            else return bcdtoint(version, 2);
        }

        std::tuple<iv::camera_calib_params, cam_temperature_data, thermal_loop_params> get_f200_calibration(uint8_t * rawCalibData, size_t len)
        {
            uint8_t * bufParams = rawCalibData + 4;

            iv::cam_calibration CalibrationData;
            IVCAMTesterData TesterData;

            memset(&CalibrationData, 0, sizeof(cam_calibration));

            int ver = get_version_of_calibration(bufParams, bufParams + 2);

            if (ver > IVCAM_MIN_SUPPORTED_VERSION)
            {
                rawCalibData = rawCalibData + 4;

                size_t size = (sizeof(cam_calibration) > len) ? len : sizeof(cam_calibration);

                auto fixWithVersionInfo = [&](cam_calibration &d, int size, uint8_t * data)
                {
                    memcpy((uint8_t*)&d + sizeof(int), data, size - sizeof(int));
                };

                fixWithVersionInfo(CalibrationData, (int)size, rawCalibData);

                iv::camera_calib_params calibration;
                memcpy(&calibration, &CalibrationData.CalibrationParameters, sizeof(iv::camera_calib_params));
                memcpy(&TesterData, rawCalibData, SIZE_OF_CALIB_HEADER_BYTES); //copy the header: valid + version

                //copy the tester data from end of calibration
                int EndOfCalibratioData = SIZE_OF_CALIB_PARAM_BYTES + SIZE_OF_CALIB_HEADER_BYTES;
                memcpy((uint8_t*)&TesterData + SIZE_OF_CALIB_HEADER_BYTES, rawCalibData + EndOfCalibratioData, sizeof(IVCAMTesterData) - SIZE_OF_CALIB_HEADER_BYTES);
                return std::make_tuple(calibration, TesterData.TemperatureData, TesterData.ThermalLoopParams);
            }

            if (ver == IVCAM_MIN_SUPPORTED_VERSION)
            {
                float *params = (float *)bufParams;

                iv::camera_calib_params calibration;
                memcpy(&calibration, params + 1, sizeof(iv::camera_calib_params)); // skip the first float or 2 uint16
                memcpy(&TesterData, bufParams, SIZE_OF_CALIB_HEADER_BYTES);

                memset((uint8_t*)&TesterData + SIZE_OF_CALIB_HEADER_BYTES, 0, sizeof(IVCAMTesterData) - SIZE_OF_CALIB_HEADER_BYTES);
                return std::make_tuple(calibration, TesterData.TemperatureData, TesterData.ThermalLoopParams);
            }

            throw std::runtime_error("calibration table is not compatible with this API");
        }

        std::tuple<iv::camera_calib_params, cam_temperature_data, thermal_loop_params> read_f200_calibration(uvc::device & device, std::timed_mutex & mutex)
        {
            uint8_t rawCalibrationBuffer[HW_MONITOR_BUFFER_SIZE];
            size_t bufferLength = HW_MONITOR_BUFFER_SIZE;
            get_f200_calibration_raw_data(device, mutex, rawCalibrationBuffer, bufferLength);
            return get_f200_calibration(rawCalibrationBuffer, bufferLength);
        }

        void generate_asic_calibration_coefficients(const iv::camera_calib_params & compensated_calibration, std::vector<int> resolution, const bool isZMode, float * values)
        {
            auto params = compensated_calibration;

            //handle vertical crop at 360p - change to full 640x480 and crop at the end
            bool isQres = resolution[0] == 640 && resolution[1] == 360;

            if (isQres)
            {
                resolution[1] = 480;
            }

            //generate coefficients
            int scale = 5;

            float width = (float)resolution[0] * scale;
            float height = (float)resolution[1];

            int PrecisionBits = 16;
            int CodeBits = 14;
            int TexturePrecisionBits = 12;
            float ypscale = (float)(1 << (CodeBits + 1 - 10));
            float ypoff = 0;

            float s1 = (float)(1 << (PrecisionBits)) / 2047; // (range max)
            float s2 = (float)(1 << (CodeBits)) - ypscale*0.5f;

            float alpha = 2 / (width*params.Kc[0][0]);
            float beta = -(params.Kc[0][2] + 1) / params.Kc[0][0];
            float gamma = 2 / (height*params.Kc[1][1]);
            float delta = -(params.Kc[1][2] + 1) / params.Kc[1][1];

            float a = alpha / gamma;
            float a1 = 1;
            float b = 0.5f*scale*a + beta / gamma;
            float c = 0.5f*a1 + delta / gamma;

            float d0 = 1;
            float d1 = params.Invdistc[0] * pow(gamma, (float)2.0);
            float d2 = params.Invdistc[1] * pow(gamma, (float)4.0);
            float d5 = (float)((double)(params.Invdistc[4]) * pow((double)gamma, 6.0));
            float d3 = params.Invdistc[2] * gamma;
            float d4 = params.Invdistc[3] * gamma;

            float q = 1 / pow(gamma, (float)2.0);
            float p1 = params.Pp[2][3] * s1;
            float p2 = -s1*s2*(params.Pp[1][3] + params.Pp[2][3]);

            if (isZMode)
            {
                p1 = p1*sqrt(q);
                p2 = p2*sqrt(q);
            }

            float p3 = -params.Pp[2][0];
            float p4 = -params.Pp[2][1];
            float p5 = -params.Pp[2][2] / gamma;
            float p6 = s2*(params.Pp[1][0] + params.Pp[2][0]);
            float p7 = s2*(params.Pp[1][1] + params.Pp[2][1]);
            float p8 = s2*(params.Pp[1][2] + params.Pp[2][2]) / gamma;

            //Reprojection parameters
            float sreproj = 2;
            float ax = -(1 + params.Kp[0][2]) / params.Kp[0][0];
            float ay = -(1 + params.Kp[1][2]) / params.Kp[1][1];

            float f0 = (params.Pp[0][1] + params.Pp[2][1]) / (params.Pp[0][0] + params.Pp[2][0]) / params.Kp[0][0];
            float f1 = (params.Pp[0][2] + params.Pp[2][2]) / (params.Pp[0][0] + params.Pp[2][0]) / gamma / params.Kp[0][0];
            float f2 = 0; //(Pp(2,1)+Pp(3,1)) / (Pp(1,1)+Pp(3,1)) / Kp(5);
            float f3 = 0; //(Pp(2,2)+Pp(3,2)) / (Pp(1,1)+Pp(3,1)) / Kp(5);
            float f4 = 0; //(Pp(2,3)+Pp(3,3)) / (Pp(1,1)+Pp(3,1)) / gamma / Kp(5);
            float f5 = 2 * params.Pp[2][0] / (params.Pp[0][0] + params.Pp[2][0]) / sreproj;
            float f6 = 2 * params.Pp[2][1] / (params.Pp[0][0] + params.Pp[2][0]) / sreproj;
            float f7 = 2 * params.Pp[2][2] / (params.Pp[0][0] + params.Pp[2][0]) / sreproj / gamma;
            float f8 = (float)((double)(params.Pp[0][3] + params.Pp[2][3]) / (params.Pp[0][0] + params.Pp[2][0]) * s1 / params.Kp[0][0]);
            float f9 = (params.Pp[1][3] + params.Pp[2][3]) / (params.Pp[0][0] + params.Pp[2][0]) * s1 / params.Kp[1][1];
            float f10 = 2 * params.Pp[2][3] / (params.Pp[0][0] + params.Pp[2][0]) * s1 / sreproj;
            if (isZMode)
            {
                f8 = f8  * sqrt(q);
                f9 = 0; //f9  * sqrt(q);
                f10 = f10 * sqrt(q);
            }
            float f11 = 1 / params.Kp[0][0];

            // Fix projection center 
            f11 = f11 + ax*f5;
            f0 = f0 + ax*f6;
            f1 = f1 + ax*f7;
            f8 = f8 + ax*f10;
            f2 = f2 + ay*f5;
            f3 = f3 + ay*f6;
            f4 = f4 + ay*f7;
            f9 = f9 + ay*f10;

            // Texture coeffs
            float suv = (float)((1 << TexturePrecisionBits) - 1);

            float h0 = (params.Pt[0][1] + params.Pt[2][1]) / (params.Pt[0][0] + params.Pt[2][0]);
            float h1 = (params.Pt[0][2] + params.Pt[2][2]) / (params.Pt[0][0] + params.Pt[2][0]) / gamma;
            float h2 = (params.Pt[1][0] + params.Pt[2][0]) / (params.Pt[0][0] + params.Pt[2][0]);
            float h3 = (params.Pt[1][1] + params.Pt[2][1]) / (params.Pt[0][0] + params.Pt[2][0]);
            float h4 = (params.Pt[1][2] + params.Pt[2][2]) / (params.Pt[0][0] + params.Pt[2][0]) / gamma;
            float h5 = 2 * params.Pt[2][0] / (params.Pt[0][0] + params.Pt[2][0]) / suv;
            float h6 = 2 * params.Pt[2][1] / (params.Pt[0][0] + params.Pt[2][0]) / suv;
            float h7 = 2 * params.Pt[2][2] / (params.Pt[0][0] + params.Pt[2][0]) / suv / gamma;
            float h8 = (params.Pt[0][3] + params.Pt[2][3]) / (params.Pt[0][0] + params.Pt[2][0]) * s1;
            float h9 = (params.Pt[1][3] + params.Pt[2][3]) / (params.Pt[0][0] + params.Pt[2][0]) * s1;
            float h10 = 2 * params.Pt[2][3] / (params.Pt[0][0] + params.Pt[2][0]) * s1 / suv;
            float h11 = 1;

            if (isZMode)
            {
                h8 = h8  * sqrt(q);
                h9 = h9  * sqrt(q);
                h10 = h10 * sqrt(q);
            }

            float o1 = (1 + params.Kp[0][2]) / params.Kp[0][0];
            float o2 = -(1 + params.Kp[1][2]) / params.Kp[1][1];
            float o3 = 1 / s2 / params.Kp[1][1];
            float o4 = 0; //s2*(1+Kp(8));

            float dp1 = params.Distp[0];
            float dp2 = params.Distp[1];
            float dp3 = params.Distp[2];
            float dp4 = params.Distp[3];
            float dp5 = params.Distp[4];

            float ip0 = params.Kp[1][1] * s2;
            float ip1 = (s2*params.Invdistp[0] * params.Kp[1][1]) + s2*(1 + params.Kp[1][2]);
            float ip2 = (s2*params.Invdistp[1] * params.Kp[1][1]);
            float ip3 = (s2*params.Invdistp[2] * params.Kp[1][1]);
            float ip4 = (s2*params.Invdistp[3] * params.Kp[1][1]);
            float ip5 = (s2*params.Invdistp[4] * params.Kp[1][1]);

            if (isQres)
                c *= 0.75;

            //to simplify the ordering of the coefficients, initialize it in list syntax and copy to allocated memory.
            float coeffs[NUM_OF_CALIBRATION_COEFFS] = { 1.0f,3.0f,a,a1,b,c,d0,d1,d2,
                d3,d4,d5,q,p1,p2,p3,p4,p5,p6,p7,p8,h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11,f0,f1,
                f2,f3,f4,f5,f6,f7,f8,f9,f10,f11,o1,o2,o3,o4,dp1,dp2,dp3,dp4,dp5,ip0,ip1,ip2,ip3,
                ip4,ip5,ypscale,ypoff,0,0 };

            memcpy(values, coeffs, NUM_OF_CALIBRATION_COEFFS * sizeof(float));
            return;
        }

        void update_asic_coefficients(uvc::device & device, std::timed_mutex & mutex, const iv::camera_calib_params & compensated_params)
        {
            cam_asic_coefficients coeffs = {};
            generate_asic_calibration_coefficients(compensated_params, { 640, 480 }, true, coeffs.CoefValueArray); // todo - fix hardcoded resolution parameters
            set_asic_coefficients(device, mutex, coeffs);
        }

    } // namespace f200
} // namespace rsimpl
