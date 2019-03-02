using System;
using System.Runtime.InteropServices;
using System.Collections.Generic;
using System.Linq;

namespace Intel.RealSense
{
    public interface IProcessingBlock : IOptions
    {
        Frame Process(Frame original);
        FrameSet Process(FrameSet original);
    }

    public class ProcessingBlock : IProcessingBlock, IDisposable
    {
        public readonly FrameQueue queue = new FrameQueue(1);

        internal HandleRef m_instance;

        Sensor.SensorOptions m_options;
        public IOptionsContainer Options
        {
            get
            {
                return m_options = m_options ?? new Sensor.SensorOptions(m_instance.Handle);
            }
        }

        internal ProcessingBlock(IntPtr ptr)
        {
            m_instance = new HandleRef(this, ptr);
        }

        protected ProcessingBlock()
        {}

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
        ~ProcessingBlock()
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
                NativeMethods.rs2_delete_processing_block(m_instance.Handle);
            m_instance = new HandleRef(this, IntPtr.Zero);
        }

        /// <summary>
        /// Process frame and return the result
        /// </summary>
        /// <param name="original"></param>
        /// <returns></returns>
        public Frame Process(Frame original)
        {
            object error;
            NativeMethods.rs2_frame_add_ref(original.m_instance.Handle, out error);
            NativeMethods.rs2_process_frame(m_instance.Handle, original.m_instance.Handle, out error);
            Frame f;
            if (queue.PollForFrame(out f))
            {
                return f;
            }
            return original;
        }

        public FrameSet Process(FrameSet original)
        {
            FrameSet rv;
            using (var singleOriginal = original.AsFrame())
            {
                using (var processed = Process(singleOriginal))
                {
                    rv = FrameSet.FromFrame(processed);
                }
            }
            return rv;
        }

        #region Extensions

        /// <summary>
        /// retrieve mapping between the units of the depth image and meters
        /// </summary>
        /// <returns> depth in meters corresponding to a depth value of 1</returns>

        public bool Is(Extension e)
        {
            object error;
            return NativeMethods.rs2_is_processing_block_extendable_to(m_instance.Handle, e, out error) > 0;
        }

        #endregion
    }

    public static class IProcessingBlockExtensions
    {
        public static Frame ApplyFilter(this Frame frame, IProcessingBlock block)
        {
            return block.Process(frame);
        }

        public static FrameSet ApplyFilter(this FrameSet frames, IProcessingBlock block)
        {
            return block.Process(frames);
        }
    }
}