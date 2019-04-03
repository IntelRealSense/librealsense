// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include "TrackingCommon.h"
#include <cstring>
#include <math.h>

namespace perc
{
    #define SENSOR_TYPE_POS (0)
    #define SENSOR_TYPE_MASK (0x001F << SENSOR_TYPE_POS) /**< Bits 0-4 */
    #define SENSOR_INDEX_POS (5)
    #define SENSOR_INDEX_MASK (0x0007 << SENSOR_INDEX_POS) /**< Bits 5-7 */
    #define SET_SENSOR_ID(_type, _index) (((_type << SENSOR_TYPE_POS) & SENSOR_TYPE_MASK) | ((_index << SENSOR_INDEX_POS) & SENSOR_INDEX_MASK))
    #define GET_SENSOR_INDEX(_sensorID) ((_sensorID & SENSOR_INDEX_MASK) >> SENSOR_INDEX_POS)
    #define GET_SENSOR_TYPE(_sensorID) ((_sensorID & SENSOR_TYPE_MASK) >> SENSOR_TYPE_POS)
    #define MAX_VIDEO_STREAMS 8
    #define MAX_SUPPORTED_STREAMS 32
    #define MAX_USB_TREE_DEPTH 64
    #define MAX_LOCALIZATION_DATA_CHUNK_SIZE 1024
    #define MAX_CONFIGURATION_SIZE 1016
    #define MAX_VERSION 0xFFFF
    #define MAX_LOG_BUFFER_ENTRIES 1024
    #define MAX_LOG_BUFFER_ENTRY_SIZE (256)
    #define MAX_LOG_BUFFER_MODULE_SIZE (32)
    #define MAX_CALIBRATION_SIZE 10000
    
    enum ProfileType
    {
        HMD = 0,        
        Controller1 = 1,
        Controller2 = 2,
        ProfileTypeMax = 3,
    };

    enum VideoProfileType
    {
        VideoProfile0 = 0,  /* Sensor index 0 - HMD High exposure left camera                           */
        VideoProfile1 = 1,  /* Sensor index 1 - HMD High exposure right camera                          */
        VideoProfile2 = 2,  /* Sensor index 2 - HMD Low exposure left camera for controller 1 tracking  */
        VideoProfile3 = 3,  /* Sensor index 3 - HMD Low exposure right camera for controller 2 tracking */
        VideoProfileMax = 4,
    };

    enum AccelerometerProfileType
    {
        AccelerometerProfile0 = 0,  /* Sensor index 0 - HMD Accelerometer          */
        AccelerometerProfile1 = 1,  /* Sensor index 1 - Controller 1 Accelerometer */
        AccelerometerProfile2 = 2,  /* Sensor index 2 - Controller 2 Accelerometer */
        AccelerometerProfileMax = 3,
    };

    enum GyroProfileType
    {
        GyroProfile0 = 0,   /* Sensor index 0 - HMD Gyro          */
        GyroProfile1 = 1,   /* Sensor index 1 - Controller 1 Gyro */
        GyroProfile2 = 2,   /* Sensor index 2 - Controller 2 Gyro */
        GyroProfileMax = 3,
    };

    enum VelocimeterProfileType
    {
        VelocimeterProfile0 = 0,   /* External Sensor index 0 - HMD Velocimeter */
        VelocimeterProfile1 = 1,   /* External Sensor index 1 - HMD Velocimeter */
        VelocimeterProfileMax = 2,
    };

    enum SixDofProfileType
    {
        SixDofProfile0 = 0,   /* Source index 0 - HMD 6dof          */
        SixDofProfile1 = 1,   /* Source index 1 - Controller 1 6dof */
        SixDofProfile2 = 2,   /* Source index 2 - Controller 2 6dof */
        SixDofProfileMax = 3,
    };

    enum LogVerbosityLevel {
        None = 0x0000,    /**< Logging is disabled                                                                                                              */
        Error = 0x0001,   /**< Error conditions                                                                                                                 */
        Info = 0x0002,    /**< High importance informational entries. this verbosity level exists to allow logging of general information on production images. */
        Warning = 0x0003, /**< Warning messages                                                                                                                 */
        Debug = 0x0004,   /**< Debug informational entries                                                                                                      */
        Verbose = 0x0005, /**< Even more informational entries                                                                                                  */
        Trace = 0x0006    /**< On entry and exit from every function call. also shall be used for profiling                                                     */
    };

    enum LogOutputMode {
        LogOutputModeScreen = 0x0000, /**< All logs print to screen output */
        LogOutputModeBuffer = 0x0001, /**< All logs saved to buffer        */
        LogOutputModeMax = 0x0002,
    };

    enum TemperatureSensorType {
        TemperatureSensorVPU = 0x0000, /**< Temperature Sensor located in the Vision Processing Unit    */
        TemperatureSensorIMU = 0x0001, /**< Temperature Sensor located in the Inertial Measurement Unit */
        TemperatureSensorBLE = 0x0002, /**< Temperature Sensor located in the Bluetooth Low Energy Unit */
        TemperatureSensorMax = 0x0003,
    };

    enum LockType {
        HardwareLock = 0x0000, /**< The locking is done in hardware by locking the upper quarter of the EEPROM memory                         */
        SoftwareLock = 0x0001, /**< The locking is done in software by the firmware managing a lock bits in each configuration table metadata */
        MaxLockType = 0x0002,
    };

    enum EepromLockState {
        LockStateWriteable = 0x0000,       /**< The EEPROM is fully writeable                                         */
        LockStateLocked = 0x0001,          /**< The upper quarter of the EEPROM memory is write-protected             */
        LockStatePermanentLocked = 0x0002, /**< The upper quarter of the EEPROM memory is permanently write-protected */
        LockStateMax = 0x0003,
    };

