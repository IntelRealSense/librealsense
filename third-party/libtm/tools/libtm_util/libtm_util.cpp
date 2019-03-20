// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#define LOG_TAG "libtmutil"
#include "Log.h"
#include "TrackingManager.h"
#include <algorithm>
#include <array>
#include <map>
#include <thread>
#include <iostream>
#include <string>
#include <list>
#include <vector>
#include <cinttypes>
#include <fstream> 
#include <sstream> 
#include <atomic>
#include <time.h>
#include <iomanip>
#include <mutex>
#include <limits>
#include <stdlib.h>
#include "Version.h"
#include "Utils.h"
#include "fw.h"

#define LIBTM_UTIL_VERSION_MAJOR 1
#define LIBTM_UTIL_VERSION_MINOR 0

#define MAX_RUN_TIME_SEC 86400 /* 24 hours */
#define MIN_RUN_TIME_SEC 2
#define START_STREAM_TIME_SEC 10
#define STOP_STREAM_TIME_SEC 2
#define RESET_DEVICE_TIME_SEC 5
#define MAX_SUPPORTED_RAW_STREAMS 20
#define MAX_START_STOP_LOOP_COUNT 1
#define WAIT_FOR_DEVICE_SEC 1
#define WAIT_FOR_CONTROLLERS_MSEC 100
#define MAX_WAIT_FOR_CONTROLLERS_MSEC (3000 * WAIT_FOR_CONTROLLERS_MSEC)
#define MAX_WAIT_FOR_CALIBRATED_CONTROLLERS_MSEC (600 * WAIT_FOR_CONTROLLERS_MSEC)
#define WAIT_FOR_LOCALIZATION_SEC 1
#define MAX_WAIT_FOR_LOCALIZATION_SEC (WAIT_FOR_LOCALIZATION_SEC * 10)
#define MAX_FIRST_POSE_DELAY_MSEC 1200
#define MAX_LOCALIZATION_MAP_SIZE 104857600 /* 100 MB*/
#define CONTROLLER_CONNECT_TIMEOUT_MSEC 16000
#define MAX_WAIT_FOR_LOG_MSEC 500
#define MIN_WAIT_FOR_LOG_MSEC 4
#define WAIT_FOR_LOG_MULTIPLIER_STEP 5
#define MAX_WAIT_FOR_IMAGE_MSEC 100
#define TEMPERATURE_THRESHOLD_OVERRIDE_KEY 0xC10F
#define MAX_WAIT_FOR_STATIC_NODE_MSEC 10
#define MAX_FRAME_LATENCY_MSEC 100

/* Max float accuracy */
typedef std::numeric_limits<float> flt;

std::ostringstream poseCSV;
std::ostringstream poseTUM;
std::ostringstream videoCSV;
std::ostringstream gyroCSV;
std::ostringstream velocimeterCSV;
std::ostringstream accelerometerCSV;
std::ostringstream controllerCSV;
std::ostringstream rssiCSV;
char gFileHeaderName[80];

using namespace std;
shared_ptr<perc::TrackingManager> gManagerInstance;

using namespace perc;


const std::string statusToString(perc::Status status)
{
    switch (status)
    {
        case Status::SUCCESS:                                     return "SUCCESS";
        case Status::COMMON_ERROR:                                return "COMMON_ERROR";
        case Status::FEATURE_UNSUPPORTED:                         return "FEATURE_UNSUPPORTED";
        case Status::ERROR_PARAMETER_INVALID:                     return "ERROR_PARAMETER_INVALID";
        case Status::INIT_FAILED:                                 return "INIT_FAILED";
        case Status::ALLOC_FAILED:                                return "ALLOC_FAILED";
        case Status::ERROR_USB_TRANSFER:                          return "ERROR_USB_TRANSFER";
        case Status::ERROR_EEPROM_VERIFY_FAIL:                    return "ERROR_EEPROM_VERIFY_FAIL";
        case Status::ERROR_FW_INTERNAL:                           return "ERROR_FW_INTERNAL";
        case Status::BUFFER_TOO_SMALL:                            return "BUFFER_TOO_SMALL";
        case Status::NOT_SUPPORTED_BY_FW:                         return "NOT_SUPPORTED_BY_FW";
        case Status::DEVICE_BUSY:                                 return "DEVICE_BUSY";
        case Status::TIMEOUT:                                     return "TIMEOUT";
        case Status::TABLE_NOT_EXIST:                             return "TABLE_NOT_EXIST";
        case Status::TABLE_LOCKED:                                return "TABLE_LOCKED";
        case Status::DEVICE_STOPPED:                              return "DEVICE_STOPPED";
        case Status::TEMPERATURE_WARNING:                         return "TEMPERATURE_WARNING";
        case Status::TEMPERATURE_STOP:                            return "TEMPERATURE_STOP";
        case Status::CRC_ERROR:                                   return "CRC_ERROR";
        case Status::INCOMPATIBLE:                                return "INCOMPATIBLE";
        case Status::SLAM_NO_DICTIONARY:                          return "SLAM_NO_DICTIONARY";
        case Status::NO_CALIBRATION_DATA:                         return "NO_CALIBRATION_DATA";
        case Status::SLAM_LOCALIZATION_DATA_SET_SUCCESS:          return "SLAM_LOCALIZATION_DATA_SET_SUCCESS";
        case Status::SLAM_ERROR_CODE_VISION:                      return "SLAM_ERROR_CODE_VISION";
        case Status::SLAM_ERROR_CODE_SPEED:                       return "SLAM_ERROR_CODE_SPEED";
        case Status::SLAM_ERROR_CODE_OTHER:                       return "SLAM_ERROR_CODE_OTHER";
        case Status::CONTROLLER_CALIBRATION_VALIDATION_FAILURE:   return "CONTROLLER_CALIBRATION_VALIDATION_FAILURE";
        case Status::CONTROLLER_CALIBRATION_FLASH_ACCESS_FAILURE: return "CONTROLLER_CALIBRATION_FLASH_ACCESS_FAILURE";
        case Status::CONTROLLER_CALIBRATION_IMU_FAILURE:          return "CONTROLLER_CALIBRATION_IMU_FAILURE";
        case Status::CONTROLLER_CALIBRATION_INTERNAL_FAILURE:     return "CONTROLLER_CALIBRATION_INTERNAL_FAILURE";
        default:                                                  return "UNKNOWN STATUS";
    }
}

const char* fwLogVerbosityLevel(LogVerbosityLevel level)
{
    switch (level)
    {
        case Error:   return "E";
        case Info:    return "I";
        case Warning: return "W";
        case Debug:   return "D";
        case Verbose: return "V";
        case Trace:   return "T";
    }
    return "X";
}

class CalibrationInfo {
public:
    CalibrationInfo() : accelerometerBiasX(0), accelerometerBiasY(0), accelerometerBiasZ(0) {
        memset(accelerometerScale, 0, sizeof(accelerometerScale));
    }
    float accelerometerScale[3][3];
    float accelerometerBiasX;
    float accelerometerBiasY;
    float accelerometerBiasZ;
};

class UtilTimeStamps {
public:
    class Loadtime {
    public:
        unsigned long long fw;
        unsigned long long host;
        unsigned long long hostCorrelated;
    } loadTime = { 0,0,0 };

    UtilTimeStamps(TrackingData::TimestampedData timeStampData)
    {
        loadTime.fw = timeStampData.timestamp;
        loadTime.hostCorrelated = timeStampData.systemTimestamp;
    };

    void setTime(TrackingData::TimestampedData timeStampData)
    {
        hostCurrentSystemTime = systemTime();
        if (loadTime.host == 0)
        {
            loadTime.host = hostCurrentSystemTime;
        }
        hostTimeStamp = hostCurrentSystemTime - loadTime.host;
        hostCorrelatedTimeStamp = timeStampData.systemTimestamp - loadTime.hostCorrelated;
        fwTimeStamp = timeStampData.timestamp;
        arrivalTimeStamp = timeStampData.arrivalTimeStamp;
        latency = hostCurrentSystemTime - timeStampData.systemTimestamp;
    }

    unsigned long long fwTimeStamp;
    unsigned long long hostTimeStamp;
    unsigned long long hostCorrelatedTimeStamp;
    unsigned long long arrivalTimeStamp;
    unsigned long long hostCurrentSystemTime;
    unsigned long long latency;
};

enum LocalizationType
{
    LocalizationTypeGet = 0,
    LocalizationTypeSet = 1,
    LocalizationTypeReset = 2,
    LocalizationTypeMax = 3,
};

enum ControllerBurnState
{
    ControllerBurnStateNotStarted = 0, /* Controller FW burn not started             */
    ControllerBurnStateProcessing = 1, /* Controller FW burn is currently in process */
    ControllerBurnStateDone = 2,       /* Controller FW burn is done                 */
    ControllerBurnStateMax = 3,
};

enum ControllerBurnConfigure
{
    ControllerBurnDisabled = 0,     /* Don't burn controller on discovery                                            */
    ControllerBurnOnNewVersion = 1, /* Burn controller on discovery if image version is newer than discovery version */
    ControllerBurnForce = 2,        /* Burn controller on discovery                                                  */
    ControllerBurnMax = 3,
};

class ArgumentConfigurarion {
public:
    ArgumentConfigurarion() : inputFilename(""), controllerDataFilename(""), velocimeterFilename(""), nodeFilename(""), temperature{0}, maxLoop(MAX_START_STOP_LOOP_COUNT), startStreamTime(START_STREAM_TIME_SEC), stopStreamTime(STOP_STREAM_TIME_SEC),
        resetLoop(0), statistics(false), jtag(false), errorCheck(false), errorExit(false), videoFile(false), videoCount(0), gyroCount(0), velocimeterCount(0), accelerometerCount(0), sixdofCount(0),
        controllersCount(0), setExposure(false), verifyConfiguration(true), geoLocationEnabled(false), gpioEnabled(0), gpioControlBitMask(0), mode("live"), stereoMode(false), tumFormat(0), lowPowerEnabled(true)
    {
        for (uint8_t i = 0; i < LogSourceMax; i++)
        {
            logConfiguration[i].setLog = false;
            logConfiguration[i].logOutputMode = LogOutputModeScreen;
            logConfiguration[i].logRolloverMode = false;
            logConfiguration[i].logVerbosity = None;
        }

        for (uint8_t i = 0; i < VideoProfileMax; i++)
        {
            video[i].enabled = false;
            video[i].outputEnabled = false;
        }

        for (uint8_t i = 0; i < GyroProfileMax; i++)
        {
            gyro[i].enabled = false;
            gyro[i].outputEnabled = false;
            gyro[i].fps = 0;
        }

        for (uint8_t i = 0; i < AccelerometerProfileMax; i++)
        {
            accelerometer[i].enabled = false;
            accelerometer[i].outputEnabled = false;
            accelerometer[i].fps = 0;
        }

        for (uint8_t i = 0; i < SixDofProfileMax; i++)
        {
            sixdof[i].enabled = false;
            sixdof[i].mode = SIXDOF_MODE_MAX;
        }

        for (uint8_t i = 0; i < ProfileTypeMax; i++)
        {
            memset(controllers[i].macAddress, 0, MAC_ADDRESS_SIZE);
            controllers[i].controllerID = ProfileTypeMax;
            controllers[i].needed = false;
        }

        for (uint8_t i = 0; i < ProfileTypeMax; i++)
        {
            rssi[i].time = 0;
        }

        for (uint8_t i = 0; i < LocalizationTypeMax; i++)
        {
            localization[i].enabled = false;
            localization[i].filename = "";
        }

        geoLocation.set(0, 0, 0);
    }

    std::string inputFilename;
    std::string controllerDataFilename;
    std::string velocimeterFilename;
    std::string nodeFilename;
    std::string recordFilename;
    std::string mode;
    bool stereoMode;
    uint32_t maxLoop;
    uint32_t startStreamTime;
    uint32_t stopStreamTime;

    class Localization {
    public:
        std::fstream file;
        std::string filename;
        bool enabled;
    };

    Localization localization[LocalizationTypeMax];

    class Calibration {
    public:
        CalibrationType type;
        std::string filename;
    };

    Calibration calibration;
    bool lowPowerEnabled;

    TrackingData::GeoLocalization geoLocation;
    bool geoLocationEnabled;

    uint8_t gpioControlBitMask;
    bool gpioEnabled;

    class Enable {
    public:
        bool enabled;
        bool outputEnabled;
        uint16_t width;
        uint16_t height;
        uint16_t fps;
    };

    class ControllerBurn {
    public:
        ControllerBurn() : configure(ControllerBurnDisabled), state(ControllerBurnStateNotStarted) {}
        ControllerBurnConfigure configure;
        ControllerBurnState state;
    };

    class Controller {
    public:
        uint8_t macAddress[MAC_ADDRESS_SIZE];
        uint8_t controllerID;
        ControllerBurn burn;
        bool calibrate;
        bool needed;
    };

    class Rssi {
    public:
        uint32_t time;
    };

    ProfileType findController(uint8_t* macAddress)
    {
        for (uint32_t i = Controller1; i < ProfileTypeMax; i++)
        {
            if (memcmp(macAddress, controllers[i].macAddress, MAC_ADDRESS_SIZE) == 0)
            {
                return (ProfileType)i;
            }
        }

        return ProfileTypeMax;
    }

    Enable video[VideoProfileMax];
    uint8_t videoCount;

    Enable gyro[GyroProfileMax];
    uint8_t gyroCount;

    Enable velocimeter[VelocimeterProfileMax];
    uint8_t velocimeterCount;

    Enable accelerometer[AccelerometerProfileMax];
    uint8_t accelerometerCount;

    class Sixdof {
    public:
        bool enabled;
        uint8_t mode;
    };

    Sixdof sixdof[SixDofProfileMax];
    uint8_t sixdofCount;

    std::string controllerFWFile;
    std::string controllerFWFileType;
    Controller controllers[ProfileTypeMax];
    uint8_t controllersCount;

    Rssi rssi[ProfileTypeMax];
    uint8_t rssiCount;

    class Temperature {
    public:
        class Threshold {
        public:
            float_t temperature;
            bool set;
        };

        bool check;
        Threshold threshold[TemperatureSensorMax];
    };

    Temperature temperature;

    uint32_t resetLoop;
    bool statistics;
    bool jtag;
    bool videoFile;
    bool errorCheck;
    bool errorExit;
    uint8_t tumFormat;

    class LogConfiguration
    {
    public:
        bool setLog;
        bool logRolloverMode;
        LogOutputMode logOutputMode;
        LogVerbosityLevel logVerbosity;
    };

    LogConfiguration logConfiguration[LogSourceMax];

    bool setExposure;

    TrackingData::Exposure exposure;

    bool verifyConfiguration;
};
ArgumentConfigurarion gConfiguration;

class SensorStatistics {
public:
    SensorStatistics() : frames(0), frameRate(0), outputMode(0), enabled(0) {}

    void reset(void)
    {
        frames = 0;
        frameRate = 0;
        totalLatency = 0;
        nanFrame = 0;
        enabled = 0;
        outputMode = 0;
        frameDrops = 0;
        frameReorder = 0;
        prevFrameId = 0;
        prevFrameTimeStamp = 0;
    }

    uint64_t frames;            /* Number of frames                                                                                       */
    uint64_t frameDrops;        /* Number of frame drops                                                                                  */
    uint64_t frameReorder;      /* Number of frame reorder                                                                                */
    uint16_t frameRate;         /* Frame per second                                                                                       */
    uint64_t totalLatency;      /* total Latency (Motion->Host) in NanoSec - for average latency - divide with frames                     */
    uint64_t nanFrame;          /* pose NAN frame received                                                                                */
    uint8_t  enabled;           /* Enabled/Disabled on FW                                                                                 */
    uint8_t  outputMode;        /* For Video/Gyro/Accelerometer: 0x0 - send sensor outputs to the internal middlewares only               */
                                /*                               0x1 - Send this sensor outputs also to the host over the USB interface   */
                                /* For Pose: 0x0 - no interrupts                                                                          */
                                /*           0x1 - interrupts on every fisheye camera update (30 interrupts per second for TM2)           */
                                /*           0x2 - interrupts on every IMU update (400-500 interrupts per second for TM2) - default value */
    uint32_t prevFrameId;       /* Previous frame ID - for drop check                                                                     */
    int64_t prevFrameTimeStamp; /* Previous frame TimeStamp - for drop check                                                              */
};

class GyroStatistics : public SensorStatistics {};
class VelocimeterStatistics : public SensorStatistics {};
class VideoStatistics : public SensorStatistics {};
class AccelerometerStatistics : public SensorStatistics {};
class PoseStatistics : public SensorStatistics {
public:
    void reset(void)
    {
        SensorStatistics::reset();
        trackerConfidence = 0;
        trackerState = 0;
    }

    uint32_t trackerConfidence{0};
    uint32_t trackerState{0};
};
class ControllerStatistics : public SensorStatistics {
public:
    std::atomic<bool> isConnected;
    std::atomic<bool> isCalibrated;
    uint32_t FWProgress;
};
class RssiStatistics : public SensorStatistics {};
class LocalizationStatistics : public SensorStatistics {
public:
    void reset(void)
    {
        SensorStatistics::reset();
        isCompleted = false;
        fileSize = 0;
    }

    bool isCompleted;
    uint64_t fileSize;
};

class ControllerDiscoveryEventStatistics {
public:
    ControllerDiscoveryEventStatistics() : frames(0) 
    {
        memset(macAddress, 0, MAC_ADDRESS_SIZE);
    }

    ControllerDiscoveryEventStatistics(uint8_t* _macAddress) : frames(0) 
    {
        perc::copy(macAddress, _macAddress, MAC_ADDRESS_SIZE);
    }

    uint8_t macAddress[MAC_ADDRESS_SIZE]; /* Controller Mac Address */
    uint64_t frames;                      /* Number of frames       */
};

class ErrorStatistics {
public:
    ErrorStatistics() : count(0) {
    }

    void reset(void)
    {
        count = 0;
    }

    uint32_t count; /* Number of errors */
};

class LedStatistics {
public:
    LedStatistics() : count(0) {
    }

    void reset(void)
    {
        count = 0;
    }

    uint32_t count; /* Number of led events */
};

class StatusStatistics {
public:
    StatusStatistics() : count(0) {
    }

    void reset(void)
    {
        count = 0;
    }

    uint32_t count; /* Number of statuses */
};

class RunTimeStatistics {
public:
    RunTimeStatistics() : startTimeMsec(0), stopTimeMsec(0), diffMsec(0), startToken(0), stopToken(0) {}

    void reset(void)
    {
        startTimeMsec = 0;
        stopTimeMsec = 0;
        diffMsec = 0;
        startToken = 0;
        stopToken = 0;
    }

    void start(void)
    {
        if (startToken == stopToken)
        {
            startTimeMsec = ns2ms(systemTime());
            stopTimeMsec = 0;
            diffMsec = 0;
            startToken++;
        }

        /* Else already started, do nothing */

    }

    void stop(void)
    {
        if (startToken == (stopToken + 1))
        {
            stopTimeMsec = ns2ms(systemTime());
            diffMsec = (stopTimeMsec - startTimeMsec);
            stopToken++;
        }

        /* Else already stopped, do nothing */
    }

    uint32_t startTimeMsec;
    uint32_t stopTimeMsec;
    uint32_t diffMsec;
    uint32_t startToken;
    uint32_t stopToken;
};

class Statistics {
public:
    Statistics() {}
    ~Statistics()
    {
        this->reset();
    }

    /* Reset all Statistics fields */
    void reset(void)
    {
        for (uint8_t i = 0; i < VideoProfileMax; i++)
        {
            video[i].reset();
        }

        for (uint8_t i = 0; i < GyroProfileMax; i++)
        {
            gyro[i].reset();
        }

        for (uint8_t i = 0; i < VelocimeterProfileMax; i++)
        {
            velocimeter[i].reset();
        }

        for (uint8_t i = 0; i < AccelerometerProfileMax; i++)
        {
            accelerometer[i].reset();
        }

        for (uint8_t i = 0; i < SixDofProfileMax; i++)
        {
            pose[i].reset();
        }

        for (uint8_t i = Controller1; i < ProfileTypeMax; i++)
        {
            controller[i].reset();
        }

        for (uint8_t i = Controller1; i < ProfileTypeMax; i++)
        {
            rssi[i].reset();
        }

        for (uint8_t i = LocalizationTypeGet; i < LocalizationTypeMax; i++)
        {
            localization[i].reset();
        }

        for (uint8_t i = 0; i < ProfileTypeMax; i++)
        {
            runTime[i].reset();
        }

        controllerDiscoveryEventVector.clear();

        error.reset();
        status.reset();
        led.reset();
    }

    void print()
    {
        LOGD("Statistics");
        LOGD("--------------------------+-------------------+--------+---------+--------+------------+----------+----------+--------");
        LOGD("           Type           |       Info        | Frames | Enabled | Output | Latency    | Run Time | Expected | Actual ");
        LOGD("                          |                   | PerSec |         | Mode   | AVG (mSec) | (mSec)   | Frames   | Frames ");
        LOGD("--------------------------+-------------------+--------+---------+--------+------------+----------+----------+--------");

        for (uint8_t i = 0; i < VideoProfileMax; i++)
        {
            LOGD(" Video[%01d]                 | %-17s | %-6d | 0x%01X     | 0x%01X    | %-10"PRId64" | %-8d | %-8d | %d", i, (i > VideoProfile1) ? "Low Exposure " : "High Exposure",
                video[i].frameRate, video[i].enabled, video[i].outputMode, (video[i].frames > 0) ? (video[i].totalLatency / video[i].frames) : 0, runTime[HMD].diffMsec, (video[i].frameRate * video[i].enabled * video[i].outputMode *  runTime[HMD].diffMsec) / 1000, video[i].frames);
        }

        for (uint8_t i = 0; i < GyroProfileMax; i++)
        {
            LOGD(" Gyro[%01d]                  | %-17s | %-6d | 0x%01X     | 0x%01X    | %-10"PRId64" | %-8d | %-8d | %d", i, (i > GyroProfile0) ? ((i > GyroProfile1) ? "Controller 2" : "Controller 1") : "HMD",
                gyro[i].frameRate, gyro[i].enabled, gyro[i].outputMode, (gyro[i].frames > 0) ? (gyro[i].totalLatency / gyro[i].frames) : 0, runTime[i].diffMsec, (gyro[i].frameRate * gyro[i].enabled * gyro[i].outputMode * runTime[i].diffMsec) / 1000, gyro[i].frames);
        }

        for (uint8_t i = 0; i < AccelerometerProfileMax; i++)
        {
            LOGD(" Accelerometer[%01d]         | %-17s | %-6d | 0x%01X     | 0x%01X    | %-10"PRId64" | %-8d | %-8d | %d", i, (i > AccelerometerProfile0) ? ((i > AccelerometerProfile1) ? "Controller 2" : "Controller 1") : "HMD",
                accelerometer[i].frameRate, accelerometer[i].enabled, accelerometer[i].outputMode, (accelerometer[i].frames > 0) ? (accelerometer[i].totalLatency / accelerometer[i].frames) : 0, runTime[i].diffMsec, (accelerometer[i].frameRate * accelerometer[i].enabled * accelerometer[i].outputMode * runTime[i].diffMsec) / 1000, accelerometer[i].frames);
        }

        for (uint8_t i = 0; i < VelocimeterProfileMax; i++)
        {
            LOGD(" Velocimeter[%01d]           | External Sensor   |        | 0x%01X     | 0x%01X    | %-10"PRId64" | %-8d |          | %d", i, 
                velocimeter[i].enabled, velocimeter[i].outputMode, (velocimeter[i].frames > 0) ? (velocimeter[i].totalLatency / velocimeter[i].frames) : 0, runTime[HMD].diffMsec, velocimeter[i].frames);
        }

        for (uint8_t i = 0; i < SixDofProfileMax; i++)
        {
            uint32_t poseRunTimeMsec = 0;

            if (runTime[i].diffMsec > MAX_FIRST_POSE_DELAY_MSEC)
            {
                poseRunTimeMsec = (runTime[i].diffMsec - MAX_FIRST_POSE_DELAY_MSEC);
            }

            LOGD(" Pose[%01d]                  | %-17s | %-6d | 0x%01X     | 0x%01X    | %-10"PRId64" | %-8d | %-8d | %d", i, (i > SixDofProfile0) ? ((i > SixDofProfile1) ? "Controller 2" : "Controller 1") : "HMD",
                pose[i].frameRate, pose[i].enabled, pose[i].outputMode, (pose[i].frames > 0) ? (pose[i].totalLatency / pose[i].frames) : 0, runTime[i].diffMsec, (pose[i].frameRate * pose[i].enabled * pose[i].outputMode * poseRunTimeMsec) / 1000, pose[i].frames);
        }

        for (auto it = controllerDiscoveryEventVector.begin(); it != controllerDiscoveryEventVector.end(); it++)
        {
            LOGD(" ControllerDiscoveryEvent | %02X:%02X:%02X:%02X:%02X:%02X |        |         |        |            |          |          | %d",
                (*it).get()->macAddress[0], (*it).get()->macAddress[1], (*it).get()->macAddress[2], (*it).get()->macAddress[3], (*it).get()->macAddress[4], (*it).get()->macAddress[5], (*it).get()->frames);
        }

        if (controllerDiscoveryEventVector.size() == 0)
        {
            LOGD(" ControllerDiscoveryEvent | All controllers   |        |         |        |            |          |          | 0");
        }

        for (uint8_t i = Controller1; i < ProfileTypeMax; i++)
        {
            LOGD(" Controller[%01d]            | Controller %01d      |        |         |        |            |          |          | %d", i, i, controller[i].frames);
        }

        for (uint8_t i = Controller1; i < ProfileTypeMax; i++)
        {
            LOGD(" Rssi[%01d]                  | Controller %01d      |        |         |        |            |          |          | %d", i, i, rssi[i].frames);
        }

        LOGD(" Errors                   | All error types   |        |         |        |            |          |          | %d", error.count);
        LOGD(" Statuses                 | All status types  |        |         |        |            |          |          | %d", status.count);
        LOGD(" Led Event                | Led Event         |        |         |        |            |          |          | %d", led.count);
        LOGD("--------------------------+-------------------+--------+---------+--------+------------+----------+----------+--------");
    }

