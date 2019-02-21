using System;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    [StructLayout(LayoutKind.Sequential)]
    public class SoftwareMotionFrame
    {
        public IntPtr data;
        public frame_deleter deleter = delegate { };
        public double timestamp;
        public TimestampDomain domain;
        public int frame_number;
        public IntPtr  profile;
    }

}
