// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Runtime.InteropServices;

    /// <summary>
    /// Inherit frame class with additional point cloud related attributs/functions
    /// </summary>
    public class Points : Frame
    {
        public Points(IntPtr ptr)
            : base(ptr)
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

        /// <summary>Gets a pointer to an array of 3D vertices of the model</summary>
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

        private void Copy<T>(IntPtr src, IntPtr dst)
        {
            Debug.Assert(src != IntPtr.Zero);
            Debug.Assert(dst != IntPtr.Zero);
            NativeMethods.Memcpy(dst, src, Count * Marshal.SizeOf(typeof(T)));
        }

        /// <summary>
        /// Copy vertex data to managed array
        /// </summary>
        /// <typeparam name="T">array element type</typeparam>
        /// <param name="vertices">Array to copy into</param>
        /// <exception cref="ArgumentNullException">Thrown when <paramref name="vertices"/> is null</exception>
        public void CopyVertices<T>(T[] vertices)
        {
            if (vertices == null)
            {
                throw new ArgumentNullException(nameof(vertices));
            }

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

        /// <summary>Gets a pointer to an array of texture coordinates per vertex</summary>
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
        /// <typeparam name="T">array element type</typeparam>
        /// <param name="textureArray">Array to copy into</param>
        /// <exception cref="ArgumentNullException">Thrown when <paramref name="textureArray"/> is null</exception>
        public void CopyTextureCoords<T>(T[] textureArray)
        {
            if (textureArray == null)
            {
                throw new ArgumentNullException(nameof(textureArray));
            }

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

        public void ExportToPLY(string fname, Frame texture)
        {
            object error;
            NativeMethods.rs2_export_to_ply(Handle, fname, texture.Handle, out error);
        }
    }
}
