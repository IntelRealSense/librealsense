// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include "TrackingCommon.h"
#include "TrackingData.h"

#define IN
#define OUT

namespace perc
{
    class DLL_EXPORT TrackingDevice
    {
    public:
        class DLL_EXPORT Listener
        {
        public:

            /**
            * @brief onPoseFrame
            *        The function will be called once TrackingDevice has a new pose data
            *
            * @param pose - Pose object containing the pose data
            */
            virtual void onPoseFrame(OUT TrackingData::PoseFrame& pose) {}

            /**
            * @brief onVideoFrame
            *        The function will be called once TrackingDevice has a new video (color / depth / fisheye) frame
            *
            * @param frame - VideoFrame object containing frame
            */
            virtual void onVideoFrame(OUT TrackingData::VideoFrame& frame) {}

            /**
            * @brief onAccelerometerFrame
            *        The function will be called once TrackingDevice has a new accelerometer data
            *
            * @param frame - Accelerometer object containing frame
            */
            virtual void onAccelerometerFrame(OUT TrackingData::AccelerometerFrame& frame) {}

            /**
            * @brief onGyroFrame
            *        The function will be called once TrackingDevice has a new gyro data
            *
            * @param frame - Gyro object containing frame
            */
            virtual void onGyroFrame(OUT TrackingData::GyroFrame& frame) {}

            /**
            * @brief onVelocimeterFrame
            *        The function will be called once TrackingDevice has a new velocimeter data
            *
            * @param frame - Velocimeter object containing frame
            */
            virtual void onVelocimeterFrame(OUT TrackingData::VelocimeterFrame& frame) {}

            /**
            * @brief onControllerDiscoveryEventFrame
            *        The function will be called once TrackingDevice has a new controller discovery event
            *
            * @param frame - Controller object containing frame
            */
            virtual void onControllerDiscoveryEventFrame(OUT TrackingData::ControllerDiscoveryEventFrame& frame) {}

            /**
            * @brief onControllerFrame
            *        The function will be called once TrackingDevice has a new controller frame
            *
            * @param frame - Controller object containing frame
            */
            virtual void onControllerFrame(OUT TrackingData::ControllerFrame& frame) {}

            /**
            * @brief onControllerConnectedEventFrame
            *        The function will be called once TrackingDevice has a new controller connected event
            *
            * @param frame - Controller object containing frame
            */
            virtual void onControllerConnectedEventFrame(OUT TrackingData::ControllerConnectedEventFrame& frame) {}

            /**
            * @brief onControllerDisconnectedEventFrame
            *        The function will be called once TrackingDevice has a new controller disconnected event
            *
            * @param frame - Controller object containing frame
            */
            virtual void onControllerDisconnectedEventFrame(OUT TrackingData::ControllerDisconnectedEventFrame& frame) {}

            /**
            * @brief onControllerCalibrationEventFrame
            *        The function will be called once the controller calibration process is finished
            *
            * @param frame - Controller object containing frame
            */
            virtual void onControllerCalibrationEventFrame(OUT TrackingData::ControllerCalibrationEventFrame& frame) {}

            /**
            * @brief onRssiFrame
            *        The function will be called once TrackingDevice has a new RSSI frame
            *
            * @param frame - Rssi object containing frame
            */
            virtual void onRssiFrame(OUT TrackingData::RssiFrame& frame) {}

            /**
            * @brief onLocalizationDataEventFrame
            *        The function will be called once TrackingDevice has a new localization data event
            *
            * @param frame - Localization Data object containing frame for Set or Get localization
            *                Get localization data - relevant fields: status, mapIndex, moreData, chunkIndex, length, buffer
            *                Set localization data - relevant fields: status, mapIndex
            */
            virtual void onLocalizationDataEventFrame(OUT TrackingData::LocalizationDataFrame& frame) {}

            /**
            * @brief onRelocalizationEvent
            *        The function will be called once TrackingDevice has a new relocalization event
            *
            * @param event - Relocalization event
            */
            virtual void onRelocalizationEvent(OUT TrackingData::RelocalizationEvent& event) {}

            /**
            * @brief onFWUpdateEvent
            *        The host interface shall support firmware update progress events
            *
            * @param frame - Firmware update event containing frame indicates the progress status
            */
            virtual void onFWUpdateEvent(OUT TrackingData::ControllerFWEventFrame& frame) {}

