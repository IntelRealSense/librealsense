using Intel.RealSense.Types;
using System;
using System.Runtime.InteropServices;

namespace Intel.RealSense.Profiles
{
    public class StreamProfile : IDisposable
    {
        internal static readonly ProfilePool<StreamProfile> Pool;

        static StreamProfile()
        {
            Pool = new ProfilePool<StreamProfile>();
        }

        public IntPtr Ptr => Instance.Handle;

        public Stream Stream => stream;
        public Format Format => format;
        public int Framerate => framerate;
        public int Index => index;
        public int UniqueID => uniqueID;

        internal Stream stream;
        internal Format format;
        internal int framerate;
        internal int index;
        internal int uniqueID;

        internal HandleRef Instance;

        public StreamProfile(IntPtr ptr)
        {
            Instance = new HandleRef(this, ptr);

            NativeMethods.rs2_get_stream_profile_data(Instance.Handle, out stream, out format, out index, out uniqueID, out framerate, out var e);
        }

        public Extrinsics GetExtrinsicsTo(StreamProfile other)
        {
            NativeMethods.rs2_get_extrinsics(Instance.Handle, other.Instance.Handle, out Extrinsics extrinsics, out var error);
            return extrinsics;
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
            //if(m_instance.Handle != IntPtr.Zero)
            //NativeMethods.rs2_delete_str(m_instance.Handle);
            Instance = new HandleRef(this, IntPtr.Zero);
            Pool.Release(this);
        }
    }
}
