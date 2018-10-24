using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public class FrameSet : ICompositeDisposable, IEnumerable<Frame>
    {
        internal HandleRef m_instance;
        internal int m_count;
        internal static readonly FrameSetPool Pool = new FrameSetPool();
        internal readonly FrameEnumerator m_enum;

        public IntPtr NativePtr { get { return m_instance.Handle; } }

        public Frame AsFrame()
        {
            object error;
            NativeMethods.rs2_frame_add_ref(m_instance.Handle, out error);
            return Frame.CreateFrame(m_instance.Handle);
        }

        public static FrameSet FromFrame(Frame composite)
        {
            if (composite.IsComposite)
            {
                object error;
                NativeMethods.rs2_frame_add_ref(composite.m_instance.Handle, out error);
                return FrameSet.Pool.Get(composite.m_instance.Handle);
            }
            throw new ArgumentException("The frame is a not composite frame", nameof(composite));
        }

        [Obsolete("This method is obsolete. Use DisposeWith method instead")]
        public static FrameSet FromFrame(Frame composite, FramesReleaser releaser)
        {
            return FromFrame(composite).DisposeWith(releaser);
        }

        public void ForEach(Action<Frame> action)
        {
            for (int i = 0; i < m_count; i++)
            {
                using (var frame = this[i])
                    action(frame);
            }
        }

        public T FirstOrDefault<T>(Stream stream, Format format = Format.Any) where T : Frame
        {
            for (int i = 0; i < m_count; i++)
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
            for (int i = 0; i < m_count; i++)
            {
                var frame = this[i];
                if (predicate(frame))
                    return frame as T;
                frame.Dispose();
            }
            return null;
        }

        public DepthFrame DepthFrame
        {
            get
            {
                return FirstOrDefault<DepthFrame>(Stream.Depth, Format.Z16);
            }
        }

        public VideoFrame ColorFrame
        {
            get
            {
                return FirstOrDefault<VideoFrame>(Stream.Color);
            }
        }

        public VideoFrame InfraredFrame
        {
            get
            {
                return FirstOrDefault<VideoFrame>(Stream.Infrared);
            }
        }

        public IEnumerator<Frame> GetEnumerator()
        {
            m_enum.Reset();
            return m_enum;
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            m_enum.Reset();
            return m_enum;
        }

        public int Count
        {
            get
            {
                return m_count;
            }
        }

        public Frame this[int index]
        {
            get
            {
                object error;
                var ptr = NativeMethods.rs2_extract_frame(m_instance.Handle, index, out error);
                return Frame.CreateFrame(ptr);
            }
        }

        public Frame this[Stream stream, int index = 0]
        {
            get
            {
                return FirstOrDefault<Frame>(f =>
                {
                    using (var p = f.Profile)
                        return p.Stream == stream && p.Index == index;
                });
            }
        }


        public Frame this[Stream stream, Format format, int index = 0]
        {
            get
            {
                return FirstOrDefault<Frame>(f =>
                {
                    using (var p = f.Profile)
                        return p.Stream == stream && p.Format == format && p.Index == index;
                });
            }
        }

        internal FrameSet(IntPtr ptr)
        {
            m_instance = new HandleRef(this, ptr);
            object error;
            m_count = NativeMethods.rs2_embedded_frames_count(m_instance.Handle, out error);
            m_enum = new FrameEnumerator(this);
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
            if (m_instance.Handle != IntPtr.Zero)
                NativeMethods.rs2_release_frame(m_instance.Handle);
            m_instance = new HandleRef(this, IntPtr.Zero);
            Pool.Release(this);
        }

        internal readonly List<IDisposable> disposables = new List<IDisposable>();
        public void AddDisposable(IDisposable disposable)
        {
            disposables.Add(disposable);
        }
    }

    public static class FrameSetExtensions {
        public static FrameSet AsFrameSet(this Frame frame)
        {
            return FrameSet.FromFrame(frame);
        }
    }

    public class FrameSetPool
    {
        readonly Stack<FrameSet> stack = new Stack<FrameSet>();
        readonly object locker = new object();
        public FrameSet Get(IntPtr ptr)
        {
            lock (locker)
            {
                if (stack.Count != 0)
                {
                    FrameSet f = stack.Pop();
                    f.m_instance = new HandleRef(f, ptr);
                    f.disposedValue = false;
                    object error;
                    f.m_count = NativeMethods.rs2_embedded_frames_count(f.m_instance.Handle, out error);
                    f.m_enum.Reset();
                    //f.m_disposable = new EmptyDisposable();
                    f.disposables.Clear();
                    return f;
                }
                else
                {
                    return new FrameSet(ptr);
                }
            }

        }

        public void Release(FrameSet t)
        {
            stack.Push(t);
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

        public Frame Current
        {
            get
            {
                return current;
            }
        }

        object IEnumerator.Current
        {
            get
            {
                if (index == 0 || index == fs.m_count + 1)
                {
                    throw new InvalidOperationException();
                }
                return Current;
            }
        }

        public void Dispose()
        {
            // Method intentionally left empty.
        }

        public bool MoveNext()
        {
            if ((uint)index < (uint)fs.m_count)
            {
                object error;
                var ptr = NativeMethods.rs2_extract_frame(fs.m_instance.Handle, index, out error);
                current = Frame.CreateFrame(ptr);
                index++;
                return true;
            }
            index = fs.m_count + 1;
            current = null;
            return false;
        }

        public void Reset()
        {
            index = 0;
            current = null;
        }
    }
}
