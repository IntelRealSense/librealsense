using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public class PipelineProfile : IDisposable
    {
        HandleRef m_instance;

        public PipelineProfile(IntPtr p)
        {
            m_instance = new HandleRef(this, p);
        }

        public Device Device
        {
            get
            {
                object error;
                var ptr = NativeMethods.rs2_pipeline_profile_get_device(m_instance.Handle, out error);
                return new Device(ptr);
            }
        }

        public StreamProfileList Streams
        {
            get
            {
                object error;
                var ptr = NativeMethods.rs2_pipeline_profile_get_streams(m_instance.Handle, out error);
                return new StreamProfileList(ptr);
            }
        }

        public StreamProfile GetStream(Stream s, int index = -1)
        {
            foreach(var x in Streams)
            {
                if (x.Stream == s && (index != -1 ? x.Index == index : true))
                    return x;
            }
            return null;
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
        ~PipelineProfile()
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
                NativeMethods.rs2_delete_pipeline_profile(m_instance.Handle);
            m_instance = new HandleRef(this, IntPtr.Zero);
        }
    }
}