    /* Check frame count and latency for video/gyro/accelerometer/6dof and check error events */
    bool check()
    {
        bool checkerFailed = false;
        for (uint8_t i = 0; i < VideoProfileMax; i++)
        {
            /* Check if video sensor started */
            if ((video[i].enabled) && (video[i].outputMode) && (runTime[HMD].diffMsec == 0))
            {
                LOGE("Error: Enabled video[%d] didn't start", HMD);
                checkerFailed = true;
            }
            else
            {
                /* Check if received min frames of runtime - 1 sec */
                if (video[i].frames < (video[i].enabled * video[i].outputMode * video[i].frameRate * (runTime[HMD].diffMsec / 1000 - 1)))
                {
                    LOGE("Error: Got too few video[%d] frames [Min Expected %d] [Actual %d]", i, (video[i].enabled * video[i].outputMode * video[i].frameRate * (runTime[HMD].diffMsec / 1000 - 1)), video[i].frames);
                    checkerFailed = true;
                }

                if (video[i].frames > 0)
                {
                    /* Check if received max frames of runtime + 1 sec */
                    if (video[i].frames > (video[i].enabled * video[i].outputMode * video[i].frameRate * (runTime[HMD].diffMsec / 1000 + 1)))
                    {
                        LOGE("Error: Got too many video[%d] frames [Max Expected %d] [Actual %d]", i, (video[i].enabled * video[i].outputMode * video[i].frameRate * (runTime[HMD].diffMsec / 1000 + 1)), video[i].frames);
                        checkerFailed = true;
                    }

                    /* Check latency less than 100 msec */
                    if ((video[i].totalLatency / video[i].frames) > MAX_FRAME_LATENCY_MSEC)
                    {
                        LOGE("Error: video[%d] frame motion->host latency is too high (%d)", i, (video[i].totalLatency / video[i].frames));
                        checkerFailed = true;
                    }

                    /* Check frame drops */
                    if (video[i].frameDrops > 0)
                    {
                        LOGE("Error: video[%d] frame drops occurred (%d)", i, video[i].frameDrops);
                        checkerFailed = true;
                    }

                    /* Check frame reorder */
                    if (video[i].frameReorder > 0)
                    {
                        LOGE("Error: video[%d] frame reorder occurred (%d)", i, video[i].frameReorder);
                        checkerFailed = true;
                    }
                }
            }
        }

        for (uint8_t i = 0; i < GyroProfileMax; i++)
        {
            /* Check if gyro sensor started */
            if ((gyro[i].enabled) && (gyro[i].outputMode) && (runTime[i].diffMsec == 0))
            {
                LOGE("Error: Enabled gyro[%d] didn't start", i);
                checkerFailed = true;
            }
            else
            {
                /* Check if received min frames of runtime - 1 sec */
                if (gyro[i].frames < (gyro[i].enabled * gyro[i].outputMode * gyro[i].frameRate * (runTime[i].diffMsec / 1000 - 1)))
                {
                    LOGE("Error: Got too few gyro[%d] frames [Min Expected %d] [Actual %d]", i, (gyro[i].enabled * gyro[i].outputMode * gyro[i].frameRate * (runTime[i].diffMsec / 1000 - 1)), gyro[i].frames);
                    checkerFailed = true;
                }

                if (gyro[i].frames > 0)
                {
                    /* Check if received max frames of runtime + 1 sec */
                    if (gyro[i].frames > (gyro[i].enabled * gyro[i].outputMode * gyro[i].frameRate * (runTime[i].diffMsec / 1000 + 1)))
                    {
                        LOGE("Error: Got too many gyro[%d] frames [Max Expected %d] [Actual %d]", i, (gyro[i].enabled * gyro[i].outputMode * gyro[i].frameRate * (runTime[i].diffMsec / 1000 + 1)), gyro[i].frames);
                        checkerFailed = true;
                    }

                    /* Check latency less than 100 msec */
                    if ((gyro[i].totalLatency / gyro[i].frames) > MAX_FRAME_LATENCY_MSEC)
                    {
                        LOGE("Error: gyro[%d] frame motion->host latency is too high (%d)", i, (gyro[i].totalLatency / gyro[i].frames));
                        checkerFailed = true;
                    }

                    /* Check frame drops */
                    if (gyro[i].frameDrops > 0)
                    {
                        LOGE("Error: gyro[%d] frame drops occurred (%d)", i, gyro[i].frameDrops);
                        checkerFailed = true;
                    }

                    /* Check frame reorder */
                    if (gyro[i].frameReorder > 0)
                    {
                        LOGE("Error: gyro[%d] frame reorder occurred (%d)", i, gyro[i].frameReorder);
                        checkerFailed = true;
                    }
                }
            }
        }

        for (uint8_t i = 0; i < AccelerometerProfileMax; i++)
        {
            /* Check if accelerometer sensor started */
            if ((accelerometer[i].enabled) && (accelerometer[i].outputMode) && (runTime[i].diffMsec == 0))
            {
                LOGE("Error: Enabled accelerometer[%d] didn't start", i);
                checkerFailed = true;
            }
            else
            {
                /* Check if received min frames of runtime - 1 sec */
                if (accelerometer[i].frames < (accelerometer[i].enabled * accelerometer[i].outputMode * accelerometer[i].frameRate * (runTime[i].diffMsec / 1000 - 1)))
                {
                    LOGE("Error: Got too few accelerometer[%d] frames [Min Expected %d] [Actual %d]", i, (accelerometer[i].enabled * accelerometer[i].outputMode * accelerometer[i].frameRate * (runTime[i].diffMsec / 1000 - 1)), accelerometer[i].frames);
                    checkerFailed = true;
                }

                if (accelerometer[i].frames > 0)
                {
                    /* Check if received max frames of runtime + 1 sec */
                    if (accelerometer[i].frames > (accelerometer[i].enabled * accelerometer[i].outputMode * accelerometer[i].frameRate * (runTime[i].diffMsec / 1000 + 1)))
                    {
                        LOGE("Error: Got too many accelerometer[%d] frames [Max Expected %d] [Actual %d]", i, (accelerometer[i].enabled * accelerometer[i].outputMode * accelerometer[i].frameRate * (runTime[i].diffMsec / 1000 + 1)), accelerometer[i].frames);
                        checkerFailed = true;
                    }

                    /* Check latency less than 100 msec */
                    if ((accelerometer[i].totalLatency / accelerometer[i].frames) > MAX_FRAME_LATENCY_MSEC)
                    {
                        LOGE("Error: accelerometer[%d] frame motion->host latency is too high (%d)", i, (accelerometer[i].totalLatency / accelerometer[i].frames));
                        checkerFailed = true;
                    }

                    /* Check frame drops */
                    if (accelerometer[i].frameDrops > 0)
                    {
                        LOGE("Error: accelerometer[%d] frame drops occurred (%d)", i, accelerometer[i].frameDrops);
                        checkerFailed = true;
                    }

                    /* Check frame reorder */
                    if (accelerometer[i].frameReorder > 0)
                    {
                        LOGE("Error: accelerometer[%d] frame reorder occurred (%d)", i, accelerometer[i].frameReorder);
                        checkerFailed = true;
                    }
                }
            }
        }

        for (uint8_t i = 0; i < SixDofProfileMax; i++)
        {
            uint32_t poseRunTimeMsecMin = 0;
            uint32_t poseRunTimeMsecMax = 0;
            uint32_t poseRunTimeMsec = 0;
            
            if (runTime[i].diffMsec > MAX_FIRST_POSE_DELAY_MSEC)
            {
                poseRunTimeMsecMin = ((runTime[i].diffMsec - 500) - MAX_FIRST_POSE_DELAY_MSEC);
                poseRunTimeMsecMax = ((runTime[i].diffMsec + 500) - MAX_FIRST_POSE_DELAY_MSEC);
                poseRunTimeMsec = (runTime[i].diffMsec - MAX_FIRST_POSE_DELAY_MSEC);
            }

            /* Check if pose started */
            if ((pose[i].enabled) && (pose[i].outputMode) && (runTime[i].diffMsec == 0))
            {
                LOGE("Error: Enabled pose[%d] didn't start", i);
                checkerFailed = true;
            }
            else
            {
                /* Check if received min frames of runtime - 500 msec */
                if (pose[i].frames < ((pose[i].enabled * pose[i].outputMode * ((std::min)((uint16_t)100, pose[i].frameRate)) * poseRunTimeMsecMin) / 1000))
                {
                    LOGE("Error: Got too few pose[%d] frames [Expected %d] [Actual %d]", i, ((pose[i].enabled * pose[i].outputMode * pose[i].frameRate * poseRunTimeMsec) / 1000), pose[i].frames);
                    checkerFailed = true;
                }

                if (pose[i].frames > 0)
                {
                    /* Check if received max frames of runtime + 500 msec */
                    if (pose[i].frames > ((pose[i].enabled * pose[i].outputMode * ((std::max)((uint16_t)100, pose[i].frameRate)) * poseRunTimeMsecMax) / 1000))
                    {
                        LOGE("Error: Got too many pose[%d] frames [Expected %d] [Actual %d]", i, ((pose[i].enabled * pose[i].outputMode * pose[i].frameRate * poseRunTimeMsec) / 1000), pose[i].frames);
                        checkerFailed = true;
                    }

                    /* Check latency less than 100 msec */
                    if ((pose[i].totalLatency / pose[i].frames) > MAX_FRAME_LATENCY_MSEC)
                    {
                        LOGE("Error: pose[%d] frame motion->host latency is too high (%d)", i, (pose[i].totalLatency / pose[i].frames));
                        checkerFailed = true;
                    }

                    /* Check frame drops */
                    if (pose[i].frameDrops > 0)
                    {
                        LOGE("Error: pose[%d] frame drops occurred (%d)", i, pose[i].frameDrops);
                        checkerFailed = true;
                    }

                    /* Check frame reorder */
                    if (pose[i].frameReorder > 0)
                    {
                        LOGE("Error: pose[%d] frame reorder occurred (%d)", i, pose[i].frameReorder);
                        checkerFailed = true;
                    }

                    /* Check NAN frame */
                    if (pose[i].nanFrame > 0)
                    {
                        LOGE("Error: pose[%d] NAN frame received (%d)", i, pose[i].nanFrame);
                        checkerFailed = true;
                    }
                }
            }
        }

        /* Check if got error events */
        if (error.count > 0)
        {
            LOGE("Error: Got %d error events", error.count);
            checkerFailed = true;
        }

        return checkerFailed;
    }

    /* Statistics comparison overload */
    bool operator == (const Statistics &ref) const
    {
        for (uint8_t i = 0; i < VideoProfileMax; i++)
        {
            if (this->video[i].frames != ref.video[i].frames)
            {
                return false;
            }
        }

        for (uint8_t i = 0; i < GyroProfileMax; i++)
        {
            if (this->gyro[i].frames != ref.gyro[i].frames)
            {
                return false;
            }
        }

        for (uint8_t i = 0; i < VelocimeterProfileMax; i++)
        {
            if (this->velocimeter[i].frames != ref.velocimeter[i].frames)
            {
                return false;
            }
        }

        for (uint8_t i = 0; i < AccelerometerProfileMax; i++)
        {
            if (this->accelerometer[i].frames != ref.accelerometer[i].frames)
            {
                return false;
            }
        }

        for (uint8_t i = 0; i < SixDofProfileMax; i++)
        {
            if (this->pose[i].frames != ref.pose[i].frames)
            {
                return false;
            }
        }

        for (uint8_t i = Controller1; i < ProfileTypeMax; i++)
        {
            if (this->controller[i].frames != ref.controller[i].frames)
            {
                return false;
            }
        }

        for (uint8_t i = Controller1; i < ProfileTypeMax; i++)
        {
            if (this->rssi[i].frames != ref.rssi[i].frames)
            {
                return false;
            }
        }

        for (uint8_t i = LocalizationTypeGet; i < LocalizationTypeMax; i++)
        {
            if (this->localization[i].frames != ref.localization[i].frames)
            {
                return false;
            }
        }

        return true;
    }

    /* Increase error count */
    void inc(void)
    {
        error.count++;
        if (gConfiguration.errorExit == true)
        {
            LOGE("Error event occurred");
            exit(EXIT_FAILURE);
        }
    }

    /* Increase led count  */
    void inc(TrackingData::ControllerLedEventFrame& ledFrame)
    {
        led.count++;
    }

    /* Increase status count */
    void inc(TrackingData::StatusEventFrame& statusFrame)
    {
        status.count++;
    }

    /* Increase gyro frame count */
    void inc(TrackingData::GyroFrame& gyroFrame)
    {
        gyro[gyroFrame.sensorIndex].frames++;
        gyro[gyroFrame.sensorIndex].totalLatency += ns2ms(systemTime() - gyroFrame.systemTimestamp);

        int64_t offset = (int64_t)gyroFrame.frameId - (int64_t)gyro[gyroFrame.sensorIndex].prevFrameId;
        if (gyro[gyroFrame.sensorIndex].prevFrameTimeStamp != 0)
        {
            if (offset > 1)
            {
                gyro[gyroFrame.sensorIndex].frameDrops++;
                if (gConfiguration.errorExit == true)
                {
                    LOGE("Gyro[%d] frame drops occurred - %lld missing frames [FrameId %d-%d], time diff = %d (msec)", gyroFrame.sensorIndex, offset-1, gyro[gyroFrame.sensorIndex].prevFrameId+1, gyroFrame.frameId-1, ns2ms(gyroFrame.timestamp - gyro[gyroFrame.sensorIndex].prevFrameTimeStamp));
                    exit(EXIT_FAILURE);
                }
            }
            else if (offset < 1)
            {
                gyro[gyroFrame.sensorIndex].frameReorder++;
                if (gConfiguration.errorExit == true)
                {
                    LOGE("Gyro[%d] frame reorder occurred - prev frameId = %d, new frameId = %d, time diff = %d (msec)", gyroFrame.sensorIndex, gyro[gyroFrame.sensorIndex].prevFrameId, gyroFrame.frameId, ns2ms(gyroFrame.timestamp - gyro[gyroFrame.sensorIndex].prevFrameTimeStamp));
                    exit(EXIT_FAILURE);
                }
            }
        }

        gyro[gyroFrame.sensorIndex].prevFrameTimeStamp = gyroFrame.timestamp;
        gyro[gyroFrame.sensorIndex].prevFrameId = gyroFrame.frameId;
    }

    /* Increase velocimeter frame count */
    void inc(TrackingData::VelocimeterFrame& velocimeterFrame)
    {
        velocimeter[velocimeterFrame.sensorIndex].frames++;
        velocimeter[velocimeterFrame.sensorIndex].totalLatency += ns2ms(systemTime() - velocimeterFrame.systemTimestamp);

        int64_t offset = (int64_t)velocimeterFrame.frameId - (int64_t)velocimeter[velocimeterFrame.sensorIndex].prevFrameId;
        if (velocimeter[velocimeterFrame.sensorIndex].prevFrameTimeStamp != 0)
        {
            if (offset > 1)
            {
                velocimeter[velocimeterFrame.sensorIndex].frameDrops++;
                if (gConfiguration.errorExit == true)
                {
                    LOGE("Velocimeter[%d] frame drops occurred - %lld missing frames [FrameId %d-%d], time diff = %d (msec)", velocimeterFrame.sensorIndex, offset-1, velocimeter[velocimeterFrame.sensorIndex].prevFrameId+1, velocimeterFrame.frameId-1, ns2ms(velocimeterFrame.timestamp - velocimeter[velocimeterFrame.sensorIndex].prevFrameTimeStamp));
                    exit(EXIT_FAILURE);
                }
            }
            else if (offset < 1)
            {
                velocimeter[velocimeterFrame.sensorIndex].frameReorder++;
                if (gConfiguration.errorExit == true)
                {
                    LOGE("Velocimeter[%d] frame reorder occurred - prev frameId = %d, new frameId = %d, time diff = %d (msec)", velocimeterFrame.sensorIndex, velocimeter[velocimeterFrame.sensorIndex].prevFrameId, velocimeterFrame.frameId, ns2ms(velocimeterFrame.timestamp - velocimeter[velocimeterFrame.sensorIndex].prevFrameTimeStamp));
                    exit(EXIT_FAILURE);
                }
            }
        }

        velocimeter[velocimeterFrame.sensorIndex].prevFrameTimeStamp = velocimeterFrame.timestamp;
        velocimeter[velocimeterFrame.sensorIndex].prevFrameId = velocimeterFrame.frameId;
    }

    /* Increase accelerometer frame count */
    void inc(TrackingData::AccelerometerFrame& accelerometerFrame)
    {
        accelerometer[accelerometerFrame.sensorIndex].frames++;
        accelerometer[accelerometerFrame.sensorIndex].totalLatency += ns2ms(systemTime() - accelerometerFrame.systemTimestamp);

        int64_t offset = (int64_t)accelerometerFrame.frameId - (int64_t)accelerometer[accelerometerFrame.sensorIndex].prevFrameId;
        if (accelerometer[accelerometerFrame.sensorIndex].prevFrameTimeStamp != 0)
        {
            if (offset > 1)
            {
                accelerometer[accelerometerFrame.sensorIndex].frameDrops++;
                if (gConfiguration.errorExit == true)
                {
                    LOGE("Accelerometer[%d] frame drops occurred - %lld missing frames [FrameId %d-%d], time diff = %d (msec)", accelerometerFrame.sensorIndex, offset-1, accelerometer[accelerometerFrame.sensorIndex].prevFrameId+1, accelerometerFrame.frameId-1, ns2ms(accelerometerFrame.timestamp - accelerometer[accelerometerFrame.sensorIndex].prevFrameTimeStamp));
                    exit(EXIT_FAILURE);
                }

            }
            else if (offset < 1)
            {
                accelerometer[accelerometerFrame.sensorIndex].frameReorder++;
                if (gConfiguration.errorExit == true)
                {
                    LOGE("Accelerometer[%d] frame reorder occurred - prev frameId = %d, new frameId = %d, time diff = %d (msec)", accelerometerFrame.sensorIndex, accelerometer[accelerometerFrame.sensorIndex].prevFrameId, accelerometerFrame.frameId, ns2ms(accelerometerFrame.timestamp - accelerometer[accelerometerFrame.sensorIndex].prevFrameTimeStamp));
                    exit(EXIT_FAILURE);
                }
            }
        }

        accelerometer[accelerometerFrame.sensorIndex].prevFrameTimeStamp = accelerometerFrame.timestamp;
        accelerometer[accelerometerFrame.sensorIndex].prevFrameId = accelerometerFrame.frameId;
    }

    /* Increase pose frame count */
    void inc(TrackingData::PoseFrame& poseFrame)
    {
        pose[poseFrame.sourceIndex].frames++;
        pose[poseFrame.sourceIndex].totalLatency += ns2ms(systemTime() - poseFrame.systemTimestamp);

        if ((gConfiguration.errorCheck == true) && isnan(poseFrame.translation.x) && (poseFrame.trackerConfidence != 0))
        {
            pose[poseFrame.sourceIndex].nanFrame++;
            if (gConfiguration.errorExit == true)
            {
                LOGE("Got NAN Pose[%u] (%" PRId64 "): Timestamp %" PRId64 ", Translation[%f, %f, %f], TrackerConfidence = 0x%X", poseFrame.sourceIndex, pose[poseFrame.sourceIndex].frames, poseFrame.timestamp, poseFrame.translation.x, poseFrame.translation.y, poseFrame.translation.z, poseFrame.trackerConfidence);
                exit(EXIT_FAILURE);
            }
        }

        /* Pose must arrive every 5 msec */
        int64_t offset = (poseFrame.timestamp - pose[poseFrame.sourceIndex].prevFrameTimeStamp) / 5000000;
        if (pose[poseFrame.sourceIndex].prevFrameTimeStamp != 0)
        {
            if (offset > 5)
            {
                pose[poseFrame.sourceIndex].frameDrops++;
                if (gConfiguration.errorExit == true)
                {
                    LOGE("Pose[%d] frame drops occurred - %lld missing frames, prev pose time = %lld, new pose time = %lld, time diff = %d (msec)", poseFrame.sourceIndex, offset-1, pose[poseFrame.sourceIndex].prevFrameTimeStamp, poseFrame.timestamp, ns2ms(poseFrame.timestamp - pose[poseFrame.sourceIndex].prevFrameTimeStamp));
                    exit(EXIT_FAILURE);
                }
            }
            else if (offset < 0)
            {
                pose[poseFrame.sourceIndex].frameReorder++;
                if (gConfiguration.errorExit == true)
                {
                    LOGE("Pose[%d] frame reorder occurred - prev pose time = %lld, new pose time = %lld, time diff = %d (msec)", poseFrame.sourceIndex, pose[poseFrame.sourceIndex].prevFrameTimeStamp, poseFrame.timestamp, ns2ms(poseFrame.timestamp - pose[poseFrame.sourceIndex].prevFrameTimeStamp));
                    exit(EXIT_FAILURE);
                }
            }
        }

        pose[poseFrame.sourceIndex].prevFrameTimeStamp = poseFrame.timestamp;
        pose[poseFrame.sourceIndex].trackerConfidence = poseFrame.trackerConfidence;
        pose[poseFrame.sourceIndex].trackerState = poseFrame.trackerState;
    }

    /* Increase video (left/right FishEye) frame count */
    void inc(TrackingData::VideoFrame& videoFrame)
    {
        video[videoFrame.sensorIndex].frames++;
        video[videoFrame.sensorIndex].totalLatency += ns2ms(systemTime() - videoFrame.systemTimestamp);

        int64_t offset = (int64_t)videoFrame.frameId - (int64_t)video[videoFrame.sensorIndex].prevFrameId;
        if (video[videoFrame.sensorIndex].prevFrameTimeStamp != 0)
        {
            if (offset > 1)
            {
                video[videoFrame.sensorIndex].frameDrops++;
                if (gConfiguration.errorExit == true)
                {
                    LOGE("Video[%d] frame drops occurred - %lld missing frames [FrameId %d-%d], time diff = %d (msec)", videoFrame.sensorIndex, offset-1, video[videoFrame.sensorIndex].prevFrameId+1, videoFrame.frameId-1, ns2ms(videoFrame.timestamp - video[videoFrame.sensorIndex].prevFrameTimeStamp));
                    exit(EXIT_FAILURE);
                }
            }
            else if (offset < 1)
            {
                video[videoFrame.sensorIndex].frameReorder++;
                if (gConfiguration.errorExit == true)
                {
                    LOGE("Video[%d] frame reorder occurred - prev frameId = %d, new frameId = %d, time diff = %d (msec)", videoFrame.sensorIndex, video[videoFrame.sensorIndex].prevFrameId, videoFrame.frameId, ns2ms(videoFrame.timestamp - video[videoFrame.sensorIndex].prevFrameTimeStamp));
                    exit(EXIT_FAILURE);
                }
            }
        }

        video[videoFrame.sensorIndex].prevFrameTimeStamp = videoFrame.timestamp;
        video[videoFrame.sensorIndex].prevFrameId = videoFrame.frameId;
    }

