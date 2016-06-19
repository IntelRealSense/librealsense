// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <algorithm>

#include "hw-monitor.h"
#include "sr300-private.h"

using namespace rsimpl::hw_monitor;
using namespace rsimpl::iv;

namespace rsimpl {
namespace sr300 {

        struct SR300RawCalibration
        {
            uint16_t tableVersion;
            uint16_t tableID;
            uint32_t dataSize;
            uint32_t reserved;
            int crc;
            iv::camera_calib_params CalibrationParameters;
            uint8_t reserved_1[176];
            uint8_t reserved21[148];
        };

        enum class cam_data_source : uint32_t
        {
            TakeFromRO = 0,
            TakeFromRW = 1,
            TakeFromRAM = 2
        };

        ///////////////////
        // USB functions //
        ///////////////////
        void get_sr300_calibration_raw_data(uvc::device & device, std::timed_mutex & mutex, uint8_t * data, size_t & bytesReturned)
        {
            hwmon_cmd command((uint8_t)fw_cmd::GetCalibrationTable);
            command.Param1 = (uint32_t)cam_data_source::TakeFromRAM;

            perform_and_send_monitor_command(device, mutex, command);
            memcpy(data, command.receivedCommandData, HW_MONITOR_BUFFER_SIZE);
            bytesReturned = command.receivedCommandDataLength;
        }

        iv::camera_calib_params read_sr300_calibration(uvc::device & device, std::timed_mutex & mutex)
        {
            uint8_t rawCalibrationBuffer[HW_MONITOR_BUFFER_SIZE];
            size_t bufferLength = HW_MONITOR_BUFFER_SIZE;
            get_sr300_calibration_raw_data(device, mutex, rawCalibrationBuffer, bufferLength);

            SR300RawCalibration rawCalib;
            memcpy(&rawCalib, rawCalibrationBuffer, std::min(sizeof(rawCalib), bufferLength)); // Is this longer or shorter than the rawCalib struct?
            return rawCalib.CalibrationParameters;
        }

        void set_wakeup_device(uvc::device & device, std::timed_mutex & mutex, const uint32_t& phase1Period, const uint32_t& phase1FPS, const uint32_t& phase2Period, const uint32_t& phase2FPS)
        {
            hwmon_cmd cmd((uint8_t)fw_cmd::OnSuspendResume);

            wakeup_dev_params params = { phase1Period, static_cast<e_suspend_fps>(phase1FPS), phase2Period, static_cast<e_suspend_fps>(phase2FPS) };
            if (!params.isValid())
                throw std::logic_error("missing/invalid wake_up command parameters");
            cmd.Param1 = 1;                                                                 // TODO Specification could not be found in IVCAM
            auto trg = reinterpret_cast<wakeup_dev_params*>(cmd.data);
            *trg = params;
            cmd.sizeOfSendCommandData = sizeof(wakeup_dev_params);

            perform_and_send_monitor_command(device, mutex, cmd);
        }

        void reset_wakeup_device(uvc::device & device, std::timed_mutex & mutex)
        {
            hwmon_cmd cmd((uint8_t)fw_cmd::OnSuspendResume);

            perform_and_send_monitor_command(device, mutex, cmd);
        }

        void get_wakeup_reason(uvc::device & device, std::timed_mutex & mutex, unsigned char &cReason)
        {
            hwmon_cmd cmdWUReason((uint8_t)fw_cmd::GetWakeReason);

            perform_and_send_monitor_command(device, mutex, cmdWUReason);

            if (cmdWUReason.receivedCommandDataLength >= 4)     // TODO - better guard condition ?
            {
                unsigned char rslt = (*reinterpret_cast<int32_t *>(cmdWUReason.receivedCommandData)) && (0xFF);
                if (rslt >= (uint8_t)wakeonusb_reason::eMaxWakeOnReason)
                    throw std::logic_error("undefined wakeonusb_reason provided");
                cReason = rslt;
            }
            else
                throw std::runtime_error("no valid wakeonusb_reason provided");
        }

        void get_wakeup_confidence(uvc::device & device, std::timed_mutex & mutex, unsigned char &cConfidence)
        {
            hwmon_cmd cmdCnfd((uint8_t)fw_cmd::GetWakeConfidence);
            perform_and_send_monitor_command(device, mutex, cmdCnfd);

            if (cmdCnfd.receivedCommandDataLength >= 4)
            {
                int32_t rslt = *reinterpret_cast<int32_t *>(cmdCnfd.receivedCommandData);
                cConfidence = (unsigned char)(rslt & 0xFF);
            }
            else
                throw std::runtime_error("no valid wakeonusb_confidence provided");
        }

    } // namespace rsimpl::sr300
} // namespace rsimpl
