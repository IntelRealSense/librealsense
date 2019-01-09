using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public class ProfilePool<T> where T : StreamProfile
    {
        readonly Stack<T> stack = new Stack<T>();
        readonly object locker = new object();

        public T Get(IntPtr ptr)
        {

            object error;
            bool isVideo = NativeMethods.rs2_stream_profile_is(ptr, Extension.VideoProfile, out error) > 0;

            T p;
            lock (locker)
            {
                if (stack.Count != 0)
                    p = stack.Pop();
                else
                    p = (isVideo  ? new VideoStreamProfile(ptr) : new StreamProfile(ptr)) as T;
            }

            p.m_instance = new HandleRef(p, ptr);
            p.disposedValue = false;

            NativeMethods.rs2_get_stream_profile_data(ptr, out p._stream, out p._format, out p._index, out p._uniqueId, out p._framerate, out error);

            if(isVideo)
            {
                var v = p as VideoStreamProfile;
                NativeMethods.rs2_get_video_stream_resolution(ptr, out v.width, out v.height, out error);
                return v as T;
            }

            return p;
        }

        public void Release(T t)
        {
            lock (locker)
            {
                stack.Push(t);
            }
        }
    }
}
