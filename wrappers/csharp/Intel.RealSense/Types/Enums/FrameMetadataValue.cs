// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    /// <summary>
    /// Per-Frame-Metadata are set of read-only properties that might be exposed for each individual frame
    /// </summary>
    public enum FrameMetadataValue
    {
        /// <summary>A sequential index managed per-stream. Integer value</summary>
        FrameCounter = 0,

        /// <summary>Timestamp set by device clock when data readout and transmit commence. usec</summary>
        FrameTimestamp = 1,

        /// <summary>Timestamp of the middle of sensor's exposure calculated by device. usec</summary>
        SensorTimestamp = 2,

        /// <summary>Sensor's exposure width. When Auto Exposure (AE) is on the value is controlled by firmware. usec</summary>
        ActualExposure = 3,

        /// <summary>A relative value increasing which will increase the Sensor's gain factor. \ When AE is set On, the value is controlled by firmware. Integer value</summary>
        GainLevel = 4,

        /// <summary>Auto Exposure Mode indicator. Zero corresponds to AE switched off.</summary>
        AutoExposure = 5,

        /// <summary>White Balance setting as a color temperature. Kelvin degrees</summary>
        WhiteBalance = 6,

        /// <summary>Time of arrival in system clock</summary>
        TimeOfArrival = 7,

        /// <summary>Temperature of the device, measured at the time of the frame capture. Celsius degrees</summary>
        Temperature = 8,

        /// <summary>Timestamp get from uvc driver. usec</summary>
        BackendTimestamp = 9,

        /// <summary>Actual fps</summary>
        ActualFps = 10,

        /// <summary>Laser power value 0-360.</summary>
        FrameLaserPower = 11,

        /// <summary>Laser power mode. Zero corresponds to Laser power switched off and one for switched on.</summary>
        FrameLaserPowerMode = 12,

        /// <summary>Exposure priority.</summary>
        ExposurePriority = 13,

        /// <summary>Left region of interest for the auto exposure Algorithm.</summary>
        ExposureRoiLeft = 14,

        /// <summary>Right region of interest for the auto exposure Algorithm.</summary>
        ExposureRoiRight = 15,

        /// <summary>Top region of interest for the auto exposure Algorithm.</summary>
        ExposureRoiTop = 16,

        /// <summary>Bottom region of interest for the auto exposure Algorithm.</summary>
        ExposureRoiBottom = 17,

        /// <summary>Color image brightness.</summary>
        Brightness = 18,

        /// <summary>Color image contrast.</summary>
        Contrast = 19,

        /// <summary>Color image saturation.</summary>
        Saturation = 20,

        /// <summary>Color image sharpness.</summary>
        Sharpness = 21,

        /// <summary>Auto white balance temperature Mode indicator. Zero corresponds to automatic mode switched off.</summary>
        AutoWhiteBalanceTemperature = 22,

        /// <summary>Color backlight compensation. Zero corresponds to switched off.</summary>
        BacklightCompensation = 23,

        /// <summary>Color image hue.</summary>
        Hue = 24,

        /// <summary>Color image gamma.</summary>
        Gamma = 25,

        /// <summary>Color image white balance.</summary>
        ManualWhiteBalance = 26,

        /// <summary>Power Line Frequency for anti-flickering Off/50Hz/60Hz/Auto.</summary>
        PowerLineFrequency = 27,

        /// <summary>Color lowlight compensation. Zero corresponds to switched off.</summary>
        LowLightCompensation = 28,

        /// <summary>Emitter mode: 0 – all emitters disabled. 1 – laser enabled. 2 – auto laser enabled (opt). 3 – LED enabled (opt).</summary>
        FrameEmitterMode = 29,

        /// <summary>Led power value 0-360.</summary>
        FrameLedPower = 30,

        /// <summary>The number of transmitted payload bytes, not including metadata.</summary>
        RawFrameSizeBytes = 31,
    }
}
