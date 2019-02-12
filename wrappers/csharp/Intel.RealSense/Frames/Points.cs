using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public class Points : Frame
    {
        public static readonly new FramePool<Points> Pool = new FramePool<Points>(ptr => new Points(ptr));

        [System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Sequential)]
        public struct Vertex
        {
            public float x;
            public float y;
            public float z;
        }
        public struct TextureCoordinate
        {
            public float u;
            public float v;
        }

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

        /// <summary>
        /// Copy frame data to Vertex array
        /// </summary>
        /// <param name="array"></param>
        public void CopyTo(Vertex[] array)
        {
            if (array == null)
                throw new ArgumentNullException(nameof(array));
            var handle = GCHandle.Alloc(array, GCHandleType.Pinned);
            try
            {
                NativeMethods.memcpy(handle.AddrOfPinnedObject(), VertexData, Count * Marshal.SizeOf(typeof(Vertex)));
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
        /// Copy frame data to TextureCoordinate array
        /// </summary>
        /// <param name="textureArray"></param>
        public void CopyTo(TextureCoordinate[] textureArray)
        {
            if (textureArray == null)
                throw new ArgumentNullException(nameof(textureArray));

            var handle = GCHandle.Alloc(textureArray, GCHandleType.Pinned);
            try
            {
                var size = Count * Marshal.SizeOf(typeof(TextureCoordinate));
                NativeMethods.memcpy(handle.AddrOfPinnedObject(), TextureData, size);
            }
            finally
            {
                handle.Free();
            }
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
