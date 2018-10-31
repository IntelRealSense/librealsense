using Intel.RealSense.Types;
using System;
#if NET35
/*
 * ---------------------------------------------------------------------
 * This code block is only for legacy Unity (version < 2018.1) support.
 * ---------------------------------------------------------------------
 */
#else
    using System.Collections.Concurrent;
#endif
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

namespace Intel.RealSense.Profiles
{
    internal class ProfilePool<T> where T : StreamProfile
    {
#if NET35
        /*
         * ---------------------------------------------------------------------
         * This code block is only for legacy Unity (version < 2018.1) support.
         * ---------------------------------------------------------------------
         */
        private readonly Stack<T> stack;
        private readonly object stackLock;
#else
        private readonly ConcurrentStack<T> stack;
#endif

        public ProfilePool()
        {
#if NET35
            /*
             * ---------------------------------------------------------------------
             * This code block is only for legacy Unity (version < 2018.1) support.
             * ---------------------------------------------------------------------
             */
            stack = new Stack<T>();
            stackLock = new object();
#else
            stack = new ConcurrentStack<T>();
#endif
        }
#if NET35
        /*
         * ---------------------------------------------------------------------
         * This code block is only for legacy Unity (version < 2018.1) support.
         * ---------------------------------------------------------------------
         */
        public T Get(IntPtr ptr)
        {
            bool isVideo = NativeMethods.rs2_stream_profile_is(ptr, Extension.VideoProfile, out var error) > 0;

            int count;

            lock (stackLock)
            {
                count = stack.Count;
            }

            T profile;

            if (count > 0)
            {
                lock (stackLock)
                {
                    profile = stack.Pop();
                }
            }
            else
            {
                profile = (isVideo ? new VideoStreamProfile(ptr) : new StreamProfile(ptr)) as T;
            }
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
            lock (stackLock)
            {
                stack.Push(t);
            }
        }
#else
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
#endif
    }
}
