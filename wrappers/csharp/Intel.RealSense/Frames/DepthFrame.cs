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

        /// <summary>Given the 2D depth coordinate (x,y) provide the corresponding depth in metric units</summary>
        public float GetDistance(int x, int y)
        {
            object error;
            return NativeMethods.rs2_depth_frame_get_distance(Handle, x, y, out error);
        }
    }
}
