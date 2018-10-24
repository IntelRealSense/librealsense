using Intel.RealSense.Types;
using System;
using System.Collections;
using System.Collections.Generic;

namespace Intel.RealSense.Profiles
{
    public class StreamProfileList : IDisposable, IEnumerable<StreamProfile>
    {        
        public StreamProfile this[int index]
        {
            get
            {
                var ptr = NativeMethods.rs2_get_stream_profile(instance, index, out var error);
                if (NativeMethods.rs2_stream_profile_is(ptr, Extension.VideoProfile, out error) > 0)
                    return VideoStreamProfile.Pool.Get(ptr);
                else
                    return StreamProfile.Pool.Get(ptr);
            }
        }

        private IntPtr instance;

        public StreamProfileList(IntPtr ptr)
        {
            instance = ptr;
        }

        public IEnumerator<StreamProfile> GetEnumerator()
        {

            int deviceCount = NativeMethods.rs2_get_stream_profiles_count(instance, out var error);
            for (int i = 0; i < deviceCount; i++)
            {
                var ptr = NativeMethods.rs2_get_stream_profile(instance, i, out error);

                if (NativeMethods.rs2_stream_profile_is(ptr, Extension.VideoProfile, out error) > 0)
                    yield return VideoStreamProfile.Pool.Get(ptr);
                else
                    yield return StreamProfile.Pool.Get(ptr);
            }
        }

        IEnumerator IEnumerable.GetEnumerator()
            => GetEnumerator();

        public int Count
        {
            get
            {
                int deviceCount = NativeMethods.rs2_get_stream_profiles_count(instance, out var error);
                return deviceCount;
            }
        }

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
                NativeMethods.rs2_delete_stream_profiles_list(instance);
                instance = IntPtr.Zero;

                disposedValue = true;
            }
        }

        //TODO: override a finalizer only if Dispose(bool disposing) above has code to free unmanaged resources.
        ~StreamProfileList()
        {
            // Do not change this code. Put cleanup code in Dispose(bool disposing) above.
            Dispose(false);
        }
        #endregion
    }
}