            /**
            * @brief onStatusEvent
            *        The host interface shall support general status event
            *
            * @param frame - Status event frame indicates the current status
            */
            virtual void onStatusEvent(OUT TrackingData::StatusEventFrame& frame) {}

            /**
            * @brief onControllerLedEvent
            *
            * @param frame - Controller Led event frame indicates the controller led intensity [0-100]
            */
            virtual void onControllerLedEvent(OUT TrackingData::ControllerLedEventFrame& frame) {}
        };


    public:
        virtual ~TrackingDevice() {}

        /**
        * @brief GetSupportedProfile
        *        Initialize basic profile according to all supported streams - adding only the first profile on every profile index
        *        Gyro: sensorIndex = all supported sensorIndex, fps = first supported gyro stream with this sensorIndex, enabled = false, outputEnabled = false
        *        Accelerometer: sensorIndex = all supported sensorIndex, fps = first supported accelerometer stream with this sensorIndex, enabled = false, outputEnabled = false
        *        Video: sensorIndex = all supported sensorIndex, fps/RawProfile = first supported video stream with this sensorIndex, enabled = false, outputEnabled = false 
        *        SixDof: profileType = all profileTypes, rate = SIXDOF_INTERRUPT_RATE_IMU, enabled = false
        *        
        * @return Status
        */
        virtual Status GetSupportedProfile(OUT TrackingData::Profile& profile) = 0;

        /**
         * @brief start
         *        Start streaming of all stream that were previously configured.
         * @param Data listener
         * @param <Profile> includes all required configuration according to stream session.
         *        User shouldn't change running configuration during streaming
         *        TODO: review set/get functions
         * @return Status
         */
        virtual Status Start(IN Listener*, IN TrackingData::Profile* = NULL) = 0;

        /**
         * @brief stop
         *        Stop streaming of all stream that were previously configured, all stream configuration parameters will be cleared.
         * @return Status
         */
        virtual Status Stop(void) = 0;

        /**
         * @brief GetDeviceInfo
         *        Retrieve information on the TM2 device
         * @param message - Device info message buffer
         * @return Status
         */
        virtual Status GetDeviceInfo(OUT TrackingData::DeviceInfo& info) = 0;

        /**
         * @brief GetSupportedRawStreams
         *        Get all FW supported raw streams: Video, Gyro, Accelerometer. Velocimeter.
         * @param videoProfiles - Video profile buffer containing all supported video streams (Buffer size should be according to DeviceInfo.numVideoProfiles)
         * @param gyroProfiles - Gyro profile buffer containing all supported gyro streams (Buffer size should be according to DeviceInfo.numGyroProfiles)
         * @param accelerometerProfiles - Accelerometer profile buffer containing all supported accelerometer streams (Buffer size should be according to DeviceInfo.numAccelerometerProfiles)
         * @param velocimeterProfiles - Velocimeter profile buffer containing all supported velocimeter streams (Buffer size should be according to DeviceInfo.numVelocimeterProfile)
         * @return Status
         */
        virtual Status GetSupportedRawStreams(OUT TrackingData::VideoProfile* videoProfiles, OUT TrackingData::GyroProfile* gyroProfiles, OUT TrackingData::AccelerometerProfile* accelerometerProfiles, OUT TrackingData::VelocimeterProfile* velocimeterProfiles = nullptr) = 0;

        /**
        * @brief SetFWLogControl
        *        Controls the FW logging parameters, such as verbosity and log mode.
        * @param logControl - logControl object to fill with verbosity and log mode
        * @return Status
        */
        virtual Status SetFWLogControl(IN const TrackingData::LogControl& logControl) = 0;

        /**
        * @brief GetFWLog
        *        Get FW log entries - every call to this function will get and clean all FW log entries
        *        Need to enable FW logs (SetFWLogControl) before calling GetFWLog
        *        Use entries and maxEntries fields to calculate the GetFWLog call rate to get max FW logs with min GetFWLog requests,
        *        For example: In case log entries is equal to log maxEntries, FW log buffer is completely filled so several FW logs are dumped, 
        *        Increase the GetFWLog call rate to decrease the dumped logs amount.
        * @param log - log object to fill with log entries
        * @return Status
        */
        virtual Status GetFWLog(OUT TrackingData::Log& log) = 0;

