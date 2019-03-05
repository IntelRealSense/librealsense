using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public class FrameSet : ICompositeDisposable, IEnumerable<Frame>
    {
        readonly static ObjectPool Pool = new ObjectPool((obj, ptr) =>
        {
            var fs = obj as FrameSet;
            fs.m_instance = new HandleRef(fs, ptr);
            fs.disposedValue = false;
            object error;
            fs.m_count = NativeMethods.rs2_embedded_frames_count(fs.m_instance.Handle, out error);
            fs.m_enum.Reset();
            fs.disposables.Clear();
        });


        internal HandleRef m_instance;
        internal int m_count;
        internal readonly Enumerator m_enum;

        public IntPtr NativePtr { get { return m_instance.Handle; } }

        public Frame AsFrame()
        {
            object error;
            NativeMethods.rs2_frame_add_ref(m_instance.Handle, out error);
            return Frame.Create(m_instance.Handle);
        }

        public static FrameSet FromFrame(Frame composite)
        {
            if (composite.IsComposite)
            {
                object error;
                NativeMethods.rs2_frame_add_ref(composite.m_instance.Handle, out error);
                return Create(composite.m_instance.Handle);
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
            return FirstOrDefault(stream, format)?.As<T>();
        }

        public Frame FirstOrDefault(Stream stream, Format format = Format.Any)
        {
            for (int i = 0; i < m_count; i++)
            {
                var frame = this[i];
                using (var fp = frame.Profile)
                    if (fp.Stream == stream && (format == Format.Any || fp.Format == format))
                        return frame;
                frame.Dispose();
            }
            return null;
        }

        public T FirstOrDefault<T>(Predicate<T> predicate) where T : Frame
        {
            object error;
            for (int i = 0; i < m_count; i++)
            {
                var ptr = NativeMethods.rs2_extract_frame(m_instance.Handle, i, out error);
                var frame = Frame.Create<T>(ptr);
                if (predicate(frame))
                    return frame;
                frame.Dispose();
            }
            return null;
        }

        public DepthFrame DepthFrame
        {
            get
            {
                return FirstOrDefault(Stream.Depth, Format.Z16)?.As<DepthFrame>();
            }
        }

        public VideoFrame ColorFrame
        {
            get
            {
                return FirstOrDefault(Stream.Color)?.As<VideoFrame>();
            }
        }

        public VideoFrame InfraredFrame
        {
            get
            {
                return FirstOrDefault(Stream.Infrared)?.As<VideoFrame>();
            }
        }

        public VideoFrame FishEyeFrame
        {
            get
            {
                return FirstOrDefault(Stream.Fisheye)?.As<VideoFrame>();
            }
        }

        public PoseFrame PoseFrame
        {
            get
            {
                return FirstOrDefault(Stream.Pose)?.As<PoseFrame>();
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
                return Frame.Create(ptr);
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
            m_enum = new Enumerator(this);
        }

        public static FrameSet Create(IntPtr ptr) {
            return Pool.Get<FrameSet>(ptr);
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
            FrameSet.Pool.Release(this);
        }

        internal readonly List<IDisposable> disposables = new List<IDisposable>();
        public void AddDisposable(IDisposable disposable)
        {
            disposables.Add(disposable);
        }

        internal sealed class Enumerator : IEnumerator<Frame>
        {
            private readonly FrameSet fs;
            private Frame current;
            private int index;

            public Enumerator(FrameSet fs)
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
                    current = Frame.Create(ptr);
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

    public static class FrameSetExtensions {
        public static FrameSet AsFrameSet(this Frame frame)
        {
            return FrameSet.FromFrame(frame);
        }
    }
}
