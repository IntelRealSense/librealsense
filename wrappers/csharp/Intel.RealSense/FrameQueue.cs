using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;

namespace Intel.RealSense
{
    public class FrameQueue : IDisposable, IEnumerable<Frame>
    {
        internal HandleRef m_instance;

        public FrameQueue(int capacity = 10)
        {
            object error;
            m_instance = new HandleRef(this, NativeMethods.rs2_create_frame_queue(capacity, out error));
        }

        public bool PollForFrame(out Frame frame, FramesReleaser releaser = null)
        {
            object error;
            if (NativeMethods.rs2_poll_for_frame(m_instance.Handle, out frame, out error) > 0)
            {
                frame = FramesReleaser.ScopedReturn(releaser, FrameSet.CreateFrame(frame.m_instance.Handle));
                return true;
            }
            return false;
        }

        public Frame WaitForFrame(FramesReleaser releaser = null)
        {
            object error;
            var ptr = NativeMethods.rs2_wait_for_frame(m_instance.Handle, 5000, out error);
            return FramesReleaser.ScopedReturn(releaser, FrameSet.CreateFrame(ptr));
        }

        public FrameSet WaitForFrames(FramesReleaser releaser = null)
        {
            object error;
            var ptr = NativeMethods.rs2_wait_for_frame(m_instance.Handle, 5000, out error);
            return FramesReleaser.ScopedReturn(releaser, new FrameSet(ptr));
        }

        public void Enqueue(Frame f)
        {
            object error;
            NativeMethods.rs2_frame_add_ref(f.m_instance.Handle, out error);
            NativeMethods.rs2_enqueue_frame(f.m_instance.Handle, m_instance.Handle);
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
                NativeMethods.rs2_delete_frame_queue(m_instance.Handle);

                disposedValue = true;
            }
        }

        // TODO: override a finalizer only if Dispose(bool disposing) above has code to free unmanaged resources.
        ~FrameQueue()
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

        public IEnumerator<Frame> GetEnumerator()
        {
            Frame frame;
            while (PollForFrame(out frame))
            {
                yield return frame;
            }
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }
        #endregion
    }
}