        /**
         * @brief GetCameraIntrinsics
         *        Retrieves the intrinsic parameters of an individual camera in the device
         * @param id - Sensor Id (Sensor type + sensor index)
         * @param intrinsics - intrinsic object to fill
         * @return Status
         */
         virtual Status GetCameraIntrinsics(IN SensorId id, OUT TrackingData::CameraIntrinsics& intrinsics) = 0;

        /**
         * @brief SetCameraIntrinsics
         *        Set the intrinsic parameters of an individual camera in the device
         * @param id - Sensor Id (Sensor type + sensor index)
         * @param message - intrinsic parameters to set
         * @return Status
         */
         virtual Status SetCameraIntrinsics(IN SensorId id, IN const TrackingData::CameraIntrinsics& intrinsics) = 0;

        /**
         * @brief GetMotionModuleIntrinsics
         *        Retrieves the intrinsic parameters of an individual motion module in the device
         * @param id - Sensor Id (Sensor type + sensor index)
         * @param message - Buffer of intrinsic parameters
         * @return Status
         */
         virtual Status GetMotionModuleIntrinsics(IN SensorId id, OUT TrackingData::MotionIntrinsics& intrinsics) = 0;

        /**
         * @brief SetMotionModuleIntrinsics
         *        Set the intrinsic parameters of an individual motion module in the device
         * @param id - Sensor Id (Sensor type + sensor index)
         * @param message - Buffer of intrinsic parameters
         * @return Status
         */
         virtual Status SetMotionModuleIntrinsics(IN SensorId id, IN const TrackingData::MotionIntrinsics& intrinsics) = 0;

        /**
         * @brief GetExtrinsics
         *        Retrieves extrinsic pose of on individual sensor in the device relative to another one
         * @param id - Sensor Id (Sensor type + sensor index)
         * @param extrinsics - Output container for extrinsic parameters
         * @return Status
         */
         virtual Status GetExtrinsics(IN SensorId id, OUT TrackingData::SensorExtrinsics& extrinsics) = 0;

        /**
        * @brief SetOccupancyMapControl
        *        Enables/disables occupancy map calculation. Occupancy map calculation is based on 6DoF calculation, 
        *        So it cannot be enabled when 6DoF is disabled, and an UNSUPPORTED error will be returned
        *        If no SetOccupancyMapControl command was called before the host calls to Start, the default value used shall be disabled.
        * @param enable - Enable/Disable occupancy map calculation
        * @return Status
        */
        /* Currently not supported at FW side */
        //virtual Status SetOccupancyMapControl(uint8_t enable) = 0;

        /**
         * @brief GetPose
         *        Retrieves the latest pose of the camera relative to its initial position
         * @param pose - pose frame container
         * @param sourceIndex - 0x0 = HMD, 0x1 - controller 1, 0x2 - controller 2
         * @return Status
         */
         /* Currently not supported at FW side */
         //virtual Status GetPose(TrackingData::PoseFrame& pose, uint8_t sourceIndex) = 0;

        /**
        * @brief SetExposureModeControl
        *        Enable/disable the auto-exposure management of the different video streams, and configure the powerline frequency rate.
        *        Calling this message is only supported before the streams are started.
        *        The default values for video streams 0 and 1 shall be auto-exposure enable with 50Hz flicker rate.
        *        The firmware only supports the following:
        *        1. Disabling auto-exposure for all streams (bVideoStreamsMask==0)
        *        2. Enabling auto-exposure for both video stream 0 and 1 together (bVideoStreamsMask ==0x3).
        * @param mode - VideoStreamsMask and AntiFlickerMode
        * @return Status
        */
        virtual Status SetExposureModeControl(IN const TrackingData::ExposureModeControl& mode) = 0;

        /**
        * @brief SetExposure
        *        Sets manual values for the video streams integration time and gain.
        * @param exposure - Exposure data for all video streams
        * @return Status
        */
        virtual Status SetExposure(IN const TrackingData::Exposure& exposure) = 0;

        /**
        * @brief GetTemperature
        *        Returns temperature and temperature threshold from all temperature sensors (VPU, IMU, BLE)
        * @param temperature - Sensor temperature container
        * @return Status
        */
        virtual Status GetTemperature(OUT TrackingData::Temperature& temperature) = 0;

