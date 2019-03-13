using System;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
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

    /// <summary>
    /// Base class for disposable objects with native resources
    /// </summary>
    public abstract class Object : IDisposable
    {
        //TODO: rename, kept for backwards compatiblity
        internal readonly DeleterHandle m_instance;

        public IntPtr Handle
        {
            get
            {
                if (m_instance.IsInvalid)
                    throw new ObjectDisposedException(GetType().Name);
                return m_instance.Handle;
            }
        }

        protected Object(IntPtr ptr, Deleter deleter)
        {
            if (ptr == IntPtr.Zero)
                throw new ArgumentNullException(nameof(ptr));
            m_instance = new DeleterHandle(ptr, deleter);
        }

        protected virtual void Dispose(bool disposing)
        {
            m_instance.Dispose();
        }

        public void Dispose()
        {
            this.Dispose(true);
        }
    }

    /// <summary>
    /// Base class for objects in an <cref see="ObjectPool">ObjectPool</cref>
    /// </summary>
    public abstract class PooledObject : Object
    {
        protected readonly static ObjectPool Pool = new ObjectPool();

        protected PooledObject(IntPtr ptr, Deleter deleter) : base(ptr, deleter)
        {
        }

        internal abstract void Initialize();

        protected override void Dispose(bool disposing)
        {
            base.Dispose(disposing);
            Pool.Release(this);
        }
    }
}
