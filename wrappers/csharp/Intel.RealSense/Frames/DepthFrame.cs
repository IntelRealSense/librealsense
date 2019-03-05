using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public class DepthFrame : VideoFrame
    {
        public DepthFrame(IntPtr ptr) : base(ptr)
        {
        }

        public float GetDistance(int x, int y)
        {
            object error;
            return NativeMethods.rs2_depth_frame_get_distance(m_instance.Handle, x, y, out error);
        }
    }
}