    enum AddressType {
        AddressTypePublic = 0x0000, /**< Public Address - Static IEEE assigned Mac Address build from manufacturer assigned ID (24 bits) and unique device ID (24 bits) */
        AddressTypeRandom = 0x0001, /**< Random Address - Randomized temporary Address used for a certain amount of time (48 bits)                                      */
        AddressTypeMax = 0x0002,    /**< Unspecified Address */
    };

    enum CalibrationType {
        CalibrationTypeNew = 0x0000,    /**< New calibration overrides previous calibration */
        CalibrationTypeAppend = 0x0001, /**< Append calibration to previous calibration     */
        CalibrationTypeMax = 0x0002,    /**< Unspecified calibration type                   */
    };

    class TrackingData
    {
    public:
        class Axis {
        public:
            Axis(float _x = 0.0, float _y = 0.0, float _z = 0.0) : x(_x), y(_y), z(_z) {}
            void set(float _x, float _y, float _z)
            {
                x = _x;
                y = _y;
                z = _z;
            }
            float x; /**< X value, used for translation/acceleration/velocity */
            float y; /**< Y value, used for translation/acceleration/velocity */
            float z; /**< Z value, used for translation/acceleration/velocity */
        };

        class EulerAngles {
        public:
            EulerAngles(float _x = 0.0, float _y = 0.0, float _z = 0.0) : x(_x), y(_y), z(_z) {}
            void set(float _x, float _y, float _z)
            {
                x = _x;
                y = _y;
                z = _z;
            }
            float x; /**< X value, used for angular velocity/angular acceleration */
            float y; /**< Y value, used for angular velocity/angular acceleration */
            float z; /**< Z value, used for angular velocity/angular acceleration */
        };

        class Quaternion {
        public:
            Quaternion(float _i = 0.0, float _j = 0.0, float _k = 0.0, float _r = 0.0) : i(_i), j(_j), k(_k), r(_r) {}
            void set(float _i, float _j, float _k, float _r)
            {
                i = _i;
                j = _j;
                k = _k;
                r = _r;
            }
            float i; /**< Qi components of rotation as represented in quaternion rotation */
            float j; /**< Qj components of rotation as represented in quaternion rotation */
            float k; /**< Qk components of rotation as represented in quaternion rotation */
            float r; /**< Qr components of rotation as represented in quaternion rotation */
        };

        class Version {
        public:
            Version(uint32_t _major = MAX_VERSION, uint32_t _minor = MAX_VERSION, uint32_t _patch = MAX_VERSION, uint32_t _build = MAX_VERSION) {
                major = _major;
                minor = _minor;
                patch = _patch;
                build = _build;
            }

            void set(uint32_t _major = MAX_VERSION, uint32_t _minor = MAX_VERSION, uint32_t _patch = MAX_VERSION, uint32_t _build = MAX_VERSION) {
                major = _major;
                minor = _minor;
                patch = _patch;
                build = _build;
            }

            uint32_t major; /**< Major part of version */
            uint32_t minor; /**< Minor part of version */
            uint32_t patch; /**< Patch part of version */
            uint32_t build; /**< Build part of version */
        };

        class TimestampedData
        {
        public:
            TimestampedData() : timestamp(0), arrivalTimeStamp(0), systemTimestamp(0) {};
            int64_t timestamp;        /**< Frame integration timestamp, as measured in nanoseconds since device initialization */
            int64_t arrivalTimeStamp; /**< Frame arrival timestamp, as measured in nanoseconds since device initialization     */
            int64_t systemTimestamp;  /**< Host correlated time stamp in nanoseconds                                           */
        };

        class PoseFrame : public TimestampedData {
        public:
            PoseFrame() : sourceIndex(0), trackerConfidence(0), mapperConfidence(0), trackerState(0) {};
            uint8_t sourceIndex;             /**< Index of HMD or controller - 0x0 = HMD, 0x1 - controller 1, 0x2 - controller 2                             */
            Axis translation;                /**< X, Y, Z values of translation, in meters (relative to initial position)                                    */
            Axis velocity;                   /**< X, Y, Z values of velocity, in meter/sec                                                                   */
            Axis acceleration;               /**< X, Y, Z values of acceleration, in meter/sec^2                                                             */
            Quaternion rotation;             /**< Qi, Qj, Qk, Qr components of rotation as represented in quaternion rotation (relative to initial position) */
            EulerAngles angularVelocity;     /**< X, Y, Z values of angular velocity, in radians/sec                                                         */
            EulerAngles angularAcceleration; /**< X, Y, Z values of angular acceleration, in radians/sec^2                                                   */
            uint32_t trackerConfidence;      /**< pose data confidence 0x0 - Failed, 0x1 - Low, 0x2 - Medium, 0x3 - High                                     */
            uint32_t mapperConfidence;       /**< pose data confidence 0x0 - Failed, 0x1 - Low, 0x2 - Medium, 0x3 - High                                     */
            uint32_t trackerState;           /**< tracker state 0x0 - Inactive, 0x3 - Running (3dof), 0x4 - Running (6dof), 0x7 - Inertial Only (3dof)       */
        };

        class GyroFrame : public TimestampedData {
        public:
            GyroFrame() : temperature(0), sensorIndex(0), frameId(0) {};
            uint8_t sensorIndex;  /**< Zero based index of sensor with the same type within device         */
            uint32_t frameId;     /**< A running index of frames from every unique sensor, starting from 0 */
            Axis angularVelocity; /**< X, Y, Z values of gyro, in radians/sec                              */
            float temperature;    /**< Gyro temperature                                                    */
        };

        class VelocimeterFrame : public TimestampedData {
        public:
            VelocimeterFrame() : temperature(0), sensorIndex(0), frameId(0) {};
            uint8_t sensorIndex;  /**< Zero based index of sensor with the same type within device         */
            uint32_t frameId;     /**< A running index of frames from every unique sensor, starting from 0 */
            Axis translationalVelocity; /**< X, Y, Z values of velocimeter, in meters/sec                       */
            float temperature;    /**< Velocimeter temperature                                             */
        };

