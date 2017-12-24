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
                    yield return new VideoStreamProfile(ptr);
                else 
                    yield return new StreamProfile(ptr);
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
                    return new StreamProfile(ptr);
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

    public class StreamProfile : IDisposable
    {
        internal HandleRef m_instance;

        public StreamProfile(IntPtr ptr)
        {
            //if (ptr == IntPtr.Zero)
            //    throw new ArgumentNullException("ptr");
            m_instance = new HandleRef(this, ptr);

            object e;
            Stream stream;
            Format fmt;
            int fps;
            int idx;
            int uid;

            NativeMethods.rs2_get_stream_profile_data(m_instance.Handle, out stream, out fmt, out idx, out uid, out fps, out e);

            Stream = stream;
            Format = fmt;
            Framerate = fps;
            Index = idx;
            UniqueID = uid;
        }

        public Stream Stream { get; private set; }
        public Format Format { get; private set; }

        public int Framerate { get; private set; }

        public int Index { get; private set; }

        public int UniqueID { get; private set; }

        public Extrinsics GetExtrinsicsTo(StreamProfile other)
        {
            object error;
            Extrinsics extrinsics;
            NativeMethods.rs2_get_extrinsics(m_instance.Handle, other.m_instance.Handle, out extrinsics, out error);
            return extrinsics;
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
        ~StreamProfile()
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
            //if(m_instance.Handle != IntPtr.Zero)
                //NativeMethods.rs2_delete_str(m_instance.Handle);
            m_instance = new HandleRef(this, IntPtr.Zero);
        }
    }

    public class VideoStreamProfile : StreamProfile
    {
        public VideoStreamProfile(IntPtr ptr) : base(ptr)
        {
            int width;
            int height;

            object error;
            NativeMethods.rs2_get_video_stream_resolution(ptr, out width, out height, out error);

            Width = width;
            Height = height;
        }

        public Intrinsics GetIntrinsics()
        {
            object error;
            Intrinsics intrinsics;
            NativeMethods.rs2_get_video_stream_intrinsics(m_instance.Handle, out intrinsics, out error);
            return intrinsics;
        }

        public int Width { get; private set; }

        public int Height { get; private set; }

    }
}
