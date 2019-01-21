using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public class Pipeline : IDisposable, IEnumerable<Frame>
    {
        internal HandleRef m_instance;

        //public delegate void FrameCallback<Frame, T>(Frame frame);
        public delegate void FrameCallback(Frame frame);
        private frame_callback m_callback;

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

        public PipelineProfile Start()
        {
            object error;
            var res = NativeMethods.rs2_pipeline_start(m_instance.Handle, out error);
            var prof = new PipelineProfile(res);
            return prof;
        }

        public PipelineProfile Start(Config cfg)
        {
            object error;
            var res = NativeMethods.rs2_pipeline_start_with_config(m_instance.Handle, cfg.m_instance.Handle, out error);
            var prof = new PipelineProfile(res);
            return prof;
        }

        public PipelineProfile Start(FrameCallback cb)
        {
            object error;
            frame_callback cb2 = (IntPtr f, IntPtr u) =>
            {
                using (var frame = new Frame(f))
                    cb(frame);
            };
            m_callback = cb2;
            var res = NativeMethods.rs2_pipeline_start_with_callback(m_instance.Handle, cb2, IntPtr.Zero, out error);
            var prof = new PipelineProfile(res);
            return prof;
        }

        public PipelineProfile Start(Config cfg, FrameCallback cb)
        {
            object error;
            frame_callback cb2 = (IntPtr f, IntPtr u) =>
            {
                using (var frame = new Frame(f))
                    cb(frame);
            };
            m_callback = cb2;
            var res = NativeMethods.rs2_pipeline_start_with_config_and_callback(m_instance.Handle, cfg.m_instance.Handle, cb2, IntPtr.Zero, out error);
            var prof = new PipelineProfile(res);
            return prof;
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
            return FrameSet.Pool.Get(ptr);
        }

        public bool PollForFrames(out FrameSet result)
        {
            object error;
            IntPtr fs;
            if (NativeMethods.rs2_pipeline_poll_for_frames(m_instance.Handle, out fs, out error) > 0)
            {
                result = FrameSet.Pool.Get(fs);
                return true;
            }
            result = null;
            return false;
        }

        public IEnumerator<Frame> GetEnumerator()
        {
            FrameSet frames;
            while (PollForFrames(out frames))
            {
                using (frames)
                using (var frame = frames.AsFrame())
                    yield return frame;
            }
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
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
