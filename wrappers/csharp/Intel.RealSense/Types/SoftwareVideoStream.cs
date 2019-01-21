using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    [System.Serializable]
    [StructLayout(LayoutKind.Sequential)]
    public struct SoftwareVideoStream
    {
        public Stream type;
        public int index;
        public int uid;
        public int width;
        public int height;
        public int fps;
        public int bpp;
        public Format format;
        public Intrinsics intrinsics;
    }
}
