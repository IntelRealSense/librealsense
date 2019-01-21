using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    [System.Serializable]
    [StructLayout(LayoutKind.Sequential)]
    public class SoftwareVideoFrame
    {
        public IntPtr pixels;
        public frame_deleter deleter = delegate { };
        public int stride;
        public int bpp;
        public double timestamp;
        public TimestampDomain domain;
        public int frame_number;
        public IntPtr profile;
    }
}
