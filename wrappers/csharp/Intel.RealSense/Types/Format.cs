using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public enum Format
    {
        Any = 0,
        Z16 = 1,
        Disparity16 = 2,
        Xyz32f = 3,
        Yuyv = 4,
        Rgb8 = 5,
        Bgr8 = 6,
        Rgba8 = 7,
        Bgra8 = 8,
        Y8 = 9,
        Y16 = 10,
        Raw10 = 11,
        Raw16 = 12,
        Raw8 = 13,
        Uyvy = 14,
        MotionRaw = 15,
        MotionXyz32f = 16,
        GpioRaw = 17,
        SixDOF = 18,
        Disparity32 = 19,
    }
}
