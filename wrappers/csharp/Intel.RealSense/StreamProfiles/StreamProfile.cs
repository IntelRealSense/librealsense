using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public class StreamProfile : Base.PooledObject
    {
        internal override void Initialize()
        {
            object error;
            NativeMethods.rs2_get_stream_profile_data(Handle, out _stream, out _format, out _index, out _uniqueId, out _framerate, out error);
            IsDefault = NativeMethods.rs2_is_stream_profile_default(Handle, out error) > 0;
        }

        internal StreamProfile(IntPtr ptr) : base(ptr, null)
        {
            this.Initialize();
        }

        internal Stream _stream;
        internal Format _format;
        internal int _framerate;
        internal int _index;
        internal int _uniqueId;

        public Stream Stream { get { return _stream; } }
        public Format Format { get { return _format; } }
        public int Framerate { get { return _framerate; } }
        public int Index { get { return _index; } }
        public int UniqueID { get { return _uniqueId; } }

        public bool IsDefault { get; private set; }

        public Extrinsics GetExtrinsicsTo(StreamProfile other)
        {
            object error;
            Extrinsics extrinsics;
            NativeMethods.rs2_get_extrinsics(Handle, other.Handle, out extrinsics, out error);
            return extrinsics;
        }

        public void RegisterExtrinsicsTo(StreamProfile other, Extrinsics extrinsics)
        {
            object error;
            NativeMethods.rs2_register_extrinsics(Handle, other.Handle, extrinsics, out error);
        }

        public bool Is(Extension e)
        {
            object error;
            return NativeMethods.rs2_stream_profile_is(Handle, e, out error) > 0;
        }

        public T As<T>() where T : StreamProfile
        {
            using (this)
            {
                return Create<T>(Handle);
            }
        }

        internal static T Create<T>(IntPtr ptr) where T : StreamProfile
        {
            return ObjectPool.Get<T>(ptr);
        }

        internal static StreamProfile Create(IntPtr ptr)
        {
            return Create<StreamProfile>(ptr);
        }
    }
}
