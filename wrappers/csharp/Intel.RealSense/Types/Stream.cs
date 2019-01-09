using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public enum Stream
    {
        Any = 0,
        Depth = 1,
        Color = 2,
        Infrared = 3,
        Fisheye = 4,
        Gyro = 5,
        Accel = 6,
        Gpio = 7,
        Pose = 8,
        Confidence = 9,
    }
}
