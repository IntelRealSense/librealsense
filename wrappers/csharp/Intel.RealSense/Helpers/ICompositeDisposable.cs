// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;

    /// <summary>
    /// A container for disposing of multiple <see cref="IDisposable"/> objects
    /// </summary>
    public interface ICompositeDisposable : IDisposable
    {
        /// <summary>
        /// Add an <see cref="IDisposable"/> object to to be disposed along with this instance
        /// </summary>
        /// <param name="disposable">an <see cref="IDisposable"/> to to be disposed along with this instance</param>
        void AddDisposable(IDisposable disposable);
    }

    // https://leeoades.wordpress.com/2012/08/29/neat-disposal-pattern/
    public static class DisposableExtensions
    {
        /// <summary>
        /// Generic extension method to help dispose of objects
        /// </summary>
        /// <typeparam name="T">type implementing <see cref="IDisposable"/></typeparam>
        /// <param name="disposable"><see cref="IDisposable"/> object to add to <paramref name="composite"/></param>
        /// <param name="composite">composite disposable container</param>
        /// <returns>the <paramref name="disposable"/> object</returns>
        public static T DisposeWith<T>(this T disposable, ICompositeDisposable composite)
            where T : class, IDisposable
        {
            if (disposable == null || composite == null)
            {
                return disposable;
            }

            composite.AddDisposable(disposable);
            return disposable;
        }
    }
}