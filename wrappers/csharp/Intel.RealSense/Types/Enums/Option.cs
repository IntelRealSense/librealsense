// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    /// <summary>
    /// Defines general configuration controls.
    /// <para>
    /// These can generally be mapped to camera UVC controls, and unless stated otherwise, can be set/queried at any time.
    /// </para>
    /// </summary>
    public enum Option
    {
        /// <summary>Enable / disable color backlight compensation</summary>
        BacklightCompensation = 0,

        /// <summary>Color image brightness</summary>
        Brightness = 1,

        /// <summary>Color image contrast</summary>
        Contrast = 2,

        /// <summary>Controls exposure time of color camera. Setting any value will disable auto exposure</summary>
        Exposure = 3,

        /// <summary>Color image gain</summary>
        Gain = 4,

        /// <summary>Color image gamma setting</summary>
        Gamma = 5,

        /// <summary>Color image hue</summary>
        Hue = 6,

        /// <summary>Color image saturation setting</summary>
        Saturation = 7,

        /// <summary>Color image sharpness setting</summary>
        Sharpness = 8,

        /// <summary>Controls white balance of color image. Setting any value will disable auto white balance</summary>
        WhiteBalance = 9,

        /// <summary>Enable / disable color image auto-exposure</summary>
        EnableAutoExposure = 10,

        /// <summary>Enable / disable color image auto-white-balance</summary>
        EnableAutoWhiteBalance = 11,

        /// <summary>Provide access to several recommend sets of option presets for the depth camera</summary>
        VisualPreset = 12,

        /// <summary>Power of the F200 / SR300 projector, with 0 meaning projector off</summary>
        LaserPower = 13,

        /// <summary>Set the number of patterns projected per frame. The higher the accuracy value the more patterns projected. Increasing the number of patterns help to achieve better accuracy. Note that this control is affecting the Depth FPS</summary>
        Accuracy = 14,

        /// <summary>Motion vs. Range trade-off, with lower values allowing for better motion sensitivity and higher values allowing for better depth range</summary>
        MotionRange = 15,

        /// <summary>Set the filter to apply to each depth frame. Each one of the filter is optimized per the application requirements</summary>
        FilterOption = 16,

        /// <summary>The confidence level threshold used by the Depth algorithm pipe to set whether a pixel will get a valid range or will be marked with invalid range</summary>
        ConfidenceThreshold = 17,

        /// <summary>Laser Emitter enabled</summary>
        EmitterEnabled = 18,

        /// <summary>Number of frames the user is allowed to keep per stream. Trying to hold-on to more frames will cause frame-drops.</summary>
        FramesQueueSize = 19,

        /// <summary>Total number of detected frame drops from all streams</summary>
        TotalFrameDrops = 20,

        /// <summary>Auto-Exposure modes: Static, Anti-Flicker and Hybrid</summary>
        AutoExposureMode = 21,

        /// <summary>Power Line Frequency control for anti-flickering Off/50Hz/60Hz/Auto</summary>
        PowerLineFrequency = 22,

        /// <summary>Current Asic Temperature</summary>
        AsicTemperature = 23,

        /// <summary>disable error handling</summary>
        ErrorPollingEnabled = 24,

        /// <summary>Current Projector Temperature</summary>
        ProjectorTemperature = 25,

        /// <summary>Enable / disable trigger to be outputed from the camera to any external device on every depth frame</summary>
        OutputTriggerEnabled = 26,

        /// <summary>Current Motion-Module Temperature</summary>
        MotionModuleTemperature = 27,

        /// <summary>Number of meters represented by a single depth unit</summary>
        DepthUnits = 28,

        /// <summary>Enable/Disable automatic correction of the motion data</summary>
        EnableMotionCorrection = 29,

        /// <summary>Allows sensor to dynamically ajust the frame rate depending on lighting conditions</summary>
        AutoExposurePriority = 30,

        /// <summary>Color scheme for data visualization</summary>
        ColorScheme = 31,

        /// <summary>Perform histogram equalization post-processing on the depth data</summary>
        HistogramEqualizationEnabled = 32,

        /// <summary>Minimal distance to the target</summary>
        MinDistance = 33,

        /// <summary>Maximum distance to the target</summary>
        MaxDistance = 34,

        /// <summary>Texture mapping stream unique ID</summary>
        TextureSource = 35,

        /// <summary>The 2D-filter effect. The specific interpretation is given within the context of the filter</summary>
        FilterMagnitude = 36,

        /// <summary>2D-filter parameter controls the weight/radius for smoothing.</summary>
        FilterSmoothAlpha = 37,

        /// <summary>2D-filter range/validity threshold</summary>
        FilterSmoothDelta = 38,

        /// <summary>Enhance depth data post-processing with holes filling where appropriate</summary>
        HolesFill = 39,

        /// <summary>The distance in mm between the first and the second imagers in stereo-based depth cameras</summary>
        StereoBaseline = 40,

        /// <summary>Allows dynamically ajust the converge step value of the target exposure in Auto-Exposure algorithm</summary>
        AutoExposureConvergeStep = 41,

        /// <summary>Impose Inter-camera HW synchronization mode. Applicable for D400/Rolling Shutter SKUs</summary>
        InterCamSyncMode = 42,

        /// <summary>Select a stream to process</summary>
        StreamFilter = 43,

        /// <summary>Select a stream format to process</summary>
        StreamFormatFilter = 44,

        /// <summary>Select a stream index to process</summary>
        StreamIndexFilter = 45,

        /// <summary>When supported, this option make the camera to switch the emitter state every frame. 0 for disabled, 1 for enabled</summary>
        EmitterOnOff = 46,

        /// <summary>Zero order point x</summary>
        ZeroOrderPointX = 47,

        /// <summary>Zero order point y</summary>
        ZeroOrderPointY = 48,

        /// <summary>LLD temperature</summary>
        LLDTemperature = 49,

        /// <summary>MC temperature</summary>
        MCTemperature = 50,

        /// <summary>MA temperature</summary>
        MATemperature = 51
    }
}
