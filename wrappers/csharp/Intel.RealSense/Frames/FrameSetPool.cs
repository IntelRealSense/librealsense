using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
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
            lock (locker)
            {
                stack.Push(t);
            }
        }
    }
}
