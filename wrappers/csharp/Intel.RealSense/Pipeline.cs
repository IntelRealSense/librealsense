using System;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public class Pipeline : IDisposable
    {
        internal HandleRef instance;

        public Pipeline(Context ctx)
        {
            instance = new HandleRef(this, NativeMethods.rs2_create_pipeline(ctx.Instance.Handle, out var error));
        }
        public Pipeline()
        {
            var ctx = new Context();
            instance = new HandleRef(this, NativeMethods.rs2_create_pipeline(ctx.Instance.Handle, out var error));
        }

        public PipelineProfile Start()
        {
            var res = NativeMethods.rs2_pipeline_start(instance.Handle, out var error);
            return new PipelineProfile(res);
        }
        public PipelineProfile Start(Config cfg)
        {
            var res = NativeMethods.rs2_pipeline_start_with_config(instance.Handle, cfg.Instance.Handle, out var error);
            return new PipelineProfile(res);
        }

        public void Stop()
            => NativeMethods.rs2_pipeline_stop(instance.Handle, out var error);

        public FrameSet WaitForFrames(uint timeout_ms = 5000, FramesReleaser releaser = null)
        {
            var ptr = NativeMethods.rs2_pipeline_wait_for_frames(instance.Handle, timeout_ms, out var error);
            return FramesReleaser.ScopedReturn(releaser, new FrameSet(ptr));
        }

        public bool PollForFrames(out FrameSet result, FramesReleaser releaser = null)
        {
            if (NativeMethods.rs2_pipeline_poll_for_frames(instance.Handle, out FrameSet fs, out var error) > 0)
            {
                result = FramesReleaser.ScopedReturn(releaser, fs);
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
            if (instance.Handle != IntPtr.Zero)
                NativeMethods.rs2_delete_pipeline(instance.Handle);
            instance = new HandleRef(this, IntPtr.Zero);
        }
    }
}
