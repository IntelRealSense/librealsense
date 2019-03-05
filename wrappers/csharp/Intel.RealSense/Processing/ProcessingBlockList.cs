using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public class ProcessingBlockList : IDisposable, IEnumerable<ProcessingBlock>
    {
        internal IntPtr m_instance;

        public ProcessingBlockList(IntPtr ptr)
        {
            m_instance = ptr;
        }

        public IEnumerator<ProcessingBlock> GetEnumerator()
        {
            object error;

            int deviceCount = NativeMethods.rs2_get_recommended_processing_blocks_count(m_instance, out error);
            for (int i = 0; i < deviceCount; i++)
            {
                var ptr = NativeMethods.rs2_get_processing_block(m_instance, i, out error);
                yield return new ProcessingBlock(ptr);
            }
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }

        public int Count
        {
            get
            {
                object error;
                int deviceCount = NativeMethods.rs2_get_recommended_processing_blocks_count(m_instance, out error);
                return deviceCount;
            }
        }

        public T GetProcessingBlock<T>(int index) where T : ProcessingBlock
        {
            object error;
            return new ProcessingBlock(NativeMethods.rs2_get_processing_block(m_instance, index, out error)) as T;
        }

        public ProcessingBlock this[int index]
        {
            get
            {
                return GetProcessingBlock<ProcessingBlock>(index);
            }
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
                NativeMethods.rs2_delete_recommended_processing_blocks(m_instance);
                m_instance = IntPtr.Zero;

                disposedValue = true;
            }
        }

        //TODO: override a finalizer only if Dispose(bool disposing) above has code to free unmanaged resources.
        ~ProcessingBlockList()
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
    }
}
