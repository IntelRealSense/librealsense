// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Runtime.InteropServices;

    public class FramesReleaser : ICompositeDisposable
    {
        private readonly List<IDisposable> disposables = new List<IDisposable>();

        /// <summary>
        /// Add an object to a releaser (if one is provided) and return the object
        /// </summary>
        /// <typeparam name="T">type implementing <see cref="IDisposable"/></typeparam>
        /// <param name="releaser">Optional <see cref="FramesReleaser"/> to add and object to, or null</param>
        /// <param name="obj">object to release</param>
        /// <returns>the <paramref name="obj"/></returns>
        [Obsolete("This method is obsolete. Use AddDisposable method instead")]
        public static T ScopedReturn<T>(FramesReleaser releaser, T obj)
            where T : IDisposable
        {
            if (releaser != null)
            {
                releaser.AddDisposable(obj);
            }

            return obj;
        }

        /// <summary>
        /// Add an object to a releaser
        /// </summary>
        /// <param name="disposable">object to add</param>
        public void AddDisposable(IDisposable disposable)
        {
            disposables.Add(disposable);
        }

        private bool disposedValue = false; // To detect redundant calls

        protected virtual void Dispose(bool disposing)
        {
            if (!disposedValue)
            {
                disposables.ForEach(d => d?.Dispose());
                disposedValue = true;
            }
        }

        ~FramesReleaser()
        {
            Dispose(false);
        }

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }
    }
}