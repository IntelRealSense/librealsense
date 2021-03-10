// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    /// <summary>
    /// Streams are different types of data provided by RealSense devices
    /// </summary>
    public enum Stream
    {
        Any = 0,

        /// <summary>Native stream of depth data produced by RealSense device</summary>
        Depth = 1,

        /// <summary>Native stream of color data captured by RealSense device</summary>
        Color = 2,

        /// <summary>Native stream of infrared data captured by RealSense device</summary>
        Infrared = 3,

        /// <summary>Native stream of fish-eye (wide) data captured from the dedicate motion camera</summary>
        Fisheye = 4,

        /// <summary>Native stream of gyroscope motion data produced by RealSense device</summary>
        Gyro = 5,

        /// <summary>Native stream of accelerometer motion data produced by RealSense device</summary>
        Accel = 6,

        /// <summary>Signals from external device connected through GPIO</summary>
        Gpio = 7,

        /// <summary>6 Degrees of Freedom pose data, calculated by RealSense device</summary>
        Pose = 8,
        Confidence = 9,
    }
}
