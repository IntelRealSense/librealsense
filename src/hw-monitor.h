// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "sensor.h"
#include <mutex>

namespace librealsense
{
    const uint8_t   IV_COMMAND_FIRMWARE_UPDATE_MODE = 0x01;
    const uint8_t   IV_COMMAND_GET_CALIBRATION_DATA = 0x02;
    const uint8_t   IV_COMMAND_LASER_POWER          = 0x03;
    const uint8_t   IV_COMMAND_DEPTH_ACCURACY       = 0x04;
    const uint8_t   IV_COMMAND_ZUNIT                = 0x05;
    const uint8_t   IV_COMMAND_LOW_CONFIDENCE_LEVEL = 0x06;
    const uint8_t   IV_COMMAND_INTENSITY_IMAGE_TYPE = 0x07;
    const uint8_t   IV_COMMAND_MOTION_VS_RANGE_TRADE= 0x08;
    const uint8_t   IV_COMMAND_POWER_GEAR           = 0x09;
    const uint8_t   IV_COMMAND_FILTER_OPTION        = 0x0A;
    const uint8_t   IV_COMMAND_VERSION              = 0x0B;
    const uint8_t   IV_COMMAND_CONFIDENCE_THRESHHOLD= 0x0C;

    const uint8_t   IVCAM_MONITOR_INTERFACE         = 0x4;
    const uint8_t   IVCAM_MONITOR_ENDPOINT_OUT      = 0x1;
    const uint8_t   IVCAM_MONITOR_ENDPOINT_IN       = 0x81;
    const uint8_t   IVCAM_MIN_SUPPORTED_VERSION     = 13;
    const uint8_t   IVCAM_MONITOR_HEADER_SIZE       = (sizeof(uint32_t) * 6);
    const uint8_t   NUM_OF_CALIBRATION_PARAMS       = 100;
    const uint8_t   PARAMETERS2_BUFFER_SIZE          = 50;
    const uint8_t   SIZE_OF_CALIB_HEADER_BYTES      = 4;
    const uint8_t   NUM_OF_CALIBRATION_COEFFS       = 64;

    const uint16_t  MAX_SIZE_OF_CALIB_PARAM_BYTES   = 800;
    const uint16_t  SIZE_OF_CALIB_PARAM_BYTES       = 512;
    const uint16_t  IVCAM_MONITOR_MAGIC_NUMBER      = 0xcdab;
    const uint16_t  IVCAM_MONITOR_MAX_BUFFER_SIZE   = 1024;
    const uint16_t  IVCAM_MONITOR_MUTEX_TIMEOUT     = 3000;
    const uint16_t  HW_MONITOR_COMMAND_SIZE         = 1000;
    const uint16_t  HW_MONITOR_BUFFER_SIZE          = 1024;
    const uint16_t  HW_MONITOR_DATA_SIZE_OFFSET     = 1020;
    const uint16_t  SIZE_OF_HW_MONITOR_HEADER       = 4;

    class uvc_sensor;

