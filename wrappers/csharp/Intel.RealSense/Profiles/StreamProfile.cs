using Intel.RealSense.Types;
using System;
using System.Runtime.InteropServices;

namespace Intel.RealSense.Profiles
{
    public class StreamProfile : IDisposable
    {
        public IntPtr Ptr => Instance.Handle;

        public readonly Stream Stream;
        public readonly Format Format;

        public readonly int Framerate;

        public readonly int Index;

        public readonly int UniqueID;

        internal HandleRef Instance;

        public StreamProfile(IntPtr ptr)
        {
            Instance = new HandleRef(this, ptr);

            NativeMethods.rs2_get_stream_profile_data(Instance.Handle, out Stream, out Format, out Index, out UniqueID, out Framerate, out var e);
        }

        public Extrinsics GetExtrinsicsTo(StreamProfile other)
        {
            NativeMethods.rs2_get_extrinsics(Instance.Handle, other.Instance.Handle, out Extrinsics extrinsics, out var error);
            return extrinsics;
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
        }
    }
}