        class AccelerometerFrame : public TimestampedData {
        public:
            AccelerometerFrame() : temperature(0), sensorIndex(0), frameId(0) {};
            uint8_t sensorIndex; /**< Zero based index of sensor with the same type within device         */
            uint32_t frameId;    /**< A running index of frames from every unique sensor, starting from 0 */
            Axis acceleration;   /**< X, Y, Z values of acceleration, in meter/sec^2                      */
            float temperature;   /**< Accelerometer temperature                                           */
        };

        class ControllerFrame : public TimestampedData {
        public:
            ControllerFrame() : sensorIndex(0), frameId(0), eventId(0), instanceId(0), sensorData{0} {};
            uint8_t sensorIndex;                             /**< Zero based index of sensor with the same type within device                     */
            uint32_t frameId;                                /**< A running index of frames from every unique sensor, starting from 0             */
            uint8_t eventId;                                 /**< Event ID - button, trackpad or battery (vendor specific), supported values 0-63 */
            uint8_t instanceId;                              /**< Instance of the sensor in case of multiple sensors                              */
            uint8_t sensorData[CONTROLLER_SENSOR_DATA_SIZE]; /**< Sensor data that is pass-through from the controller firmware                   */
        };

        class RssiFrame : public TimestampedData {
        public:
            RssiFrame() : sensorIndex(0), frameId(0), signalStrength(0) {};
            uint8_t sensorIndex;    /**< Zero based index of sensor with the same type within device                   */
            uint32_t frameId;       /**< A running index of frames from every unique sensor, starting from 0           */
            float_t signalStrength; /**< Sampled signal strength (dB), a value closer to 0 indicates a stronger signal */
        };

        class ControllerDiscoveryEventFrame : public TimestampedData {
        public:
            ControllerDiscoveryEventFrame() : macAddress{0}, addressType(AddressTypeRandom), manufacturerId(0), vendorData(0), protocol(), app(), softDevice(), bootLoader() {};
            uint8_t macAddress[MAC_ADDRESS_SIZE]; /**< Discovered device byte array of MAC address                                */
            AddressType addressType;              /**< Discovered device address type: Public or Random                           */
            uint16_t manufacturerId;              /**< Identifier of the controller manufacturer                                  */
            uint16_t vendorData;                  /**< vendor specific data copied from the controller advertisement              */
            Version protocol;                     /**< Controller BLE protocol version  - only major part is active               */
            Version app;                          /**< Controller App version - only major, minor, build parts are active         */
            Version softDevice;                   /**< Controller Soft Device version - only major part is active                 */
            Version bootLoader;                   /**< Controller Boot Loader version - only major, minor, build parts are active */
        };

        class ControllerConnectedEventFrame : public TimestampedData {
        public:
            ControllerConnectedEventFrame() : status(Status::SUCCESS), controllerId(0), manufacturerId(0), protocol(), app(), softDevice(), bootLoader() {};
            Status status;           /**< Connection status: SUCCESS - connection succeeded                                                              */
                                     /**<                    TIMEOUT - connection timed out                                                              */
                                     /**<                    INCOMPATIBLE - connection succeeded but controller version is incompatible with TM2 version */
            uint8_t controllerId;    /**< Disconnected controller identifier (1 or 2)                                                                    */
            uint16_t manufacturerId; /**< Identifier of the controller manufacturer                                                                      */
            Version protocol;        /**< Controller BLE protocol version  - only major part is active                                                   */
            Version app;             /**< Controller App version - only major, minor, build parts are active                                             */
            Version softDevice;      /**< Controller Soft Device version - only major part is active                                                     */
            Version bootLoader;      /**< Controller Boot Loader version - only major, minor, build parts are active                                     */
        };

        class ControllerDisconnectedEventFrame : public TimestampedData {
        public:
            ControllerDisconnectedEventFrame() : controllerId(0) {};
            uint8_t controllerId; /**< Disconnected controller identifier (1 or 2) */
        };

        class ControllerCalibrationEventFrame : public TimestampedData {
        public:
            ControllerCalibrationEventFrame() : controllerId(0), status(Status::SUCCESS) {};
            uint8_t controllerId; /**< Calibrated controller identifier (1 or 2)                                              */
            Status status;        /**< Calibration status: SUCCESS  - Controller was calibrated successfully                  */
                                  /**<                     CONTROLLER_CALIBRATION_VALIDATION_FAILURE - validation failure     */
                                  /**<                     CONTROLLER_CALIBRATION_FLASH_ACCESS_FAILURE - flash access failure */
                                  /**<                     CONTROLLER_CALIBRATION_IMU_FAILURE - IMU failure                   */
                                  /**<                     CONTROLLER_CALIBRATION_INTERNAL_FAILURE  - Internal failure        */
        };

        class ControllerFWEventFrame : public TimestampedData {
        public:
            ControllerFWEventFrame() : status(Status::SUCCESS), macAddress{0}, progress(0) {};
            Status status;                        /**< Progress status: SUCCESS  - update progresses successfully                                    */
                                                  /**<                  CRC_ERROR - error in image integrity                                         */
                                                  /**<                  AUTH_ERROR - error in image signature                                        */
                                                  /**<                  LIST_TOO_BIG - image size too big                                            */
                                                  /**<                  TIMEOUT  - operation did not complete due to device disconnect or timeout    */
            uint8_t macAddress[MAC_ADDRESS_SIZE]; /**< MAC address of the updated device. All zeros for central FW update                              */
            uint8_t progress;                     /**< Progress counter (percentage of update complete)                                                */
        };

        class StatusEventFrame : public TimestampedData {
        public:
            StatusEventFrame() : status(Status::SUCCESS) {};
            Status status; /**< Status code */
        };

