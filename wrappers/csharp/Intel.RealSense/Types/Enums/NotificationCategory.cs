// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Collections.Generic;
    using System.Runtime.InteropServices;

    /// <summary>
    /// Category of the librealsense notifications
    /// </summary>
    public enum NotificationCategory
    {
        /// <summary> Frames didn't arrived within 5 seconds</summary>
        FramesTimeout = 0,

        /// <summary> Received partial/incomplete frame</summary>
        FrameCorrupted = 1,

        /// <summary> Error reported from the device</summary>
        HardwareError = 2,

        /// <summary> General Hardeware notification that is not an error</summary>
        HardwareEvent = 3,

        /// <summary> Received unknown error from the device</summary>
        UnknownError = 4,

        /// <summary> Current firmware version installed is not the latest available</summary>
        FirmwareUpdateRecommended = 5,

        /// <summary> A relocalization event has updated the pose provided by a pose sensor</summary>
        PoseRelocalization = 6,
    }
}
