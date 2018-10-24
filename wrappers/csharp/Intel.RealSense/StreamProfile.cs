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

    public class StreamProfile : IDisposable
    {
        internal HandleRef m_instance;

        internal static readonly ProfilePool<StreamProfile> Pool = new ProfilePool<StreamProfile>();

        public StreamProfile(IntPtr ptr)
        {
            m_instance = new HandleRef(this, ptr);

            object e;
            NativeMethods.rs2_get_stream_profile_data(m_instance.Handle, out _stream, out _format, out _index, out _uniqueId, out _framerate, out e);
        }

        internal Stream _stream;
        internal Format _format;
        internal int _framerate;
        internal int _index;
        internal int _uniqueId;

        public Stream Stream { get { return _stream; } }
        public Format Format { get { return _format; } }
        public int Framerate { get { return _framerate; } }
        public int Index { get { return _index; } }
        public int UniqueID { get { return _uniqueId; } }

        public IntPtr Ptr
        {
            get { return m_instance.Handle; }
        }

        public Extrinsics GetExtrinsicsTo(StreamProfile other)
        {
            object error;
            Extrinsics extrinsics;
            NativeMethods.rs2_get_extrinsics(m_instance.Handle, other.m_instance.Handle, out extrinsics, out error);
            return extrinsics;
        }

        #region IDisposable Support
        internal bool disposedValue = false; // To detect redundant calls

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
            m_instance = new HandleRef(this, IntPtr.Zero);
            Pool.Release(this);
        }
    }

    public class VideoStreamProfile : StreamProfile
    {
        public VideoStreamProfile(IntPtr ptr) : base(ptr)
        {
            object error;
            NativeMethods.rs2_get_video_stream_resolution(ptr, out width, out height, out error);
        }

        public Intrinsics GetIntrinsics()
        {
            object error;
            Intrinsics intrinsics;
            NativeMethods.rs2_get_video_stream_intrinsics(m_instance.Handle, out intrinsics, out error);
            return intrinsics;
        }

        public int Width { get { return width; } }

        public int Height { get { return height; } }

        internal int width;
        internal int height;
    }


    public class ProfilePool<T> where T : StreamProfile
    {
        readonly Stack<T> stack = new Stack<T>();
        readonly object locker = new object();

        public T Get(IntPtr ptr)
        {

            object error;
            bool isVideo = NativeMethods.rs2_stream_profile_is(ptr, Extension.VideoProfile, out error) > 0;

            T p;
            lock (locker)
            {
                if (stack.Count != 0)
                    p = stack.Pop();
                else
                    p = (isVideo  ? new VideoStreamProfile(ptr) : new StreamProfile(ptr)) as T;
            }

            p.m_instance = new HandleRef(p, ptr);
            p.disposedValue = false;

            NativeMethods.rs2_get_stream_profile_data(ptr, out p._stream, out p._format, out p._index, out p._uniqueId, out p._framerate, out error);

            if(isVideo)
            {
                var v = p as VideoStreamProfile;
                NativeMethods.rs2_get_video_stream_resolution(ptr, out v.width, out v.height, out error);
                return v as T;
            }

            return p;
        }

        public void Release(T t)
        {
            lock (locker)
            {
                stack.Push(t);
            }
        }
    }
}
