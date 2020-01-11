// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Runtime.InteropServices;

    public class VideoFrame : Frame
    {
        public VideoFrame(IntPtr ptr)
            : base(ptr)
        {
            object error;
            Debug.Assert(NativeMethods.rs2_is_frame_extendable_to(ptr, Extension.VideoFrame, out error) > 0);
        }

        /// <summary>Gets the frame width in pixels</summary>
        /// <value>frame width in pixels</value>
        public int Width
        {
            get
            {
                object error;
                var w = NativeMethods.rs2_get_frame_width(Handle, out error);
                return w;
            }
        }

        /// <summary>Gets the frame height in pixels</summary>
        /// <value>frame height in pixels</value>
        public int Height
        {
            get
            {
                object error;
                var h = NativeMethods.rs2_get_frame_height(Handle, out error);
                return h;
            }
        }

        /// <summary>Gets the frame stride, meaning the actual line width in memory in bytes (not the logical image width)</summary>
        /// <value>stride in bytes</value>
        public int Stride
        {
            get
            {
                object error;
                var stride = NativeMethods.rs2_get_frame_stride_in_bytes(Handle, out error);
                return stride;
            }
        }

        /// <summary>Gets the bits per pixels in the frame image</summary>
        /// <remarks>
        /// (note that bits per pixel is not necessarily divided by 8, as in 12bpp)
        /// </remarks>
        /// <value>bits per pixel</value>
        public int BitsPerPixel
        {
            get
            {
                object error;
                var bpp = NativeMethods.rs2_get_frame_bits_per_pixel(Handle, out error);
                return bpp;
            }
        }

        /// <summary>
        /// Copy frame data to managed typed array
        /// </summary>
        /// <typeparam name="T">array element type</typeparam>
        /// <param name="array">array to copy to</param>
        /// <exception cref="ArgumentNullException">Thrown when <paramref name="array"/> is null</exception>
        public void CopyTo<T>(T[] array)
        {
            if (array == null)
            {
                throw new ArgumentNullException(nameof(array));
            }

            var handle = GCHandle.Alloc(array, GCHandleType.Pinned);
            try
            {
                // System.Diagnostics.Debug.Assert((array.Length * Marshal.SizeOf(typeof(T))) == (Stride * Height));
                CopyTo(handle.AddrOfPinnedObject());
            }
            finally
            {
                handle.Free();
            }
        }

        /// <summary>
        /// Copy frame data to pointer
        /// </summary>
        /// <param name="ptr">destination pointer</param>
        /// <exception cref="ArgumentNullException">Thrown when <paramref name="ptr"/> is null</exception>
        public void CopyTo(IntPtr ptr)
        {
            if (ptr == IntPtr.Zero)
            {
                throw new ArgumentNullException(nameof(ptr));
            }

            NativeMethods.Memcpy(ptr, Data, Stride * Height);
        }

        /// <summary>
        /// Copy data from managed array
        /// </summary>
        /// <typeparam name="T">array element type</typeparam>
        /// <param name="array">array to copy from</param>
        /// <exception cref="ArgumentNullException">Thrown when <paramref name="array"/> is null</exception>
        public void CopyFrom<T>(T[] array)
        {
            if (array == null)
            {
                throw new ArgumentNullException(nameof(array));
            }

            var handle = GCHandle.Alloc(array, GCHandleType.Pinned);
            try
            {
                // System.Diagnostics.Debug.Assert((array.Length * Marshal.SizeOf(typeof(T))) == (Stride * Height));
                CopyFrom(handle.AddrOfPinnedObject());
            }
            finally
            {
                handle.Free();
            }
        }

        /// <summary>
        /// Copy data from pointer
        /// </summary>
        /// <param name="ptr">source pointer</param>
        /// <exception cref="ArgumentNullException">Thrown when <paramref name="ptr"/> is null</exception>
        public void CopyFrom(IntPtr ptr)
        {
            if (ptr == IntPtr.Zero)
            {
                throw new ArgumentNullException(nameof(ptr));
            }

            NativeMethods.Memcpy(Data, ptr, Stride * Height);
        }
    }
}