    enum hwmon_response: int32_t
    {
        hwm_Success                         =  0,
        hwm_WrongCommand                    = -1,
        hwm_StartNGEndAddr                  = -2,
        hwm_AddressSpaceNotAligned          = -3,
        hwm_AddressSpaceTooSmall            = -4,
        hwm_ReadOnly                        = -5,
        hwm_WrongParameter                  = -6,
        hwm_HWNotReady                      = -7,
        hwm_I2CAccessFailed                 = -8,
        hwm_NoExpectedUserAction            = -9,
        hwm_IntegrityError                  = -10,
        hwm_NullOrZeroSizeString            = -11,
        hwm_GPIOPinNumberInvalid            = -12,
        hwm_GPIOPinDirectionInvalid         = -13,
        hwm_IllegalAddress                  = -14,
        hwm_IllegalSize                     = -15,
        hwm_ParamsTableNotValid             = -16,
        hwm_ParamsTableIdNotValid           = -17,
        hwm_ParamsTableWrongExistingSize    = -18,
        hwm_WrongCRC                        = -19,
        hwm_NotAuthorisedFlashWrite         = -20,
        hwm_NoDataToReturn                  = -21,
        hwm_SpiReadFailed                   = -22,
        hwm_SpiWriteFailed                  = -23,
        hwm_SpiEraseSectorFailed            = -24,
        hwm_TableIsEmpty                    = -25,
        hwm_I2cSeqDelay                     = -26,
        hwm_CommandIsLocked                 = -27,
        hwm_CalibrationWrongTableId         = -28,
        hwm_ValueOutOfRange                 = -29,
        hwm_InvalidDepthFormat              = -30,
        hwm_DepthFlowError                  = -31,
        hwm_Timeout                         = -32,
        hwm_NotSafeCheckFailed              = -33,
        hwm_FlashRegionIsLocked             = -34,
        hwm_SummingEventTimeout             = -35,
        hwm_SDSCorrupted                    = -36,
        hwm_SDSVerifyFailed                 = -37,
        hwm_IllegalHwState                  = -38,
        hwm_RealtekNotLoaded                = -39,
        hwm_WakeUpDeviceNotSupported        = -40,
        hwm_ResourceBusy                    = -41,
        hwm_MaxErrorValue                   = -42,
        hwm_PwmNotSupported                 = -43,
        hwm_PwmStereoModuleNotConnected     = -44,
        hwm_UvcStreamInvalidStreamRequest   = -45,
        hwm_UvcControlManualExposureInvalid = -46,
        hwm_UvcControlManualGainInvalid     = -47,
        hwm_EyesafetyPayloadFailure         = -48,
        hwm_ProjectorTestFailed             = -49,
        hwm_ThreadModifyFailed              = -50,
        hwm_HotLaserPwrReduce               = -51, // reported to error depth XU control
        hwm_HotLaserDisable                 = -52, // reported to error depth XU control
        hwm_FlagBLaserDisable               = -53, // reported to error depth XU control
        hwm_NoStateChange                   = -54,
        hwm_EEPROMIsLocked                  = -55,
        hwm_OEMIdWrong                      = -56,
        hwm_RealtekNotUpdated               = -57,
        hwm_FunctionNotSupported            = -58,
        hwm_IspNotImplemented               = -59,
        hwm_IspNotSupported                 = -60,
        hwm_IspNotPermited                  = -61,
        hwm_IspNotExists                    = -62,
        hwm_IspFail                         = -63,
        hwm_Unknown                         = -64,
        hwm_LastError                       = hwm_Unknown - 1,
    };