    /* Increase controller frame count */
    void inc(TrackingData::ControllerFrame& controllerFrame)
    {
        controller[controllerFrame.sensorIndex].frames++;
        controller[controllerFrame.sensorIndex].totalLatency += ns2ms(systemTime() - controllerFrame.systemTimestamp);
    }

    /* Add new controller discovery event statistics and increase controller discovery event frame count */
    void inc(TrackingData::ControllerDiscoveryEventFrame& frame)
    {
        auto it = std::find_if(controllerDiscoveryEventVector.begin(), controllerDiscoveryEventVector.end(), [&frame](std::shared_ptr<ControllerDiscoveryEventStatistics> v) -> bool
        { return !memcmp(v->macAddress, frame.macAddress, MAC_ADDRESS_SIZE);});

        if (it == controllerDiscoveryEventVector.end())
        {
            std::shared_ptr<ControllerDiscoveryEventStatistics> statistics(new ControllerDiscoveryEventStatistics(frame.macAddress));
            statistics->frames++;
            controllerDiscoveryEventVector.push_back(statistics);
        }
        else
        {
            (*it)->frames++;
        }
    }

    /* Increase Rssi frame count */
    void inc(TrackingData::RssiFrame& rssiFrame)
    {
        rssi[rssiFrame.sensorIndex].frames++;
        rssi[rssiFrame.sensorIndex].totalLatency += ns2ms(systemTime() - rssiFrame.systemTimestamp);
    }

    /* Increase Localization Get/Set frame count */
    void inc(TrackingData::LocalizationDataFrame& localizationFrame)
    {
        LocalizationType type = LocalizationTypeGet;

        if (localizationFrame.buffer == NULL)
        {
            type = LocalizationTypeSet;
        }

        localization[type].frames++;
        localization[type].fileSize += localizationFrame.length;

        if (localizationFrame.moreData == false)
        {
            localization[type].isCompleted = true;
        }

    }

    /* Check if controller mac address is already connected */
    bool isConnected(uint8_t* macAddress)
    {
        auto it = std::find_if(controllerDiscoveryEventVector.begin(), controllerDiscoveryEventVector.end(), [macAddress](std::shared_ptr<ControllerDiscoveryEventStatistics> v) -> bool
        {
            return !memcmp(v->macAddress, macAddress, MAC_ADDRESS_SIZE); }
        );

        return (it != controllerDiscoveryEventVector.end());
    }

    /* Configure Video sensor rate and outpurMode for statistics calculations */
    void configure(TrackingData::VideoProfile profile, uint8_t enabled, uint8_t outputMode)
    {
        video[profile.sensorIndex].frameRate = profile.fps;
        video[profile.sensorIndex].enabled = enabled;
        video[profile.sensorIndex].outputMode = outputMode;
    }

    /* Configure Gyro sensor rate and outpurMode for statistics calculations */
    void configure(TrackingData::GyroProfile profile, uint8_t enabled, uint8_t outputMode)
    {
        gyro[profile.sensorIndex].frameRate = profile.fps;
        gyro[profile.sensorIndex].enabled = enabled;
        gyro[profile.sensorIndex].outputMode = outputMode;
    }

    /* Configure Velocimeter sensor rate and outpurMode for statistics calculations */
    void configure(TrackingData::VelocimeterProfile profile, uint8_t enabled, uint8_t outputMode)
    {
        velocimeter[profile.sensorIndex].frameRate = profile.fps;
        velocimeter[profile.sensorIndex].enabled = enabled;
        velocimeter[profile.sensorIndex].outputMode = outputMode;
    }

    /* Configure Accelerometer sensor rate and outpurMode for statistics calculations */
    void configure(TrackingData::AccelerometerProfile profile, uint8_t enabled, uint8_t outputMode)
    {
        accelerometer[profile.sensorIndex].frameRate = profile.fps;
        accelerometer[profile.sensorIndex].enabled = enabled;
        accelerometer[profile.sensorIndex].outputMode = outputMode;
    }

    /* Configure 6dof rate and outpurMode for statistics calculations */
    void configure(TrackingData::SixDofProfile profile, uint8_t enabled, uint16_t frameRate)
    {
        pose[profile.profileType].frameRate = frameRate;
        pose[profile.profileType].enabled = enabled;
        pose[profile.profileType].outputMode = enabled;
    }

    RunTimeStatistics runTime[ProfileTypeMax];
    ErrorStatistics error;
    StatusStatistics status;
    LedStatistics led;
    VideoStatistics video[VideoProfileMax];
    GyroStatistics gyro[GyroProfileMax];
    VelocimeterStatistics velocimeter[VelocimeterProfileMax];
    AccelerometerStatistics accelerometer[AccelerometerProfileMax];
    PoseStatistics pose[SixDofProfileMax];
    ControllerStatistics controller[ProfileTypeMax];
    RssiStatistics rssi[ProfileTypeMax];
    LocalizationStatistics localization[LocalizationTypeMax];
    std::vector<std::shared_ptr<ControllerDiscoveryEventStatistics>> controllerDiscoveryEventVector;

    std::mutex videoFrameListMutex;
    std::list<TrackingData::VideoFrame*> videoList;
};

std::atomic_bool gIsDisposed(false);
TrackingDevice* gDevice;
Statistics gStatistics;
CalibrationInfo calibrationInfo;
uint32_t resetCount = 0;

void updateControllerFW(TrackingData::ControllerDiscoveryEventFrame& message, ProfileType controllerId)
{
    struct FwFileHeader
    {
        uint32_t headerSize;
        uint32_t dataSize;
        uint32_t fileFormatVersion;
        uint32_t versionSize;
        uint8_t version[7]; /* 3 bytes for major,minor,patch, 4 byte for build number */
    };

    std::ifstream file(gConfiguration.controllerFWFile, std::ios::binary);
    if (!file)
    {
        LOGE("Error: controller fw file %s doesn't exists", gConfiguration.controllerFWFile.c_str());
        return;
    }

    uint32_t headerSize = 0;
    if (!file.read((char*)&headerSize, sizeof(headerSize)))
    {
        LOGE("Error: Failed to read controller file %s", gConfiguration.controllerFWFile.c_str());
        return;
    }

    file.seekg(0);

    if (headerSize == 0)
    {
        LOGE("Error: Invalid controller fw file header size");
        return;
    }
    std::vector<uint8_t> headerArr(headerSize, 0);
    FwFileHeader* header = (FwFileHeader*)headerArr.data();
    if (!file.read((char*)header, headerSize))
    {
        LOGE("Error: Failed to read header in controller file %s", gConfiguration.controllerFWFile.c_str());
        return;
    }

    TrackingData::Version discoverdVersion;
    if (gConfiguration.controllerFWFileType == "app")
    {
        discoverdVersion = message.app;
    }
    else if (gConfiguration.controllerFWFileType == "bl")
    {
        discoverdVersion = message.bootLoader;
    }
    else
    {
        LOGE("Invalid image type input for burning controller fw: %s", gConfiguration.controllerFWFileType);
        return;
    }

    if ((header->versionSize != 3) && (header->versionSize != 7))
    {
        LOGE("Unsupported BLE image version size (%d)", header->versionSize);
        return;
    }

    /* Burning controller image only if image and discovery versions are different or configured to force burn */
    if (header->version[0] != discoverdVersion.major || 
        header->version[1] != discoverdVersion.minor || 
        header->version[2] != discoverdVersion.patch ||
        gConfiguration.controllers[controllerId].burn.configure == ControllerBurnForce)
    {
        LOGD("Updating controller FW %s application with new version %u.%u.%u", (gConfiguration.controllerFWFileType == "app")?"Application":"Boot Loader", header->version[0], header->version[1], header->version[2]);
 
        std::vector<uint8_t> imageArr(header->dataSize, 0);
        file.read((char*)imageArr.data(), header->dataSize);

        TrackingData::ControllerFW fw(message.macAddress, header->dataSize, imageArr.data(), message.addressType);
        auto status = gDevice->ControllerFWUpdate(fw);
        if (status != Status::SUCCESS)
        {
            LOGE("Failed to update controller %s firmware: %s (0x%X)", gConfiguration.controllerFWFileType.c_str(), statusToString(status).c_str(), status);
            return;
        }

        /* Burn succeeded, no need to burn this controller again */
        gConfiguration.controllers[controllerId].burn.state = ControllerBurnStateProcessing;
        return;
    }

    LOGD("Controller %d versions are equal [%u.%u.%u], skipping controller FW update", controllerId, header->version[0], header->version[1], header->version[2]);

    /* Versions are equal, no need to burn this controller */
    gConfiguration.controllers[controllerId].burn.state = ControllerBurnStateDone;
    return;
}

class CommonListener : public TrackingManager::Listener, public TrackingDevice::Listener
{
public:

    // [TrackingManager::Listener]
    virtual void onStateChanged(TrackingManager::EventType state, TrackingDevice* device, TrackingData::DeviceInfo deviceInfo)
    {
        switch (state)
        {
            case TrackingManager::ATTACH:
                gDevice = device;
                LOGD("Device (0x%p) Attached - Serial Number %llx", device, (deviceInfo.serialNumber >> 16));
                break;

            case TrackingManager::DETACH:
                LOGD("Device (0x%p) Detached - Serial Number %llx", device, (deviceInfo.serialNumber >> 16));
                gDevice = NULL;
                break;
        }
    };

    virtual void onError(Status error, TrackingDevice* dev) override
    {
        gStatistics.inc();
        LOGE("Error occurred while connecting device: %p Error: %s (0x%X)", dev, statusToString(error).c_str(), error);
    }

    virtual void onPoseFrame(TrackingData::PoseFrame& pose)
    {
        gStatistics.inc(pose);

        LOGV("Got Pose[%u] (%" PRId64 "): Timestamp %" PRId64 ", Translation[%f, %f, %f], TrackerConfidence = 0x%X, TrackerState = 0x%X",
            pose.sourceIndex, gStatistics.pose[pose.sourceIndex].frames, pose.timestamp, pose.translation.x, pose.translation.y, pose.translation.z, pose.trackerConfidence, pose.trackerState);

        if (gConfiguration.statistics == true)
        {
            static UtilTimeStamps timeStamp(pose);
            timeStamp.setTime(pose);

            poseCSV << std::fixed << (unsigned int)(pose.sourceIndex) << "," << gStatistics.pose[pose.sourceIndex].frames << ","
                << timeStamp.hostCurrentSystemTime << "," << pose.systemTimestamp << "," << timeStamp.fwTimeStamp << "," << timeStamp.latency << ","
                << pose.translation.x << "," << pose.translation.y << "," << pose.translation.z << ","
                << pose.velocity.x << "," << pose.velocity.y << "," << pose.velocity.z << ","
                << pose.acceleration.x << "," << pose.acceleration.y << "," << pose.acceleration.z << ","
                << pose.rotation.i << "," << pose.rotation.j << "," << pose.rotation.k << "," << pose.rotation.r << ","
                << pose.angularVelocity.x << "," << pose.angularVelocity.y << "," << pose.angularVelocity.z << ","
                << pose.angularAcceleration.x << "," << pose.angularAcceleration.y << "," << pose.angularAcceleration.z << ","
                << pose.trackerConfidence << "," << pose.mapperConfidence << "," << pose.trackerState << "\n";
        }

        switch (gConfiguration.tumFormat)
        {
            case 1:
                /* TUM */
                poseTUM << std::fixed << static_cast<double>(pose.timestamp)/1000000000 << " " << pose.translation.x << " " << pose.translation.y << " " << pose.translation.z << " " << pose.rotation.i << " " << pose.rotation.j << " " << pose.rotation.k << " " << pose.rotation.r << "\n";
                break;
            case 2:
                /* TUMX */
                poseTUM << std::fixed << static_cast<double>(pose.timestamp)/1000000000 << " " << pose.translation.x << " " << pose.translation.y << " " << pose.translation.z << " " << pose.rotation.i << " " << pose.rotation.j << " " << pose.rotation.k << " " << pose.rotation.r << " " << (unsigned int)(pose.sourceIndex) << "\n";
                break;
            default:
                break;
        }
    };

    virtual void onVideoFrame(TrackingData::VideoFrame& frame)
    {
        gStatistics.inc(frame);
        static UtilTimeStamps timeStamp(frame);
        timeStamp.setTime(frame);

        LOGV("Got Video[%u] frame (%" PRId64 "): Timestamp %" PRId64 ", FrameId = %u, Exposure = %u, Gain = %f, FrameLength = %u, size %dx%d pixels", frame.sensorIndex, gStatistics.video[frame.sensorIndex].frames, frame.timestamp,
            frame.frameId, frame.exposuretime, frame.gain,  frame.frameLength, frame.profile.width, frame.profile.height);

        if (((frame.frameLength < (uint32_t)(frame.profile.height * frame.profile.stride))) || (((frame.frameLength < (uint32_t)(frame.profile.height * frame.profile.width)))))
        {
            LOGE("Error: Video[%u] frame (%" PRId64 ") size mismatch:  FrameId = %u, FrameLength = %u, Width = %u, Height = %u, Stride = %u", 
                frame.sensorIndex, gStatistics.video[frame.sensorIndex].frames, frame.frameId, frame.frameLength, frame.profile.width, frame.profile.height, frame.profile.stride);
        }

        if (gConfiguration.statistics == true)
        {
            videoCSV << std::fixed << (unsigned int)(frame.sensorIndex) << "," << gStatistics.video[frame.sensorIndex].frames << ","
                << timeStamp.hostCurrentSystemTime << "," << frame.systemTimestamp << "," << timeStamp.fwTimeStamp << "," << timeStamp.arrivalTimeStamp << "," << timeStamp.latency << ","
                << frame.frameId << "," << frame.frameLength << "," << frame.exposuretime << "," << frame.gain << "," << frame.profile.stride << "," << frame.profile.width << "," << frame.profile.height << "," << frame.profile.pixelFormat << "\n";
        }

        if ((gConfiguration.videoFile == true) && (frame.frameLength == (frame.profile.width * frame.profile.height)))
        {
            TrackingData::VideoFrame* newFrame = new TrackingData::VideoFrame(frame);
            auto imageSize = (newFrame->profile.width * newFrame->profile.height);
            uint8_t* data = (uint8_t*)malloc(frame.frameLength);
            newFrame->data = data;
            perc::copy((uint8_t*)newFrame->data, frame.data, frame.frameLength);
            gStatistics.videoFrameListMutex.lock();
            gStatistics.videoList.push_back(newFrame);
            gStatistics.videoFrameListMutex.unlock();
        }
    };

    virtual void onAccelerometerFrame(TrackingData::AccelerometerFrame& message)
    {
        gStatistics.inc(message);

        auto calibratedAccelerationX = ((message.acceleration.x * calibrationInfo.accelerometerScale[0][0]) - calibrationInfo.accelerometerBiasX);
        auto calibratedAccelerationY = ((message.acceleration.y * calibrationInfo.accelerometerScale[1][1]) - calibrationInfo.accelerometerBiasY);
        auto calibratedAccelerationZ = ((message.acceleration.z * calibrationInfo.accelerometerScale[2][2]) - calibrationInfo.accelerometerBiasZ);
        auto magnitude = sqrt((calibratedAccelerationX * calibratedAccelerationX) + (calibratedAccelerationY * calibratedAccelerationY) + (calibratedAccelerationZ * calibratedAccelerationZ));

        LOGV("Got Accelerometer[%u] frame (%" PRId64 "): Timestamp %" PRId64 ", FrameID = %d, Temperature = %.0f, Acceleration[%f, %f, %f], Calibrated Acceleration[%f, %f, %f], Magnitude = %f",
            message.sensorIndex, gStatistics.accelerometer[message.sensorIndex].frames, message.timestamp, message.frameId, message.temperature, message.acceleration.x, message.acceleration.y, message.acceleration.z,
            calibratedAccelerationX, calibratedAccelerationY, calibratedAccelerationZ, magnitude);

        if (gConfiguration.statistics == true)
        {
            static UtilTimeStamps timeStamp(message);
            timeStamp.setTime(message);

            accelerometerCSV << std::fixed << unsigned(message.sensorIndex) << "," << gStatistics.accelerometer[message.sensorIndex].frames << ","
                << timeStamp.hostCurrentSystemTime << "," << message.systemTimestamp << "," << timeStamp.fwTimeStamp << "," << timeStamp.arrivalTimeStamp << "," << timeStamp.latency << ","
                << message.frameId << "," << message.temperature << "," << message.acceleration.x << "," << message.acceleration.y << "," << message.acceleration.z << ","
                << calibratedAccelerationX << "," << calibratedAccelerationY << "," << calibratedAccelerationZ << "," << magnitude << "\n";
        }
    };

    virtual void onGyroFrame(TrackingData::GyroFrame& message)
    {
        gStatistics.inc(message);

        LOGV("Got Gyro[%u] frame (%" PRId64 "): Timestamp %" PRId64 ", FrameID = %d, Temperature = %.0f, AngularVelocity[%f, %f, %f]", message.sensorIndex, gStatistics.gyro[message.sensorIndex].frames,
            message.timestamp, message.frameId, message.temperature, message.angularVelocity.x, message.angularVelocity.y, message.angularVelocity.z);

        if (gConfiguration.statistics == true)
        {
            static UtilTimeStamps timeStamp(message);
            timeStamp.setTime(message);

            gyroCSV << std::fixed << unsigned(message.sensorIndex) << "," << gStatistics.gyro[message.sensorIndex].frames << ","
                << timeStamp.hostCurrentSystemTime << "," << message.systemTimestamp << "," << timeStamp.fwTimeStamp << "," << timeStamp.arrivalTimeStamp << "," << timeStamp.latency << ","
                << message.frameId << "," << message.temperature << "," << message.angularVelocity.x << "," << message.angularVelocity.y << "," << message.angularVelocity.z << "\n";
        }
    };

    virtual void onVelocimeterFrame(TrackingData::VelocimeterFrame& message)
    {
        gStatistics.inc(message);

        LOGV("Got Velocimeter[%u] frame (%" PRId64 "): Timestamp %" PRId64 ", FrameID = %d, Temperature = %.0f, AngularVelocity[%f, %f, %f]", message.sensorIndex, gStatistics.velocimeter[message.sensorIndex].frames,
            message.timestamp, message.frameId, message.temperature, message.translationalVelocity.x, message.translationalVelocity.y, message.translationalVelocity.z);

        if (gConfiguration.statistics == true)
        {
            static UtilTimeStamps timeStamp(message);
            timeStamp.setTime(message);

            velocimeterCSV << std::fixed << unsigned(message.sensorIndex) << "," << gStatistics.velocimeter[message.sensorIndex].frames << ","
                << timeStamp.hostCurrentSystemTime << "," << message.systemTimestamp << "," << timeStamp.fwTimeStamp << "," << timeStamp.arrivalTimeStamp << "," << timeStamp.latency << ","
                << message.frameId << "," << message.temperature << "," << message.translationalVelocity.x << "," << message.translationalVelocity.y << "," << message.translationalVelocity.z << "\n";
        }
    };

    virtual void onControllerDiscoveryEventFrame(TrackingData::ControllerDiscoveryEventFrame& message)
    {
        //LOGD("Got Controller Discovery Event: on MacAddress [%02X:%02X:%02X:%02X:%02X:%02X], AddressType [0x%X], Manufacturer ID [0x%X], Vendor Data [0x%X], App Version [%u.%u.%u], Boot Loader Version [%u.%u.%u], Soft Device Version [%u], Protocol Version [%u]",
        //    message.macAddress[0], message.macAddress[1], message.macAddress[2], message.macAddress[3], message.macAddress[4], message.macAddress[5],
        //    message.addressType,
        //    message.manufacturerId,
        //    message.vendorData,
        //    message.app.major, message.app.minor, message.app.patch,
        //    message.bootLoader.major, message.bootLoader.minor, message.bootLoader.patch,
        //    message.softDevice.major,
        //    message.protocol.major);

        if (gConfiguration.controllersCount > 0)
        {
            ProfileType controllerId = ProfileTypeMax;

            /* Check if controller is in argument list and we didn't send connect request yet */
            if (((controllerId = gConfiguration.findController(message.macAddress)) != ProfileTypeMax) && (gStatistics.isConnected(message.macAddress) == 0))
            {
                /* Burning controller FW */
                if (gConfiguration.controllers[controllerId].burn.configure > ControllerBurnDisabled)
                {

                    switch (gConfiguration.controllers[controllerId].burn.state)
                    {
                        case ControllerBurnStateNotStarted:
                            /* Burn not started yet, initiating burn process */
                            LOGD("Start updating FW of controller %d MacAddress %02X:%02X:%02X:%02X:%02X:%02X", controllerId, message.macAddress[0], message.macAddress[1], message.macAddress[2], message.macAddress[3], message.macAddress[4], message.macAddress[5]);
                            updateControllerFW(message, controllerId);

                            /* If update controller FW succeeded, skipping connection attempt until next discovery event */
                            /* If update controller FW failed, return and try to burn again on next discovery            */
                            return;
                            break;

                        case ControllerBurnStateProcessing:
                            /* Burn is already in progress, exiting */
                            LOGD("FW Update is already in progress on controller %d MacAddress %02X:%02X:%02X:%02X:%02X:%02X", controllerId, message.macAddress[0], message.macAddress[1], message.macAddress[2], message.macAddress[3], message.macAddress[4], message.macAddress[5]);
                            return;
                            break;

                        case ControllerBurnStateDone:
                            /* Burn is done, continue to controller connect */
                            LOGD("Controller FW is updated on controller %d MacAddress %02X:%02X:%02X:%02X:%02X:%02X", controllerId, message.macAddress[0], message.macAddress[1], message.macAddress[2], message.macAddress[3], message.macAddress[4], message.macAddress[5]);
                            break;
                    }
                }

                Status status = Status::SUCCESS;
                uint8_t controllerId = ProfileTypeMax;

                /* Found our controller, trying to connect */
                LOGD("Connecting to Controller with MacAddress = %02X:%02X:%02X:%02X:%02X:%02X, AddressType [0x%X]", message.macAddress[0], message.macAddress[1], message.macAddress[2], message.macAddress[3], message.macAddress[4], message.macAddress[5], message.addressType);

                TrackingData::ControllerDeviceConnect device(message.macAddress, CONTROLLER_CONNECT_TIMEOUT_MSEC, message.addressType);
                status = gDevice->ControllerConnect(device, controllerId);
                if (status != Status::SUCCESS)
                {
                    LOGE("Error: Failed connecting controller MacAddress = %02X:%02X:%02X:%02X:%02X:%02X , Status %s (0x%X)",
                        message.macAddress[0], message.macAddress[1], message.macAddress[2], message.macAddress[3], message.macAddress[4], message.macAddress[5], statusToString(status).c_str(), status);
                }
                else
                {
                    gStatistics.inc(message);
                }
            }
        }
        else
        {
            LOGE("Error: Got Controller DiscoveryEvent on MacAddress = %02X:%02X:%02X:%02X:%02X:%02X while controllers are disabled",
                message.macAddress[0], message.macAddress[1], message.macAddress[2], message.macAddress[3], message.macAddress[4], message.macAddress[5]);
        }
    };

