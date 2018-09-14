using Intel.RealSense.Frames;
using Intel.RealSense.Types;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public class FrameSet : IDisposable, IEnumerable<Frame>
    {
        public DepthFrame DepthFrame => FirstOrDefault<DepthFrame>(Stream.Depth);
        public VideoFrame ColorFrame => FirstOrDefault<VideoFrame>(Stream.Color);

        internal HandleRef Instance;
        private readonly int count;

        public Frame AsFrame()
        {
            NativeMethods.rs2_frame_add_ref(Instance.Handle, out var error);
            return CreateFrame(Instance.Handle);
        }

        public T FirstOrDefault<T>(Stream stream, Format format = Format.Any) where T : Frame
        {
            foreach (Frame frame in this)
            {
                if (frame.Profile.Stream == stream && (Format.Any == format || frame.Profile.Format == format))
                    return frame as T;
                frame.Dispose();
            }
            return null;
        }

        public IEnumerator<Frame> GetEnumerator()
        {
            for (int i = 0; i < Count; i++)
            {
                var ptr = NativeMethods.rs2_extract_frame(Instance.Handle, i, out var error);
                yield return CreateFrame(ptr);
            }
        }

        IEnumerator IEnumerable.GetEnumerator() 
            => GetEnumerator();

        public int Count => count;
        
        public Frame this[int index]
        {
            get
            {
                var ptr = NativeMethods.rs2_extract_frame(Instance.Handle, index, out var error);
                return CreateFrame(ptr);
            }
        }

        public Frame this[Stream stream, int index = 0]
        {
            get
            {
                foreach (Frame frame in this)
                {
                    var p = frame.Profile;
                    if (p.Stream == stream && p.Index == index)
                        return frame;
                    frame.Dispose();
                }
                return null;
            }
        }

        internal FrameSet(IntPtr ptr)
        {
            Instance = new HandleRef(this, ptr);
            count = NativeMethods.rs2_embedded_frames_count(Instance.Handle, out var error);
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
            if (Instance.Handle != IntPtr.Zero)
                NativeMethods.rs2_release_frame(Instance.Handle);
            Instance = new HandleRef(this, IntPtr.Zero);
        }

        public static FrameSet FromFrame(Frame composite, FramesReleaser releaser = null)
        {
            if (NativeMethods.rs2_is_frame_extendable_to(composite.Instance.Handle,
                Extension.CompositeFrame, out var error) > 0)
            {
                NativeMethods.rs2_frame_add_ref(composite.Instance.Handle, out error);
                return FramesReleaser.ScopedReturn(releaser, new FrameSet(composite.Instance.Handle));
            }
            throw new Exception("The frame is a not composite frame");
        }

        internal static Frame CreateFrame(IntPtr ptr)
        {
            if (NativeMethods.rs2_is_frame_extendable_to(ptr, Extension.Points, out var error) > 0)
                return new Points(ptr);
            else if (NativeMethods.rs2_is_frame_extendable_to(ptr, Extension.DepthFrame, out error) > 0)
                return new DepthFrame(ptr);
            else if (NativeMethods.rs2_is_frame_extendable_to(ptr, Extension.VideoFrame, out error) > 0)
                return new VideoFrame(ptr);
            else
                return new Frame(ptr);
        }
    }
}