    // Elaborate HW Monitor response
    const std::map< hwmon_response, std::string> hwmon_response_report = {
        { hwmon_response::hwm_Success,                      "Success" },
        { hwmon_response::hwm_WrongCommand,                 "WrongCommand" },
        { hwmon_response::hwm_StartNGEndAddr,               "StartNGEndAddr" },
        { hwmon_response::hwm_AddressSpaceNotAligned,       "AddressSpaceNotAligned" },
        { hwmon_response::hwm_AddressSpaceTooSmall,         "AddressSpaceTooSmall" },
        { hwmon_response::hwm_ReadOnly,                     "ReadOnly" },
        { hwmon_response::hwm_WrongParameter,               "WrongParameter" },
        { hwmon_response::hwm_HWNotReady,                   "HWNotReady" },
        { hwmon_response::hwm_I2CAccessFailed,              "I2CAccessFailed" },
        { hwmon_response::hwm_NoExpectedUserAction,         "NoExpectedUserAction" },
        { hwmon_response::hwm_IntegrityError,               "IntegrityError" },
        { hwmon_response::hwm_NullOrZeroSizeString,         "NullOrZeroSizeString" },
        { hwmon_response::hwm_GPIOPinNumberInvalid,         "GPIOPinNumberInvalid" },
        { hwmon_response::hwm_GPIOPinDirectionInvalid,      "GPIOPinDirectionInvalid" },
        { hwmon_response::hwm_IllegalAddress,               "IllegalAddress" },
        { hwmon_response::hwm_IllegalSize,                  "IllegalSize" },
        { hwmon_response::hwm_ParamsTableNotValid,          "ParamsTableNotValid" },
        { hwmon_response::hwm_ParamsTableIdNotValid,        "ParamsTableIdNotValid" },
        { hwmon_response::hwm_ParamsTableWrongExistingSize, "ParamsTableWrongExistingSize" },
        { hwmon_response::hwm_WrongCRC,                     "WrongCRC" },
        { hwmon_response::hwm_NotAuthorisedFlashWrite,      "NotAuthorisedFlashWrite" },
        { hwmon_response::hwm_NoDataToReturn,               "NoDataToReturn" },
        { hwmon_response::hwm_SpiReadFailed,                "SpiReadFailed" },
        { hwmon_response::hwm_SpiWriteFailed,               "SpiWriteFailed" },
        { hwmon_response::hwm_SpiEraseSectorFailed,         "SpiEraseSectorFailed" },
        { hwmon_response::hwm_TableIsEmpty,                 "TableIsEmpty" },
        { hwmon_response::hwm_I2cSeqDelay,                  "I2cSeqDelay" },
        { hwmon_response::hwm_CommandIsLocked,              "CommandIsLocked" },
        { hwmon_response::hwm_CalibrationWrongTableId,      "CalibrationWrongTableId" },
        { hwmon_response::hwm_ValueOutOfRange,              "ValueOutOfRange" },
        { hwmon_response::hwm_InvalidDepthFormat,           "InvalidDepthFormat" },
        { hwmon_response::hwm_DepthFlowError,               "DepthFlowError" },
        { hwmon_response::hwm_Timeout,                      "Timeout" },
        { hwmon_response::hwm_NotSafeCheckFailed,           "NotSafeCheckFailed" },
        { hwmon_response::hwm_FlashRegionIsLocked,          "FlashRegionIsLocked" },
        { hwmon_response::hwm_SummingEventTimeout,          "SummingEventTimeout" },
        { hwmon_response::hwm_SDSCorrupted,                 "SDSCorrupted" },
        { hwmon_response::hwm_SDSVerifyFailed,              "SDSVerifyFailed" },
        { hwmon_response::hwm_IllegalHwState,               "IllegalHwState" },
        { hwmon_response::hwm_RealtekNotLoaded,             "RealtekNotLoaded" },
        { hwmon_response::hwm_WakeUpDeviceNotSupported,     "WakeUpDeviceNotSupported" },
        { hwmon_response::hwm_ResourceBusy,                 "ResourceBusy" },
        { hwmon_response::hwm_MaxErrorValue,                "MaxErrorValue" },
        { hwmon_response::hwm_PwmNotSupported,              "PwmNotSupported" },
        { hwmon_response::hwm_PwmStereoModuleNotConnected,  "PwmStereoModuleNotConnected" },
        { hwmon_response::hwm_UvcStreamInvalidStreamRequest,"UvcStreamInvalidStreamRequest" },
        { hwmon_response::hwm_UvcControlManualExposureInvalid,"UvcControlManualExposureInvalid" },
        { hwmon_response::hwm_UvcControlManualGainInvalid,  "UvcControlManualGainInvalid" },
        { hwmon_response::hwm_EyesafetyPayloadFailure,      "EyesafetyPayloadFailure" },
        { hwmon_response::hwm_ProjectorTestFailed,          "ProjectorTestFailed" },
        { hwmon_response::hwm_ThreadModifyFailed,           "ThreadModifyFailed" },
        { hwmon_response::hwm_HotLaserPwrReduce,            "HotLaserPwrReduce" },
        { hwmon_response::hwm_HotLaserDisable,              "HotLaserDisable" },
        { hwmon_response::hwm_FlagBLaserDisable,            "FlagBLaserDisable" },
        { hwmon_response::hwm_NoStateChange,                "NoStateChange" },
        { hwmon_response::hwm_EEPROMIsLocked,               "EEPROMIsLocked" },
        { hwmon_response::hwm_EEPROMIsLocked,               "EEPROMIsLocked" },
        { hwmon_response::hwm_OEMIdWrong,                   "OEMIdWrong" },
        { hwmon_response::hwm_RealtekNotUpdated,            "RealtekNotUpdated" },
        { hwmon_response::hwm_FunctionNotSupported,         "FunctionNotSupported" },
        { hwmon_response::hwm_IspNotImplemented,            "IspNotImplemented" },
        { hwmon_response::hwm_IspNotSupported,              "IspNotSupported" },
        { hwmon_response::hwm_IspNotPermited,               "IspNotPermited" },
        { hwmon_response::hwm_IspNotExists,                 "IspNotExists" },
        { hwmon_response::hwm_IspFail,                      "IspFail" },
        { hwmon_response::hwm_Unknown,                      "Unknown" },
        { hwmon_response::hwm_LastError,                    "LastError" },
    };

    inline std::string hwmon_error2str(hwmon_response e) {
        if (hwmon_response_report.find(e) != hwmon_response_report.end())
            return hwmon_response_report.at(e);
        return {};
    }

    class locked_transfer
    {
    public:
        locked_transfer(std::shared_ptr<platform::command_transfer> command_transfer, uvc_sensor& uvc_ep)
            :_command_transfer(command_transfer),
             _uvc_sensor_base(uvc_ep)
        {}

