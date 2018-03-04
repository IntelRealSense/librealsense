using System;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public class Frame : IDisposable
    {
        internal HandleRef m_instance;

        public Frame(IntPtr ptr)
        {
            m_instance = new HandleRef(this, ptr);
            NativeMethods.rs2_keep_frame(m_instance.Handle);
        }

        #region IDisposable Support
        private bool disposedValue = false; // To detect redundant calls

        protected virtual void Dispose(bool disposing)
        {
            if (!disposedValue)
            {
                if (disposing)
                {
                    // TODO: dispose managed state (managed objects).
                }

                // TODO: free unmanaged resources (unmanaged objects) and override a finalizer below.
                // TODO: set large fields to null.
                Release();
                disposedValue = true;
            }
        }

        // TODO: override a finalizer only if Dispose(bool disposing) above has code to free unmanaged resources.
        ~Frame()
        {
            // Do not change this code. Put cleanup code in Dispose(bool disposing) above.
            Dispose(false);
        }

        // This code added to correctly implement the disposable pattern.
        public void Dispose()
        {
            // Do not change this code. Put cleanup code in Dispose(bool disposing) above.
            Dispose(true);
            // TODO: uncomment the following line if the finalizer is overridden above.
            GC.SuppressFinalize(this);
        }
        #endregion

        public void Release()
        {
            if (m_instance.Handle != IntPtr.Zero)
                NativeMethods.rs2_release_frame(m_instance.Handle);
            m_instance = new HandleRef(this, IntPtr.Zero);
        }

        public Frame Clone()
        {
            object error;
            NativeMethods.rs2_frame_add_ref(m_instance.Handle, out error);
            return new Frame(m_instance.Handle);
        }

        public IntPtr Data
        {
            get
            {
                object error;
                return NativeMethods.rs2_get_frame_data(m_instance.Handle, out error);
            }
        }

        public StreamProfile Profile
        {
            get
            {
                object error;
                return new StreamProfile(NativeMethods.rs2_get_frame_stream_profile(m_instance.Handle, out error));
            }
        }

        public ulong Number
        {
            get
            {
                object error;
                var frameNumber = NativeMethods.rs2_get_frame_number(m_instance.Handle, out error);
                return frameNumber;
            }
        }

        public double Timestamp
        {
            get
            {
                object error;
                var timestamp = NativeMethods.rs2_get_frame_timestamp(m_instance.Handle, out error);
                return timestamp;
            }
        }


        public TimestampDomain TimestampDomain
        {
            get
            {
                object error;
                var timestampDomain = NativeMethods.rs2_get_frame_timestamp_domain(m_instance.Handle, out error);
                return timestampDomain;
            }
        }
    }

    public class VideoFrame : Frame
    {
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

    public class DepthFrame : VideoFrame
    {
        public DepthFrame(IntPtr ptr) : base(ptr)
        {
        }

        public float GetDistance(int x, int y)
        {
            object error;
            return NativeMethods.rs2_depth_frame_get_distance(m_instance.Handle, x, y, out error);
        }
    }


    public class Points : Frame
    {
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

        private IntPtr VertexData
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
                throw new ArgumentNullException("array");
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

        private IntPtr TextureData
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
                throw new ArgumentNullException("textureArray");

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
    }



    class FrameMarshaler : ICustomMarshaler
    {
        private static FrameMarshaler Instance;

        public static ICustomMarshaler GetInstance(string s)
        {
            if (Instance == null)
            {
                Instance = new FrameMarshaler();
            }
            return Instance;
        }

        public void CleanUpManagedData(object ManagedObj)
        {
        }

        public void CleanUpNativeData(IntPtr pNativeData)
        {
        }

        public int GetNativeDataSize()
        {
            //return IntPtr.Size;
            return -1;
        }

        public IntPtr MarshalManagedToNative(object ManagedObj)
        {
            throw new NotImplementedException();
        }

        public object MarshalNativeToManaged(IntPtr pNativeData)
        {
            return new Frame(pNativeData);
        }
    }
}