    virtual void onControllerConnectedEventFrame(TrackingData::ControllerConnectedEventFrame& message)
    {
        if ((message.controllerId >= ProfileTypeMax) || (message.controllerId < Controller1))
        {
            LOGE("Error: Got Bad Controller Connected Event: Status = %s (0x%X), ControllerID = %d, ManufactorId = 0x%X, Protocol = %u, App = %u.%u.%u, SoftDevice = %u, BootLoader = %u.%u.%u",
                statusToString(message.status).c_str(), message.status, message.controllerId, message.manufacturerId, message.protocol.major, message.app.major, message.app.minor, message.app.build, message.softDevice.major, message.bootLoader.major, message.bootLoader.minor, message.bootLoader.build);
            return;
        }

        LOGD("Got Controller[%d] Connected Event with status = %s (0x%X)", message.controllerId, statusToString(message.status).c_str(), message.status);

        if (message.status == Status::SUCCESS)
        {
            gStatistics.controller[message.controllerId].isConnected = true;

            if (gConfiguration.controllers[message.controllerId].calibrate == true)
            {
                LOGD("Start calibrating controller %d", message.controllerId);
                Status status = gDevice->ControllerStartCalibration(message.controllerId);
                if (status != Status::SUCCESS)
                {
                    LOGE("Error: Failed calibrating controller %d, Status %s (0x%X)", message.controllerId, statusToString(status).c_str(), status);
                    gStatistics.runTime[message.controllerId].start();
                }
            }
            else
            {
                gStatistics.runTime[message.controllerId].start();
            }
        }
        else
        {
            LOGE("Error: Failed connecting to Controller[%d] - status = %s (0x%X)", message.controllerId, statusToString(message.status).c_str(), message.status);
        }
    };

    virtual void onControllerDisconnectedEventFrame(TrackingData::ControllerDisconnectedEventFrame& message)
    {
        if ((message.controllerId >= ProfileTypeMax) || (message.controllerId < Controller1))
        {
            LOGE("Error: Got Bad Controller Disconnected Event on ControllerID = %d", message.controllerId);
            return;
        }

        LOGD("Got Controller Disconnected Event on ControllerID = %d", message.controllerId);
        gStatistics.controller[message.controllerId].isConnected = false;
        gStatistics.controller[message.controllerId].isCalibrated = false;
        gStatistics.runTime[message.controllerId].stop();
    };


    virtual void onControllerCalibrationEventFrame(TrackingData::ControllerCalibrationEventFrame& message)
    {
        if ((message.controllerId >= ProfileTypeMax) || (message.controllerId < Controller1))
        {
            LOGE("Error: Got Bad Controller Calibration Event on ControllerID = %d", message.controllerId);
            return;
        }

        if (message.status != Status::SUCCESS)
        {
            LOGE("Error: Got Controller Calibration Event on ControllerID = %d, Status %s (0x%X)", message.controllerId, statusToString(message.status).c_str(), message.status);
            return;
        }

        LOGD("Got Controller[%d] Calibration Event with status %s (0x%X)", message.controllerId, statusToString(message.status).c_str(), message.status);

        gStatistics.controller[message.controllerId].isCalibrated = true;
        gStatistics.runTime[message.controllerId].start();
    };

    virtual void onControllerFrame(TrackingData::ControllerFrame& message)
    {
        gStatistics.inc(message);

        if (gStatistics.controller[message.sensorIndex].isConnected == true)
        {
            LOGD("Got Controller[%u] frame: Timestamp %" PRId64 ", EventId = 0x%X, InstanceId = 0x%X, SensorData = %02X:%02X:%02X:%02X:%02X:%02X", message.sensorIndex, message.timestamp,
                message.eventId, message.instanceId, message.sensorData[0], message.sensorData[1], message.sensorData[2], message.sensorData[3], message.sensorData[4], message.sensorData[5]);
        }
        else
        {
            LOGE("Error: Got Controller[%u] frame on disconnected controller: Timestamp %" PRId64 ", EventId = 0x%X, InstanceId = 0x%X, SensorData = %02X:%02X:%02X:%02X:%02X:%02X", message.sensorIndex, message.timestamp,
                message.eventId, message.instanceId, message.sensorData[0], message.sensorData[1], message.sensorData[2], message.sensorData[3], message.sensorData[4], message.sensorData[5]);
        }

        if (gConfiguration.statistics == true)
        {
            static UtilTimeStamps timeStamp(message);
            timeStamp.setTime(message);

            controllerCSV << std::fixed << unsigned(message.sensorIndex) << "," << gStatistics.controller[message.sensorIndex].frames << ","
                          << timeStamp.hostCurrentSystemTime << "," << message.systemTimestamp << "," << message.timestamp << "," << timeStamp.fwTimeStamp << "," << message.arrivalTimeStamp << "," << timeStamp.arrivalTimeStamp << "," << timeStamp.latency << ","
                          << unsigned(message.eventId) << "," << unsigned(message.instanceId) << ","
                          << unsigned(message.sensorData[0]) << "," << unsigned(message.sensorData[1]) << "," << unsigned(message.sensorData[2]) << ","
                          << unsigned(message.sensorData[3]) << "," << unsigned(message.sensorData[4]) << "," << unsigned(message.sensorData[5]) << "\n";
        }
    };

    virtual void onRssiFrame(TrackingData::RssiFrame& message)
    {
        gStatistics.inc(message);

        LOGV("Got Rssi[%u] frame (%" PRId64 "): Timestamp %" PRId64 ", FrameID = %d, SignalStrength = %.0f", message.sensorIndex, gStatistics.rssi[message.sensorIndex].frames, message.timestamp, message.frameId, message.signalStrength);

        if (gConfiguration.statistics == true)
        {
            static UtilTimeStamps timeStamp(message);
            timeStamp.setTime(message);

            rssiCSV << std::fixed << unsigned(message.sensorIndex) << "," << gStatistics.rssi[message.sensorIndex].frames << ","
                    << timeStamp.hostCurrentSystemTime << "," << message.systemTimestamp << "," << timeStamp.fwTimeStamp << "," << timeStamp.arrivalTimeStamp << "," << timeStamp.latency << ","
                    << message.frameId << "," << message.signalStrength << "\n";
        }
    };

    virtual void onLocalizationDataEventFrame(TrackingData::LocalizationDataFrame& message)
    {
        gStatistics.inc(message);

        /* On Get localization map, saving to output file */
        if (message.length > 0)
        {
            LOGD("Got Localization Data frame: status = %s (0x%X), moredata = %s, chunkIndex = %d, length = %d", statusToString(message.status).c_str(), message.status, (message.moreData) ? "True" : "False", message.chunkIndex, message.length);

            std::fstream & mapFile = gConfiguration.localization[LocalizationTypeGet].file;
            string filename = gConfiguration.localization[LocalizationTypeGet].filename;
            if (mapFile.is_open())
            {
                mapFile.write((const char*)message.buffer, message.length);
            }
            else
            {
                LOGE("Error: Can't write localization data to file %s", filename.c_str());
            }
        }
        else
        {
            LOGD("Got Set Localization Data frame complete: status = %s (0x%X)", statusToString(message.status).c_str(), message.status);
        }
    };

    virtual void onFWUpdateEvent(OUT TrackingData::ControllerFWEventFrame& frame) override
    {
        ProfileType controllerId = gConfiguration.findController(frame.macAddress);
        if (controllerId != ProfileTypeMax)
        {
            gStatistics.controller[controllerId].FWProgress = frame.progress;
            LOGV("Got controller %d FW update Mac Address [%02X:%02X:%02X:%02X:%02X:%02X], progress %u%%, status 0x%X", controllerId,
                frame.macAddress[0], frame.macAddress[1], frame.macAddress[2], frame.macAddress[3], frame.macAddress[4], frame.macAddress[5], frame.progress, frame.status);

            if (frame.progress == 100)
            {
                gConfiguration.controllers[controllerId].burn.state = ControllerBurnStateDone;
            }
        }
    }

    virtual void onStatusEvent(OUT TrackingData::StatusEventFrame& frame) override
    {
        gStatistics.inc(frame);
        LOGD("Got status = %s (0x%X)", statusToString(frame.status).c_str(), frame.status);
    }

    virtual void onControllerLedEvent(OUT TrackingData::ControllerLedEventFrame& frame) override
    {
        gStatistics.inc(frame);
        LOGD("Got Controller[%d] Led[%d] intensity %" PRId64 " Event", frame.controllerId, frame.ledId, frame.intensity);
    }
};

