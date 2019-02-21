using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public class Points : Frame
    {
        public Points(IntPtr ptr) : base(ptr)
        {
        }

        public int Count
        {
            get
            {
                object error;
                var h = NativeMethods.rs2_get_frame_points_count(m_instance.Handle, out error);
                return h;
            }
        }

        public IntPtr VertexData
        {
            get
            {
                object error;
                return NativeMethods.rs2_get_frame_vertices(m_instance.Handle, out error);
            }
        }


        void Copy<T>(IntPtr src, IntPtr dst)
        {
            Debug.Assert(src != IntPtr.Zero);
            Debug.Assert(dst != IntPtr.Zero);
            NativeMethods.memcpy(dst, src, Count * Marshal.SizeOf(typeof(T)));
        }

        /// <summary>
        /// Copy vertex data to managed array
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="vertices">Array of size <see cref="Count">Count</see> * 3 * sizeof(float)</param>
        public void CopyVertices<T>(T[] vertices)
        {
            if (vertices == null)
                throw new ArgumentNullException(nameof(vertices));
            Debug.Assert(vertices.Length * Marshal.SizeOf(typeof(T)) == Count * 3 * sizeof(float));
            var handle = GCHandle.Alloc(vertices, GCHandleType.Pinned);
            try
            {
                Copy<T>(VertexData, handle.AddrOfPinnedObject());
            }
            finally
            {
                handle.Free();
            }
        }

        public IntPtr TextureData
        {
            get
            {
                object error;
                return NativeMethods.rs2_get_frame_texture_coordinates(m_instance.Handle, out error);
            }
        }

        /// <summary>
        /// Copy texture coordinates to managed array
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="textureArray">Array of size <see cref="Count">Count</see> * 2 * sizeof(float)</param>
        public void CopyTextureCoords<T>(T[] textureArray)
        {
            if (textureArray == null)
                throw new ArgumentNullException(nameof(textureArray));
            Debug.Assert(textureArray.Length * Marshal.SizeOf(typeof(T)) == Count * 2 * sizeof(float));
            var handle = GCHandle.Alloc(textureArray, GCHandleType.Pinned);
            try
            {
                Copy<T>(TextureData, handle.AddrOfPinnedObject());
            }
            finally
            {
                handle.Free();
            }
        }
    }
}
