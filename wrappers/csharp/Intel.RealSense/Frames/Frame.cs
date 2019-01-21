using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public class Frame : IDisposable
    {
        internal HandleRef m_instance;
        public static readonly FramePool<Frame> Pool = new FramePool<Frame>(ptr => new Frame(ptr));

        public IntPtr NativePtr { get { return m_instance.Handle; } }

        public Frame(IntPtr ptr)
        {
            m_instance = new HandleRef(this, ptr);
        }

        internal static Frame CreateFrame(IntPtr ptr)
        {
            object error;
            if (NativeMethods.rs2_is_frame_extendable_to(ptr, Extension.Points, out error) > 0)
                return Points.Pool.Get(ptr);
            else if (NativeMethods.rs2_is_frame_extendable_to(ptr, Extension.DepthFrame, out error) > 0)
                return DepthFrame.Pool.Get(ptr);
            else if (NativeMethods.rs2_is_frame_extendable_to(ptr, Extension.VideoFrame, out error) > 0)
                return VideoFrame.Pool.Get(ptr);
            else
                return Frame.Pool.Get(ptr);
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
        ~Frame()
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

        public virtual void Release()
        {
            if (m_instance.Handle != IntPtr.Zero)
                NativeMethods.rs2_release_frame(m_instance.Handle);
            m_instance = new HandleRef(this, IntPtr.Zero);
            Pool.Release(this);
        }

        public Frame Clone()
        {
            object error;
            NativeMethods.rs2_frame_add_ref(m_instance.Handle, out error);
            return CreateFrame(m_instance.Handle);
        }

        public bool IsComposite
        {
            get
            {
                object error;
                return NativeMethods.rs2_is_frame_extendable_to(m_instance.Handle, Extension.CompositeFrame, out error) > 0;
            }
        }

        public IntPtr Data
        {
            get
            {
                object error;
                return NativeMethods.rs2_get_frame_data(m_instance.Handle, out error);
            }
        }

        public StreamProfile Profile
        {
            get
            {
                object error;
                return StreamProfile.Pool.Get(NativeMethods.rs2_get_frame_stream_profile(m_instance.Handle, out error));
            }
        }

        public ulong Number
        {
            get
            {
                object error;
                var frameNumber = NativeMethods.rs2_get_frame_number(m_instance.Handle, out error);
                return frameNumber;
            }
        }

        public double Timestamp
        {
            get
            {
                object error;
                var timestamp = NativeMethods.rs2_get_frame_timestamp(m_instance.Handle, out error);
                return timestamp;
            }
        }


        public TimestampDomain TimestampDomain
        {
            get
            {
                object error;
                var timestampDomain = NativeMethods.rs2_get_frame_timestamp_domain(m_instance.Handle, out error);
                return timestampDomain;
            }
        }
    }
}
