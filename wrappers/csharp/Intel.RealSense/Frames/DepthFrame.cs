using System;

namespace Intel.RealSense.Frames
{
    public class DepthFrame : VideoFrame
    {
        public DepthFrame(IntPtr ptr) : base(ptr)
        {
        }

        public float GetDistance(int x, int y) 
            => NativeMethods.rs2_depth_frame_get_distance(Instance.Handle, x, y, out var error);
    }
}
