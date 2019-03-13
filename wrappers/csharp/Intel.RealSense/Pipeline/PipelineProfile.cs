using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public class PipelineProfile : Base.Object
    {

        public PipelineProfile(IntPtr ptr) : base(ptr, NativeMethods.rs2_delete_pipeline_profile)
        {
        }

        public Device Device
        {
            get
            {
                object error;
                var ptr = NativeMethods.rs2_pipeline_profile_get_device(Handle, out error);
                return Device.Create<Device>(ptr, NativeMethods.rs2_delete_device);
            }
        }

        public StreamProfileList Streams
        {
            get
            {
                object error;
                var ptr = NativeMethods.rs2_pipeline_profile_get_streams(Handle, out error);
                return new StreamProfileList(ptr);
            }
        }

        public StreamProfile GetStream(Stream s, int index = -1)
        {
            return GetStream<StreamProfile>(s, index);
        }

        public T GetStream<T>(Stream s, int index = -1) where T : StreamProfile
        {
            using (var streams = Streams)
            {
                object error;
                int count = streams.Count;
                for (int i = 0; i < count; i++)
                {
                    var ptr = NativeMethods.rs2_get_stream_profile(streams.Handle, i, out error);
                    var t = StreamProfile.Create<T>(ptr);
                    if (t.Stream == s && (index == -1 || t.Index == index))
                        return t;
                    t.Dispose();
                }
                return null;
            }
        }
    }
}
