using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public class VideoStreamProfile : StreamProfile
    {
        public VideoStreamProfile(IntPtr ptr) : base(ptr)
        {
            object error;
            NativeMethods.rs2_get_video_stream_resolution(ptr, out width, out height, out error);
        }

        public Intrinsics GetIntrinsics()
        {
            object error;
            Intrinsics intrinsics;
            NativeMethods.rs2_get_video_stream_intrinsics(m_instance.Handle, out intrinsics, out error);
            return intrinsics;
        }

        public int Width { get { return width; } }

        public int Height { get { return height; } }

        internal int width;
        internal int height;
    }
}
