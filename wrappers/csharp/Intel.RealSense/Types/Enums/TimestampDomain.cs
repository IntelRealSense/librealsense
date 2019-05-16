// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    /// <summary>
    /// Specifies the clock in relation to which the frame timestamp was measured.
    /// </summary>
    public enum TimestampDomain
    {
        /// <summary> Frame timestamp was measured in relation to the camera clock</summary>
        HardwareClock = 0,

        /// <summary> Frame timestamp was measured in relation to the OS system clock</summary>
        SystemTime = 1,

        GlobalTime = 2,
    }
}
