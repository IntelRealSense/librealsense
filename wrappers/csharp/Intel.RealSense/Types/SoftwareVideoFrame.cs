using System;
using System.Runtime.InteropServices;

namespace Intel.RealSense.Types
{
    [Serializable]
    [StructLayout(LayoutKind.Sequential)]
    public class SoftwareVideoFrame
    {
        public IntPtr pixels;
        public Deleter deleter;
        public int stride;
        public int bpp;
        public double timestamp;
        public TimestampDomain domain;
        public int frame_number;
        public IntPtr profile;

        public SoftwareVideoFrame()
        {
            deleter = delegate { };
        }
    }
}