void handleThreadFunction()
{
    while (!gIsDisposed)
    {
        if (!gManagerInstance)
        {
            std::this_thread::yield();
            continue;
        }
        gManagerInstance->handleEvents();
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
}


void saveImage(TrackingData::VideoFrame* frame)
{
    std::ostringstream fwTimeStamp;
    fwTimeStamp << std::setw(16) << std::setfill('0') << frame->timestamp;
    std::string fileHeaderName(gFileHeaderName);
    std::ofstream save_file("Image_" + fileHeaderName + "_Libtm_" + std::to_string(LIBTM_VERSION_MAJOR) + "_" + std::to_string(LIBTM_VERSION_MINOR) + "_" + std::to_string(LIBTM_VERSION_PATCH) +
                            "_FW_" + FW_VERSION +
                            "_FW_TimeStamp_" + fwTimeStamp.str() +
                            "_FE" + std::to_string(frame->sensorIndex) +
                            "_FrameID_" + std::to_string(frame->frameId) +
                            ".pgm", std::ofstream::binary);

    std::string pgm_header = "P5 " + std::to_string(frame->profile.width) + " " + std::to_string(frame->profile.height) + " 255\n";
    save_file.write(pgm_header.c_str(), pgm_header.size());
    save_file.write((char*)frame->data, frame->profile.height * frame->profile.stride);
    save_file.close();
}

void imageThreadFunction()
{
    uint32_t frameCount = 0;

    LOGD("Starting Image thread");

    while (true)
    {
        if (!gManagerInstance)
        {
            std::this_thread::yield();
            continue;
        }

        while (gStatistics.videoList.size())
        {
            gStatistics.videoFrameListMutex.lock();
            auto frame = gStatistics.videoList.front();
            gStatistics.videoList.pop_front();
            gStatistics.videoFrameListMutex.unlock();
            saveImage(frame);

            frameCount++;
            free((void*)frame->data);
        }

        if(gIsDisposed)
        {
            while (gStatistics.videoList.size())
            {
                gStatistics.videoFrameListMutex.lock();
                auto frame = gStatistics.videoList.front();
                gStatistics.videoList.pop_front();
                gStatistics.videoFrameListMutex.unlock();

                saveImage(frame);

                frameCount++;
                free((void*)frame->data);
            }

            LOGD("Closing Image thread - saved %d images (PGM)", frameCount);
            return;
        }
    } // end of while
}

void hostLogThreadFunction()
{
    TrackingData::Log log;
    FILE *logThreadStream = stdout;
    uint32_t waitForLogMsec = MAX_WAIT_FOR_LOG_MSEC;

    std::string fileHeaderName(gFileHeaderName);
    std::string fileName = "Host_Log_" + fileHeaderName + "_Libtm_" + std::to_string(LIBTM_VERSION_MAJOR) + "_" + std::to_string(LIBTM_VERSION_MINOR) + "_" + std::to_string(LIBTM_VERSION_PATCH) + "_FW_" + FW_VERSION + ".log";

    auto rc = fopen_s(&logThreadStream, fileName.c_str(), "w");
    if (rc != 0)
    {
        LOGE("Error while opening file %s", fileName.c_str());
        return;
    }

    LOGD("Starting Host Log thread");
    fprintf(logThreadStream, "Host log: Libtm %u.%u.%u.%u, FW %s\n\n", LIBTM_VERSION_MAJOR, LIBTM_VERSION_MINOR, LIBTM_VERSION_PATCH, LIBTM_VERSION_BUILD, FW_VERSION);

    while (true)
    {
        if (!gManagerInstance)
        {
            std::this_thread::yield();
            continue;
        }

        gManagerInstance.get()->getHostLog(&log);

        if (log.entries > 0)
        {
            fprintf(logThreadStream, "Received %d Host log entries from last %d msec (max entries %d)\n", log.entries, waitForLogMsec, log.maxEntries);
        }

        for (uint32_t i = 0; i < log.entries; i++)
        {
            char deviceId[30] = { 0 };
            short device = ((uintptr_t)log.entry[i].deviceID & 0xFFFF);
            if (device != 0)
            {
                snprintf(deviceId, sizeof(deviceId), "-%04hX", device);
            }

            fprintf(logThreadStream, "%02d:%02d:%02d:%03d [%06x] [%s] %s%s(%d): %s\n",
                log.entry[i].localTimeStamp.hour, log.entry[i].localTimeStamp.minute, log.entry[i].localTimeStamp.second, log.entry[i].localTimeStamp.milliseconds,
                log.entry[i].threadID,
                fwLogVerbosityLevel(log.entry[i].verbosity),
                log.entry[i].moduleID,
                deviceId,
                log.entry[i].lineNumber,
                log.entry[i].payload);

        }
        if (log.entries > 0)
        {
            fprintf(logThreadStream, "\n");
        }

        if ((log.entries > ((log.maxEntries * 2) / 5)) && (waitForLogMsec > MIN_WAIT_FOR_LOG_MSEC))
        {
            waitForLogMsec = waitForLogMsec / WAIT_FOR_LOG_MULTIPLIER_STEP;
        }
        else if ((log.entries < ((log.maxEntries * 1) / 5)) && (waitForLogMsec < MAX_WAIT_FOR_LOG_MSEC))
        {
            waitForLogMsec = waitForLogMsec * WAIT_FOR_LOG_MULTIPLIER_STEP;
        }

        if (gIsDisposed) // Object 0 = Stop event
        {
            if (logThreadStream != NULL)
            {
                fclose(logThreadStream);
                LOGD("Host logs saved to %s", fileName.c_str());
            }

            LOGD("Closing Host Log thread");
            return;
        }
    } // end of while
}

void fwLogThreadFunction()
{
    TrackingData::Log log;
    FILE *logThreadStream = stdout;
    uint32_t waitForLogMsec = MAX_WAIT_FOR_LOG_MSEC;
    Status status = Status::SUCCESS;
    char deviceBuf[30] = { 0 };\

    std::string fileHeaderName(gFileHeaderName);
    std::string fileName = "FW_Log_" + fileHeaderName + "_Libtm_" + std::to_string(LIBTM_VERSION_MAJOR) + "_" + std::to_string(LIBTM_VERSION_MINOR) + "_" + std::to_string(LIBTM_VERSION_PATCH) + "_FW_" + FW_VERSION + ".log";

    if (gConfiguration.logConfiguration[LogSourceFW].logOutputMode == LogOutputModeBuffer)
    {
        auto rc = fopen_s(&logThreadStream, fileName.c_str(), "w");
        if (rc != 0)
        {
            LOGE("Error while opening file %s", fileName.c_str());
            return;
        }
    }

    LOGD("Starting Log thread");
    if (logThreadStream != stdout)
    {
        fprintf(logThreadStream, "FW log: Libtm %u.%u.%u.%u, FW %s\n\n", LIBTM_VERSION_MAJOR, LIBTM_VERSION_MINOR, LIBTM_VERSION_PATCH, LIBTM_VERSION_BUILD, FW_VERSION);
    }

    while (true)
    {
        if (!gManagerInstance)
        {
            std::this_thread::yield();
            continue;
        }

        if (gDevice == NULL)
        {
            continue;
        }

        short device = ((uintptr_t)gDevice & 0xFFFF);

        status = gDevice->GetFWLog(log);
        if (status == Status::SUCCESS)
        {
            if (logThreadStream != stdout)
            {
                if (log.entries > 0)
                {
                    fprintf(logThreadStream, "Received %d FW log entries from last %d msec (max entries %d)\n", log.entries, waitForLogMsec, log.maxEntries);
                }

                for (uint32_t i = 0; i < log.entries; i++)
                {
                    short device = ((uintptr_t)log.entry[i].deviceID & 0xFFFF);

                    fprintf(logThreadStream, "%02d:%02d:%02d:%03d Device-%04hX: %" PRId64 " [%s] [%02d] [0x%X] [%s](%d): %s\n",
                            log.entry[i].localTimeStamp.hour, log.entry[i].localTimeStamp.minute, log.entry[i].localTimeStamp.second, log.entry[i].localTimeStamp.milliseconds,
                            device,
                            log.entry[i].timeStamp,
                            fwLogVerbosityLevel(log.entry[i].verbosity),
                            log.entry[i].threadID,
                            log.entry[i].functionSymbol,
                            log.entry[i].moduleID,
                            log.entry[i].lineNumber,
                            log.entry[i].payload
                    );
                }
                if (log.entries > 0)
                {
                    fprintf(logThreadStream, "\n");
                }
            }

            if ((log.entries > ((log.maxEntries * 2) / 5)) && (waitForLogMsec > MIN_WAIT_FOR_LOG_MSEC))
            {
                waitForLogMsec = waitForLogMsec / WAIT_FOR_LOG_MULTIPLIER_STEP;
            }
            else if ((log.entries < ((log.maxEntries * 1) / 5)) && (waitForLogMsec < MAX_WAIT_FOR_LOG_MSEC))
            {
                waitForLogMsec = waitForLogMsec * WAIT_FOR_LOG_MULTIPLIER_STEP;
            }
        }
        else
        {
            fprintf(logThreadStream, "Device-%04hX: Failed to receive FW log entries from last %d (msec)\n", device, waitForLogMsec);
            log.entries = 0;
        }

        if (gIsDisposed) // Object 0 = Stop event
        {
            if ((logThreadStream != NULL) && (gConfiguration.logConfiguration[LogSourceFW].logOutputMode == LogOutputModeBuffer))
            {
                fclose(logThreadStream);
                LOGD("FW logs saved to %s", fileName.c_str());
            }

            LOGD("Closing Log thread");
            return;
        }
    } // end of while
}

void showArguments()
{
    printf("libtm_util version %d.%d\n", LIBTM_UTIL_VERSION_MAJOR, LIBTM_UTIL_VERSION_MINOR);
    printf("Usage: libtm_util [-h] [-y] [-reset <count> (optional)] [-time <time>] [-stop <time>] [-loop <count>] [-check <Exit on Error[0-1]>] [-video  <SensorIndex[0-3]> <Output [0-1]> <Width Height (Optional)>]\n");
    printf("       [-gyro <SensorIndex[0-2]> <Output [0-1]> <FPS (Optional)>] [-accl <SensorIndex[0-2]> <Output [0-1]> <FPS (Optional)>]\n");
    printf("       [-6dof <Source [0-2]> <Mode [0|1|2|4] (Optional)>] [-enable_all] [-controller <controller index [1-2]> <MacAddress [AABBCCDDEEFF]> <Calibrate [0-1]>]\n");
    printf("       [-rssi <controller [1-2]> <Time (Sec)>] [-exposure <SensorIndex[0-3]> <integration time (uSec)> <gain>] [-fw <filename>] [-statistics] [-image] [-tum <type [1-2]>]\n");
    printf("       [-log <Source [fw/host]> <Verbosity [0-6]> <Mode [0-1]> <OutputMode [0-1]>] [-calibrate <Type [new/append]> <filename>] [-power <high/low>] [-map <reset/set/get> <filename>] [-jtag]\n");
    printf("       [-temperature <get/set> <sensor [VPU, IMU, BLE] (Optional)> <threshold (optional)>] [-geo <latitude> <longitude> <altitude>] [-gpio <controlBitMask>] [-velocimeter <filename>]\n");
    printf("       [-controller_data <filename>] [-node <filename>] [-burn <type [bl/app]> <filename> <force (optional)>] \n\n");

    printf("Options:\n");
    printf("    -h,-help..........Show this help message\n");
    printf("    -y................Skip configuration verification\n");
    printf("    -reset............Reset device before start [-reset -h for more info]\n");
    printf("    -time ............Run Time Between Start to Stop stream (Default 10 sec) [-time -h for more info]\n");
    printf("    -stop ............Stop Time Between Stop to Start stream (Default 2 sec) [-stop -h for more info]\n");
    printf("    -loop ............Start/Stop loop count [-loop -h for more info]\n");
    printf("    -check ...........Check for errors [-check -h for more info]\n");
    printf("    -video ...........Enable video capture and output to host [-video -h for more info]\n");
    printf("    -gyro ............Enable gyro capture and output to host [-gyro -h for more info]\n");
    printf("    -accl ............Enable accelerometer capture and output to host [-accl -h for more info]\n");
    printf("    -6dof ............Enable 6Dof stream [-6dof -h for more info]\n");
    printf("    -enable_all ......Enable all sensors (Video, Gyro, Accelerometer) and all 6dofs [-enable_all -h for more info]\n");
    printf("    -controller ......Enable controller stream [-controller -h for more info]\n");
    printf("    -rssi ............Enable signal strength measurement [-rssi -h for more info]\n");
    printf("    -exposure ........Set exposure values [-exposure -h for more info]\n");
    printf("    -fw ..............Set external FW (mvcmd) [-fw -h for more info]\n");
    printf("    -statistics ......Save statistics of all enabled sensors, 6dofs and controllers to CSV file [-statistics -h for more info]\n");
    printf("    -tum .............Save 6dof (pose) statistics in Tum format [-tum -h for more info]\n");
    printf("    -image............Output video images to files [-image -h for more info]\n");
    printf("    -log .............Set FW/Host log verbosity, rollover mode and output mode [-log -h for more info]\n");
    printf("    -calibrate .......Calibrate device [-calibrate -h for more info]\n");
    printf("    -power ...........Set High/Low power mode [-power -h for more info]\n");
    printf("    -map .............Set/Get Localization Map [-map -h for more info]\n");
    printf("    -jtag ............Load FW from JTAG [-jtag -h for more info]\n");
    printf("    -temperature .....Get/Set temperature sensors [-temperature -h for more info]\n");
    printf("    -geo .............Sets the geographical location [-geo -h for more info]\n");
    printf("    -gpio ............Sets GPIO control bit mask [-gpio -h for more info]\n");
    printf("    -velocimeter .....Enable velocimeter stream [-velocimeter -h for more info]\n");
    printf("    -controller_data .Sends Controller data to connected controllers [-controller_data -h for more info]\n");
    printf("    -node ............Sends static node data [-node -h for more info]\n");
    printf("    -burn ............Burns controller fw to connected controllers [-burn -h for more info]\n");
}

int parseArguments(int argc, char *argv[])
{
    uint8_t sensorEnabled = 0;
    int input = 0;
    time_t now = time(NULL);
    std::tm* lt = std::localtime(&now);

    snprintf(gFileHeaderName, sizeof(gFileHeaderName), "%02d%02d%04d_%02d%02d%02d", lt->tm_mday, lt->tm_mon + 1, 1900 + lt->tm_year, lt->tm_hour, lt->tm_min, lt->tm_sec);

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if ((arg == "-h") || (arg == "-help") || (arg == "?"))
        {
            showArguments();
            return -1;
        }
        else if ((arg == "-y"))
        {
            gConfiguration.verifyConfiguration = false;
        }
        else if ((arg == "-time"))
        {
            bool parseError = true;

            /* Make sure we aren't at the end of argv */
            if ((i + 1 < argc) && (strstr(argv[i + 1], "-") != argv[i + 1]))
            {
                uint32_t runTime = atoi(argv[++i]);

                if ((runTime >= MIN_RUN_TIME_SEC) && (runTime <= MAX_RUN_TIME_SEC))
                {
                    gConfiguration.startStreamTime = runTime;
                    parseError = false;
                }
            }

            if (parseError == true)
            {
                printf("-time : Run Time Between START to STOP stream\n");
                printf("        Parameters: <Time (Sec)> (Min = 2, Max = 86400, Default = 10)\n");
                printf("        Example: \"libtm_util.exe -gyro 0 1 -time 60\" - Call START stream and wait for 60 seconds before calling STOP stream\n");
                return -1;
            }
        }
        else if ((arg == "-stop"))
        {
            bool parseError = true;

            /* Make sure we aren't at the end of argv */
            if ((i + 1 < argc) && (strstr(argv[i + 1], "-") != argv[i + 1]))
            {
                uint32_t stopTime = atoi(argv[++i]);

                if ((stopTime >= MIN_RUN_TIME_SEC) && (stopTime <= MAX_RUN_TIME_SEC))
                {
                    gConfiguration.stopStreamTime = stopTime;
                    parseError = false;
                }
            }

            if (parseError == true)
            {
                printf("-stop : Time Between STOP to next START stream\n");
                printf("        Parameters: <Time (Sec)> (Min = 2, Max = 86400, Default = 10)\n");
                printf("        Example: \"libtm_util.exe -gyro 0 1 -time 60 -stop 10 -loop 3\" - Call START stream, wait for 60 seconds before calling STOP stream, wait 10 seconds before calling START again\n");
                return -1;
            }
        }
        else if ((arg == "-reset"))
        {
            gConfiguration.resetLoop = 1;

            /* Make sure we aren't at the end of argv */
            if (i + 1 < argc)
            {
                if (strstr(argv[i + 1], "-") != argv[i + 1])
                {
                    gConfiguration.resetLoop = atoi(argv[++i]);
                }
                else
                {
                    if (strncmp("-h", argv[i + 1], 2) == 0)
                    {
                        printf("-reset : Reset FW upon APP load and between START/STOP loops, this will remove any already loaded FW and load a new internal or external FW\n");
                        printf("         Parameters: <count (optional) (default = 1)>\n");
                        printf("         Example: \"libtm_util.exe -gyro 0 1 -reset\" - Reset FW 1 time\n");
                        printf("         Example: \"libtm_util.exe -gyro 0 1 -reset 100\" - Reset FW 1 time upon load and 99 between START/STOP gyro stream\n");
                        return -1;
                    }
                }
            }
        }
        else if ((arg == "-loop"))
        {
            /* Make sure we aren't at the end of argv */
            if ((i + 1 < argc) && (strstr(argv[i + 1], "-") != argv[i + 1]))
            {
                gConfiguration.maxLoop = atoi(argv[++i]);
            }
            else
            {
                printf("-loop : Start/Stop loop count\n");
                printf("        Parameters: <count>\n");
                printf("        Example: \"libtm_util.exe -gyro 0 1 -loop 100\" - Run 100 times the START/STOP loop of gyro stream\n");
                return -1;
            }
        }
        else if ((arg == "-check"))
        {
            /* Make sure we aren't at the end of argv */
            if ((i + 1 < argc) && (strstr(argv[i + 1], "-") != argv[i + 1]))
            {
                gConfiguration.errorExit = bool(atoi(argv[++i]));
                gConfiguration.errorCheck = true;
            }
            else
            {
                printf("-check : Checking and exiting on the following statistics errors:\n");
                printf("         - Pose get NAN values with valid confidence\n");
                printf("         - More than expected frames of Video/Gyro/Accelerometer/6dof\n");
                printf("         - Less than expected frames of Video/Gyro/Accelerometer/6dof\n");
                printf("         - Latency of Video/Gyro/Accelerometer/6dof frames is too high\n");
                printf("         - Frame drops of Video/Gyro/Accelerometer/Velocimeter/6dof\n");
                printf("         - Frame reorder of Video/Gyro/Accelerometer/Velocimeter/6dof\n");
                printf("         - Error events occurred\n");
                printf("         Parameters:  <Exit on Error [0-1]>\n");
                printf("         Example: \"libtm_util.exe -6dof 0 check 1\" - Enable HMD 6dof, check and exit run on all statistics errors\n");
                return -1;
            }
        }
        else if ((arg == "-video"))
        {
            bool parseError = true;

            /* Make sure we aren't at the end of argv */
            /* -video 0 1 */
            if ((i + 2 < argc) && (strstr(argv[i + 1], "-") != argv[i + 1]) && (strstr(argv[i + 2], "-") != argv[i + 2]))
            {
                uint8_t index = atoi(argv[++i]);
                uint8_t outputEnabled = atoi(argv[++i]);
                uint16_t fps = 0; /* Default*/
                uint16_t width = 0; /* Default*/
                uint16_t height = 0; /* Default*/

                /* -video 0 1 30 848 800 */
                if ((i + 3 < argc) && (strstr(argv[i + 1], "-") != argv[i + 1]) && (strstr(argv[i + 2], "-") != argv[i + 2]) && (strstr(argv[i + 3], "-") != argv[i + 3]))
                {
                    fps = atoi(argv[++i]);
                    width = atoi(argv[++i]);
                    height = atoi(argv[++i]);
                }

                if (index < VideoProfileMax)
                {
                    gConfiguration.video[index].enabled = true;
                    gConfiguration.video[index].outputEnabled = (bool)outputEnabled;
                    gConfiguration.video[index].width = width;
                    gConfiguration.video[index].height = height;
                    gConfiguration.video[index].fps = fps;
                    gConfiguration.videoCount++;
                    parseError = false;
                }
            }

            if (parseError == true)
            {
                printf("-video : Enable video capture and output to host per sensor index\n");
                printf("         Parameters: <SensorIndex [0-3]> <Output [0-1]> <FPS Width Height (Optional)>\n");
                printf("         Example: \"libtm_util.exe -video 0 1 -video 1 1\" : Enable video sensors 0 + 1 in the FW and output them to host\n");
                printf("         Example: \"libtm_util.exe -video 0 1 30 848 800 -video 1 1 30 848 800\" : Enable video sensors 0 + 1 with FPS 30, width 848, height 800 and output them to host\n");
                return -1;
            }
        }
        else if ((arg == "-gyro"))
        {
            bool parseError = true;

            /* Make sure we aren't at the end of argv */
            /* -gyro 0 1 */
            if ((i + 2 < argc) && (strstr(argv[i + 1], "-") != argv[i + 1]) && (strstr(argv[i + 2], "-") != argv[i + 2]))
            {
                uint8_t index = atoi(argv[++i]);
                uint8_t outputEnabled = atoi(argv[++i]);
                uint16_t fps = 0; /* Default*/

                /* -gyro 0 1 1000 */
                if ((i + 1 < argc) && (strstr(argv[i + 1], "-") != argv[i + 1]))
                {
                    fps = atoi(argv[++i]);

                }

                if (index < GyroProfileMax)
                {
                    gConfiguration.gyro[index].enabled = true;
                    gConfiguration.gyro[index].outputEnabled = (bool)outputEnabled;
                    gConfiguration.gyro[index].fps = fps;
                    gConfiguration.gyroCount++;
                    if (index > GyroProfile0)
                    {
                        gConfiguration.controllers[index].needed = true;
                    }

                    parseError = false;
                }
            }

            if (parseError == true)
            {
                printf("-gyro : Enable gyro capture and output to host per sensor index\n");
                printf("        Parameters: <SensorIndex [0-2]> <Output [0-1]> <FPS (Optional)>\n");
                printf("        Example: \"libtm_util.exe -gyro 0 1\"      : Enable HMD gyro and output all gyro frames to host with lowest FPS\n");
                printf("        Example: \"libtm_util.exe -gyro 1 1 1000\" : Enable controller 1 gyro, output all gyro frames to host and set FPS to 1000\n");
                return -1;
            }
        }
        else if ((arg == "-accelerometer") || (arg == "-accl"))
        {
            bool parseError = true;

            /* Make sure we aren't at the end of argv */
            /* -accl 0 1 */
            if ((i + 2 < argc) && (strstr(argv[i + 1], "-") != argv[i + 1]) && (strstr(argv[i + 2], "-") != argv[i + 2]))
            {
                uint8_t index = atoi(argv[++i]);
                uint8_t outputEnabled = atoi(argv[++i]);
                uint16_t fps = 0; /* Default*/

                /* -accl 0 1 1000 */
                if ((i + 1 < argc) && (strstr(argv[i + 1], "-") != argv[i + 1]))
                {
                    fps = atoi(argv[++i]);
                }

                if (index < AccelerometerProfileMax)
                {
                    gConfiguration.accelerometer[index].enabled = true;
                    gConfiguration.accelerometer[index].outputEnabled = (bool)outputEnabled;
                    gConfiguration.accelerometer[index].fps = fps;
                    gConfiguration.accelerometerCount++;
                    if (index > AccelerometerProfile0)
                    {
                        gConfiguration.controllers[index].needed = true;
                    }

                    parseError = false;
                }
            }

            if (parseError == true)
            {
                printf("-accl : Enable accelerometer capture and output to host per sensor index\n");
                printf("        Parameters: <SensorIndex [0-2]> <Output [0-1]> <FPS (Optional)>\n");
                printf("        Example: \"libtm_util.exe -accl 0 1\"      : Enable HMD accelerometer and output all accelerometer frames to host with lowest FPS\n");
                printf("        Example: \"libtm_util.exe -accl 1 1 1000\" : Enable controller 1 accelerometer, output all accelerometer frames to host and set FPS to 1000\n");
                return -1;
            }
        }
        else if ((arg == "-6dof"))
        {
            bool parseError = true;

            /* Make sure we aren't at the end of argv */
            /* -6dof 0 */
            if ((i + 1 < argc) && (strstr(argv[i + 1], "-") != argv[i + 1]))
            {
                uint8_t index = atoi(argv[++i]);
                if (index < SixDofProfileMax)
                {
                    gConfiguration.sixdof[index].enabled = true;
                    gConfiguration.sixdofCount++;
                    if (index > SixDofProfile0)
                    {
                        gConfiguration.controllers[index].needed = true;
                    }

                    parseError = false;

                    /* -6dof 0 6 */
                    if ((i + 1 < argc) && (strstr(argv[i + 1], "-") != argv[i + 1]))
                    {
                        uint8_t mode = atoi(argv[++i]);
                        if (mode < SIXDOF_MODE_MAX)
                        {
                            gConfiguration.sixdof[index].mode = mode;
                        }
                        else
                        {
                            parseError = true;
                        }
                    }
                    else
                    {
                        TrackingData::Profile profile; /* using default values */
                        gConfiguration.sixdof[index].mode = profile.sixDof[index].mode;
                    }
                }
            }

            if (parseError == true)
            {
                printf("-6dof : Enable 6Dof stream per source (HMD/Controllers)\n");
                printf("        Parameters: <Source [0-2]> <Mode [0|1|2|4] (Optional)>\n");
                printf("        Source: 0 - Enable 6dof of HMD\n");
                printf("                1 - Enable 6dof of Controller 1\n");
                printf("                2 - Enable 6dof of Controller 2\n");
                printf("        Mode: 0 - Normal mode\n");
                printf("              1 - Fast playback mode\n");
                printf("              2 - Enable mapping mode\n");
                printf("              4 - Enable relocalization mode\n");
                printf("              1|2|4 - Any combination of modes above\n");
                printf("        Example: \"libtm_util.exe -6dof 0\" : Enable 6dof for HMD\n");
                printf("        Example: \"libtm_util.exe -6dof 1 -6dof 2\" : Enable 6dof for Controller 1 + 2\n");
                printf("        Example: \"libtm_util.exe -6dof 0 6\" : Enable 6dof for HMD with mapping and relocalization modes enabled\n");
                return -1;
            }
        }
        else if ((arg == "-enable_all"))
        {
            if ((i + 1 < argc) && (strncmp("-h", argv[i + 1], 2) == 0))
            {
                printf("libtm_util.exe -enable_all : Enable all HMD sensors (Video, Gyro, Accelerometer) and all HMD 6dofs\n");
                return -1;
            }
            else
            {
                uint8_t index = 0;

                for (index = VideoProfile0; index < VideoProfile2; index++)
                {
                    gConfiguration.video[index].enabled = true;
                    gConfiguration.video[index].outputEnabled = true;
                    gConfiguration.video[index].width = 0;
                    gConfiguration.video[index].height = 0;
                    gConfiguration.video[index].fps = 0;
                    gConfiguration.videoCount++;
                }

                for (index = GyroProfile0; index < GyroProfile1; index++)
                {
                    gConfiguration.gyro[index].enabled = true;
                    gConfiguration.gyro[index].outputEnabled = true;
                    gConfiguration.gyro[index].fps = 0;
                    gConfiguration.gyroCount++;
                    if (index > GyroProfile0)
                    {
                        gConfiguration.controllers[index].needed = true;
                    }
                }

                for (index = AccelerometerProfile0; index < AccelerometerProfile1; index++)
                {
                    gConfiguration.accelerometer[index].enabled = true;
                    gConfiguration.accelerometer[index].outputEnabled = true;
                    gConfiguration.accelerometer[index].fps = 0;
                    gConfiguration.accelerometerCount++;
                    if (index > AccelerometerProfile0)
                    {
                        gConfiguration.controllers[index].needed = true;
                    }
                }

                for (index = SixDofProfile0; index < SixDofProfile1; index++)
                {
                    gConfiguration.sixdof[index].enabled = true;
                    gConfiguration.sixdof[index].mode = (SIXDOF_MODE_ENABLE_MAPPING | SIXDOF_MODE_ENABLE_RELOCALIZATION);
                    gConfiguration.sixdofCount++;
                    if (index > SixDofProfile0)
                    {
                        gConfiguration.controllers[index].needed = true;
                    }
                }
            }
        }
        else if ((arg == "-controller"))
        {
            bool parseError = true;

            /* Make sure we aren't at the end of argv */
            if ((i + 3 < argc) && (strstr(argv[i + 1], "-") != argv[i + 1]) && (strstr(argv[i + 2], "-") != argv[i + 2]) && (strstr(argv[i + 3], "-") != argv[i + 3]))
            {
                uint8_t index = atoi(argv[++i]);
                if ((index > HMD) && (index < ProfileTypeMax))
                {
                    uint64_t macAddress = strtoull(argv[++i], NULL, 16);
                    gConfiguration.controllers[index].macAddress[0] = (macAddress >> 40) & 0xFF;
                    gConfiguration.controllers[index].macAddress[1] = (macAddress >> 32) & 0xFF;
                    gConfiguration.controllers[index].macAddress[2] = (macAddress >> 24) & 0xFF;
                    gConfiguration.controllers[index].macAddress[3] = (macAddress >> 16) & 0xFF;
                    gConfiguration.controllers[index].macAddress[4] = (macAddress >> 8) & 0xFF;
                    gConfiguration.controllers[index].macAddress[5] = (macAddress >> 0) & 0xFF;
                    gConfiguration.controllers[index].calibrate = atoi(argv[++i]);
                    gConfiguration.controllers[index].controllerID = index;
                    gConfiguration.controllersCount++;
                    parseError = false;
                }
            }

            if (parseError == true)
            {
                printf("-controller : Enable controller stream per controller\n");
                printf("              Parameters: <controller index [1-2]> <MacAddress [AABBCCDDEEFF]> <Calibrate [0-1]>\n");
                printf("              Controller index: 1 - Enable controller 1\n");
                printf("                                2 - Enable controller 2\n");
                printf("              Mac Address: 6 Byte array of BLE mac address\n");
                printf("              Calibrate: 0 - Skip controller calibration\n");
                printf("                         1 - Run controller calibration\n");
                printf("              Example: \"libtm_util.exe -6dof 1 -controller 1 AABBCCDDEEFF 1\" : Connect to Controller 1 with Mac Address AABBCCDDEEFF and run controller calibration\n");
                printf("              Notice: To get controller frames, make sure you also enable controller sensors (gyro, accelerometer, 6dof)\n");
                return -1;
            }
        }
        else if ((arg == "-rssi"))
        {
            bool parseError = true;

            /* Make sure we aren't at the end of argv */
            if ((i + 2 < argc) && (strstr(argv[i + 1], "-") != argv[i + 1]) && (strstr(argv[i + 2], "-") != argv[i + 2]))
            {
                uint8_t index = atoi(argv[++i]);
                if ((index > HMD) && (index < ProfileTypeMax))
                {
                    gConfiguration.rssi[index].time = atoi(argv[++i]);
                    gConfiguration.rssiCount++;
                    parseError = false;
                }
            }

            if (parseError == true)
            {
                printf("-rssi : Enable signal strength measurement\n");
                printf("        Parameters: <controller [1-2]> <Time (Sec)>\n");
                printf("        Example: \"libtm_util.exe -6dof 1 -controller 1 AABBCCDDEEFF 1 -rssi 1 10\" : Upon connecting to controller 1 - start RSSI test on controller 1 for 10 seconds\n");
                printf("        Notice: To start RSSI test, make sure you also connect to controller using -controller param and enable controller sensors (gyro, accelerometer, 6dof)\n");
                return -1;
            }
        }
        else if ((arg == "-exposure"))
        {
            bool parseError = true;

            /* Make sure we aren't at the end of argv */
            if ((i + 3 < argc) && (strstr(argv[i + 1], "-") != argv[i + 1]) && (strstr(argv[i + 2], "-") != argv[i + 2]) && (strstr(argv[i + 3], "-") != argv[i + 3]))
            {
                if (gConfiguration.exposure.numOfVideoStreams > MAX_VIDEO_STREAMS)
                {
                    printf("ERROR: Max num of exposure settings exceeded (%d)\n", MAX_VIDEO_STREAMS);
                }
                else
                {
                    uint8_t index = atoi(argv[++i]);
                    if ((index < VideoProfile0) || (index > VideoProfile3))
                    {
                        printf("ERROR: exposure sensor index unknown (%d)\n", index);
                    }
                    else
                    {
                        gConfiguration.setExposure = true;
                        uint32_t integrationTime = atoi(argv[++i]);
                        float_t gain = (float_t)atof(argv[++i]);
                        TrackingData::Exposure::StreamExposure streamExposure(SET_SENSOR_ID(SensorType::Fisheye, index), integrationTime, gain);
                        gConfiguration.exposure.stream[gConfiguration.exposure.numOfVideoStreams] = streamExposure;
                        gConfiguration.exposure.numOfVideoStreams++;
                        parseError = false;
                    }
                }
            }

            if (parseError == true)
            {
                printf("-exposure : Set exposure values per video sensor index\n");
                printf("            Parameters: <SensorIndex[0-3]> <integration time (uSec)> <gain>\n");
                printf("            Example: \"libtm_util.exe -video 0 1 -video 1 1 -exposure 0 10000 1\" : Set exposure for video sensor 0 for 10000 uSec integration time and 1 gain\n");
                return -1;
            }
        }
        else if ((arg == "-FW") || (arg == "-fw"))
        {
            /* Make sure we aren't at the end of argv */
            if ((i + 1 < argc) && (strstr(argv[i + 1], "-") != argv[i + 1]))
            {
                gConfiguration.inputFilename = argv[++i];
            }
            else
            {
                printf("-fw : Set external FW (mvcmd)\n");
                printf("      Parameters: <filename>\n");
                printf("      Example: \"libtm_util.exe -fw target.mvcmd\" : Load external FW from file target.mvcmd\n");
                return -1;
            }
        }
        else if ((arg == "-statistics") || (arg == "-stat"))
        {
            if ((i + 1 < argc) && (strncmp("-h", argv[i + 1], 2) == 0))
            {
                printf("-statistics : Save statistics of all enabled sensors, 6dofs and controllers which are outputted to host into CSV file\n");
                printf("              Example: \"libtm_util.exe -gyro 0 1 -gyro 1 0 -6dof 1 -statistics\" : Save statistics of gyro 0 and 6dof 1 to CSV file (gyro 1 is not outputted)\n");
                return -1;
            }
            else
            {
                gConfiguration.statistics = true;

                poseCSV.precision(flt::max_digits10);
                videoCSV.precision(flt::max_digits10);
                gyroCSV.precision(flt::max_digits10);
                velocimeterCSV.precision(flt::max_digits10);
                accelerometerCSV.precision(flt::max_digits10);
                controllerCSV.precision(flt::max_digits10);
                rssiCSV.precision(flt::max_digits10);
            }
        }
        else if ((arg == "-tum"))
        {
            bool parseError = true;

            /* Make sure we aren't at the end of argv */
            if ((i + 1 < argc) && (strstr(argv[i + 1], "-") != argv[i + 1]))
            {
                uint8_t index = atoi(argv[++i]);
                if ((index > 0) && (index < 3))
                {
                    gConfiguration.tumFormat = index;
                    parseError = false;
                    poseTUM.precision(flt::max_digits10);
                }
            }

            if (parseError == true)
            {
                printf("-tum : Save 6dof (pose) statistics in Tum format\n");
                printf("       Parameters: <Format [1-2]>\n");
                printf("                   Format 1: Timestamp(seconds) translation.x translation.y translation.z rotation.Qi rotation.Qj rotation.Qk rotation.Qw\n");
                printf("                   Format 2: Timestamp(seconds) translation.x translation.y translation.z rotation.Qi rotation.Qj rotation.Qk rotation.Qw DeviceId(HMD/Controller1/Controller2)\n");
                printf("       Example: \"libtm_util.exe -6dof 0 -tum 1\" : Enable 6dof for HMD and save all 6dof statistics in Tum type 1 format\n");
                return -1;
            }
        }
        else if (arg == "-image")
        {
            if ((i + 1 < argc) && (strncmp("-h", argv[i + 1], 2) == 0))
            {
                printf("-image : Output all enabled video sensors images to PGM files\n");
                printf("         Example: \"libtm_util.exe -video 0 1 -video 1 1 -image\" : Output all enabled video sensors images (video sensors 0 + 1) to PGM files\n");
                return -1;
            }
            else
            {
                gConfiguration.videoFile = true;
            }
        }
        else if ((arg == "-log"))
        {
            bool parseError = true;

            /* Make sure we aren't at the end of argv */
            if ((i + 4 < argc) && (strstr(argv[i + 1], "-") != argv[i + 1]) && (strstr(argv[i + 2], "-") != argv[i + 2]) && (strstr(argv[i + 3], "-") != argv[i + 3]) && (strstr(argv[i + 4], "-") != argv[i + 4]))
            {
                std::string logSource = argv[++i];
                LogSource source = LogSourceMax;

                if ((logSource == "FW") || (logSource == "fw"))
                {
                    source = LogSourceFW;
                }
                else if ((logSource == "HOST") || (logSource == "host"))
                {
                    source = LogSourceHost;
                }

                uint8_t verbosity = atoi(argv[++i]);
                uint8_t logRolloverMode = atoi(argv[++i]);
                uint32_t logOutputMode = atoi(argv[++i]);
                if ((source < LogSourceMax) && (verbosity <= Trace) && (logRolloverMode <= 1) && (logOutputMode < LogOutputModeMax))
                {
                    gConfiguration.logConfiguration[source].logVerbosity = (LogVerbosityLevel)verbosity;
                    gConfiguration.logConfiguration[source].logRolloverMode = (bool)logRolloverMode;
                    gConfiguration.logConfiguration[source].logOutputMode = (LogOutputMode)logOutputMode;
                    gConfiguration.logConfiguration[source].setLog = true;
                    parseError = false;
                }
            }

            if (parseError == true)
            {
                printf("-log : Set FW/host log verbosity, rollover mode and output mode\n");
                printf("       Parameters: <Source [fw/host]> <Verbosity [0-6]> <Mode [0-1]> <OutputMode [0-1]>\n");
                printf("       Source:       \"fw\"   - configure fw log\n");
                printf("                     \"host\" - configure host log\n");
                printf("       Verbosity:    0 - None: Logging is disabled\n");
                printf("                     1 - Error: Error conditions\n");
                printf("                     2 - Info: High importance informational entries - for general information on production images\n");
                printf("                     3 - Warning: Warning messages\n");
                printf("                     4 - Debug: Even more informational entries\n");
                printf("                     5 - Verbose: max messages available\n");
                printf("                     6 - Trace: On entry and exit from every function call - for profiling\n");
                printf("       RolloverMode: 0 - No rollover, logging will be paused after log is filled. Until cleared, first events will be stored\n");
                printf("                     1 - Rollover mode\n");
                printf("       OutputMode:   0 - Output to screen\n");
                printf("                     1 - Output to buffer\n");
                printf("       Example: \"libtm_util.exe -gyro 0 1 -log fw 6 0 1\" : Set FW log to Trace, disable rollover mode and output to buffer\n");
                printf("       Example: \"libtm_util.exe -gyro 0 1 -log host 4 1 0\" : Set Host log to Debug, enable rollover mode and output to screen\n");
                return -1;
            }
        }
        else if ((arg == "-calibrate"))
        {
            bool parseError = true;

            /* Make sure we aren't at the end of argv */
            if ((i + 1 < argc) && (strstr(argv[i + 1], "-") != argv[i + 1]))
            {
                if (strncmp("new", argv[++i], 3) == 0)
                {
                    gConfiguration.calibration.type = CalibrationTypeNew;
                    gConfiguration.calibration.filename = argv[++i];
                    parseError = false;
                }
                else if (strncmp("append", argv[i], 6) == 0)
                {
                    gConfiguration.calibration.type = CalibrationTypeAppend;
                    gConfiguration.calibration.filename = argv[++i];
                    parseError = false;
                }
            }

            if (parseError == true)
            {
                printf("-calibrate : Set new or append calibration from an external file\n");
                printf("             Parameters: <new/append> <filename>\n");
                printf("             Example: \"libtm_util.exe -calibrate new calibration_file.json\"    : set new calibration from file calibration_file.json\n");
                printf("             Example: \"libtm_util.exe -calibrate append calibration_file.json\" : append calibration from file calibration_file.json\n");
                return -1;
            }
        }
        else if ((arg == "-power"))
        {
            bool parseError = true;

            /* Make sure we aren't at the end of argv */
            if ((i + 1 < argc) && (strstr(argv[i + 1], "-") != argv[i + 1]))
            {
                if (strncmp("high", argv[++i], 4) == 0)
                {
                    gConfiguration.lowPowerEnabled = false;
                    parseError = false;
                }
                else if (strncmp("low", argv[i], 3) == 0)
                {
                    gConfiguration.lowPowerEnabled = true;
                    parseError = false;
                }
            }

            if (parseError == true)
            {
                printf("-power : Set high/low power mode\n");
                printf("         Default mode: Low power\n");
                printf("         Parameters: <high/low>\n");
                printf("         Example: \"libtm_util.exe -power high\" : Set high power mode - FW will never go to sleep\n");
                printf("         Example: \"libtm_util.exe -power low\"  : Set low power mode - FW will go to sleep before start stream / after stop stream\n");
                return -1;
            }
        }
        else if ((arg == "-map"))
        {
            bool parseError = true;

            /* Make sure we aren't at the end of argv */
            if ((i + 1 < argc) && (strstr(argv[i + 1], "-") != argv[i + 1]))
            {
                if (strncmp("reset", argv[++i], 3) == 0)
                {
                    gConfiguration.localization[LocalizationTypeReset].enabled = true;
                    parseError = false;
                }
                else if ((i + 1 < argc) && (strstr(argv[i + 1], "-") != argv[i + 1]))
                {
                    if (strncmp("set", argv[i], 3) == 0)
                    {
                        gConfiguration.localization[LocalizationTypeSet].enabled = true;
                        gConfiguration.localization[LocalizationTypeSet].filename = argv[++i];
                        parseError = false;
                    }
                    else if (strncmp("get", argv[i], 3) == 0)
                    {
                        gConfiguration.localization[LocalizationTypeGet].enabled = true;
                        gConfiguration.localization[LocalizationTypeGet].filename = argv[++i];
                        parseError = false;
                    }
                }
            }

            if (parseError == true)
            {
                printf("-map : Reset/Set/Get localization map from/to an external file\n");
                printf("       On multiple call, the order will be:  Get, Set, Reset\n");
                printf("       Parameters: <reset/set/get> <filename>\n");
                printf("       Example: \"libtm_util.exe -map reset\"                            : reset localization map\n");
                printf("       Example: \"libtm_util.exe -map set localization_map_input_file\"  : set localization from file localization_map_input_file\n");
                printf("       Example: \"libtm_util.exe -map get localization_map_output_file\" : get localization to a new file localization_map_output_file\n");
                printf("       Notice:  To get localization map, 6dof must be enabled\n");
                return -1;
            }
        }
        else if ((arg == "-jtag"))
        {
            if ((i + 1 < argc) && (strncmp("-h", argv[i + 1], 2) == 0))
            {
                printf("-jtag : Load FW from JTAG, libtm_util will wait until getting new USB ATTACH after JTAG load\n");
                return -1;
            }
            else
            {
                gConfiguration.jtag = true;
            }
        }
        else if ((arg == "-temperature"))
        {
            bool parseError = true;

            /* Make sure we aren't at the end of argv */
            /* -temperature XXX */
            if ((i + 1 < argc) && (strstr(argv[i + 1], "-") != argv[i + 1]))
            {
                std::string command = argv[++i];
                if (command == "get")
                {
                    /* -temperature show */
                    gConfiguration.temperature.check = true;
                    parseError = false;
                }
                else if (command == "set")
                {
                    /* -temperature set XXX XXX */
                    if ((i + 2 < argc) && (strstr(argv[i + 1], "-") != argv[i + 1]) && (strstr(argv[i + 2], "-") != argv[i + 2]))
                    {
                        std::string sensor = argv[++i];
                        TemperatureSensorType type = TemperatureSensorMax;

                        if ((sensor == "vpu") || (sensor == "VPU"))
                        {
                            type = TemperatureSensorVPU;
                        }
                        else if ((sensor == "imu") || (sensor == "IMU"))
                        {
                            type = TemperatureSensorIMU;
                        }
                        if ((sensor == "ble") || (sensor == "BLE"))
                        {
                            type = TemperatureSensorBLE;
                        }

                        if (type != TemperatureSensorMax)
                        {
                            float_t threshold = float_t(atof(argv[++i]));
                            gConfiguration.temperature.check = true;
                            gConfiguration.temperature.threshold[type].set = true;
                            gConfiguration.temperature.threshold[type].temperature = threshold;
                            parseError = false;
                        }
                    }
                }
            }

            if (parseError == true)
            {
                printf("-temperature : Get and set temperature and temperature threshold from all sensors (VPU, IMU, BLE) before, during and after START\n");
                printf("               Parameters: <get/set> <sensor [VPU, IMU, BLE] (Optional)> <threshold (optional)>\n");
                printf("               Example: \"libtm_util.exe -temperature get\" : Get temperature from all sensors before, during and after START\n");
                printf("               Example: \"libtm_util.exe -temperature set BLE 60\" : Set temperature threshold to BLE sensor to 60 (Celsius)\n");
                return -1;
            }
        }
        else if ((arg == "-geo"))
        {
            bool parseError = true;

            /* Make sure we aren't at the end of argv */
            if (i + 3 < argc)
            {
                gConfiguration.geoLocation.latitude = double_t(atof(argv[++i]));
                gConfiguration.geoLocation.longitude = double_t(atof(argv[++i]));
                gConfiguration.geoLocation.altitude = double_t(atof(argv[++i]));
                gConfiguration.geoLocationEnabled = true;
                parseError = false;
            }

            if (parseError == true)
            {
                printf("-geo : Sets the geographical location (e.g.GPS data). this data can be later used by the algorithms to correct IMU readings.\n");
                printf("       Parameters: <latitude> <longitude> <altitude>\n");
                printf("       Example: \"libtm_util.exe -geo -39.1344245 175.6525533 1788\"\n");
                return -1;
            }
        }
        else if ((arg == "-gpio"))
        {
            bool parseError = true;

            /* Make sure we aren't at the end of argv */
            if ((i + 1 < argc) && (strstr(argv[i + 1], "-") != argv[i + 1]))
            {
                uint8_t gpioControlBitMask = 0;
                if (strncmp("0x", argv[++i], 2) == 0)
                {
                    gpioControlBitMask = (uint8_t)strtol(argv[i], NULL, 16);
                }
                else
                {
                    gpioControlBitMask = atoi(argv[i]);
                }

                if (gpioControlBitMask <= 0xF)
                {
                    gConfiguration.gpioControlBitMask = gpioControlBitMask;
                    gConfiguration.gpioEnabled = true;
                    parseError = false;
                }
            }

            if (parseError == true)
            {
                printf("-gpio : Sets GPIO control bit mask.\n");
                printf("        Parameters: <controlBitMask - decimal or hex>\n");
                printf("        controlBitMask: Bit 0 - GPIO 74\n");
                printf("                      : Bit 1 - GPIO 75\n");
                printf("                      : Bit 2 - GPIO 76\n");
                printf("                      : Bit 3 - GPIO 77\n");
                printf("                      : Bit 4-7 - Reserved\n");
                printf("        Example: \"libtm_util.exe -gpio 11\"  : Toggle GPIO 74, 75, 77 to High (1011 in binary)\n");
                printf("        Example: \"libtm_util.exe -gpio 0xF\" : Toggle GPIO 74, 75, 76, 77 to High (1111 in binary)\n");
                return -1;
            }
        }
        else if ((arg == "-velocimeter"))
        {
            if ((i + 1 < argc) && (strstr(argv[i + 1], "-") != argv[i + 1]))
            {
                gConfiguration.velocimeterFilename = argv[++i];
                gConfiguration.velocimeter[0].enabled = true;
                gConfiguration.velocimeter[0].outputEnabled = true;
                gConfiguration.velocimeter[0].fps = 0;
                gConfiguration.velocimeterCount++;
            }
            else
            {
                printf("-velocimeter : Enable external velocimeter sensor for recording and playback\n");;
                printf("               Parameters: <filename>\n");
                printf("               Example: \"libtm_util.exe -velocimeter velocimeterfile.csv\" : Enable Velocimeter and sends all velocimeter frames input file to the FW\n");
                printf("               Velocimeter file must include the following line(s) in the following pattern:\n");
                printf("               FrameId,translationalVelocity.X,translationalVelocity.Y,translationalVelocity.Z,timestamp,arrivaltimeStamp\n");
                printf("               File example: 0,1.0,2.0,3.0,0,0 : Velocimeter frame ID 0 with velocity (1.0,2.0,3.0) and timestamp 0\n");
                return -1;
            }
        }
        else if ((arg == "-controller_data"))
        {
            /* Make sure we aren't at the end of argv */
            if ((i + 1 < argc) && (strstr(argv[i + 1], "-") != argv[i + 1]))
            {
                gConfiguration.controllerDataFilename = argv[++i];
            }
            else
            {
                printf("-controller_data : Sends controller data to connected controllers\n");
                printf("                   Parameters: <filename>\n");
                printf("                   Example: \"libtm_util.exe -controller_data file.csv\" : Sends all controller commands from the file to the connected controllers\n\n");
                printf("                   Command file must include the following line(s) in the following pattern:\n");
                printf("                   ControllerId(1-2),CommandId(0-63),BufferSize,BufferData1,BufferData2...\n");
                printf("                   Example: 1,11,5,72,65,108,108,111        : Send to Controller 1, Command 11, Size 5, BufferData=\"Hello\"\n");
                printf("                   Example: 2,11,5,0x4C,0x69,0x62,0x54,0x6D : Send to Controller 2, Command 11, Size 5, BufferData=\"LibTm\"\n");
                return -1;
            }
        }
        else if ((arg == "-node"))
        {
            /* Make sure we aren't at the end of argv */
            if ((i + 1 < argc) && (strstr(argv[i + 1], "-") != argv[i + 1]))
            {
                gConfiguration.nodeFilename = argv[++i];
            }
            else
            {
                printf("-node : Sends static node data from file\n");
                printf("        Parameters: <filename>\n");
                printf("        Example: \"libtm_util.exe -node StaticNodeFile.csv\" : Sends all static nodes from the file\n\n");
                printf("        Node file must include the following line(s) in the following pattern:\n");
                printf("        Guid (1-127 length string),TranslationX,TranslationY,TranslationZ,rotationI,rotationJ,rotationK,rotationR\n");
                printf("        Example: Wall,1.1,2.2,3.3,4.4,5.5,6.6,7.7 : GUID = \"Wall\", Translation = [1.1,2.2,3.3]. Rotation = [4.4,5.5,6.6,7.7]\n");
                return -1;
            }
        }
        else if ((arg == "-burn"))
        {
            bool parseError = true;
            if ((i + 2 < argc) && (strstr(argv[i + 1], "-") != argv[i + 1]) && (strstr(argv[i + 2], "-") != argv[i + 2]))
            {
                gConfiguration.controllerFWFileType = argv[++i];
                gConfiguration.controllerFWFile = argv[++i];

                if ((gConfiguration.controllerFWFileType == "app") || (gConfiguration.controllerFWFileType == "bl"))
                {
                    /* -burn app/bl image XXX */
                    if ((i + 1 < argc) && (strstr(argv[i + 1], "-") != argv[i + 1]))
                    {
                        std::string burnConfig = argv[i + 1];
                        if (burnConfig == "force")
                        {
                            for (uint8_t id = Controller1; id < ProfileTypeMax; id++)
                            {
                                gConfiguration.controllers[id].burn.configure = ControllerBurnForce;
                            }
                            parseError = false;
                            i++;
                        }
                    }
                    else
                    {
                        for (uint8_t id = Controller1; id < ProfileTypeMax; id++)
                        {
                            gConfiguration.controllers[id].burn.configure = ControllerBurnOnNewVersion;
                        }
                        parseError = false;
                    }
                }
            }

            if (parseError)
            {
                printf("-burn : Burn controller firmware to connected controllers\n");
                printf("        Parameters: <Controller firmware image type [bl/app]> <Controller firmware image> <force (optional)>\n");
                printf("        Example: \"libtm_util.exe -6dof 1 -controller AABBCCDDEEFF 0 -burn app controller_app_file\" : burn controller app file controller_fw_file to controller only if controller version is older than file version\n");
                printf("        Example: \"libtm_util.exe -6dof 1 -controller AABBCCDDEEFF 0 -burn bl controller_bl_file -force\" : force burn controller bl file controller_bl_file to controller (ignore controller versions)\n");
                return -1;
            }
        }
        else
        {
            printf("Error: unknown parameter %s\n\n", arg.c_str());
            showArguments();
            return -1;
        }
    }

    printf("-------------------------------------------------------------------\n");
    printf("libtm_util Run Configuration:\n");
    printf(" - Run Time = %d (sec)\n", gConfiguration.startStreamTime);
    printf(" - Stop Time = %d (sec)\n", gConfiguration.stopStreamTime);

    if (gConfiguration.videoCount + gConfiguration.gyroCount + gConfiguration.accelerometerCount + gConfiguration.sixdofCount + gConfiguration.controllersCount + gConfiguration.rssiCount == 0)
    {
        /* Skipping START/STOP loop in case there are no enabled Videos/IMU/Poses */
        gConfiguration.maxLoop = 0;
    }
    printf(" - Loop Count = %d\n", gConfiguration.maxLoop);
    printf(" - Reset Count = %d\n", gConfiguration.resetLoop);
    printf(" - Error Check = %s %s\n", (gConfiguration.errorCheck ? "Enabled" : "Disabled"), (gConfiguration.errorExit ? "(Exit on error)" : ""));
    printf(" - FW image = %s (%s)\n", (gConfiguration.inputFilename.empty() ? "Internal" : "External"), (gConfiguration.inputFilename.empty() ? FW_VERSION : gConfiguration.inputFilename.c_str()));

    if (gConfiguration.controllers[Controller1].burn.configure > ControllerBurnDisabled)
    {
        printf(" - Controller %sburn %s image = %s %s\n", (gConfiguration.controllers[Controller1].burn.configure == ControllerBurnForce)?"Force ":"", gConfiguration.controllerFWFileType.c_str(), gConfiguration.controllerFWFile.c_str(),
            (!std::ifstream(gConfiguration.controllerFWFile.c_str())) ? "(Warning: file not found)" : (gConfiguration.sixdof[SixDofProfile1].enabled | gConfiguration.sixdof[SixDofProfile2].enabled) ? "" : "(Warning: All controller sensors are disabled (Gyro, Accelerometer, 6dof))");
    }

    printf(" - Streaming mode = %s\n", gConfiguration.mode.c_str());
    for (uint8_t i = LogSourceHost; i < LogSourceMax; i++)
    {
        if (gConfiguration.logConfiguration[i].setLog == true)
        {
            printf(" - %s Log: verbosity = 0x%X, mode = %sRollover, log to %s\n", (i == LogSourceHost) ? "Host" : "FW",
                gConfiguration.logConfiguration[i].logVerbosity, (gConfiguration.logConfiguration[i].logRolloverMode == false) ? "No " : "", ((gConfiguration.logConfiguration[i].logOutputMode == LogOutputModeScreen) ? "Screen" : "Buffer"));
        }
    }

    if (gConfiguration.calibration.filename.empty() == false)
    {
        printf(" - Calibration = Type: %s, Input File: %s %s\n", (gConfiguration.calibration.type == CalibrationTypeNew)?"New":"Append", gConfiguration.calibration.filename.c_str(), (!std::ifstream(gConfiguration.calibration.filename.c_str())) ? "(Warning: file not found)" : "");
    }

    printf(" - Low Power Mode = %s\n", (gConfiguration.lowPowerEnabled ? "Enabled" : "Disabled"));
    printf(" - JTAG = %s\n", (gConfiguration.jtag ? "True" : "False"));
    printf(" - Images output = %s\n", (gConfiguration.videoFile == true) ? "Enabled" : "Disabled");

    if (gConfiguration.gpioEnabled == true)
    {
        printf(" - GPIO Control BitMask = 0x%X\n", gConfiguration.gpioControlBitMask);
    }

    printf(" - Temperature check = %s\n", (gConfiguration.temperature.check ? "True" : "False"));

    for (uint8_t i = TemperatureSensorVPU; i < TemperatureSensorMax; i++)
    {
        if (gConfiguration.temperature.threshold[i].set == true)
        {
            printf(" - Temperature threshold %s = %.2f\n", (i == TemperatureSensorVPU) ? "VPU" : (i == TemperatureSensorIMU) ? "IMU" : "BLE", gConfiguration.temperature.threshold[i].temperature);
        }
    }

    if (gConfiguration.geoLocationEnabled)
    {
        printf(" - GEO Location [Latitude, Longitude, Altitude] = [%lf, %lf, %lf]\n", gConfiguration.geoLocation.latitude, gConfiguration.geoLocation.longitude, gConfiguration.geoLocation.altitude);
    }

    printf(" - Statistics output = %s\n", (gConfiguration.statistics == true) ? "True" : "False");
    printf(" - TUM output = %s\n", (gConfiguration.tumFormat > 0) ? "True" : "False");

    for (uint8_t i = LocalizationTypeGet; i < LocalizationTypeMax; i++)
    {
        if (gConfiguration.localization[i].enabled == true)
        {
            printf(" - Localization map %s %s %s %s\n",
                ((i == LocalizationTypeGet) ? "Get to" : (i == LocalizationTypeSet) ? "Set from" : "Reset"),
                ((i == LocalizationTypeGet) ? "output file:" : (i == LocalizationTypeSet) ? "input file:" : ""),
                gConfiguration.localization[i].filename.c_str(),
                ((i == LocalizationTypeGet) && (std::ifstream(gConfiguration.localization[i].filename.c_str()))) ? "(Warning: file already exists)" :
                ((i == LocalizationTypeSet) && (!std::ifstream(gConfiguration.localization[i].filename.c_str()))) ? "(Warning: file not found)" : "");
        }
    }

    for (uint8_t i = Controller1; i < ProfileTypeMax; i++)
    {
        uint8_t* macAddress = gConfiguration.controllers[i].macAddress;
        if ((macAddress[0] == 0) && (macAddress[1] == 0) && (macAddress[2] == 0) && (macAddress[3] == 0) && (macAddress[4] == 0) && (macAddress[5] == 0))
        {
            printf(" - Controller %d = None %s\n", i, (gConfiguration.controllers[i].needed == true) ? "(Warning: several controller sensors are enabled, need to enable controller)" : "");
        }
        else
        {
            printf(" - Controller %d = %02X:%02X:%02X:%02X:%02X:%02X, Calibrate = %s %s\n", gConfiguration.controllers[i].controllerID, macAddress[0], macAddress[1], macAddress[2], macAddress[3], macAddress[4], macAddress[5], (gConfiguration.controllers[i].calibrate == false)?"False":"True",
                (gConfiguration.controllers[i].needed == false) ? "(Warning: All controller sensors are disabled (Gyro, Accelerometer, 6dof))" : "");
        }
    }

    printf(" - Static Node Input = %s %s\n", (gConfiguration.nodeFilename.empty() ? "False" : std::string("True (" + gConfiguration.nodeFilename + ")").c_str()),
        ((gConfiguration.nodeFilename.empty() == false) && ((bool)std::ifstream(gConfiguration.nodeFilename.c_str()) == false)) ? "(Warning: file not found)" : "");

    printf(" - Controller Data Input = %s %s\n", (gConfiguration.controllerDataFilename.empty() ? "False" : std::string("True (" + gConfiguration.controllerDataFilename + ")").c_str()),
                                        ((gConfiguration.controllerDataFilename.empty() == false) && ((bool)std::ifstream(gConfiguration.controllerDataFilename.c_str()) == false)) ? "(Warning: file not found)" : "");

    printf(" - Velocimeter Input = %s %s\n", (gConfiguration.velocimeterFilename.empty() ? "False" : std::string("True (" + gConfiguration.velocimeterFilename + ")").c_str()),
        ((gConfiguration.velocimeterFilename.empty() == false) && ((bool)std::ifstream(gConfiguration.velocimeterFilename.c_str()) == false)) ? "(Warning: file not found)" : "");

    for (uint8_t i = Controller1; i < ProfileTypeMax; i++)
    {
        if (gConfiguration.rssi[i].time != 0)
        {
            printf(" - Rssi Controller %d = %d (sec) %s\n", i, gConfiguration.rssi[i].time, ((gConfiguration.rssi[i].time != 0) && (gConfiguration.controllers[i].controllerID == ProfileTypeMax) ? "(Warning: controller is disabled)" : ""));
        }
        else
        {
            printf(" - Rssi Controller %d = False\n", i);
        }
    }

    printf(" - Manual Exposure = %s on %d streams\n", gConfiguration.setExposure ? "Enabled" : "Disabled", gConfiguration.exposure.numOfVideoStreams);
    if (gConfiguration.setExposure == true)
    {
        for (uint8_t i = 0; i < gConfiguration.exposure.numOfVideoStreams; i++)
        {
            printf("   - Video %d, Integrationtime = %d, Gain = %f\n", GET_SENSOR_INDEX(gConfiguration.exposure.stream[i].cameraId), gConfiguration.exposure.stream[i].integrationTime, gConfiguration.exposure.stream[i].gain);
        }
    }

    std::stringstream indexEnabled;
    std::stringstream indexOutput;
    std::stringstream indexFPS;

    uint32_t totalProfiles = 1;

    if (gConfiguration.videoCount + gConfiguration.gyroCount + gConfiguration.accelerometerCount + gConfiguration.sixdofCount + gConfiguration.rssiCount + gConfiguration.velocimeterCount> 0)
    {
        printf("---+---------------+--------+---------+--------+---------+---------+--------\n");
        printf(" # |    Enabled    | Sensor | Frames  |  Mode  |  Width  | Height  | Output \n");
        printf("   |    Sensors    | Index  | PerSec  |        |         |         | Mode   \n");
        printf("---+---------------+--------+---------+--------+---------+---------+--------\n");
    }

    std::string width = "Default";
    std::string height = "Default";
    std::string fps = "Default";

    for (uint8_t i = 0; i < VideoProfileMax; i++)
    {
        if (gConfiguration.video[i].enabled)
        {
            if ((gConfiguration.video[i].width > 0) || (gConfiguration.video[i].height > 0) || (gConfiguration.video[i].fps > 0))
            {
                width = std::to_string(gConfiguration.video[i].width);
                height = std::to_string(gConfiguration.video[i].height);
                fps = std::to_string(gConfiguration.video[i].fps);
            }
            else
            {
                width = "Default";
                height = "Default";
                fps = "Default";
            }

            printf("%02d | Video         |   %01d    | %-7s |        | %-7s | %-7s | %-5d\n", 
                totalProfiles, i, fps.c_str(), width.c_str(), height.c_str(), gConfiguration.video[i].outputEnabled);
            totalProfiles++;
        }       
    }

    for (uint8_t i = 0; i < GyroProfileMax; i++)
    {
        if (gConfiguration.gyro[i].enabled)
        {
            if (gConfiguration.gyro[i].fps > 0)
            {
                fps = std::to_string(gConfiguration.gyro[i].fps);
            }
            else
            {
                fps = "Default";
            }

            printf("%02d | Gyro          |   %01d    | %-7s |        |         |         | %-5d\n", totalProfiles, i, fps.c_str(), gConfiguration.gyro[i].outputEnabled);
            totalProfiles++;
        }
    }

    for (uint8_t i = 0; i < VelocimeterProfileMax; i++)
    {
        if (gConfiguration.velocimeter[i].enabled)
        {
            printf("%02d | velocimeter   |   %01d    |         |        |         |         | %-5d\n", totalProfiles, i, gConfiguration.velocimeter[i].outputEnabled);
            totalProfiles++;
        }
    }

    for (uint8_t i = 0; i < AccelerometerProfileMax; i++)
    {
        if (gConfiguration.accelerometer[i].enabled)
        {
            if (gConfiguration.accelerometer[i].fps > 0)
            {
                fps = std::to_string(gConfiguration.accelerometer[i].fps);
            }
            else
            {
                fps = "Default";
            }

            printf("%02d | Accelerometer |   %01d    | %-7s |        |         |         | %-5d\n", totalProfiles, i, fps.c_str(), gConfiguration.accelerometer[i].outputEnabled);
            totalProfiles++;
        }
    }

    for (uint8_t i = 0; i < SixDofProfileMax; i++)
    {
        if (gConfiguration.sixdof[i].enabled == true)
        {
            printf("%02d | 6Dof          |   %01d    |         |  0x%02X  |         |         | 1\n", totalProfiles, i, gConfiguration.sixdof[i].mode);
            totalProfiles++;
        }
    }

    for (uint8_t i = 0; i < ProfileTypeMax; i++)
    {
        if (gConfiguration.rssi[i].time != 0)
        {
            printf("%02d | RSSI          |   %01d    |         |        |         |         | 1\n", totalProfiles, i);
            totalProfiles++;
        }
    }


    if (gConfiguration.videoCount + gConfiguration.gyroCount + gConfiguration.accelerometerCount + gConfiguration.sixdofCount + gConfiguration.rssiCount + gConfiguration.velocimeterCount == 0)
    {
        printf("----------------------------------------------------------------------------\n");
    }
    else
    {
        printf("---+---------------+--------+---------+--------+---------+---------+--------\n");
    }

    if (gConfiguration.verifyConfiguration == true)
    {
        printf("Continue? (Y/N) - ");
        input = getchar();
        if ((input == 'N') || (input == 'n'))
        {
            return 1;
        }
    }

    return 0;
}


