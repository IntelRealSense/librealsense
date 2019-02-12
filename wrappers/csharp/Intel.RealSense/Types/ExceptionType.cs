using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public enum ExceptionType
    {
        Unknown = 0,
        CameraDisconnected = 1,
        Backend = 2,
        InvalidValue = 3,
        WrongApiCallSequence = 4,
        NotImplemented = 5,
        DeviceInRecoveryMode = 6,
        Io = 7,
    }
}
