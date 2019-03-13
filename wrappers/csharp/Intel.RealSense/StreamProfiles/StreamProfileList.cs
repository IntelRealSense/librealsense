using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public class StreamProfileList : Base.Object, IEnumerable<StreamProfile>
    {
        public StreamProfileList(IntPtr ptr) : base(ptr, NativeMethods.rs2_delete_stream_profiles_list)
        {
        }

        public IEnumerator<StreamProfile> GetEnumerator()
        {
            object error;

            int deviceCount = NativeMethods.rs2_get_stream_profiles_count(Handle, out error);
            for (int i = 0; i < deviceCount; i++)
            {
                var ptr = NativeMethods.rs2_get_stream_profile(Handle, i, out error);
                yield return StreamProfile.Create(ptr);
            }
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }


        /// <summary>get the number of supported stream profiles</summary>
        /// <value>number of supported subdevice profiles</value>
        public int Count
        {
            get
            {
                object error;
                int deviceCount = NativeMethods.rs2_get_stream_profiles_count(Handle, out error);
                return deviceCount;
            }
        }

        public T GetProfile<T>(int index) where T : StreamProfile
        {
            object error;
            return StreamProfile.Create<T>(NativeMethods.rs2_get_stream_profile(Handle, index, out error));
        }

        public StreamProfile this[int index]
        {
            get
            {
                return GetProfile<StreamProfile>(index);
            }
        }
    }
}