void saveOutput()
{
    if ((gConfiguration.statistics == true) || (gConfiguration.tumFormat > 0))
    {
        if (!videoCSV.str().empty())
        {
            std::ostringstream csvHeader;
            csvHeader << "Video Index," << "Frame Number,"
                      << "Host Timestamp (NanoSec)," << "Host Correlated to FW Timestamp (NanoSec)," << "FW Timestamp (NanoSec)," << "Arrival Timestamp (NanoSec)," << "FW to Host Latency (NanoSec),"
                      << "Frame Id," << "Frame Length," << "Exposure time," << "Gain," << "Stride," << "Width," << "Height," << "Pixel Format" << "\n";

            static int videoCount = 0;
            std::string fileHeaderName(gFileHeaderName);
            std::string fileName = fileHeaderName + "_Libtm_" + std::to_string(LIBTM_VERSION_MAJOR) + "_" + std::to_string(LIBTM_VERSION_MINOR) +
                "_" + std::to_string(LIBTM_VERSION_PATCH) + "_FW_" + FW_VERSION + "_Video_Loop_" + std::to_string(videoCount++) + ".csv";
            std::ofstream save_file(".\\" + fileName, std::ofstream::binary);
            save_file.write((char*)csvHeader.str().data(), csvHeader.str().size());
            save_file.write((char*)videoCSV.str().data(), videoCSV.str().size());
            save_file.close();
            LOGD("Video saved to %s", fileName.c_str());
            videoCSV.str("");
        }

        if (!gyroCSV.str().empty())
        {
            std::ostringstream csvHeader;
            csvHeader << "Gyro Index," << "Frame Number,"
                      << "Host Timestamp (NanoSec)," << "Host Correlated to FW Timestamp (NanoSec)," << "FW Timestamp (NanoSec)," << "Arrival Timestamp (NanoSec)," << "FW to Host Latency (NanoSec),"
                      << "Frame Id," << "Temperature (Celsius)," << "Angular Velocity X (Radians/Sec)," << "Angular Velocity Y (Radians/Sec)," << "Angular Velocity Z (Radians/Sec)," << "\n";

            static int gyroCount = 0;
            std::string fileHeaderName(gFileHeaderName);
            std::string fileName = fileHeaderName + "_Libtm_" + std::to_string(LIBTM_VERSION_MAJOR) + "_" + std::to_string(LIBTM_VERSION_MINOR) +
                "_" + std::to_string(LIBTM_VERSION_PATCH) + "_FW_" + FW_VERSION + "_Gyro_Loop_" + std::to_string(gyroCount++) + ".csv";
            std::ofstream save_file(".\\" + fileName, std::ofstream::binary);
            save_file.write((char*)csvHeader.str().data(), csvHeader.str().size());
            save_file.write((char*)gyroCSV.str().data(), gyroCSV.str().size());
            save_file.close();
            LOGD("Gyro saved to %s", fileName.c_str());
            gyroCSV.str("");
        }

        if (!velocimeterCSV.str().empty())
        {
            std::ostringstream csvHeader;
            csvHeader << "Velocimeter Index," << "Frame Number,"
                << "Host Timestamp (NanoSec)," << "Host Correlated to FW Timestamp (NanoSec)," << "FW Timestamp (NanoSec)," << "Arrival Timestamp (NanoSec)," << "FW to Host Latency (NanoSec),"
                << "Frame Id," << "Temperature (Celsius)," << "Translational Velocity X (Meters/Sec)," << "Translational Velocity Y (Meters/Sec)," << "Translational Velocity Z (Meters/Sec)," << "\n";

            static int velocimeterCount = 0;
            std::string fileHeaderName(gFileHeaderName);
            std::string fileName = fileHeaderName + "_Libtm_" + std::to_string(LIBTM_VERSION_MAJOR) + "_" + std::to_string(LIBTM_VERSION_MINOR) +
                "_" + std::to_string(LIBTM_VERSION_PATCH) + "_FW_" + FW_VERSION + "_Velocimeter_Loop_" + std::to_string(velocimeterCount++) + ".csv";
            std::ofstream save_file(".\\" + fileName, std::ofstream::binary);
            save_file.write((char*)csvHeader.str().data(), csvHeader.str().size());
            save_file.write((char*)velocimeterCSV.str().data(), velocimeterCSV.str().size());
            save_file.close();
            LOGD("Velocimeter saved to %s", fileName.c_str());
            velocimeterCSV.str("");
        }

        if (!poseCSV.str().empty())
        {
            std::ostringstream csvHeader;
            csvHeader << "Pose Index," << "Frame Number,"
                      << "Host Timestamp (NanoSec)," << "Host Correlated to FW Timestamp (NanoSec)," << "FW Timestamp (NanoSec)," << "FW to Host Latency (NanoSec),"
                      << "Translation X (Meter)," << "Translation Y (Meter)," << "Translation Z (Meter),"
                      << "Velocity X (Meter/Sec)," << "Velocity Y (Meter/Sec)," << "Velocity Z (Meter/Sec),"
                      << "Acceleration X (Meter/Sec^2)," << "Acceleration Y (Meter/Sec^2)," << "Acceleration Z (Meter/Sec^2),"
                      << "Rotation I," << "Rotation J," << "Rotation K," << "Rotation R,"
                      << "Angular Velocity X (Radians/Sec)," << "Angular Velocity Y (Radians/Sec)," << "Angular Velocity Z (Radians/Sec),"
                      << "Angular Acceleration X (Radians/Sec^2)," << "Angular Acceleration Y (Radians/Sec^2)," << "Angular Acceleration Z (Radians/Sec^2),"
                      << "Tracker Confidence," << "Mapper Confidence," << "Tracker State," << "\n";

            static int poseCount = 0;
            std::string fileHeaderName(gFileHeaderName);
            std::string fileName = fileHeaderName + "_Libtm_" + std::to_string(LIBTM_VERSION_MAJOR) + "_" + std::to_string(LIBTM_VERSION_MINOR) +
                "_" + std::to_string(LIBTM_VERSION_PATCH) + "_FW_" + FW_VERSION + "_Pose_Loop_" + std::to_string(poseCount++) + ".csv";
            std::ofstream save_file(".\\" + fileName, std::ofstream::binary);
            save_file.write((char*)csvHeader.str().data(), csvHeader.str().size());
            save_file.write((char*)poseCSV.str().data(), poseCSV.str().size());
            save_file.close();
            LOGD("Pose saved to %s", fileName.c_str());
            poseCSV.str("");
        }

        if (!poseTUM.str().empty())
        {
            static int poseCount = 0;
            std::string fileHeaderName(gFileHeaderName);
            std::string fileName = fileHeaderName + "_Libtm_" + std::to_string(LIBTM_VERSION_MAJOR) + "_" + std::to_string(LIBTM_VERSION_MINOR) +
                "_" + std::to_string(LIBTM_VERSION_PATCH) + "_FW_" + FW_VERSION + "_Pose_Loop_" + std::to_string(poseCount++) + ".tum";
            std::ofstream save_file(".\\" + fileName, std::ofstream::binary);
            save_file.write((char*)poseTUM.str().data(), poseTUM.str().size());
            save_file.close();
            LOGD("Pose TUM saved to %s", fileName.c_str());
            poseTUM.str("");
        }

        if (!accelerometerCSV.str().empty())
        {
            std::ostringstream csvHeader;
            csvHeader << "Accelerometer Index," << "Frame Number,"
                      << "Host Timestamp (NanoSec)," << "Host Correlated to FW Timestamp (NanoSec)," << "FW Timestamp (NanoSec)," << "Arrival Timestamp (NanoSec)," << "FW to Host Latency (NanoSec),"
                      << "Frame Id," << "Temperature (Celsius)," << "Acceleration X (Meter/Sec^2)," << "Acceleration Y (Meter/Sec^2)," << "Acceleration Z (Meter/Sec^2),"
                      << "Calibrated Acceleration X (Meter/Sec^2)," << "Calibrated Acceleration Y (Meter/Sec^2)," << "Calibrated Acceleration Z (Meter/Sec^2)," << "Magnitude," << "\n";

            static int accelerometerCount = 0;
            std::string fileHeaderName(gFileHeaderName);
            std::string fileName = fileHeaderName + "_Libtm_" + std::to_string(LIBTM_VERSION_MAJOR) + "_" + std::to_string(LIBTM_VERSION_MINOR) +
                "_" + std::to_string(LIBTM_VERSION_PATCH) + "_FW_" + FW_VERSION + "_Accelerometer_Loop_" + std::to_string(accelerometerCount++) + ".csv";
            std::ofstream save_file(".\\" + fileName, std::ofstream::binary);
            save_file.write((char*)csvHeader.str().data(), csvHeader.str().size());
            save_file.write((char*)accelerometerCSV.str().data(), accelerometerCSV.str().size());
            save_file.close();
            LOGD("Accelerometer saved to %s", fileName.c_str());
            accelerometerCSV.str("");
        }

        if (!controllerCSV.str().empty())
        {
            std::ostringstream csvHeader;
            csvHeader << "Controller Index," << "Frame Number,"
                      << "Host Timestamp (NanoSec)," << "Host Correlated to FW Timestamp (NanoSec)," << "FW Timestamp (NanoSec)," << "Arrival Timestamp (NanoSec)," << "FW to Host Latency (NanoSec),"
                      << "Event Id," << "Instance Id," << "SensorData0," << "SensorData1," << "SensorData2," << "SensorData3," << "SensorData4," << "SensorData5," << "\n";

            static int controllerCount = 0;
            std::string fileHeaderName(gFileHeaderName);
            std::string fileName = fileHeaderName + "_Libtm_" + std::to_string(LIBTM_VERSION_MAJOR) + "_" + std::to_string(LIBTM_VERSION_MINOR) +
                "_" + std::to_string(LIBTM_VERSION_PATCH) + "_FW_" + FW_VERSION + "_Controller_Loop_" + std::to_string(controllerCount++) + ".csv";
            std::ofstream save_file(".\\" + fileName, std::ofstream::binary);
            save_file.write((char*)csvHeader.str().data(), csvHeader.str().size());
            save_file.write((char*)controllerCSV.str().data(), controllerCSV.str().size());
            save_file.close();
            LOGD("Controller saved to %s", fileName.c_str());
            controllerCSV.str("");
        }

        if (!rssiCSV.str().empty())
        {
            std::ostringstream csvHeader;
            csvHeader << "RSSI Index," << "Frame Number,"
                << "Host Timestamp (NanoSec)," << "Host Correlated to FW Timestamp (NanoSec)," << "FW Timestamp (NanoSec)," << "Arrival Timestamp (NanoSec)," << "FW to Host Latency (NanoSec),"
                << "Frame Id," << "Signal Strength (dB)," << "\n";

            static int rssiCount = 0;
            std::string fileHeaderName(gFileHeaderName);
            std::string fileName = fileHeaderName + "_Libtm_" + std::to_string(LIBTM_VERSION_MAJOR) + "_" + std::to_string(LIBTM_VERSION_MINOR) +
                "_" + std::to_string(LIBTM_VERSION_PATCH) + "_FW_" + FW_VERSION + "_Rssi_Loop_" + std::to_string(rssiCount++) + ".csv";
            std::ofstream save_file(".\\" + fileName, std::ofstream::binary);
            save_file.write((char*)csvHeader.str().data(), csvHeader.str().size());
            save_file.write((char*)rssiCSV.str().data(), rssiCSV.str().size());
            save_file.close();
            LOGD("Rssi saved to %s", fileName.c_str());
            rssiCSV.str("");
        }
    }
}


