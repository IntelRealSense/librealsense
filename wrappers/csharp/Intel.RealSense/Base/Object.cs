using System;

namespace Intel.RealSense.Base
{
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
}
