using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public class DisparityFrame : DepthFrame
    {
        public DisparityFrame(IntPtr ptr) : base(ptr)
        {
        }

        public float Baseline
        {
            get
            {
                object error;
                return NativeMethods.rs2_depth_stereo_frame_get_baseline(m_instance.Handle, out error);
            }
        }
    }
}