void printSupportedProfiles(TrackingData::Profile& profile)
{
    uint32_t totalProfiles = 0;

    LOGD("---+---------------+------------+--------+-------+--------+--------+--------+---------+--------");
    LOGD(" # |    Profile    |   Sensor   | Frames | Width | Stride | Height | Pixel  | Enabled | Output ");
    LOGD("   |               | Type | Idx | PerSec |       |        |        | Format |         | Mode   ");
    LOGD("---+---------------+------+-----+--------+-------+--------+--------+--------+---------+--------");

    for (uint8_t i = 0; i < VideoProfileMax; i++)
    {
        LOGD("%02d | FishEye       | 0x%02X |  %01d  | %-6d | %-5d | %-6d | %-5d  |  %-5d | %-7d | %d", totalProfiles,
            SensorType::Fisheye,
            profile.video[i].sensorIndex,
            profile.video[i].fps, profile.video[i].profile.width, profile.video[i].profile.stride, profile.video[i].profile.height,
            profile.video[i].profile.pixelFormat, profile.video[i].enabled, profile.video[i].outputEnabled);
        totalProfiles++;
    }

    for (uint8_t i = 0; i < GyroProfileMax; i++)
    {
        LOGD("%02d | Gyro          | 0x%02X |  %01d  | %-6d | %-5d | %-6d | %-5d  |  %-5d | %-7d | %d", totalProfiles, SensorType::Gyro, profile.gyro[i].sensorIndex,
            profile.gyro[i].fps, 0, 0, 0, 0, profile.gyro[i].enabled, profile.gyro[i].outputEnabled);
        totalProfiles++;
    }

    for (uint8_t i = 0; i < AccelerometerProfileMax; i++)
    {
        LOGD("%02d | Accelerometer | 0x%02X |  %01d  | %-6d | %-5d | %-6d | %-5d  |  %-5d | %-7d | %d", totalProfiles, SensorType::Accelerometer, profile.accelerometer[i].sensorIndex,
            profile.accelerometer[i].fps, 0, 0, 0, 0, profile.accelerometer[i].enabled, profile.accelerometer[i].outputEnabled);
        totalProfiles++;
    }

    for (uint8_t i = 0; i < VelocimeterProfileMax; i++)
    {
        LOGD("%02d | Velocimeter   | 0x%02X |  %01d  | %-6d | %-5d | %-6d | %-5d  |  %-5d | %-7d | %d", totalProfiles, SensorType::Velocimeter, profile.velocimeter[i].sensorIndex,
            0, 0, 0, 0, 0, profile.velocimeter[i].enabled, profile.velocimeter[i].outputEnabled);
        totalProfiles++;
    }

    for (uint8_t i = 0; i < SixDofProfileMax; i++)
    {
        uint32_t poseInterruptrate = 0;
        switch (profile.sixDof[i].interruptRate)
        {
            case SIXDOF_INTERRUPT_RATE_NONE:
                poseInterruptrate = 0;
                break;
            case SIXDOF_INTERRUPT_RATE_FISHEYE:
                poseInterruptrate = profile.video[i].fps;
                break;
            case SIXDOF_INTERRUPT_RATE_IMU:
                poseInterruptrate = profile.accelerometer[i].fps + profile.gyro[i].fps;
                break;
        }

        LOGD("%02d | Pose          | 0x%02X |  %01d  | %-6d | %-5d | %-6d | %-5d  |  %-5d | %-7d | %d", totalProfiles, 0, profile.sixDof[i].profileType,
            poseInterruptrate, 0, 0, 0, 0, profile.sixDof[i].enabled, profile.sixDof[i].enabled);
        totalProfiles++;
    }

    LOGD("%02d | Playback      | 0x%02X |  %01d  | %-6d | %-5d | %-6d | %-5d  |  %-5d | %-7d | %d", totalProfiles, 0, 0,
        0, 0, 0, 0, 0, profile.playbackEnabled, profile.playbackEnabled);

    LOGD("---+---------------+------+-----+--------+-------+--------+--------+--------+---------+--------");
}


void configureProfile(TrackingData::Profile& profile)
{
    uint16_t totalFrameRate[SixDofProfileMax] = { 0 };

    for (uint8_t i = GyroProfile0; i < GyroProfileMax; i++)
    {
        profile.gyro[i].fps = gConfiguration.gyro[i].fps;
    }

    for (uint8_t i = AccelerometerProfile0; i < AccelerometerProfileMax; i++)
    {
        profile.accelerometer[i].fps = gConfiguration.accelerometer[i].fps;
    }

    for (uint8_t i = VideoProfile0; i < VideoProfileMax; i++)
    {
        profile.video[i].profile.width = gConfiguration.video[i].width;
        profile.video[i].profile.height = gConfiguration.video[i].height;
        profile.video[i].fps = gConfiguration.video[i].fps;
    }

    gDevice->GetSupportedProfile(profile);

    for (uint8_t i = 0; i < VideoProfileMax; i++)
    {
        TrackingData::VideoProfile video = profile.video[i];
        if (i == video.sensorIndex)
        {
            profile.set(video, gConfiguration.video[i].enabled, gConfiguration.video[i].outputEnabled);
            gStatistics.configure(video, profile.video[i].enabled, profile.video[i].outputEnabled);

            if (profile.sixDof[i].interruptRate == SIXDOF_INTERRUPT_RATE_FISHEYE)
            {
                totalFrameRate[i] += video.fps;
            }
        }
    }

    for (uint8_t i = 0; i < GyroProfileMax; i++)
    {
        TrackingData::GyroProfile gyro = profile.gyro[i];
        if (i == gyro.sensorIndex)
        {
            profile.set(gyro, gConfiguration.gyro[i].enabled, gConfiguration.gyro[i].outputEnabled);
            gStatistics.configure(gyro, profile.gyro[i].enabled, profile.gyro[i].outputEnabled);

            if (profile.sixDof[i].interruptRate == SIXDOF_INTERRUPT_RATE_IMU)
            {
                totalFrameRate[i] += gyro.fps;
            }
        }
    }

    for (uint8_t i = 0; i < AccelerometerProfileMax; i++)
    {
        TrackingData::AccelerometerProfile accelerometer = profile.accelerometer[i];
        if (i == accelerometer.sensorIndex)
        {
            profile.set(accelerometer, gConfiguration.accelerometer[i].enabled, gConfiguration.accelerometer[i].outputEnabled);
            gStatistics.configure(accelerometer, profile.accelerometer[i].enabled, profile.accelerometer[i].outputEnabled);

            /* Accelerometer currently doesn't influence 6dof frame rate */
            //if (profile.sixDof[i].interruptRate == SIXDOF_INTERRUPT_RATE_IMU)
            //{
            //    totalFrameRate[i] += accelerometer.fps;
            //}
        }
    }

    for (uint8_t i = 0; i < SixDofProfileMax; i++)
    {
        TrackingData::SixDofProfile sixDof = profile.sixDof[i];
        profile.set(sixDof, gConfiguration.sixdof[i].enabled);
        if (gConfiguration.sixdof[i].mode != SIXDOF_MODE_MAX)
        {
            profile.sixDof->mode = gConfiguration.sixdof[i].mode;
        }

        gStatistics.configure(sixDof, profile.sixDof[i].enabled, totalFrameRate[i]);
    }

    for (uint8_t i = 0; i < VelocimeterProfileMax; i++)
    {
        TrackingData::VelocimeterProfile velocimeter = profile.velocimeter[i];
        if (i == velocimeter.sensorIndex)
        {
            profile.set(velocimeter, gConfiguration.velocimeter[i].enabled, gConfiguration.velocimeter[i].outputEnabled);
            gStatistics.configure(velocimeter, profile.velocimeter[i].enabled, profile.velocimeter[i].outputEnabled);
        }
    }

    profile.playbackEnabled = false;

    printSupportedProfiles(profile);
}


/* Reading calibration info from EEPROM for accelerometer magnitude calculation */
void getCalibrationInfo(CalibrationInfo* calibrationInfo)
{
    uint16_t tmp;

    gDevice->EepromRead(0x1E0, sizeof(calibrationInfo->accelerometerScale), (uint8_t*)calibrationInfo->accelerometerScale, tmp);
    gDevice->EepromRead(0x204, sizeof(calibrationInfo->accelerometerBiasX), (uint8_t*)&calibrationInfo->accelerometerBiasX, tmp);
    gDevice->EepromRead(0x208, sizeof(calibrationInfo->accelerometerBiasY), (uint8_t*)&calibrationInfo->accelerometerBiasY, tmp);
    gDevice->EepromRead(0x20C, sizeof(calibrationInfo->accelerometerBiasZ), (uint8_t*)&calibrationInfo->accelerometerBiasZ, tmp);
}

