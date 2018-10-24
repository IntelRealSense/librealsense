using Intel.RealSense.Types;
using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

namespace Intel.RealSense.Profiles
{
    internal class ProfilePool<T> where T : StreamProfile
    {
        private readonly ConcurrentStack<T> stack;

        public ProfilePool()
        {
            stack = new ConcurrentStack<T>();
        }

        public T Get(IntPtr ptr)
        {
            bool isVideo = NativeMethods.rs2_stream_profile_is(ptr, Extension.VideoProfile, out var error) > 0;

            if (!stack.TryPop(out T profile))
                profile = (isVideo ? new VideoStreamProfile(ptr) : new StreamProfile(ptr)) as T;


            profile.Instance = new HandleRef(profile, ptr);
            profile.disposedValue = false;

            NativeMethods.rs2_get_stream_profile_data(ptr, out profile.stream, out profile.format, out profile.index, out profile.uniqueID, out profile.framerate, out error);

            if (isVideo)
            {
                var v = profile as VideoStreamProfile;
                NativeMethods.rs2_get_video_stream_resolution(ptr, out v.width, out v.height, out error);
                return v as T;
            }

            return profile;
        }

        public void Release(T t)
        {
            stack.Push(t);
        }
    }
}
