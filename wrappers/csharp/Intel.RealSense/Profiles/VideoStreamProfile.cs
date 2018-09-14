using Intel.RealSense.Types;
using System;

namespace Intel.RealSense.Profiles
{
    public class VideoStreamProfile : StreamProfile
    {
        public readonly int Width;
        public readonly int Height;

        public VideoStreamProfile(IntPtr ptr) : base(ptr) 
            => NativeMethods.rs2_get_video_stream_resolution(ptr, out Width, out Height, out var error);

        public Intrinsics GetIntrinsics()
        {
            NativeMethods.rs2_get_video_stream_intrinsics(Instance.Handle, out Intrinsics intrinsics, out var error);
            return intrinsics;
        }



    }
}
