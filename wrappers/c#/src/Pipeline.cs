using System;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public class Pipeline : IDisposable
    {
        HandleRef m_instance;

        public Pipeline(Context ctx)
        {
            object error;
            m_instance = new HandleRef(this, NativeMethods.rs2_create_pipeline(ctx.m_instance.Handle, out error));
        }

        public Pipeline()
        {
            var ctx = new Context();
            object error;
            m_instance = new HandleRef(this, NativeMethods.rs2_create_pipeline(ctx.m_instance.Handle, out error));
        }

        public void Start()
        {
            object error;
            NativeMethods.rs2_pipeline_start(m_instance.Handle, out error); // TODO: Return profile
        }

        public void Stop()
        {
            object error;
            NativeMethods.rs2_pipeline_stop(m_instance.Handle, out error);
        }

        public FrameSet WaitForFrames(uint timeout_ms = 5000)
        {
            object error;
            var ptr = NativeMethods.rs2_pipeline_wait_for_frames(m_instance.Handle, timeout_ms, out error);
            return new FrameSet(ptr);
        }

        public bool PollForFrames(out FrameSet result)
        {
            object error;
            FrameSet fs;
            if (NativeMethods.rs2_pipeline_poll_for_frames(m_instance.Handle, out fs, out error) > 0)
            {
                result = fs;
                return true;
            }
            result = null;
            return false;
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
        ~Pipeline()
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
                NativeMethods.rs2_delete_pipeline(m_instance.Handle);
            m_instance = new HandleRef(this, IntPtr.Zero);
        }
    }
}
