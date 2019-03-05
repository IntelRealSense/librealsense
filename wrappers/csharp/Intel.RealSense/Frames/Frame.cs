using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public class Frame : IDisposable
    {
        internal HandleRef m_instance;

        readonly static ObjectPool Pool = new ObjectPool((obj, ptr) =>
        {
            var f = obj as Frame;
            f.m_instance = new HandleRef(f, ptr);
            f.disposedValue = false;
            GC.ReRegisterForFinalize(f);
        });

        public IntPtr NativePtr { get { return m_instance.Handle; } }

        internal Frame(IntPtr ptr)
        {
            m_instance = new HandleRef(this, ptr);
        }

        public static Frame Create(IntPtr ptr)
        {
            return Create<Frame>(ptr);
        }

        public static T Create<T>(IntPtr ptr) where T : Frame
        {
            return Pool.Get<T>(ptr);
        }

        public bool Is(Extension e)
        {
            object error;
            return NativeMethods.rs2_is_frame_extendable_to(m_instance.Handle, e, out error) > 0;
        }

        public T As<T>() where T : Frame
        {
            using (this)
            {
                object error;
                NativeMethods.rs2_frame_add_ref(m_instance.Handle, out error);
                return Create<T>(m_instance.Handle);
            }
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
                Reset();
                Pool.Release(this);

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

        internal void Reset()
        {
            if (m_instance.Handle != IntPtr.Zero)
                NativeMethods.rs2_release_frame(m_instance.Handle);
            m_instance = new HandleRef(this, IntPtr.Zero);
        }

        public Frame Clone()
        {
            object error;
            NativeMethods.rs2_frame_add_ref(m_instance.Handle, out error);
            return Create(m_instance.Handle);
        }

        public bool IsComposite
        {
            get
            {
                return Is(Extension.CompositeFrame);
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

        public T GetProfile<T>() where T : StreamProfile
        {
            object error;
            var ptr = NativeMethods.rs2_get_frame_stream_profile(m_instance.Handle, out error);
            return StreamProfile.Create<T>(ptr);
        }

        public StreamProfile Profile => GetProfile<StreamProfile>();
  
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

        public long this[FrameMetadataValue frame_metadata]
        {
            get {
                return GetFrameMetadata(frame_metadata);
            }
        }

        public long GetFrameMetadata(FrameMetadataValue frame_metadata)
        {
            object error;
            return NativeMethods.rs2_get_frame_metadata(m_instance.Handle, frame_metadata, out error);
        }

        public bool SupportsFrameMetaData(FrameMetadataValue frame_metadata)
        {
            object error;
            return NativeMethods.rs2_supports_frame_metadata(m_instance.Handle, frame_metadata, out error) != 0;
        }
    }
}
