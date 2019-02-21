using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;

namespace Intel.RealSense
{
    [System.Serializable]
    [StructLayout(LayoutKind.Sequential)]
    public struct SoftwareMotionStream
    {
        public Stream type;
        public int index;
        public int uid;
        public int fps;
        public Format fmt;
        public MotionDeviceIntrinsics intrinsics;
    }
}
