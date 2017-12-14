using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace Intel.RealSense
{
    public class FrameQueue : IDisposable, IEnumerable<Frame>
    {
        internal IntPtr m_instance;

        /// <summary>
        /// Avoid getting GC'ed 
        /// see http://stackoverflow.com/questions/6193711/call-has-been-made-on-garbage-collected-delegate-in-c
        /// </summary>
        internal frame_callback enqueue;

        public FrameQueue(int capacity = 10)
        {
            object error;
            m_instance = NativeMethods.rs2_create_frame_queue(capacity, out error);
            enqueue = NativeMethods.rs2_enqueue_frame;
        }

        public bool PollForFrame(out Frame frame)
        {
            object error;
            return NativeMethods.rs2_poll_for_frame(m_instance, out frame, out error) > 0;
        }

        public Frame WaitForFrame()
        {
            object error;
            var ptr = NativeMethods.rs2_wait_for_frame(m_instance, 5000, out error);
            var frame = new Frame(ptr);
            return frame;
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
                NativeMethods.rs2_delete_frame_queue(m_instance);

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