        std::vector<uint8_t> send_receive(
            const std::vector<uint8_t>& data,
            int timeout_ms = 5000,
            bool require_response = true)
        {
            std::lock_guard<std::recursive_mutex> lock(_local_mtx);
            return _uvc_sensor_base.invoke_powered([&]
                (platform::uvc_device& dev)
                {
                    std::lock_guard<platform::uvc_device> lock(dev);
                    return _command_transfer->send_receive(data, timeout_ms, require_response);
                });
        }

    private:
        std::shared_ptr<platform::command_transfer> _command_transfer;
        uvc_sensor& _uvc_sensor_base;
        std::recursive_mutex _local_mtx;
    };

    struct command
    {
        uint8_t cmd;
        int     param1;
        int     param2;
        int     param3;
        int     param4;
        std::vector<uint8_t> data;
        int     timeout_ms = 5000;
        bool    require_response = true;

        explicit command(uint8_t cmd, int param1 = 0, int param2 = 0,
                int param3 = 0, int param4 = 0, int timeout_ms = 5000,
                bool require_response = true)
            : cmd(cmd), param1(param1),
              param2(param2),
              param3(param3), param4(param4),
              timeout_ms(timeout_ms), require_response(require_response)
        {
        }
    };

    class hw_monitor
    {
        struct hwmon_cmd
        {
            uint8_t     cmd;
            int         param1;
            int         param2;
            int         param3;
            int         param4;
            uint8_t     data[HW_MONITOR_BUFFER_SIZE];
            int         sizeOfSendCommandData;
            long        timeOut;
            bool        oneDirection;
            uint8_t     receivedCommandData[HW_MONITOR_BUFFER_SIZE];
            size_t      receivedCommandDataLength;
            uint8_t     receivedOpcode[4];

            explicit hwmon_cmd(uint8_t cmd_id)
                : cmd(cmd_id),
                  param1(0),
                  param2(0),
                  param3(0),
                  param4(0),
                  sizeOfSendCommandData(0),
                  timeOut(5000),
                  oneDirection(false),
                  receivedCommandDataLength(0)
            {}


            explicit hwmon_cmd(const command& cmd)
                : cmd(cmd.cmd),
                  param1(cmd.param1),
                  param2(cmd.param2),
                  param3(cmd.param3),
                  param4(cmd.param4),
                  sizeOfSendCommandData(std::min((uint16_t)cmd.data.size(), HW_MONITOR_BUFFER_SIZE)),
                  timeOut(cmd.timeout_ms),
                  oneDirection(!cmd.require_response),
                  receivedCommandDataLength(0)
            {
                librealsense::copy(data, cmd.data.data(), sizeOfSendCommandData);
            }
        };

        struct hwmon_cmd_details
        {
            bool                                         oneDirection;
            std::array<uint8_t, HW_MONITOR_COMMAND_SIZE> sendCommandData;
            int                                          sizeOfSendCommandData;
            long                                         timeOut;
            std::array<uint8_t, 4>                       receivedOpcode;
            std::array<uint8_t, HW_MONITOR_BUFFER_SIZE>  receivedCommandData;
            size_t                                       receivedCommandDataLength;
        };

        static void fill_usb_buffer(int opCodeNumber, int p1, int p2, int p3, int p4, uint8_t* data, int dataLength, uint8_t* bufferToSend, int& length);
        void execute_usb_command(uint8_t *out, size_t outSize, uint32_t& op, uint8_t* in, size_t& inSize) const;
        static void update_cmd_details(hwmon_cmd_details& details, size_t receivedCmdLen, unsigned char* outputBuffer);
        void send_hw_monitor_command(hwmon_cmd_details& details) const;

        std::shared_ptr<locked_transfer> _locked_transfer;
    public:
        explicit hw_monitor(std::shared_ptr<locked_transfer> locked_transfer)
            : _locked_transfer(std::move(locked_transfer))
        {}

        std::vector<uint8_t> send(std::vector<uint8_t> data) const;
        std::vector<uint8_t> send(command cmd) const;
        void get_gvd(size_t sz, unsigned char* gvd, uint8_t gvd_cmd) const;
        std::string get_firmware_version_string(int gvd_cmd, uint32_t offset) const;
        std::string get_module_serial_string(uint8_t gvd_cmd, uint32_t offset, int size = 6) const;
        bool is_camera_locked(uint8_t gvd_cmd, uint32_t offset) const;
    };
}