        class ControllerLedEventFrame : public TimestampedData {
        public:
            uint8_t controllerId; /**< Controller identifier (1 or 2) */
            uint8_t ledId;        /**< Led identifier (1 or 2)        */
            uint32_t intensity;   /**< Led intensity [0-100]          */
        };

        class RawProfile
        {
        public:
            RawProfile() : width(0), height(0), stride(0), pixelFormat(PixelFormat::ANY) {}
            void set(uint16_t _width, uint16_t _height, uint16_t _stride, PixelFormat _format) {
                width = _width;
                height = _height;
                stride = _stride;
                pixelFormat = _format;
            }
            uint16_t stride;         /**< Length in bytes of each line in the image (including padding). 0 for non-camera streams. */
            uint16_t width;          /**< Supported width (in pixels) of first stream, 0 for non-camera streams                    */
            uint16_t height;         /**< Supported height (in pixels) or first stream, 0 for non-camera streams                   */
            PixelFormat pixelFormat; /**< Pixel format of the stream, according to enum PixelFormat                                */
        };

        class VideoFrame : public TimestampedData {
        public:
            uint8_t sensorIndex;   /**< Zero based index of sensor with the same type within device              */
            uint32_t frameId;      /**< Running index of frames from every unique sensor. Starting from 0.       */
            RawProfile profile;    /**< Frame format profile - includes width, height, stride, pixelFormat       */
            uint32_t exposuretime; /**< Exposure time of this frame in microseconds                              */
            float_t gain;          /**< Gain multiplier of this frame                                            */
            uint32_t frameLength;  /**< Length of frame below, in bytes, shall be equal to Stride X Height X BPP */
            const uint8_t* data;   /**< Frame data pointer                                                       */
        };

        class EnableConfig
        {
        public:
            EnableConfig() : enabled(false), outputEnabled(false), fps(0), sensorIndex(0) {};
            void set(bool _enabled, bool _outputEnabled, uint16_t _fps, uint8_t _sensorIndex) {
                enabled = _enabled;
                outputEnabled = _outputEnabled;
                fps = _fps;
                sensorIndex = _sensorIndex;
            }
            bool enabled;        /**< true if this profile is enabled                                                                                                     */
            bool outputEnabled;  /**< 0x0 - Send sensor outputs to the internal middlewares only, 0x1 - Send this sensor outputs also to the host over the USB interface. */
            uint16_t fps;        /**< Supported frame per second for this profile                                                                                         */
            uint8_t sensorIndex; /**< Zero based index of sensor with the same type within device                                                                         */
        };

        class GyroProfile : public EnableConfig
        {

        };

        class VelocimeterProfile : public EnableConfig
        {

        };

        class AccelerometerProfile : public EnableConfig
        {

        };

        class SixDofProfile
        {
        public:
            SixDofProfile() : enabled(false), mode(SIXDOF_MODE_ENABLE_MAPPING | SIXDOF_MODE_ENABLE_RELOCALIZATION), interruptRate(SIXDOF_INTERRUPT_RATE::SIXDOF_INTERRUPT_RATE_IMU), profileType(SixDofProfileMax) {};
            SixDofProfile(bool _enabled, uint8_t _mode, SIXDOF_INTERRUPT_RATE _interruptRate, SixDofProfileType _profileType) :
                enabled(_enabled), mode(_mode), interruptRate(_interruptRate), profileType(_profileType) {};
            void set(bool _enabled, uint8_t _mode, SIXDOF_INTERRUPT_RATE _interruptRate, SixDofProfileType _profileType)
            {
                enabled = _enabled;
                mode = _mode;
                interruptRate = _interruptRate;
                profileType = _profileType;
            }

            bool enabled;                        /**< true if this profile is enabled                                                              */
            uint8_t mode;                        /**< 0x00 - Normal Mode, 0x01 - Fast Playback, 0x02 - Enable Mapping                              */
            SixDofProfileType profileType;       /**< Type of this 6dof profile - HMD, Controller 1, Controller 2                                  */
            SIXDOF_INTERRUPT_RATE interruptRate; /**< Rate of 6DoF interrupts. The following values are supported:                                 */
                                                 /**< 0x0 - no interrupts                                                                          */
                                                 /**< 0x1 - interrupts on every fisheye camera update (30 interrupts per second for TM2)           */
                                                 /**< 0x2 - interrupts on every IMU update (400-500 interrupts per second for TM2) - default value */
        };

        class VideoProfile : public EnableConfig
        {
        public:
            RawProfile profile; /**< Raw video profile */
        };

        class Profile 
        {
        public:

            Profile() : playbackEnabled(false) {
                for (uint8_t i = 0; i < VideoProfileMax; i++)
                {
                    video[i].outputEnabled = false; 
                    video[i].enabled = false;
                    video[i].fps = 0;
                    video[i].sensorIndex = VideoProfileMax;
                    video[i].profile.set(0, 0, 0, ANY);
                }

                for (uint8_t i = 0; i < GyroProfileMax; i++)
                {
                    gyro[i].enabled = false;
                    gyro[i].outputEnabled = false;
                    gyro[i].fps = 0;
                    gyro[i].sensorIndex = GyroProfileMax;
                }

                for (uint8_t i = 0; i < VelocimeterProfileMax; i++)
                {
                    velocimeter[i].enabled = false;
                    velocimeter[i].outputEnabled = false;
                    velocimeter[i].fps = 0;
                    velocimeter[i].sensorIndex = VelocimeterProfileMax;
                }

                for (uint8_t i = 0; i < AccelerometerProfileMax; i++)
                {
                    accelerometer[i].enabled = false;
                    accelerometer[i].outputEnabled = false;
                    accelerometer[i].fps = 0;
                    accelerometer[i].sensorIndex = AccelerometerProfileMax;
                }

                for (uint8_t i = 0; i < SixDofProfileMax; i++)
                {
                    sixDof[i].enabled = false;
                    sixDof[i].mode = (SIXDOF_MODE_ENABLE_MAPPING | SIXDOF_MODE_ENABLE_RELOCALIZATION);
                    sixDof[i].interruptRate = SIXDOF_INTERRUPT_RATE_MAX;
                    sixDof[i].profileType = SixDofProfileMax;
                }
            };

