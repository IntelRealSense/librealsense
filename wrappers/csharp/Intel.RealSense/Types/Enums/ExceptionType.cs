// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    /// <summary>
    /// Exception types are the different categories of errors that RealSense API might return
    /// </summary>
    public enum ExceptionType
    {
        Unknown = 0,

        /// <summary> Device was disconnected, this can be caused by outside intervention, by internal firmware error or due to insufficient power</summary>
        CameraDisconnected = 1,

        /// <summary> Error was returned from the underlying OS-specific layer</summary>
        Backend = 2,

        /// <summary> Invalid value was passed to the API</summary>
        InvalidValue = 3,

        /// <summary> Function precondition was violated</summary>
        WrongApiCallSequence = 4,

        /// <summary> The method is not implemented at this point</summary>
        NotImplemented = 5,

        /// <summary> Device is in recovery mode and might require firmware update</summary>
        DeviceInRecoveryMode = 6,

        /// <summary> IO Device failure</summary>
        Io = 7,
    }
}
