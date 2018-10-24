using System;
using System.Runtime.InteropServices;

namespace Intel.RealSense.Frames
{
    public class DepthFrame : VideoFrame
    {
        internal static readonly new FramePool<DepthFrame> Pool; //Should be reimplemented as Threadsafe Pool.

        static DepthFrame()
        {
            Pool = new FramePool<DepthFrame>(ptr => new DepthFrame(ptr));
        }

        public DepthFrame(IntPtr ptr) : base(ptr)
        {
        }

        public float GetDistance(int x, int y) 
            => NativeMethods.rs2_depth_frame_get_distance(Instance.Handle, x, y, out var error);

        public override void Release()
        {
            if (Instance.Handle != IntPtr.Zero)
                NativeMethods.rs2_release_frame(Instance.Handle);

            Instance = new HandleRef(this, IntPtr.Zero);
            Pool.Release(this); //Should be reimplemented as Threadsafe Pool.
        }
    }
}
