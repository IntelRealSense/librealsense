using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public class StreamProfileList : IDisposable, IEnumerable<StreamProfile>
    {
        IntPtr m_instance;

        public StreamProfileList(IntPtr ptr)
        {
            m_instance = ptr;
        }


        public IEnumerator<StreamProfile> GetEnumerator()
        {
            object error;

            int deviceCount = NativeMethods.rs2_get_stream_profiles_count(m_instance, out error);
            for (int i = 0; i < deviceCount; i++)
            {
                var ptr = NativeMethods.rs2_get_stream_profile(m_instance, i, out error);

                if (NativeMethods.rs2_stream_profile_is(ptr, Extension.VideoProfile, out error) > 0)
                    yield return VideoStreamProfile.Pool.Get(ptr);
                else 
                    yield return StreamProfile.Pool.Get(ptr);
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
                int deviceCount = NativeMethods.rs2_get_stream_profiles_count(m_instance, out error);
                return deviceCount;
            }
        }

        public StreamProfile this[int index]
        {
            get
            {
                object error;
                var ptr = NativeMethods.rs2_get_stream_profile(m_instance, index, out error);
                if (NativeMethods.rs2_stream_profile_is(ptr, Extension.VideoProfile, out error) > 0)
                    return new VideoStreamProfile(ptr);
                else
                    return StreamProfile.Pool.Get(ptr);
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
                NativeMethods.rs2_delete_stream_profiles_list(m_instance);
                m_instance = IntPtr.Zero;

                disposedValue = true;
            }
        }

        //TODO: override a finalizer only if Dispose(bool disposing) above has code to free unmanaged resources.
        ~StreamProfileList()
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
