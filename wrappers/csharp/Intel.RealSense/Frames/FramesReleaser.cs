using System;
using System.Collections.Generic;

namespace Intel.RealSense.Frames
{
    public class FramesReleaser : ICompositeDisposable
    {
        private readonly HashSet<IDisposable> disposables;

        public FramesReleaser()
        {
            disposables = new HashSet<IDisposable>();
            disposedValue = false;
        }

        public void AddDisposable(IDisposable disposable)
        {
            disposables.Add(disposable);
        }

        #region IDisposable Support
        internal bool disposedValue; // To detect redundant calls

        protected virtual void Dispose(bool disposing)
        {
            if (!disposedValue)
            {

                if (disposing)
                {
                    foreach (var o in disposables)
                        o.Dispose();
                }
                // TODO: free unmanaged resources (unmanaged objects) and override a finalizer below.
                // TODO: set large fields to null.

                disposedValue = true;
            }
        }
        // TODO: override a finalizer only if Dispose(bool disposing) above has code to free unmanaged resources.
        ~FramesReleaser()
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

        [Obsolete("This method is obsolete. Use AddDisposable method instead")]
        // Add an object to a releaser (if one is provided) and return the object
        public static T ScopedReturn<T>(FramesReleaser releaser, T obj) where T : IDisposable
        {
            if (releaser != null)
                releaser.AddDisposable(obj);

            return obj;
        }
    }
}