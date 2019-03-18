using System;
using System.Runtime.InteropServices;
using System.Security;

namespace Intel.RealSense.Base
{
    [SuppressUnmanagedCodeSecurity]
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void Deleter(IntPtr ptr);

    /// <summary>
    /// Native handle with deleter delegate to release unmanaged resources
    /// </summary>
    //TODO: CriticalFinalizerObject & CER
    internal sealed class DeleterHandle : IDisposable
    {
        internal IntPtr handle;
        private Deleter m_deleter;

        public IntPtr Handle => handle;
        public bool IsInvalid => handle == IntPtr.Zero;


        public DeleterHandle(IntPtr ptr, Deleter deleter)
        {
            handle = ptr;
            m_deleter = deleter;
        }

        internal void Reset(IntPtr ptr)
        {
            handle = ptr;
            GC.ReRegisterForFinalize(this);
        }

        internal void Reset(IntPtr ptr, Deleter deleter)
        {
            m_deleter = deleter;
            Reset(ptr);
        }

        public void SetHandleAsInvalid()
        {
            handle = IntPtr.Zero;
            GC.SuppressFinalize(this);
        }

        ~DeleterHandle()
        {
            Console.WriteLine($"~{handle} {m_deleter?.Method}");
            Dispose(false);
        }

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        public void Dispose(bool disposing)
        {
            if (handle == IntPtr.Zero)
                return;
            m_deleter?.Invoke(handle);
            handle = IntPtr.Zero;
        }
    }
}