            void reset(void) {

                playbackEnabled = false;

                for (uint8_t i = 0; i < VideoProfileMax; i++)
                {
                    video[i].outputEnabled = false;
                    video[i].enabled = false;
                    video[i].fps = 0;
                    video[i].sensorIndex = VideoProfileMax;
                    video[i].profile.set(0, 0, 0, ANY);
                }

                for (uint8_t i = 0; i < GyroProfileMax; i++)
                {
                    gyro[i].enabled = false;
                    gyro[i].outputEnabled = false;
                    gyro[i].fps = 0;
                    gyro[i].sensorIndex = GyroProfileMax;
                }

                for (uint8_t i = 0; i < VelocimeterProfileMax; i++)
                {
                    velocimeter[i].enabled = false;
                    velocimeter[i].outputEnabled = false;
                    velocimeter[i].fps = 0;
                    velocimeter[i].sensorIndex = VelocimeterProfileMax;
                }

                for (uint8_t i = 0; i < AccelerometerProfileMax; i++)
                {
                    accelerometer[i].enabled = false;
                    accelerometer[i].outputEnabled = false;
                    accelerometer[i].fps = 0;
                    accelerometer[i].sensorIndex = AccelerometerProfileMax;
                }

                for (uint8_t i = 0; i < SixDofProfileMax; i++)
                {
                    sixDof[i].enabled = false;
                    sixDof[i].mode = (SIXDOF_MODE_ENABLE_MAPPING | SIXDOF_MODE_ENABLE_RELOCALIZATION);
                    sixDof[i].interruptRate = SIXDOF_INTERRUPT_RATE_MAX;
                    sixDof[i].profileType = SixDofProfileMax;
                }
            };

            void set(VideoProfile _video, bool _enabled, bool _outputEnabled) {
                if (_video.sensorIndex < VideoProfileMax)
                {
                    video[_video.sensorIndex] = _video;
                    video[_video.sensorIndex].enabled = _enabled;
                    video[_video.sensorIndex].outputEnabled = _outputEnabled;
                }
            }

            void set(GyroProfile _gyro, bool _enabled, bool _outputEnabled) {
                if (_gyro.sensorIndex < GyroProfileMax)
                {
                    gyro[_gyro.sensorIndex] = _gyro;
                    gyro[_gyro.sensorIndex].enabled = _enabled;
                    gyro[_gyro.sensorIndex].outputEnabled = _outputEnabled;
                }
            }

            void set(VelocimeterProfile _velocimeter, bool _enabled, bool _outputEnabled) {
                if (_velocimeter.sensorIndex < VelocimeterProfileMax)
                {
                    velocimeter[_velocimeter.sensorIndex] = _velocimeter;
                    velocimeter[_velocimeter.sensorIndex].enabled = _enabled;
                    velocimeter[_velocimeter.sensorIndex].outputEnabled = _outputEnabled;
                }
            }

            void set(AccelerometerProfile _accelerometer, bool _enabled, bool _outputEnabled) {
                if (_accelerometer.sensorIndex < AccelerometerProfileMax)
                {
                    accelerometer[_accelerometer.sensorIndex] = _accelerometer;
                    accelerometer[_accelerometer.sensorIndex].enabled = _enabled;
                    accelerometer[_accelerometer.sensorIndex].outputEnabled = _outputEnabled;
                }
            }

            void set(SixDofProfile _sixDof, bool _enabled) {
                if (_sixDof.profileType < SixDofProfileMax)
                {
                    sixDof[(uint8_t)_sixDof.profileType] = _sixDof;
                    sixDof[(uint8_t)_sixDof.profileType].enabled = _enabled;
                }
            }

            VideoProfile video[VideoProfileMax];                         /**< All supported Video profiles according to sensor index                                    */
            GyroProfile gyro[GyroProfileMax];                            /**< All supported Gyro profiles according to sensor index                                     */
            AccelerometerProfile accelerometer[AccelerometerProfileMax]; /**< All supported Accelerometer profiles according to sensor index                            */
            SixDofProfile sixDof[SixDofProfileMax];                      /**< All supported SixDof profiles according to sensor profile (HMD, Controller1, Controlelr2) */
            VelocimeterProfile velocimeter[VelocimeterProfileMax];       /**< All supported Velocimeter profiles                                                        */
            bool playbackEnabled;                                        /**< Enable video playback mode                                                                */
        };

        class DeviceInfo
        {
        public:
            class UsbConnectionDescriptor {
            public:
                UsbConnectionDescriptor() : idVendor(0), idProduct(0), bcdUSB(0), port(0), bus(0), portChainDepth(0), portChain{0} {}
                uint16_t idVendor;                     /**< USB Vendor ID: DFU Device = 0x03E7, Device = 0x8087                                                   */
                uint16_t idProduct;                    /**< USB Product ID: DFU Device = 0x2150, Device = 0x0AF3 for T260, 0B37 for T265                          */
                uint16_t bcdUSB;                       /**< USB specification release number: 0x100 = USB 1.0, 0x110 = USB 1.1, 0x200 = USB 2.0, 0x300 = USB 3.0  */
                uint8_t port;                          /**< Number of the port that the device is connected to                                                    */
                uint8_t bus;                           /**< Number of the bus that the device is connected to                                                     */
                uint8_t portChainDepth;                /**< Number of ports in the port tree of this device                                                       */
                uint8_t portChain[MAX_USB_TREE_DEPTH]; /**< List of all port numbers from root for this device                                                    */
            };

