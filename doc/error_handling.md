# librealsense Error Handling Scheme

## Overview
librealsense project relies on exception-based error handling. This page will review the expectations, mechanics and the post-conditions implied by this model, compared to return-code based error handling.

## Errors, Warnings and Exceptions
Whenever librealsense encounters failure of a system call it will record it as either `WARNING` or `ERROR` log entry. (see [troubleshooting.md](./troubleshooting.md) to enable librealsense log) If the problem is internally recoverable, it will be marked as `WARNING`. If the problem requires user intervention, `exception` object will be created together with `ERROR` log entry.

If the operation was not issued by the user, but rather by one of the background threads, the exception **should never** reach the global signal handler of the process. Instead, it will be converted to `rs_notification` object, and sent to the user through the notifications callback.

If the operation was issued by the user, `exception` object will be safely marshalled between module boundaries at `rs.cpp` level, by converting it to `rs_error` object. This new object can be safely consumed by C program. If the application is using `rs.hpp`, the C++ wrapper will convert `rs_error` object back to an exception object. This exception will always derive from `rs2::error` and `std::runtime_error` regardless of type of exception that caused it.

The user is safe to assume that:
* All successful operations are completed without exceptions being in-use internally by the library.
* If the API implied a state transition, but the call failed with an exception, state will not change. (will be rolled-back by the library if needed)
* As long as the camera remains connected, all reasonable usage models of the device should be possible to achieve without relying on catching exceptions. In particular the library will always provide APIs for dynamic discovery of capabilities (supported methods, valid control ranges, etc...)

*Note Regarding Disconnects:*
While librealsense2 provides a mechanism to get notified and recover from camera disconnects, any operations issued between the physical disconnect and the notification are bound to fail. `rsutil2.hpp` provides a convenience class for dealing with camera disconnects, including a way to query if the device is still connected.

*Note Regarding Destructors:*
The only case when an exception will be handled internally without notifying the user, is as part of object destruction flow. For example, when the `device` object is being destroyed we might get system call failures due to the device being disconnected. Any such errors will be documented in the log but not risen to the user to avoid throw-in-destructor problems.  

## Types of Errors
In addition to string representation of what went wrong, librealsense exceptions also provide `get_failed_function` and `get_failed_args` methods to query failed function names and argument values respectively, and error type. `rs.hpp` will automatically map error types into the following inheritance structure:
```
std::exception
└── std::runtime_error
    └── rs2::error
        ├── rs2::unrecoverable_error
        |   ├── rs2::camera_disconnected_error        // Camera was disconnected during the call
        |   ├── rs2::backend_error                    // Failure returned from system call
        |   └── rs2::device_in_recovery_mode_error    // Device requires firmware update
        └── rs2::recoverable_error
            ├── rs2::invalid_value_error              // One of the parameters passed to librealsense had invalid value
            ├── rs2::wrong_api_call_sequence_error    // API pre-condition was not met
            └── rs2::not_implemented_error            // The operation is either not implemented or not supported for this device
```
Getting `rs2::error` and not one of the concrete error classes could be considered a bug.
This inheritance structure lets the user organize his code, from the most specific case to the most general:
```cpp
try
{
    dev.start();
}
// start with the most specific handlers
catch (const rs2::camera_disconnected_error& e)
{
    cerr << "Camera was disconnected! Please connect it back" << endl;
    // wait for connect event
}
// continue with more general cases
catch (const rs2::recoverable_error& e)
{
    cerr << "Operation failed, please try again" << endl;
}
// you can also catch "anything else" raised from the library by catching rs2::error
catch (const rs2::error& e)
{
    cerr << "Some other error occurred!" << endl;
}
```
For the full list of FW errors, please refer to [ds5-private.h](../src/ds5/ds5-private.h)


## Callbacks
With current design, there is no way to propagate exceptions raised inside user callbacks into the library.
If user callback raises an exception, such event will be documented in the log as `ERROR`.
