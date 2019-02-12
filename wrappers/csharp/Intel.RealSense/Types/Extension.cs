using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
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
    }
}