            class DeviceStatus {
            public:
                DeviceStatus() : hw(Status::SUCCESS), host(Status::SUCCESS) {}
                Status host; /**< Host status */
                Status hw;   /**< HW status   */
            };

            class DeviceVersion {
            public:
                DeviceVersion() {}
                Version deviceInterface;   /**< Device supported interface API version                                                                         */
                Version host;              /**< libtm Host version                                                                                             */
                Version fw;                /**< Myriad firmware version                                                                                        */
                Version hw;                /**< ASIC Board version: 0x00 = ES0, 0x01 = ES1, 0x02 = ES2, 0x03 = ES3, 0xFF = Unknown - only major part is active */
                Version centralApp;        /**< Central firmware version                                                                                       */
                Version centralProtocol;   /**< Central BLE protocol version - only major part is active                                                       */
                Version centralBootLoader; /**< Central BLE protocol version - only major, minor, patch parts are active                                       */
                Version centralSoftDevice; /**< Central BLE protocol version - only major part is active                                                       */
                Version eeprom;            /**< EEPROM data version - only major and minor parts are active                                                    */
                Version rom;               /**< Myriad ROM version - only major part is active                                                                 */
            };

            DeviceInfo() : deviceType(0), serialNumber(0), skuInfo(0), numGyroProfile(0), numVelocimeterProfile(0), numAccelerometerProfiles(0), numVideoProfiles(0), eepromLockState(LockStateMax) {};
            UsbConnectionDescriptor usbDescriptor; /**< USB Connection Descriptor includes USB info and physical location            */
            DeviceStatus status;                   /**< HW and Host status                                                           */
            DeviceVersion version;                 /**< HW, FW, Host, Central, EEPROM, ROM, interface versions                       */
            EepromLockState eepromLockState;       /**< EEPROM Lock state                                                            */
            uint8_t deviceType;                    /**< Device identifier: 0x1 = TM2                                                 */
            uint64_t serialNumber;                 /**< Device serial number                                                         */
            uint8_t  skuInfo;                      /**< 1 for T260 - with ble, 0 for T265 - w/o ble                                  */
            uint8_t  numGyroProfile;               /**< Number of Gyro Supported Profiles returned by Supported RAW Streams          */
            uint8_t  numVelocimeterProfile;        /**< Number of Velocimeter Supported Profiles returned by Supported RAW Streams   */
            uint8_t  numAccelerometerProfiles;     /**< Number of Accelerometer Supported Profiles returned by Supported RAW Streams */
            uint8_t  numVideoProfiles;             /**< Number of Video Supported Profiles returned by Supported RAW Streams         */
        };

        class LogControl {
        public:
            LogControl() : verbosity(None), outputMode(LogOutputModeMax), rolloverMode(false) {};
            LogControl(LogVerbosityLevel _verbosity, LogOutputMode _OutputMode, bool _RolloverMode) {
                verbosity = _verbosity;
                outputMode = _OutputMode;
                rolloverMode = _RolloverMode;
            }

            LogVerbosityLevel verbosity; /**< Log Verbosity level                                                                                                                                            */
            LogOutputMode outputMode;    /**< Output mode - screen or buffer                                                                                                                                 */
            bool rolloverMode;           /**< Rollover mode - False: no rollover (logging will be paused after log is filled, Until cleared, only first logs will be stored), True: Last logs will be stored */                                                                         
        };

        class Log {
        public:
            class LocalTime {
            public:
                LocalTime() : year(0), month(0), dayOfWeek(0), day(0), hour(0), minute(0), second(0), milliseconds(0) {}
                uint16_t year;         /**< Year         */
                uint16_t month;        /**< Month        */
                uint16_t dayOfWeek;    /**< Day of week  */
                uint16_t day;          /**< Day          */
                uint16_t hour;         /**< Hour         */
                uint16_t minute;       /**< Minute       */
                uint16_t second;       /**< Second       */
                uint16_t milliseconds; /**< Milliseconds */
            };

            class LogEntry {
            public:

                LogEntry() : timeStamp(0), localTimeStamp(), verbosity(None), deviceID(0), threadID(0), moduleID{0}, functionSymbol(0), lineNumber(0), payloadSize(0), payload{ 0 } {}
                int64_t timeStamp;                         /**< Host log - Current timestamp relative to host initialization (nanosec) */
                                                           /**< FW log - Current timestamp relative to device initialization (nanosec) */
                LocalTime localTimeStamp;                  /**< Host log - Current time of log                                         */
                                                           /**< FW log - Capture time of FW log                                        */                                     
                LogVerbosityLevel verbosity;               /**< Verbosity level                                                        */
                uint64_t deviceID;                         /**< Device ID                                                              */
                uint32_t threadID;                         /**< Thread ID                                                              */
                char moduleID[MAX_LOG_BUFFER_MODULE_SIZE]; /**< Module ID                                                              */
                uint32_t functionSymbol;                   /**< Function symbol                                                        */
                uint32_t lineNumber;                       /**< Line number                                                            */
                uint32_t payloadSize;                      /**< payload size                                                           */
                char payload[MAX_LOG_BUFFER_ENTRY_SIZE];   /**< payload                                                                */
            };

            Log() : entries(0), maxEntries(0), entry{} {}
            LogEntry entry[MAX_LOG_BUFFER_ENTRIES]; /**< Log entry info and payload                        */
            uint32_t entries;                       /**< Active log entries in the buffer                  */
            uint32_t maxEntries;                    /**< Max entries (for GetFWLog call rate calculations) */
        };

        class CameraIntrinsics {
        public:
            CameraIntrinsics() : width(0), height(0), ppx(0), ppy(0), fx(0), fy(0), distortionModel(0){
                memset(coeffs, 0, sizeof(coeffs));
            };

