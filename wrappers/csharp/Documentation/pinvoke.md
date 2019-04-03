# Platform Invoke

This document contains notes on interoperability with the native SDK.

The .NET wrapper uses Platform Invoke (P/Invoke) to call functions from the unmanaged `realsense2` library.
It's a bit more tedious and error-prone than C++/CLI, but has better support across .NET implementations (.NET Framework, Core, Mono & Xamarin)

This means that the .NET wrapper exposes an object oriented API by calling into C functions, which is similar to what the C++ API does, with all the benefits and shortcomings of running in a managed environment, some of which discussed here.

## Lifetime & Resources

Just as the standard `System.IO.FileStream` is a wrapper around native OS file handles, most objects in the library are wrappers around their native counterparts.  

Unmanaged resources are handled by the .NET [IDisposable](https://docs.microsoft.com/en-us/dotnet/api/system.idisposable.dispose) interface, which provides a predictable way to release resources.
You should either directly call `Dispose` as soon as you're finished using the instance, or indirectly with the [using](https://docs.microsoft.com/en-us/dotnet/csharp/language-reference/keywords/using-statement) statement.

This is especially important for the `Frame` class and its derivatives, as the RealSense™ SDK uses a native frame pool, failing to release frames will cause the queue to fill, and no new frames will arrive.

> You cannot count on the GC to release these objects, it's unpredictable, it will not keep up, and isn't even guaranteed to run.

Trying to use a disposed object for a native call should raise an `ObjectDisposedException`.

## Memory & GC

Garbage collection means a *stop-the-world* event pauses all running threads while looking for unreferenced objects. This will lead to extreme spikes in frame times and can cause the RealSense™ device to drop frames.

We use an object pool to avoid allocating memory, and relieve GC pressure. Objects are rented from the pool, reinitialized to wrap a new unmanaged resource, and are released back to the pool when disposed.

Wrapped objects are finalizable, and will free their unmanaged resources before being collected, this should avoid native resource leaks in most but catastrophic failures.

## NativeMethods & Pointers

The `NativeMethods` class holds the extern function definitions (`rs2_*`) calling into the native `realsense2` library.

There are two types of `IntPtr` pointers used in these calls:

1. **Unmanaged** pointers coming from the native SDK itself

    They are usually wrapped and operated on by the exposed API, or passed around in the internals of library.

    These pointers are invisible to the GC, they won't be relocated, so they are safe to use in pinvoke calls, and should be freed when required by the SDK.

    For instance:
    * The pointer returned from `NativeMethods.rs2_pipeline_wait_for_frames` which is wrapped in a `Frame` object, and must be released with `NativeMethods.rs2_release_frame`

    * `Frame.Data` returns a pointer to the start of the frame data, it comes from a call to `NativeMethods.rs2_get_frame_data` with the above pointer.
    It's lifetime is bound to the frame and will be released along with it.

2. Pointers to **managed** objects

    These can be relocated or freed by the GC (even while being used in native code) and special care is taken to ensure their safe use in unmanaged calls.

    Some examples:
    * In `Pipeline.Start(FrameCallback cb)` the `cb` delegate has to be kept alive since it will be called from unmanaged code.

    * In `VideoFrame.CopyTo<T>(T[] array)` the array has to pinned so the GC won't move it while we're copying frame data.