        /**
        * @brief SetTemperatureThreshold
        *        Set temperature threshold to requested sensors (VPU, IMU, BLE)
        *        The firmware shall actively monitor the temperature of the underlying sensors.
        *        When a component temperature is 10 % close to its defined threshold, the firmware shall send a DEV_ERROR message with a TEMPERATURE_WARNING status to the host,
        *        When the temperature reach the threshold, the firmware shall stop all running algorithms and sensors (as if DEV_STOP was called), and send a TEMPERATURE_SHUTDOWN status to the user.
        * @param temperature - Temperature threshold to set per temperature sensors (relevant fields - index, threshold)
        * @param token - Secured token to override max temperature threshold
        * @return Status
        */
        virtual Status SetTemperatureThreshold(IN const TrackingData::Temperature& temperature, IN uint32_t token) = 0;

        /**
        * @brief LockConfiguration
        *        Write-protect the manufacturing configuration tables from future changes.
        *        This can be done in hardware by locking the upper quarter of the EEPROM memory, or by software by the firmware managing a lock bits in each configuration table metadata.
        * @param type - Hardware or software.
        * @param lock - False: unlock the configuration table.
        *               True: lock the configuration table.
        * @param tableType - which configuration lable to lock. valid for SW lock only!
        * @return Status
        */
        virtual Status LockConfiguration(IN LockType type, IN bool lock, IN uint16_t tableType = 0xFFFF) = 0;

        /**
        * @brief PermanentLockConfiguration
        *        Permanent Write-protect the manufacturing configuration tables from future changes.
        *        This can be done in hardware by locking the upper quarter of the EEPROM memory, or by software by the firmware managing a lock bits in each configuration table metadata.
        *        Warning - This is an irreversible action.
        * @param type - Hardware or software.
        * @param token - Secured token
        * @param tableType - which configuration lable to lock. valid for SW lock only!
        * @return Status
        */
        virtual Status PermanentLockConfiguration(IN LockType type, IN uint32_t token, IN uint16_t tableType = 0xFFFF) = 0;

        /**
        * @brief ReadConfiguration
        *        Reads a configuration table from the device memory.
        *        The host shall return UNSUPPORTED if it does not recognize the requested TableType.
        *        The host shall return TABLE_NOT_EXIST if it recognize the table type but no such table exists yet in the EEPROM.
        *        The host shall return BUFFER_TOO_SMALL if input buffer is too small for needed table type.
        * @param configurationId - The ID of the requested configuration table.
        * @param size            - EEPROM read configuration buffer size (max value = MAX_CONFIGURATION_SIZE).
        * @param buffer          - EEPROM read configuration buffer pointer.
        * @param actualSize      - Actual EEPROM read configuration buffer size.
        * @return Status
        */
        virtual Status ReadConfiguration(IN uint16_t configurationId, IN uint16_t size, OUT uint8_t* buffer, OUT uint16_t* actualSize = nullptr) = 0;

        /**
        * @brief WriteConfiguration
        *        Writes a configuration table to the device's EEPROM memory.
        *        This command shall only be supported while the device is stopped, otherwise it shall return DEVICE_BUSY.
        *        The host shall return UNSUPPORTED if it does not recognize the requested TableType.
        *        The host shall return TABLE_LOCKED if the configuration table is write protected and cannot be overridden.
        *        The new data shall be available immediately after completion without requiring a device reset, both to any firmware code or to an external client through a "read configuration" command.
        *        All internal data object in the firmware memory that were already initialized from some EEPROM data or previous "write configuration" command shall be invalidated and refreshed to the new written data.
        * @param configurationId - The ID of the requested configuration table.
        * @param size            - EEPROM read configuration buffer size (max value = MAX_CONFIGURATION_SIZE).
        * @param buffer          - EEPROM read configuration buffer pointer.
        * @return Status
        */
        virtual Status WriteConfiguration(IN uint16_t configurationId, IN uint16_t size, IN uint8_t* buffer) = 0;

        /**
        * @brief DeleteConfiguration
        *        Delete a configuration table from the internal EEPROM storage.
        * @param configurationId - The ID of the requested configuration table.
        * @return Status
        */
        virtual Status DeleteConfiguration(IN uint16_t configurationId) = 0;

        /**
        * @brief GetLocalizationData
        *        Returns the localization data as created during a 6DoF session.
        *        The response to this message is generated and streamed by the underlying firmware algorithm in run-time, so the total size of the data cannot be known in advance. 
        *        The entire data will be streams in "chunks" using callback onLocalizationDataEventFrame 
        * @param listener - Data listener
        * @return Status
        */
        virtual Status GetLocalizationData(IN Listener* listener) = 0;