            uint32_t width;           /**< Width of the image in pixels                                                                                                           */
            uint32_t height;          /**< Height of the image in pixels                                                                                                          */
            float_t ppx;              /**< Horizontal coordinate of the principal point of the image, as a pixel offset from the left edge                                        */
            float_t ppy;              /**< Vertical coordinate of the principal point of the image, as a pixel offset from the top edge                                           */
            float_t fx;               /**< Focal length of the image plane, as a multiple of pixel width                                                                          */
            float_t fy;               /**< Focal length of the image plane, as a multiple of pixel Height                                                                         */
            uint32_t distortionModel; /**< Distortion model of the image: F-THETA = 1, NONE (UNDISTORTED) = 3, KANNALA_BRANDT4 = 4                                                */
            float_t coeffs[5];        /**< Distortion coefficients                                                                                                                */
        };

        class MotionIntrinsics {
        public:
            MotionIntrinsics() {
                memset(data, 0, sizeof(data));
                memset(noiseVariances, 0, sizeof(noiseVariances));
                memset(biasVariances, 0, sizeof(biasVariances));
            };

            float_t data[3][4];        /**< Scale matrix    */
            float_t noiseVariances[3]; /**< Noise variances */
            float_t biasVariances[3];  /**< Bias variances  */
        };

        class SensorExtrinsics {
        public:
            SensorExtrinsics() : referenceSensorId(0) {
                memset(rotation, 0, sizeof(rotation));
                memset(translation, 0, sizeof(translation));
            };

            float_t rotation[9];        /**< Column-major 3x3 rotation matrix                 */
            float_t translation[3];     /**< 3 element translation vector, in meters          */
            SensorId referenceSensorId; /**< Reference sensor uses for extrinsics calculation */
        };

        class ExposureModeControl {
        public:
            ExposureModeControl() : antiFlickerMode(0) 
            {
                memset(videoStreamAutoExposure, 0, sizeof(videoStreamAutoExposure));
            };

            ExposureModeControl(uint8_t _bVideoStreamsAutoExposureMask, uint8_t _bAntiFlickerMode)
            {
                for (uint8_t i = 0; i < VideoProfileMax; i++)
                {
                    videoStreamAutoExposure[i] = (bool)((_bVideoStreamsAutoExposureMask >> i) & 1);
                }

                antiFlickerMode = _bAntiFlickerMode; 
            }

            bool videoStreamAutoExposure[VideoProfileMax]; /**< Video streams to apply the auto exposure configuration                                                        */
            uint8_t antiFlickerMode;                       /**< Anti Flicker Mode: 0 - disable (e.g. for outside use), 1 - 50Hz, 2 - 60Hz, 3 - Auto (currently not supported) */
        };

        class Exposure {
        public:

            class StreamExposure {
            public:
                StreamExposure() : cameraId(0), integrationTime(0), gain(0) {};
                StreamExposure(uint8_t _cameraId, uint32_t _integrationTime, float_t _gain) : cameraId(_cameraId), integrationTime(_integrationTime), gain(_gain) {};
                uint8_t cameraId;         /**< Bits 0-4: Type of requested camera, supported values are: Color = 0, Depth = 1, IR = 2, Fisheye = 3 */
                                          /**< Bits 5-7: Camera index - Zero based index of camera with the same type within device                */
                uint32_t integrationTime; /**< Integration time in micro-seconds (Max 16000 uSec)                                                  */
                float_t gain;             /**< Digital gain                                                                                        */
            };

            Exposure() : numOfVideoStreams(0) { memset(stream, 0, sizeof(stream)); };
            uint8_t numOfVideoStreams;                /**< Number of video streams to configure                                     */
            StreamExposure stream[MAX_VIDEO_STREAMS]; /**< Stream exposure data variable sized array, according to wNumberOfStreams */
        };

        class Temperature {
        public:
            class SensorTemperature {
            public:
                SensorTemperature() : index(TemperatureSensorMax), temperature(0.0), threshold(0.0) {};
                SensorTemperature(TemperatureSensorType _index, float_t _temperature, float_t _threshold) : index(_index), temperature(_temperature), threshold(_threshold) {};
                TemperatureSensorType index; /**< Temperature sensor index: 0x0 - VPU, 0x1 - IMU, 0x2 - BLE */
                float_t temperature;         /**< Sensor temperature (Celsius)                              */
                float_t threshold;           /**< Sensor temperature threshold (Celsius)                    */
            };

            Temperature() : numOfSensors(0) { memset(sensor, 0, sizeof(sensor)); };
            uint32_t numOfSensors;                          /**< Number of temperature sensors                                            */
            SensorTemperature sensor[TemperatureSensorMax]; /**< Stream exposure data variable sized array, according to wNumberOfStreams */
        };

        class LocalizationDataFrame {
        public:
            LocalizationDataFrame() : moreData(false), chunkIndex(0), length(0), buffer(NULL) { };
            bool moreData;       /**< moreData indicates there are more data to send. The last chunk (possibly even a zero-size chunk) shall be marked moreData = false                 */
            Status status;       /**< Set localization status                                                                                                                           */
            uint32_t chunkIndex; /**< A running counter starting in 0 identifying the chunk in a single data stream                                                                     */
            uint32_t length;     /**< The length in bytes of the localization buffer                                                                                                    */
            uint8_t* buffer;     /**< Localization data buffer pointer of max size MAX_CONFIGURATION_SIZE bytes, Data format is algorithm specific and opaque to the USB and host stack */
        };

        class RelocalizationEvent {
        public:
            int64_t timestamp;   /**< Timestamp of relocalization, measured in nanoseconds since device system initialization             */
            SessionId sessionId; /**< Id of session matched by relocalization. 0x0 for current session, greater than 0 for loaded session */
        };

