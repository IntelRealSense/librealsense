using Intel.RealSense.Frames;
using Intel.RealSense.Types;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public class FrameSet : ICompositeDisposable, IEnumerable<Frame>
    {
        internal static readonly FrameSetPool Pool; //Should be reimplemented as Threadsafe Pool.

        static FrameSet()
        {
            Pool = new FrameSetPool();
        }

        public DepthFrame DepthFrame => FirstOrDefault<DepthFrame>(Stream.Depth, Format.Z16);
        public VideoFrame InfraredFrame => FirstOrDefault<VideoFrame>(Stream.Infrared);
        public VideoFrame ColorFrame => FirstOrDefault<VideoFrame>(Stream.Color);

        public IntPtr NativePtr => Instance.Handle;

        internal HandleRef Instance;
        internal readonly FrameEnumerator enumerator;
        internal readonly List<IDisposable> disposables;

        public Frame AsFrame()
        {
            NativeMethods.rs2_frame_add_ref(Instance.Handle, out var error);
            return Frame.CreateFrame(Instance.Handle);
        }

        public T FirstOrDefault<T>(Stream stream, Format format = Format.Any) where T : Frame
        {
            for (int i = 0; i < Count; i++)
            {
                var frame = this[i];

                using (var fp = frame.Profile)
                    if (fp.Stream == stream && (format == Format.Any || fp.Format == format))
                        return frame as T;

                frame.Dispose();
            }

            return null;
        }
        public T FirstOrDefault<T>(Predicate<Frame> predicate) where T : Frame
        {
            for (int i = 0; i < Count; i++)
            {
                var frame = this[i];

                if (predicate(frame))
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

        public int Count { get; internal set; }

        public Frame this[int index]
        {
            get
            {
                var ptr = NativeMethods.rs2_extract_frame(Instance.Handle, index, out var error);
                return Frame.CreateFrame(ptr);
            }
        }
        public Frame this[Stream stream, int index = 0] => FirstOrDefault<Frame>(f =>
        {
            using (var p = f.Profile)
                return p.Stream == stream && p.Index == index;
        });
        public Frame this[Stream stream, Format format, int index = 0] => FirstOrDefault<Frame>(f =>
        {
            using (var p = f.Profile)
                return p.Stream == stream && p.Format == format && p.Index == index;
        });


        internal FrameSet(IntPtr ptr)
        {
            Instance = new HandleRef(this, ptr);
            Count = NativeMethods.rs2_embedded_frames_count(Instance.Handle, out var error);
            enumerator = new FrameEnumerator(this);
            disposables = new List<IDisposable>();
        }

        #region IDisposable Support
        internal bool disposedValue = false; // To detect redundant calls TODO: internal dispose control should never be externally influenced

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

                disposables.ForEach(d => d?.Dispose());

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

            Pool.Release(this); //TODO: Should be reimplemented as Threadsafe Pool.
        }

        public void ForEach(Action<Frame> action)
        {
            for (int i = 0; i < Count; i++)
            {
                using (var frame = this[i])
                    action(frame);
            }

        }

        public void Release(FrameSet t)
        {
            lock (locker)
            {
                stack.Push(t);
            }
        }
    }

    public class FrameEnumerator : IEnumerator<Frame>
    {
        private readonly FrameSet fs;
        private Frame current;
        private int index;

        public FrameEnumerator(FrameSet fs)
        {
            this.fs = fs;
            index = 0;
            current = default(Frame);
        }
                
        public void AddDisposable(IDisposable disposable)
        {
            disposables.Add(disposable);
        }

        public static FrameSet FromFrame(Frame composite)
        {
            if (composite.IsComposite)
            {
                NativeMethods.rs2_frame_add_ref(composite.Instance.Handle, out object error);
                return Pool.Get(composite.Instance.Handle);
            }

            throw new ArgumentException("The frame is a not composite frame", nameof(composite));
        }
        [Obsolete("This method is obsolete. Use DisposeWith method instead")]
        public static FrameSet FromFrame(Frame composite, FramesReleaser releaser)
            => FromFrame(composite).DisposeWith(releaser);

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