int main(int argc, char *argv[])
{
    TrackingData::Temperature temperature;
    CommonListener listener;

    Status status = Status::SUCCESS;
    uint32_t loop = 0;
    std::thread hostLogThread;
    std::thread fwLogThread;
    std::thread imageThread;

    gStatistics.reset();

    int rc = parseArguments(argc, argv);
    if (rc != 0)
    {
        return rc;
    }

    if (gConfiguration.videoFile == true)
    {
        imageThread = std::thread(&imageThreadFunction);
    }

    gManagerInstance = std::shared_ptr<TrackingManager>(TrackingManager::CreateInstance(&listener, (void*)gConfiguration.inputFilename.c_str()));
    if (gManagerInstance == nullptr)
    {
        LOGE("Failed to create TrackingManager");
        return -1;
    }

    auto handleThread = std::thread(&handleThreadFunction);

    if (gConfiguration.logConfiguration[LogSourceHost].setLog == true)
    {
        if (gConfiguration.logConfiguration[LogSourceHost].logOutputMode == LogOutputModeBuffer)
        {
            hostLogThread = std::thread(&hostLogThreadFunction);
        }

        TrackingData::LogControl logControl(gConfiguration.logConfiguration[LogSourceHost].logVerbosity,
                                            gConfiguration.logConfiguration[LogSourceHost].logOutputMode,
                                            gConfiguration.logConfiguration[LogSourceHost].logRolloverMode);

        gManagerInstance.get()->setHostLogControl(logControl);
    }
    else //default logger settings
    {
        TrackingData::LogControl logControl(LogVerbosityLevel::Debug,
            LogOutputMode::LogOutputModeScreen,
            true);

        gManagerInstance.get()->setHostLogControl(logControl);
    }

    string lookDeviceString("Looking for device...");
    uint32_t deviceWaitSec = WAIT_FOR_DEVICE_SEC * 3;
    while (!gDevice)
    {
        printf("%s\n", lookDeviceString.c_str());
        std::this_thread::sleep_for(std::chrono::seconds(deviceWaitSec));
        lookDeviceString = "Looking for device (Device is not connected / Locked by another instance / Processing FW upgrade)...";
        deviceWaitSec = WAIT_FOR_DEVICE_SEC;
    }

    if (gConfiguration.jtag == true)
    {
        gStatistics.reset();
        LOGD("Please connect JTAG...");
        while (gDevice != 0)
        {
            LOGD("Waiting for JTAG connection...");
            std::this_thread::sleep_for(std::chrono::seconds(WAIT_FOR_DEVICE_SEC));
        }

        while (gDevice == 0)
        {
            LOGD("Looking for device (after JTAG)...");
            std::this_thread::sleep_for(std::chrono::seconds(WAIT_FOR_DEVICE_SEC));
        }
    }

    if (gConfiguration.resetLoop > 0)
    {
        do
        {
            LOGD("Reset loop %d/%d", resetCount + 1, gConfiguration.resetLoop);

            gStatistics.reset();
            status = gDevice->Reset();
            if (status != Status::SUCCESS)
            {
                LOGE("Error: Reset device failed, status = %s (0x%X)", statusToString(status).c_str(), status);
                goto cleanup;
            }

            LOGD("Sleeping (Reset) for %d seconds...", RESET_DEVICE_TIME_SEC);
            std::this_thread::sleep_for(std::chrono::seconds(RESET_DEVICE_TIME_SEC));

            while (gDevice == 0)
            {
                LOGD("Looking for device (after reset) ...");
                std::this_thread::sleep_for(std::chrono::seconds(WAIT_FOR_DEVICE_SEC));
            }
            resetCount++;
        } while ((gConfiguration.maxLoop == 0) && (resetCount < gConfiguration.resetLoop));
    }

    if (gConfiguration.logConfiguration[LogSourceFW].setLog == true)
    {
        fwLogThread = std::thread(&fwLogThreadFunction);
        TrackingData::LogControl logControl(gConfiguration.logConfiguration[LogSourceFW].logVerbosity, gConfiguration.logConfiguration[LogSourceFW].logOutputMode, gConfiguration.logConfiguration[LogSourceFW].logRolloverMode);
        status = gDevice->SetFWLogControl(logControl);
        if (status != Status::SUCCESS)
        {
            LOGE("Failed setting FW log, status = %s (0x%X)", statusToString(status).c_str(), status);
            goto cleanup;
        }
    }

    if (gStatistics.error.count > 0)
    {
        LOGE("Got %d error events, exiting", gStatistics.error.count);
        goto cleanup;
    }

    if (gConfiguration.calibration.filename.empty() == false)
    {
        std::ifstream calibrationFile(gConfiguration.calibration.filename);
        std::string str((std::istreambuf_iterator<char>(calibrationFile)), std::istreambuf_iterator<char>());
        char* s_ptr = &str[0];

        TrackingData::CalibrationData calibrationData;
        calibrationData.length = (uint32_t)str.length();
        calibrationData.buffer = (uint8_t*)s_ptr;
        calibrationData.type = gConfiguration.calibration.type;

        status = gDevice->SetCalibration(calibrationData);
        if (status != Status::SUCCESS)
        {
            LOGE("Failed setting calibration, status = %s (0x%X)", statusToString(status).c_str(), status);
            goto cleanup;
        }
    }

    if (gConfiguration.lowPowerEnabled == false)
    {
        status = gDevice->SetLowPowerMode(gConfiguration.lowPowerEnabled);
        if (status != Status::SUCCESS)
        {
            LOGE("Failed to set power mode, status = %s (0x%X)", statusToString(status).c_str(), status);
            goto cleanup;
        }
    }

    if (gConfiguration.temperature.check == true)
    {
        TrackingData::Temperature setTemperature;

        for (uint32_t i = TemperatureSensorVPU; i < TemperatureSensorMax; i++)
        {
            if (gConfiguration.temperature.threshold[i].set == true)
            {
                setTemperature.sensor[setTemperature.numOfSensors].index = (TemperatureSensorType)(i);
                setTemperature.sensor[setTemperature.numOfSensors].threshold = gConfiguration.temperature.threshold[i].temperature;
                setTemperature.numOfSensors++;
            }
        }

        if (setTemperature.numOfSensors > 0)
        {
            status = gDevice->SetTemperatureThreshold(setTemperature, TEMPERATURE_THRESHOLD_OVERRIDE_KEY);
            if (status != Status::SUCCESS)
            {
                LOGE("Failed setting temperature, status = %s (0x%X)", statusToString(status).c_str(), status);
                goto cleanup;
            }
        }

        status = gDevice->GetTemperature(temperature);
        if (status != Status::SUCCESS)
        {
            LOGE("Failed getting temperature, status = %s (0x%X)", statusToString(status).c_str(), status);
            goto cleanup;
        }
    }

    getCalibrationInfo(&calibrationInfo);

    if (gConfiguration.geoLocationEnabled == true)
    {
        TrackingData::GeoLocalization geoLocation(gConfiguration.geoLocation.latitude, gConfiguration.geoLocation.longitude, gConfiguration.geoLocation.altitude);
        status = gDevice->SetGeoLocation(geoLocation);
        if (status != Status::SUCCESS)
        {
            LOGE("Failed to set Geo localization, status = %s (0x%X)", statusToString(status).c_str(), status);
            goto cleanup;
        }
    }

    /* Set localization */
    if (gConfiguration.localization[LocalizationTypeSet].enabled == true)
    {
        std::fstream & mapFile = gConfiguration.localization[LocalizationTypeSet].file;
        string filename = gConfiguration.localization[LocalizationTypeSet].filename;

        mapFile.open(filename, std::fstream::in | std::fstream::binary);
        if (mapFile.is_open())
        {
            /* Get the length of the map file */
            mapFile.seekg(0, mapFile.end);
            uint32_t length = (uint32_t)mapFile.tellg();
            mapFile.seekg(0, mapFile.beg);

            if ((length > MAX_LOCALIZATION_MAP_SIZE) || (length <= 0))
            {
                LOGE("Error: Bad localization file size (%u bytes) (supported 0-%u bytes)", length, MAX_LOCALIZATION_MAP_SIZE);
                goto cleanup;
            }

            /* Allocate buffer to contain the file */
            uint8_t* buffer = new uint8_t[length];
            if (buffer == NULL)
            {
                LOGE("Error allocating map buffer of size %d", length);
                goto cleanup;
            }

            LOGD("Loading map file %s (%d bytes)", filename.c_str(), length);

            /* Read file data as a block to the buffer */
            mapFile.read((char*)buffer, length);
            if (!mapFile)
            {
                LOGE("Error loading map file %s (%d bytes) to buffer", filename.c_str(), length);
                delete[] buffer;
                goto cleanup;
            }

            status = gDevice->SetLocalizationData(&listener, length, buffer);
            if (status != Status::SUCCESS)
            {
                LOGE("Failed to set localization map, status = %s (0x%X)", statusToString(status).c_str(), status);
                mapFile.close();
                delete[] buffer;
                goto cleanup;
            }

            mapFile.close();
            delete[] buffer;

            while (gStatistics.localization[LocalizationTypeSet].isCompleted == false)
            {
                LOGD("Pending for Set localization complete");
                std::this_thread::sleep_for(std::chrono::seconds(WAIT_FOR_LOCALIZATION_SEC));
            }

            LOGD("Finished loading map file %s (%d bytes)", filename.c_str(), length);
        }
        else
        {
            LOGE("Error: File %s doesn't exists", filename.c_str());
            goto cleanup;
        }
    }

    if (gConfiguration.localization[LocalizationTypeReset].enabled == true)
    {
        LOGD("Reseting localization map");
        status = gDevice->ResetLocalizationData(0);
        if (status != Status::SUCCESS)
        {
            LOGE("Failed to reset localization map, status = %s (0x%X)", statusToString(status).c_str(), status);
            goto cleanup;
        }
    }

    if (gConfiguration.gpioEnabled == true)
    {
        LOGD("Setting GPIO control bitMask 0x%X", gConfiguration.gpioControlBitMask);
        status = gDevice->SetGpioControl(gConfiguration.gpioControlBitMask);
        if (status != Status::SUCCESS)
        {
            LOGE("Failed to set GPIO control, status = %s (0x%X)", statusToString(status).c_str(), status);
            goto cleanup;
        }
    }

    if (gConfiguration.setExposure == true)
    {
        TrackingData::ExposureModeControl modeControl(0, 1);
        status = gDevice->SetExposureModeControl(modeControl);
        if (status != Status::SUCCESS)
        {
            LOGE("Failed to set manual exposure, status = %s (0x%X)", statusToString(status).c_str(), status);
            goto cleanup;
        }
    }

    while (loop < gConfiguration.maxLoop)
    {
        gStatistics.reset();

        LOGD(" ");
        LOGD("Start/Stop Stream loop %d/%d", loop + 1, gConfiguration.maxLoop);

        LOGD("Starting Stream for %d seconds with profile:", gConfiguration.startStreamTime);
        TrackingData::Profile profile;
        configureProfile(profile);

        if (gConfiguration.mode == "live")
        {
            LOGI("Starting live mode");
            status = gDevice->Start(&listener, &profile);
        }
        if (status != Status::SUCCESS)
        {
            LOGE("Error: Start device failed, status = %s (0x%X)", statusToString(status).c_str(), status);
            goto cleanup;
        }

        gStatistics.runTime[HMD].start();

        if (gConfiguration.controllersCount != 0)
        {
            uint32_t timeout = 0;
            uint8_t connected = 0;
            uint8_t calibrated = 0;
            uint8_t needToCalibrate = 0;

            LOGD("Waiting for %d controllers to get connected", gConfiguration.controllersCount);
            while ((connected < gConfiguration.controllersCount) && (timeout < MAX_WAIT_FOR_CONTROLLERS_MSEC))
            {
                if (gStatistics.controller[Controller1].FWProgress % 100 == 0 && timeout % (WAIT_FOR_CONTROLLERS_MSEC * 10) == 0)
                {
                    LOGD("Currently connected %d/%d controllers... (giving up in %d sec)", connected, gConfiguration.controllersCount, (MAX_WAIT_FOR_CONTROLLERS_MSEC - timeout) / 1000);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_FOR_CONTROLLERS_MSEC));
                if (gStatistics.controller[Controller1].FWProgress % 100 != 0)
                {
                    continue;
                }

                timeout += WAIT_FOR_CONTROLLERS_MSEC;
                connected = 0;
                for (uint8_t i = Controller1; i < ProfileTypeMax; i++)
                {
                    if (gStatistics.controller[i].isConnected == true)
                    {
                        connected++;
                    }
                }
            }
            LOGD("Currently connected %d/%d controllers...", connected, gConfiguration.controllersCount);

            if (timeout >= MAX_WAIT_FOR_CONTROLLERS_MSEC)
            {
                LOGE("Failed to connect to all controllers");
                LOGD("Stopping Stream");
                status = gDevice->Stop();
                if (status != Status::SUCCESS)
                {
                    LOGE("Error: Stop device failed, status = %s (0x%X)", statusToString(status).c_str(), status);
                }

                gStatistics.print();
                saveOutput();
                goto cleanup;
            }

            /* All controllers are connected now  - checking for controller calibration process */
            timeout = 0;
            for (uint8_t i = Controller1; i < ProfileTypeMax; i++)
            {
                if (gConfiguration.controllers[i].calibrate == true)
                {
                    needToCalibrate++;
                }
            }

            LOGD("Waiting for %d controllers to get calibrated", needToCalibrate);

            while ((calibrated < needToCalibrate) && (timeout < MAX_WAIT_FOR_CALIBRATED_CONTROLLERS_MSEC))
            {
                if (timeout % (WAIT_FOR_CONTROLLERS_MSEC * 10) == 0)
                {
                    LOGD("Currently calibrated %d/%d controllers... (giving up in %d sec)", calibrated, needToCalibrate, (MAX_WAIT_FOR_CALIBRATED_CONTROLLERS_MSEC - timeout) / 1000);
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_FOR_CONTROLLERS_MSEC));
                timeout += WAIT_FOR_CONTROLLERS_MSEC;
                calibrated = 0;
                for (uint8_t i = Controller1; i < ProfileTypeMax; i++)
                {
                    if (gStatistics.controller[i].isCalibrated == true)
                    {
                        calibrated++;
                    }
                }
            }
            LOGD("Currently calibrated %d/%d controllers...", calibrated, needToCalibrate);

            /* Sending Controller data to connected controllers */
            if (gConfiguration.controllerDataFilename.empty() == false)
            {
                std::ifstream data(gConfiguration.controllerDataFilename);
                std::string line;

                while (std::getline(data, line))
                {
                    std::stringstream lineStream(line);
                    if (lineStream.rdbuf()->in_avail())
                    {
                        std::string cell;

                        uint8_t buffer[80] = { 0 };
                        uint32_t i = 0;
                        while (std::getline(lineStream, cell, ','))
                        {
                            if ((strncmp("0x", cell.c_str(), 2) == 0) || (strncmp("0X", cell.c_str(), 2) == 0))
                            {
                                buffer[i] = (uint8_t)strtol(cell.c_str(), NULL, 16);
                            }
                            else
                            {
                                buffer[i] = (uint8_t)atoi(cell.c_str());
                            }
                            i++;
                        }

                        TrackingData::ControllerData controllerData;
                        controllerData.set(buffer[0], buffer[1], buffer[2], &buffer[3]);
                        status = gDevice->ControllerSendData(controllerData);
                        if (status != Status::SUCCESS)
                        {
                            LOGE("Failed sending controller data, status = %s (0x%X)", statusToString(status).c_str(), status);
                            goto cleanup;
                        }
                    }
                }
            }
        }

        if (gConfiguration.rssiCount > 0)
        {
            for (uint8_t i = Controller1; i < ProfileTypeMax; i++)
            {
                if (gConfiguration.rssi[i].time != 0)
                {
                    LOGD("Start RSSI test on controller %d", i);
                    gDevice->ControllerRssiTestControl(i, true);

                    LOGD("Sleeping (Start RSSI) for %d seconds...", gConfiguration.rssi[i].time);
                    std::this_thread::sleep_for(std::chrono::seconds(gConfiguration.rssi[i].time));

                    LOGD("Stop RSSI test on controller %d", i);
                    gDevice->ControllerRssiTestControl(i, false);

                    LOGD("Sleeping (Stop RSSI) for %d seconds...", gConfiguration.stopStreamTime);
                    std::this_thread::sleep_for(std::chrono::seconds(gConfiguration.stopStreamTime));
                }
            }
        }

        if (gConfiguration.setExposure == true)
        {
            status = gDevice->SetExposure(gConfiguration.exposure);
            if (status != Status::SUCCESS)
            {
                LOGE("Failed to set exposure with error, status = %s (0x%X)", statusToString(status).c_str(), status);
                goto cleanup;
            }
        }

        if (gConfiguration.nodeFilename.empty() == false)
        {
            LOGD("Waiting for 6dof tracker confidence = 3 before setting static nodes");

            while (gStatistics.pose[SixDofProfile0].trackerConfidence != 3)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(MAX_WAIT_FOR_STATIC_NODE_MSEC));
            }

            LOGD("Setting all static nodes");

            std::ifstream data(gConfiguration.nodeFilename);
            std::string line;

            while (std::getline(data, line))
            {
                std::stringstream lineStream(line);
                if (lineStream.rdbuf()->in_avail())
                {
                    std::string cell;

                    float_t buffer[7] = { 0 };
                    char guid[128] = "";

                    uint32_t i = 0;
                    while (std::getline(lineStream, cell, ','))
                    {
                        if (i == 0)
                        {
                            snprintf((char*)guid, cell.length()+1, "%s", cell.c_str());
                        }
                        else
                        {
                            buffer[i-1] = stof(cell.c_str());
                        }

                        i++;
                    }

                    TrackingData::RelativePose relativePose;
                    relativePose.translation.set(buffer[0], buffer[1], buffer[2]);
                    relativePose.rotation.set(buffer[3], buffer[4], buffer[5], buffer[6]);
                    status = gDevice->SetStaticNode(guid, relativePose);
                    if (status != Status::SUCCESS)
                    {
                        LOGE("Failed sending controller data, status = %s (0x%X)", statusToString(status).c_str(), status);
                        goto cleanup;
                    }
                }
            }
        }

        if (gConfiguration.velocimeterFilename.empty() == false)
        {
            std::ifstream data(gConfiguration.velocimeterFilename);
            std::string line;

            while (std::getline(data, line))
            {
                std::stringstream lineStream(line);
                if (lineStream.rdbuf()->in_avail())
                {
                    std::string cell;
                    uint32_t frameId = 0;
                    float_t translationalVelocity[3] = { 0 };
                    int64_t timestamp[2] = { 0 };

                    uint32_t i = 0;
                    while (std::getline(lineStream, cell, ','))
                    {
                        if (i == 0)
                        {
                            frameId = stoi(cell.c_str());
                        }
                        else if ((i == 1) || (i == 2) || (i == 3))
                        {
                            translationalVelocity[i - 1] = stof(cell.c_str());
                        }
                        else
                        {
                            timestamp[i-4] = stol(cell.c_str());
                        }

                        i++;
                    }

                    TrackingData::VelocimeterFrame frame;
                    frame.frameId = frameId;
                    frame.translationalVelocity.x = translationalVelocity[0];
                    frame.translationalVelocity.y = translationalVelocity[1];
                    frame.translationalVelocity.z = translationalVelocity[2];
                    frame.timestamp = timestamp[0];
                    frame.arrivalTimeStamp = timestamp[1];

                    LOGD("Sending velocimeter frame[%d]: TranslationalVelocity[%f, %f, %f], Timestamp %" PRId64 ", ArrivalTimestamp %" PRId64 "", frame.frameId, frame.translationalVelocity.x, frame.translationalVelocity.y, frame.translationalVelocity.z, frame.timestamp, frame.arrivalTimeStamp);

                    status = gDevice->SendFrame(frame);
                    if (status != Status::SUCCESS)
                    {
                        LOGE("Failed sending velocimeter frame, status = %s (0x%X)", statusToString(status).c_str(), status);
                        goto cleanup;
                    }
                }
            }
        }

        LOGD("Sleeping (Start) for %d seconds...", gConfiguration.startStreamTime);
        std::this_thread::sleep_for(std::chrono::seconds(gConfiguration.startStreamTime));

        if (gConfiguration.temperature.check == true)
        {
            status = gDevice->GetTemperature(temperature);
            if (status != Status::SUCCESS)
            {
                LOGE("Failed getting temperature, status = %s (0x%X)", statusToString(status).c_str(), status);
                goto cleanup;
            }
        }

        if (gConfiguration.controllersCount != 0)
        {
            for (uint8_t i = Controller1; i < ProfileTypeMax; i++)
            {
                if (gStatistics.controller[i].isConnected == true)
                {
                    LOGD("Disconnecting Controller %d", i);
                    status = gDevice->ControllerDisconnect(i);
                    if (status != Status::SUCCESS)
                    {
                        LOGE("Error: Failed to disconnect from controller %d, status = %s (0x%X)", i, statusToString(status).c_str(), status);
                    }
                    else
                    {
                        LOGD("Sent Controller Disconnected Event on ControllerID = %d", i);
                        gStatistics.controller[i].isConnected = false;
                        gStatistics.runTime[i].stop();
                    }
                }
            }
        }

        gStatistics.runTime[HMD].stop();

        if (gDevice == NULL)
        {
            LOGE("Error: device disappeared");
            gStatistics.print();
            saveOutput();
            goto cleanup;
        }

        if (gConfiguration.nodeFilename.empty() == false)
        {
            LOGD("Getting all static nodes");

            std::ifstream data(gConfiguration.nodeFilename);
            std::string line;

            while (std::getline(data, line))
            {
                std::stringstream lineStream(line);
                if (lineStream.rdbuf()->in_avail())
                {
                    std::string cell;

                    float_t buffer[7] = { 0 };
                    char guid[128] = "";

                    uint32_t i = 0;
                    while (std::getline(lineStream, cell, ','))
                    {
                        if (i == 0)
                        {
                            snprintf((char*)guid, cell.length()+1, "%s", cell.c_str());
                        }
                        else
                        {
                            buffer[i-1] = stof(cell.c_str());
                        }

                        i++;
                    }

                    TrackingData::RelativePose relativePose;
                    status = gDevice->GetStaticNode(guid, relativePose);
                    if (status != Status::SUCCESS)
                    {
                        LOGE("Failed sending controller data, status = %s (0x%X)", statusToString(status).c_str(), status);
                        goto cleanup;
                    }
                }
            }
        }

        LOGD("Stopping Stream");
        status = gDevice->Stop();
        if (status != Status::SUCCESS)
        {
            LOGE("Error: Stop device failed, status = %s (0x%X)", statusToString(status).c_str(), status);
            gStatistics.print();
            saveOutput();
            goto cleanup;
        }

        LOGD("Sleeping (Stop) for %d seconds...", gConfiguration.stopStreamTime);
        std::this_thread::sleep_for(std::chrono::seconds(gConfiguration.stopStreamTime));

        if (gConfiguration.temperature.check == true)
        {
            status = gDevice->GetTemperature(temperature);
            if (status != Status::SUCCESS)
            {
                LOGE("Failed getting temperature, status = %s (0x%X)", statusToString(status).c_str(), status);
                goto cleanup;
            }
        }

        gStatistics.print();
        saveOutput();

        if (gConfiguration.errorCheck == true)
        {
            if (gStatistics.check() == false)
            {
                LOGD("Statistics checker passed");
            }
            else
            {
                LOGE("Statistics checker failed");
                if (gConfiguration.errorExit == true)
                {
                    LOGE("Exiting...");
                    exit(EXIT_FAILURE);
                }
            }
        }

        do 
        {
            if (resetCount < gConfiguration.resetLoop)
            {
                LOGD("Reset loop %d/%d", resetCount + 1, gConfiguration.resetLoop);

                gStatistics.reset();
                status = gDevice->Reset();
                if (status != Status::SUCCESS)
                {
                    LOGE("Error: Reset device failed, status = %s (0x%X)", statusToString(status).c_str(), status);
                    goto cleanup;
                }

                LOGD("Sleeping (Reset) for %d seconds...", RESET_DEVICE_TIME_SEC);
                std::this_thread::sleep_for(std::chrono::seconds(RESET_DEVICE_TIME_SEC));

                while (gDevice == 0)
                {
                    LOGD("Looking for device (after reset) ...");
                    std::this_thread::sleep_for(std::chrono::seconds(WAIT_FOR_DEVICE_SEC));
                }
                resetCount++;

                if (gConfiguration.lowPowerEnabled == false)
                {
                    status = gDevice->SetLowPowerMode(gConfiguration.lowPowerEnabled);
                    if (status != Status::SUCCESS)
                    {
                        LOGE("Failed to set power mode, status = %s (0x%X)", statusToString(status).c_str(), status);
                        goto cleanup;
                    }
                }
            }
        } while (((loop + 1) == gConfiguration.maxLoop) && (resetCount < gConfiguration.resetLoop)); /* Check if finished with START/STOP loop, and there are more reset loops to run */

        loop++;
    }

    /* Get localization */
    if (gConfiguration.localization[LocalizationTypeGet].enabled == true)
    {
        std::fstream & mapFile = gConfiguration.localization[LocalizationTypeGet].file;
        string filename = gConfiguration.localization[LocalizationTypeGet].filename;
        uint32_t getLocalizationTime = 0;

        mapFile.open(filename, std::fstream::out | std::fstream::binary | std::fstream::trunc);
        if (!mapFile.is_open())
        {
            LOGE("Failed creating file %s", filename.c_str());
            goto cleanup;
        }

        LOGD("Getting localization map into file %s", filename.c_str());

        status = gDevice->GetLocalizationData(&listener);
        if (status != Status::SUCCESS)
        {
            LOGE("Failed to get localization map, status = %s (0x%X)", statusToString(status).c_str(), status);
            mapFile.close();
            goto cleanup;
        }

        while ((gStatistics.localization[LocalizationTypeGet].isCompleted == false) && (getLocalizationTime <= MAX_WAIT_FOR_LOCALIZATION_SEC))
        {
            LOGD("Pending for Get localization complete... (giving up in %d sec)", (MAX_WAIT_FOR_LOCALIZATION_SEC - getLocalizationTime));
            std::this_thread::sleep_for(std::chrono::seconds(WAIT_FOR_LOCALIZATION_SEC));
            getLocalizationTime += WAIT_FOR_LOCALIZATION_SEC;
        }

        if (gStatistics.localization[LocalizationTypeGet].isCompleted == true)
        {
            LOGD("Finished getting map file %s (%d bytes)", filename.c_str(), gStatistics.localization[LocalizationTypeGet].fileSize);
        }
        else
        {
            LOGE("Failed getting map file %s (%d bytes)", filename.c_str(), gStatistics.localization[LocalizationTypeGet].fileSize);
        }

        mapFile.close();
    }

cleanup:

    gIsDisposed = true;
    handleThread.join();

    if (gConfiguration.videoFile == true)
    {
        imageThread.join();
    }

    if (gConfiguration.logConfiguration[LogSourceFW].setLog == true)
    {
        fwLogThread.join();
    }

    if ((gConfiguration.logConfiguration[LogSourceHost].setLog == true) && (gConfiguration.logConfiguration[LogSourceHost].logOutputMode == LogOutputModeBuffer))
    {
        hostLogThread.join();
    }
}