        class RelativePose {
        public:
            RelativePose() {};
            Axis translation;    /**< X, Y, Z values of translation, in meters (in the coordinate system of the tracker relative to the current position)                                    */
            Quaternion rotation; /**< Qi, Qj, Qk, Qr components of rotation as represented in quaternion rotation (in the coordinate system of the tracker relative to the current position) */
        };

        class CalibrationData {
        public:
            CalibrationData() : type(CalibrationTypeMax), length(0), buffer(NULL) { };
            CalibrationType type; /**< Type of calibration (New - override previous calibration, Append - join to previous set calibration                                            */
            uint32_t length;      /**< The length in bytes of the calibration buffer                                                                                                  */
            uint8_t* buffer;      /**< Calibration data buffer pointer of max size MAX_CALIBRATION_SIZE bytes, Data format is algorithm specific and opaque to the USB and host stack */
        };

        class GeoLocalization {
        public:
            GeoLocalization() : latitude(0), longitude(0), altitude(0) { };
            GeoLocalization(double_t _latitude, double_t _longitude, double_t _altitude) : latitude(_latitude), longitude(_longitude), altitude(_altitude) {};
            void set(double_t _latitude, double_t _longitude, double_t _altitude) 
            {
                latitude = _latitude;
                longitude = _longitude;
                altitude = _altitude;
            };
            double_t latitude;  /**< Latitude in degrees                                     */
            double_t longitude; /**< Longitude in degrees                                    */
            double_t altitude;  /**< Altitude in meters above the WGS 84 reference ellipsoid */
        };

        class ControllerAssociatedDevices {
        public:
            ControllerAssociatedDevices() : macAddress1{0}, macAddress2{0}, addressType1(AddressTypeRandom), addressType2(AddressTypeRandom) {};
            ControllerAssociatedDevices(uint8_t* _macAddress1, uint8_t* _macAddress2 = nullptr, AddressType _addressType1 = AddressTypeRandom, AddressType _addressType2 = AddressTypeRandom)
            {
                set(_macAddress1, _macAddress2, _addressType1, _addressType2);
            }

            void set(uint8_t* _macAddress1, uint8_t* _macAddress2 = nullptr, AddressType _addressType1 = AddressTypeRandom, AddressType _addressType2 = AddressTypeRandom)
            {
                copy(macAddress1, _macAddress1);
                copy(macAddress2, _macAddress2);
                addressType1 = _addressType1;
                addressType2 = _addressType2;
            }

            uint8_t macAddress1[MAC_ADDRESS_SIZE]; /**< MAC address of controller 1                                              */
            uint8_t macAddress2[MAC_ADDRESS_SIZE]; /**< MAC address of controller 2, set to all zeros if controller is not setup */
            AddressType addressType1;              /**< Controller 1 Mac Address Type: Public or Random                          */
            AddressType addressType2;              /**< Controller 2 Mac Address Type: Public or Random                          */

        private:
            /**< Copy source address to destination address if source is not NULL, else zero destination address */
            void copy(uint8_t* dstAddress, uint8_t* srcAddress)
            {
                for (uint8_t i = 0; i < MAC_ADDRESS_SIZE; i++)
                {
                    dstAddress[i] = ((srcAddress == nullptr) ? 0 : srcAddress[i]);
                }
            }
        };

        class ControllerDeviceConnect {
        public:
            ControllerDeviceConnect() : macAddress{0}, timeout(0), addressType(AddressTypeRandom){};
            ControllerDeviceConnect(uint8_t* _macAddress, uint16_t _timeout, AddressType _addressType = AddressTypeRandom) : addressType(_addressType), timeout(_timeout)
            {
                for (uint8_t i = 0; i < MAC_ADDRESS_SIZE; i++) 
                {
                    macAddress[i] = _macAddress[i];
                }
            }

            uint8_t macAddress[MAC_ADDRESS_SIZE]; /**< MAC address of controller         */
            AddressType addressType;              /**< Address Type: Public or Random    */
            uint16_t timeout;                     /**< Controller connect timeout (msec) */
        };

        class ControllerData {
        public:
            ControllerData() : controllerId (0), commandId(0), buffer(NULL), bufferSize(0){};
            ControllerData(uint8_t _controllerId, uint8_t _commandId, uint16_t _bufferSize, uint8_t* _buffer) : controllerId(_controllerId), commandId(_commandId), bufferSize(_bufferSize), buffer(_buffer) {};
            void set(uint8_t _controllerId, uint8_t _commandId, uint16_t _bufferSize, uint8_t* _buffer)
            {
                controllerId = _controllerId;
                commandId = _commandId;
                bufferSize = _bufferSize;
                buffer = _buffer;
            }

            uint8_t controllerId; /**< Controller identifier (1 or 2)                                                 */
            uint8_t commandId;    /**< Command to be sent to the controller (vendor specific) - values 0-63 supported */
            uint16_t bufferSize;  /**< Size of data buffer                                                            */
            uint8_t* buffer;      /**< Controller data to be sent. Data format is vendor specific                     */
        };

        class ControllerFW {
        public:
            ControllerFW() : macAddress{0}, image(NULL), imageSize(0), addressType(AddressTypeRandom) {};
            ControllerFW(uint8_t* _macAddress, uint16_t _imageSize, uint8_t* _image, AddressType _addressType = AddressTypeRandom) : addressType(_addressType), imageSize(_imageSize), image(_image)
            {
                if (_macAddress != NULL)
                {
                    for (uint8_t i = 0; i < MAC_ADDRESS_SIZE; i++)
                    {
                        macAddress[i] = _macAddress[i];
                    }
                }
            }

            uint8_t macAddress[MAC_ADDRESS_SIZE]; /**< MAC address of controller to be updated          */
            AddressType addressType;              /**< Discovered device address type: Public or Random */
            uint32_t imageSize;                   /**< Size of Controller FW image                      */
            uint8_t* image;                       /**< New controller FW image to be updated            */
        };
    };
}
