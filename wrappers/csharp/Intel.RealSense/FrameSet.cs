using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Linq;

namespace Intel.RealSense
{
    public class FrameSet : IDisposable, IEnumerable<Frame>
    {
        internal HandleRef m_instance;

        internal static Frame CreateFrame(IntPtr ptr)
        {
            object error;
            if (NativeMethods.rs2_is_frame_extendable_to(ptr, Extension.DepthFrame, out error) > 0)
                return new DepthFrame(ptr);
            else if (NativeMethods.rs2_is_frame_extendable_to(ptr, Extension.VideoFrame, out error) > 0)
                return new VideoFrame(ptr);
            else
                return new Frame(ptr);
        }

        public DepthFrame DepthFrame
        {
            get
            {
                return this.FirstOrDefault(x => x.Profile.Stream == Stream.Depth) as DepthFrame;
            }
        }

        public VideoFrame ColorFrame
        {
            get
            {
                return this.FirstOrDefault(x => x.Profile.Stream == Stream.Color) as VideoFrame;
            }
        }

        public IEnumerator<Frame> GetEnumerator()
        {
            object error;

            int deviceCount = NativeMethods.rs2_embedded_frames_count(m_instance.Handle, out error);
            for (int i = 0; i < deviceCount; i++)
            {
                var ptr = NativeMethods.rs2_extract_frame(m_instance.Handle, i, out error);
                yield return CreateFrame(ptr);
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
                int deviceCount = NativeMethods.rs2_embedded_frames_count(m_instance.Handle, out error);
                return deviceCount;
            }
        }

        public Frame this[int index]
        {
            get
            {
                object error;
                var ptr = NativeMethods.rs2_extract_frame(m_instance.Handle, index, out error);
                return CreateFrame(ptr);
            }
        }

        public FrameSet(IntPtr ptr)
        {
            m_instance = new HandleRef(this, ptr);
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
        ~FrameSet()
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
                NativeMethods.rs2_release_frame(m_instance.Handle);
            m_instance = new HandleRef(this, IntPtr.Zero);
        }

    }


    class FrameSetMarshaler : ICustomMarshaler
    {
        private static FrameSetMarshaler Instance;

        public static ICustomMarshaler GetInstance(string s)
        {
            if (Instance == null)
            {
                Instance = new FrameSetMarshaler();
            }
            return Instance;
        }

        public void CleanUpManagedData(object ManagedObj)
        {
        }

        public void CleanUpNativeData(IntPtr pNativeData)
        {
        }

        public int GetNativeDataSize()
        {
            return -1;
        }

        public IntPtr MarshalManagedToNative(object ManagedObj)
        {
            throw new NotImplementedException();
        }

        public object MarshalNativeToManaged(IntPtr pNativeData)
        {
            return new FrameSet(pNativeData);
        }
    }
}