        /**
        * @brief SetLocalizationData
        *        Sets the localization data to be used to localize the 6DoF reports.
        *        Caution - this call may take several seconds.
        * @param listener - Data listener
        * @param length   - The length in bytes of the localization buffer
        * @param buffer   - Localization data buffer (Max size MAX_CONFIGURATION_SIZE)
        * @return Status
        */
        virtual Status SetLocalizationData(IN Listener* listener, IN uint32_t length, IN const uint8_t* buffer) = 0;

        /**
        * @brief ResetLocalizationData
        *        Resets the localization data
        * @param flag - 0 - Reset all localization data tables, 1 - reset only the map by its mapIndex
        * @return Status
        */
        virtual Status ResetLocalizationData(IN uint8_t flag) = 0;

        /**
        * @brief SetStaticNode
        *        Set a relative position of a static node
        * @param guid - Unique name (Null-terminated C-string) for the static node, max length is 127 bytes plus one byte for the terminating null character
        *               Using guid that already exist in the map overrides the previous node
        * @param relativePose - Static node relative pose data including translation  and rotation
        * @return Status
        */
        virtual Status SetStaticNode(IN const char* guid, IN const TrackingData::RelativePose& relativePose) = 0;

        /**
        * @brief GetStaticNode
        *        Get relative position of a static node
        * @param guid - Unique name (Null-terminated C-string) for the static node, max length 127 bytes plus one byte for the terminating null character
        * @param relativePose - Static node relative pose data including translation  and rotation
        * @return Status
        */
        virtual Status GetStaticNode(IN const char* guid, OUT TrackingData::RelativePose& relativePose) = 0;

        /**
        * @brief SetGeoLocation
        *        Sets the geographical location (e.g. GPS data). This data can be later used by the algorithms to correct IMU readings.
        * @param geoLocation - latitude, longitude, altitude
        * @return Status
        */
        virtual Status SetGeoLocation(IN const TrackingData::GeoLocalization& geoLocation) = 0;

        /**
        * @brief EepromRead
        *        Reads Eeprom from device
        * @param offset - Offset from start of Eeprom
        * @param size   - Size of memory to read
        * @param buffer - where to put the read data. Buffer should be at least of 'size' bytes
        * @param actual - The actual number of bytes read from EEPROM.
        *               
        * @return Status
        */
        virtual Status EepromRead(IN uint16_t offset, IN uint16_t size, OUT uint8_t* buffer, OUT uint16_t& actual) = 0;

        /**
        * @brief EepromWrite
        *        Writes Eeprom from device
        * @param offset - Offset from start of Eeprom
        * @param size   - Size of memory to write
        * @param buffer - A buffer to be written to the eeprom. 
        * @param actual - The actual number of bytes written to EEPROM. The fucntions returns this value to user.
        * @param verify - Should the written data be verified afterwards using EepromRead calls (will take more time)
        *
        * @return Status
        */
        virtual Status EepromWrite(IN uint16_t offset, IN uint16_t size, IN uint8_t* buffer, OUT uint16_t& actual, IN bool verify = false) = 0;

        /**
        * @brief Reset
        *        Resets the device and loads FW
        *        Caution - this function is non blocking, need to sleep at least 2 seconds afterwards to let the FW load again
        *
        * @return Status
        */
        virtual Status Reset(void) = 0;

        /**
        * @brief AppendCalibration
        *        Append calibration to current SLAM calibration
        * @param calibrationData - Calibration data
        *
        * @return Status
        */
        virtual Status AppendCalibration(const TrackingData::CalibrationData& calibrationData) = 0;

        /**
        * @brief SendFrame
        *        Sends video frame to the device
        * @param frame - Video frame
        *
        * @return Status
        */
        virtual Status SendFrame(IN const TrackingData::VideoFrame& frame) = 0;

        /**
        * @brief SendFrame
        *        Sends Gyro frame to the device
        * @param frame - Gyro frame
        *
        * @return Status
        */
        virtual Status SendFrame(IN const TrackingData::GyroFrame& frame) = 0;

        /**
        * @brief SendFrame
        *        Sends Velocimeter frame to the device
        * @param frame - Velocimeter frame
        *
        * @return Status
        */
        virtual Status SendFrame(IN const TrackingData::VelocimeterFrame& frame) = 0;

