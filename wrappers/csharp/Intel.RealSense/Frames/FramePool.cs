using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public class FramePool<T> where T : Frame
    {
        readonly Stack<T> stack = new Stack<T>();
        readonly object locker = new object();
        readonly Func<IntPtr, T> factory;

        public FramePool(Func<IntPtr, T> factory)
        {
            this.factory = factory;
        }

        public T Get(IntPtr ptr)
        {
            
            lock (locker)
            {
                if(stack.Count == 0)
                    return factory(ptr);
                T f = stack.Pop();
                f.m_instance = new HandleRef(f, ptr);
                f.disposedValue = false;
                //NativeMethods.rs2_keep_frame(ptr);
                return f;
            }
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
