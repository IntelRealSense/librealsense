using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public class StreamProfile : IDisposable
    {
        internal HandleRef m_instance;

        internal static readonly ProfilePool<StreamProfile> Pool = new ProfilePool<StreamProfile>();

        public StreamProfile(IntPtr ptr)
        {
            m_instance = new HandleRef(this, ptr);

            object e;
            NativeMethods.rs2_get_stream_profile_data(m_instance.Handle, out _stream, out _format, out _index, out _uniqueId, out _framerate, out e);
        }

        internal Stream _stream;
        internal Format _format;
        internal int _framerate;
        internal int _index;
        internal int _uniqueId;

        public Stream Stream { get { return _stream; } }
        public Format Format { get { return _format; } }
        public int Framerate { get { return _framerate; } }
        public int Index { get { return _index; } }
        public int UniqueID { get { return _uniqueId; } }

        public IntPtr Ptr
        {
            get { return m_instance.Handle; }
        }

        public Extrinsics GetExtrinsicsTo(StreamProfile other)
        {
            object error;
            Extrinsics extrinsics;
            NativeMethods.rs2_get_extrinsics(m_instance.Handle, other.m_instance.Handle, out extrinsics, out error);
            return extrinsics;
        }

        public void RegisterExtrinsicsTo(StreamProfile other, Extrinsics extrinsics)
        {
            object error;
            NativeMethods.rs2_register_extrinsics(m_instance.Handle, other.m_instance.Handle, extrinsics, out error);
        }

        #region IDisposable Support
        internal bool disposedValue = false; // To detect redundant calls

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
        ~StreamProfile()
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
            m_instance = new HandleRef(this, IntPtr.Zero);
            Pool.Release(this);
        }
    }
}
