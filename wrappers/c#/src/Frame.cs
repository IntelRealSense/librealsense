using System;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public class Frame : IDisposable
    {
        HandleRef m_instance;

        public Frame(IntPtr ptr)
        {
            //if (ptr == IntPtr.Zero)
            //    throw new ArgumentNullException("ptr");
            m_instance = new HandleRef(this, ptr);
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
            if(m_instance.Handle != IntPtr.Zero)
                NativeMethods.rs2_release_frame(m_instance.Handle);
            m_instance = new HandleRef(this, IntPtr.Zero);
        }

        public Frame Clone()
        {
            return new Frame(m_instance.Handle);
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
                var w = NativeMethods.rs2_get_frame_height(m_instance.Handle, out error);
                return w;
            }
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

        public int Stride
        {
            get
            {
                object error;
                var stride = NativeMethods.rs2_get_frame_stride_in_bytes(m_instance.Handle, out error);
                return stride;
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
