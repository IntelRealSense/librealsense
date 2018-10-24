using System;
using System.Runtime.InteropServices;

namespace Intel.RealSense.Frames
{
    public class VideoFrame : Frame
    {
        internal static new readonly FramePool<VideoFrame> Pool;//TODO: Should be reimplemented as Threadsafe Pool.


        static VideoFrame()
        {
            Pool = new FramePool<VideoFrame>(ptr => new VideoFrame(ptr));
        }

        public int Width => NativeMethods.rs2_get_frame_width(Instance.Handle, out var error);
        public int Height => NativeMethods.rs2_get_frame_height(Instance.Handle, out var error);
        public int Stride => NativeMethods.rs2_get_frame_stride_in_bytes(Instance.Handle, out var error);

        public int BitsPerPixel => NativeMethods.rs2_get_frame_bits_per_pixel(Instance.Handle, out var error);

        public VideoFrame(IntPtr ptr) : base(ptr)
        {
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
                NativeMethods.memcpy(handle.AddrOfPinnedObject(), Data, Stride * Height);
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
            if (Instance.Handle != IntPtr.Zero)
                NativeMethods.rs2_release_frame(Instance.Handle);

            Instance = new HandleRef(this, IntPtr.Zero);
            Pool.Release(this); //Should be reimplemented as Threadsafe Pool.
        }
    }
}
