using System;
using System.Runtime.InteropServices;

namespace Intel.RealSense.Frames
{
    public class VideoFrame : Frame
    {
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
        //public void CopyTo<T>(out T[] array)
        public void CopyTo<T>(T[] array)
        {
            if (array == null)
                throw new ArgumentNullException("array");

            var handle = GCHandle.Alloc(array, GCHandleType.Pinned);
            try
            {
                //System.Diagnostics.Debug.Assert((array.Length * Marshal.SizeOf(typeof(T))) == (Stride * Height));
                NativeMethods.memcpy(handle.AddrOfPinnedObject(), Data, Stride * Height);
            }
            finally
            {
                handle.Free();
            }
        }
    }
}
