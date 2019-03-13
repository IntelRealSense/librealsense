using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    /// <summary>
    /// Inherit frame class with additional point cloud related attributs/functions
    /// </summary>
    public class Points : Frame
    {
        public Points(IntPtr ptr) : base(ptr)
        {
            object error;
            Count = NativeMethods.rs2_get_frame_points_count(Handle, out error);
        }

        internal override void Initialize()
        {
            object error;
            Count = NativeMethods.rs2_get_frame_points_count(Handle, out error);
        }

        public int Count { get; private set; }

        /// <summary>When called on Points frame type, this method returns a pointer to an array of 3D vertices of the model</summary>
        /// <remarks>
        /// The coordinate system is: X right, Y up, Z away from the camera. Units: Meters
        /// </remarks>
        /// <value>Pointer to an array of vertices, lifetime is managed by the frame</value>
        public IntPtr VertexData
        {
            get
            {
                object error;
                return NativeMethods.rs2_get_frame_vertices(Handle, out error);
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
        /// <param name="vertices">Array to copy into</param>
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

        /// <summary>When called on Points frame type, this method returns a pointer to an array of texture coordinates per vertex</summary>
        /// <remarks>
        /// Each coordinate represent a (u,v) pair within [0,1] range, to be mapped to texture image
        /// </remarks>
        /// <value>Pointer to an array of texture coordinates, lifetime is managed by the frame</value>
        public IntPtr TextureData
        {
            get
            {
                object error;
                return NativeMethods.rs2_get_frame_texture_coordinates(Handle, out error);
            }
        }

        /// <summary>
        /// Copy texture coordinates to managed array
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="textureArray">Array to copy into</param>
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
