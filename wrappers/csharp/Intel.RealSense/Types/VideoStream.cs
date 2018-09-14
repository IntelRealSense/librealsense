﻿using System;
using System.Runtime.InteropServices;

namespace Intel.RealSense.Types
{
    [Serializable]
    [StructLayout(LayoutKind.Sequential)]
    public struct VideoStream
    {
        public Stream type;
        public int index;
        public int uid;
        public int width;
        public int height;
        public int fps;
        public int bpp;
        public Format fmt;
        public Intrinsics intrinsics;
    }
}
