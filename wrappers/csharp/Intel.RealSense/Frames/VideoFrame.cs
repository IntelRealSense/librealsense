using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public class VideoFrame : Frame
    {
        public static readonly new FramePool<VideoFrame> Pool = new FramePool<VideoFrame>(ptr => new VideoFrame(ptr));

        public VideoFrame(IntPtr ptr) : base(ptr)
        {
        }

        public int Width
        {
            get
            {
                object error;
                var w = NativeMethods.rs2_get_frame_width(m_instance.Handle, out error);
                return w;
            }
        }

        public int Height
        {
            get
            {
                object error;
                var h = NativeMethods.rs2_get_frame_height(m_instance.Handle, out error);
                return h;
            }
        }

        public int Stride
        {
            get
            {
                object error;
                var stride = NativeMethods.rs2_get_frame_stride_in_bytes(m_instance.Handle, out error);
                return stride;
            }
        }

        public int BitsPerPixel
        {
            get
            {
                object error;
                var bpp = NativeMethods.rs2_get_frame_bits_per_pixel(m_instance.Handle, out error);
                return bpp;
            }
        }

        /// <summary>
        /// Copy frame data to managed typed array
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="array"></param>
        public void CopyTo<T>(T[] array)
        {
            if (array == null)
                throw new ArgumentNullException(nameof(array));
            var handle = GCHandle.Alloc(array, GCHandleType.Pinned);
            try
            {
                //System.Diagnostics.Debug.Assert((array.Length * Marshal.SizeOf(typeof(T))) == (Stride * Height));
                CopyTo(handle.AddrOfPinnedObject());
            }
            finally
            {
                handle.Free();
            }
        }

        public void CopyTo(IntPtr ptr)
        {
            //System.Diagnostics.Debug.Assert(ptr != IntPtr.Zero);
            NativeMethods.memcpy(ptr, Data, Stride * Height);
        }

        /// <summary>
        /// Copy from data from managed array
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="array"></param>
        public void CopyFrom<T>(T[] array)
        {
            if (array == null)
                throw new ArgumentNullException(nameof(array));
            var handle = GCHandle.Alloc(array, GCHandleType.Pinned);
            try
            {
                //System.Diagnostics.Debug.Assert((array.Length * Marshal.SizeOf(typeof(T))) == (Stride * Height));
                CopyFrom(handle.AddrOfPinnedObject());
            }
            finally
            {
                handle.Free();
            }
        }

        public void CopyFrom(IntPtr ptr)
        {
            //System.Diagnostics.Debug.Assert(ptr != IntPtr.Zero);
            NativeMethods.memcpy(Data, ptr, Stride * Height);
        }

        public override void Release()
        {
            //base.Release();
            if (m_instance.Handle != IntPtr.Zero)
                NativeMethods.rs2_release_frame(m_instance.Handle);
            m_instance = new HandleRef(this, IntPtr.Zero);
            Pool.Release(this);
        }
    }
}
