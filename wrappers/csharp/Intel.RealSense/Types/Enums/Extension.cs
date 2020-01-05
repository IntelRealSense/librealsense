// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    /// <summary>
    /// Specifies advanced interfaces (capabilities) objects may implement
    /// </summary>
    public enum Extension
    {
        Unknown = 0,
        Debug = 1,
        Info = 2,
        Motion = 3,
        Options = 4,
        Video = 5,
        Roi = 6,
        DepthSensor = 7,
        VideoFrame = 8,
        MotionFrame = 9,
        CompositeFrame = 10,
        Points = 11,
        DepthFrame = 12,
        AdvancedMode = 13,
        Record = 14,
        VideoProfile = 15,
        Playback = 16,
        DepthStereoSensor = 17,
        DisparityFrame = 18,
        MotionProfile = 19,
        PoseFrame = 20,
        PoseProfile = 21,
        Tm2 = 22,
        SoftwareDevice = 23,
        SoftwareSensor = 24,
        DecimationFilter = 25,
        ThresholdFilter = 26,
        DisparityFilter = 27,
        SpatialFilter = 28,
        TemporalFilter = 29,
        HoleFillingFilter = 30,
        ZeroOrderFilter = 31,
        RecommendedFilters = 32,
        Pose = 33,
        PoseSensor = 34,
        WheelOdometer = 35,
        GlobalTimer = 36,
        Updatable = 37,
        UpdateDevice = 38,
        L500DepthSensor = 39,
        TM2Sensor = 40,
        AutoCalibratedDevice = 41,
        ColorSensor = 42,
        MotionSensor = 43,
        FisheyeSensor = 44,
        DepthHuffmanDecoder = 45
    }
}
