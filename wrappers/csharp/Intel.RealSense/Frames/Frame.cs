using Intel.RealSense.Profiles;
using Intel.RealSense.Types;
using System;
using System.Runtime.InteropServices;

namespace Intel.RealSense.Frames
{
    public class Frame : IDisposable
    {
        internal static readonly FramePool<Frame> Pool; //TODO: Should be reimplemented as Threadsafe Pool.

        static Frame()
        {
            Pool = new FramePool<Frame>(ptr => new Frame(ptr));
        }

        public bool IsComposite => NativeMethods.rs2_is_frame_extendable_to(Instance.Handle, Extension.CompositeFrame, out var error) > 0;
        public IntPtr Data => NativeMethods.rs2_get_frame_data(Instance.Handle, out var error);
        public StreamProfile Profile => StreamProfile.Pool.Get(NativeMethods.rs2_get_frame_stream_profile(Instance.Handle, out var error));
        public ulong Number
        {
            get
            {
                var frameNumber = NativeMethods.rs2_get_frame_number(Instance.Handle, out var error);
                return frameNumber;
            }
        }
        public double Timestamp => NativeMethods.rs2_get_frame_timestamp(Instance.Handle, out var error);
        public TimestampDomain TimestampDomain => NativeMethods.rs2_get_frame_timestamp_domain(Instance.Handle, out var error);

        public IntPtr NativePtr => Instance.Handle; //TODO: Native pointers should not be visible

        internal HandleRef Instance;

        public Frame(IntPtr ptr)
        {
            Instance = new HandleRef(this, ptr);
        }

        public virtual void Release()
        {
            if (Instance.Handle != IntPtr.Zero)
                NativeMethods.rs2_release_frame(Instance.Handle);

            Instance = new HandleRef(this, IntPtr.Zero);
            Pool.Releas(this);
        }

        public Frame Clone()
        {
            NativeMethods.rs2_frame_add_ref(Instance.Handle, out var error);
            return CreateFrame(Instance.Handle);
        }

        internal static Frame CreateFrame(IntPtr ptr)
        {
            if (NativeMethods.rs2_is_frame_extendable_to(ptr, Extension.Points, out var error) > 0)
                return Pool.Get(ptr);
            else if (NativeMethods.rs2_is_frame_extendable_to(ptr, Extension.DepthFrame, out error) > 0)
                return Pool.Get(ptr);
            else if (NativeMethods.rs2_is_frame_extendable_to(ptr, Extension.VideoFrame, out error) > 0)
                return Pool.Get(ptr);
            else
                return Pool.Get(ptr);
        }

        #region IDisposable Support
        internal bool disposedValue = false; // To detect redundant calls TODO: internal dispose control should never be externally influenced

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
        ~Frame()
        {
            // Do not change this code. Put cleanup code in Dispose(bool disposing) above.
            Dispose(false);
        }
        #endregion
    }
}
