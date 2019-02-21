using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public enum Distortion
    {
        None = 0,
        ModifiedBrownConrady = 1,
        InverseBrownConrady = 2,
        Ftheta = 3,
        BrownConrady = 4,
    }
}