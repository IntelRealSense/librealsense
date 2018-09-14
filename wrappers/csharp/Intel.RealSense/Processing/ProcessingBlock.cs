using System;
using System.Runtime.InteropServices;

namespace Intel.RealSense.Processing
{
    public class ProcessingBlock : IDisposable
    {
        public Sensor.SensorOptions Options => options = options ?? new Sensor.SensorOptions(Instance.Handle);

        internal HandleRef Instance;

        private Sensor.SensorOptions options;

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
        ~ProcessingBlock()
        {
            // Do not change this code. Put cleanup code in Dispose(bool disposing) above.
            Dispose(false);
        }
        #endregion

        public void Release()
        {
            if (Instance.Handle != IntPtr.Zero)
                NativeMethods.rs2_delete_processing_block(Instance.Handle);

            Instance = new HandleRef(this, IntPtr.Zero);
        }
    }
}