        /**
        * @brief SendFrame
        *        Sends Accelerometer frame to the device
        * @param frame - Accelerometer frame
        *
        * @return Status
        */
        virtual Status SendFrame(IN const TrackingData::AccelerometerFrame& frame) = 0;

        /**
        * @brief ControllerConnect
        *        Connect Controller to the device,
        *        If the device is already connected to the same MAC address, or if there are no controllers left to connect, libtm shall return a Status::DEVICE_BUSY
        *        Otherwise, libtm shall reply immediately, and the reply contains an assigned controller ID.
        *        This is not an indication that the connect succeeded, rather only that the connect started.
        *        libtm will later send a status of the connection on onControllerConnectedEventFrame.
        * @param device       - macAddress (Input) - MAC address of the controller to be connected 
        *                       timeout (Input) - Connect timeout (msec)
        * @param controllerId - controllerId (Output) - controllerId (1 or 2) assigned by the device  
        *
        * @return Status
        */
        virtual Status ControllerConnect(IN const TrackingData::ControllerDeviceConnect& device, OUT uint8_t& controllerId) = 0;

        /**
        * @brief ControllerDisconnect
        *        Disconnect Controller from the device
        *        Disconnection completion indication will be done upon onControllerDisconnectedEventFrame
        * @param controllerId - Controller identifier (1 or 2)
        *
        * @return Status
        */
        virtual Status ControllerDisconnect(IN uint8_t controllerId) = 0;

        /**
        * @brief ControllerStartCalibration
        *        Start the controller calibration process,
        *        When the controller receives the indication to start calibration, it starts reading its gyroscope output for 30 seconds.
        *        The controller averages the readings over the first 25 seconds and uses this value as the gyro bias.
        *        The controller then averages the readings over the next 5 seconds and compares the value to the gyro bias from the first 25 seconds.
        *        If the values are the same (up to TBD noise value), then the gyro bias is stored to flash and the controller sends the onControllerCalibrationEventFrame with success.
        *        Otherwise, the controller reports a failure.
        * @param controllerId - controllerId (Output) - controllerId (1 or 2)
        *
        * @return Status
        */
        virtual Status ControllerStartCalibration(IN uint8_t controllerId) = 0;

        /**
        * @brief GetAssociatedDevices
        *        Get Controller associated devices from the EEPROM 
        * @param devices - buffer to fill associated devices
        *
        * @return Status
        */
        virtual Status GetAssociatedDevices(OUT TrackingData::ControllerAssociatedDevices& devices) = 0;

        /**
        * @brief SetAssociatedDevices
        *        Set Controller associated devices to the EEPROM
        * @param devices - buffers to take associated devices
        *
        * @return Status
        */
        virtual Status SetAssociatedDevices(IN const TrackingData::ControllerAssociatedDevices& devices) = 0;

        /**
        * @brief ControllerSendData
        * Sends opaque data to the controller. The vendor's controller code is responsible for interpreting the data.
        * Example data could be a command to set a LED on the controller or set a vibration pattern on a haptic device.
        * Controller must be connected before sending data.
        * @param controllerData - data to send to the controller
        *
        * @return Status
        */
        virtual Status ControllerSendData(IN const TrackingData::ControllerData& controllerData) = 0;

        /**
        * @brief ControllerRssiTestControl
        * Start or Stop the RSSI test
        * @param controllerId - Controller identifier (1 or 2)
        * @param testControl - True = start the test, False = stop the test
        *
        * @return Status
        */
        virtual Status ControllerRssiTestControl(IN uint8_t controllerId, IN bool testControl) = 0;

        /**
        * @brief SetGpioControl
        * Enable manufacturing tools to directly control the following GPIOs - 74, 75, 76 & 77.
        * @param gpioControl - Setting each bit to 0 will toggle the corresponding GPIO low, Setting to 1 will toggle it to high.
        *                      Bit 0: GPIO 74, Bit 1: GPIO 75, Bit 2: GPIO 76, Bit 3: GPIO 77, Bits 4-7: Reserved
        *
        * @return Status
        */
        virtual Status SetGpioControl(IN uint8_t gpioControl) = 0;

        /**
        * @brief ControllerFWUpdate

        * Updates the FW image on a connected controller device.
        * @param ControllerFW - Controller FW image to update
        *
        * @return Status
        */
        virtual Status ControllerFWUpdate(const TrackingData::ControllerFW& fw) = 0;
    };
}
