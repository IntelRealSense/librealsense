// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    /// <summary>
    /// Severity of the librealsense logger
    /// </summary>
    public enum LogSeverity
    {
        /// <summary> Detailed information about ordinary operations</summary>
        Debug = 0,

        /// <summary> Terse information about ordinary operations</summary>
        Info = 1,

        /// <summary> Indication of possible failure</summary>
        Warn = 2,

        /// <summary> Indication of definite failure</summary>
        Error = 3,

        /// <summary> Indication of unrecoverable failure</summary>
        Fatal = 4,

        /// <summary> No logging will occur</summary>
        None = 5,
    }
}
