using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public class DepthFrame : VideoFrame
    {
        public static readonly new FramePool<DepthFrame> Pool = new FramePool<DepthFrame>(ptr => new DepthFrame(ptr));

        public DepthFrame(IntPtr ptr) : base(ptr)
        {
        }

        public float GetDistance(int x, int y)
        {
            object error;
            return NativeMethods.rs2_depth_frame_get_distance(m_instance.Handle, x, y, out error);
        }

        public override void Release()
        {
            //base.Release();
            if (m_instance.Handle != IntPtr.Zero)
                NativeMethods.rs2_release_frame(m_instance.Handle);
            m_instance = new HandleRef(this, IntPtr.Zero);
            Pool.Release(this);
        }
    }
}
