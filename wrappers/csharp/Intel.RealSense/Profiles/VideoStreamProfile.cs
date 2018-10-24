using Intel.RealSense.Types;
using System;

namespace Intel.RealSense.Profiles
{
    public class VideoStreamProfile : StreamProfile
    {
        public int Width => width;
        public int Height => height;
        internal int width;
        internal int height;

        public VideoStreamProfile(IntPtr ptr) : base(ptr)
            => NativeMethods.rs2_get_video_stream_resolution(ptr, out width, out height, out var error);

        public Intrinsics GetIntrinsics()
        {
            NativeMethods.rs2_get_video_stream_intrinsics(Instance.Handle, out Intrinsics intrinsics, out var error);
            return intrinsics;
        }



    }
}
