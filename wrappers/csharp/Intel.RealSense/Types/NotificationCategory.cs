using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public enum NotificationCategory
    {
        FramesTimeout = 0,
        FrameCorrupted = 1,
        HardwareError = 2,
        UnknownError = 3,
    }
}
