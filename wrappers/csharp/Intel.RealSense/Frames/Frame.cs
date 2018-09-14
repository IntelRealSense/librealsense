using Intel.RealSense.Profiles;
using Intel.RealSense.Types;
using System;
using System.Runtime.InteropServices;

namespace Intel.RealSense.Frames
{
    public class Frame : IDisposable
    {
        public IntPtr Data => NativeMethods.rs2_get_frame_data(Instance.Handle, out var error);
        public StreamProfile Profile => new StreamProfile(NativeMethods.rs2_get_frame_stream_profile(Instance.Handle, out var error));
        public ulong Number
        {
            get
            {
                var frameNumber = NativeMethods.rs2_get_frame_number(Instance.Handle, out var error);
                return frameNumber;
            }
        }
        public double Timestamp => NativeMethods.rs2_get_frame_timestamp(Instance.Handle, out var error);
        public TimestampDomain TimestampDomain => NativeMethods.rs2_get_frame_timestamp_domain(Instance.Handle, out var error);

        internal HandleRef Instance;

        public Frame(IntPtr ptr)
        {
            Instance = new HandleRef(this, ptr);
            NativeMethods.rs2_keep_frame(Instance.Handle);
        }

        public void Release()
        {
            if (Instance.Handle != IntPtr.Zero)
                NativeMethods.rs2_release_frame(Instance.Handle);

            Instance = new HandleRef(this, IntPtr.Zero);
        }

        public Frame Clone()
        {
            NativeMethods.rs2_frame_add_ref(Instance.Handle, out var error);
            return new Frame(Instance.Handle);
        }

        #region IDisposable Support
        private bool disposedValue = false; // To detect redundant calls

        // This code added to correctly implement the disposable pattern.
        public void Dispose()
        {
            // Do not change this code. Put cleanup code in Dispose(bool disposing) above.
            Dispose(true);
            // TODO: uncomment the following line if the finalizer is overridden above.
            GC.SuppressFinalize(this);
        }

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
        #endregion
    }
}
