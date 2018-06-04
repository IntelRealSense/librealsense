#pragma once
#include "librealsense2/rs.hpp"
#include <memory>

// C API

// rs_types.hpp

// rs_context.hpp

// rs_record_playback.hpp

// rs_device.hpp
template<> static mxArray* MatlabParamParser::mx_wrapper_fns<rs2::device_list>::wrap(rs2::device_list&& var)
{
    // Device list is sent as a native array of (ptr, id) pairs to preserve lazy instantiation of devices
    size_t len = var.size();
    mxArray* vec = mxCreateNumericMatrix(len, 2, mxUINT64_CLASS, mxREAL);
    uint64_t* outp = static_cast<uint64_t*>(mxGetData(vec));

    for (int i = 0; i < len; ++i)
    {
        mexLock(); // lock once for each created pointer
        outp[i] = reinterpret_cast<uint64_t>(new type_traits<rs2::device_list>::rs2_internal_t(var));
        outp[i + len] = i;
    }

    return vec;
}

// rs_sensor.hpp
template<> static mxArray* MatlabParamParser::mx_wrapper_fns<std::vector<rs2::sensor>>::wrap(std::vector<rs2::sensor>&& var)
{
    // TODO: merge with wrap_array function
    // we wrap this as a native array of sensors
    mxArray* vec = mxCreateNumericMatrix(var.size(), 1, mxUINT64_CLASS, mxREAL);
    uint64_t* outp = static_cast<uint64_t*>(mxGetData(vec));
    for (int i = var.size() - 1; i >= 0; --i) {
        mexLock(); // lock once for each created pointer
        outp[i] = reinterpret_cast<uint64_t>(new type_traits<rs2::sensor>::rs2_internal_t(var[i]));
    }
    return vec;
}

// rs_frame.hpp
// TODO: Is there really not a cleaner way to deal with cloned stream_profiles?
template<> static mxArray* MatlabParamParser::mx_wrapper_fns<rs2::stream_profile>::wrap(rs2::stream_profile&& var)
{
    mxArray *cells = mxCreateNumericMatrix(1, 2, mxUINT64_CLASS, mxREAL);
    auto *outp = static_cast<uint64_t*>(mxGetData(cells));
    // if its cloned, give the wrapper ownership of the stream_profile
    if (var.is_cloned())
    {
        mexLock();
        outp[0] = reinterpret_cast<uint64_t>(new std::shared_ptr<rs2_stream_profile>(var));
    }
    else outp[0] = reinterpret_cast<uint64_t>(nullptr);

    outp[1] = reinterpret_cast<uint64_t>(type_traits<rs2::stream_profile>::rs2_internal_t(var));
    return cells;
}
template<> static void MatlabParamParser::mx_wrapper_fns<rs2::stream_profile>::destroy(const mxArray* cell)
{
    auto ptr = mx_wrapper_fns<std::shared_ptr<rs2_stream_profile>*>::parse(cell);
    if (!ptr) return; // only destroy if wrapper owns the profile
    delete ptr;
    mexUnlock();
}
// rs2::frame does its own internal refcounting, so this interfaces with it properly
template<typename T> struct MatlabParamParser::mx_wrapper_fns<T, typename std::enable_if<std::is_base_of<rs2::frame, T>::value>::type>
{
    static mxArray* wrap(T&& var)
    {
        mxArray *cell = mxCreateNumericMatrix(1, 1, mxUINT64_CLASS, mxREAL);
        auto *outp = static_cast<uint64_t*>(mxGetData(cell));
        auto inptr = type_traits<T>::rs2_internal_t(var);
        rs2_frame_add_ref(inptr, nullptr);
        mexLock(); // Wrapper holds a reference to the internal type, which won't get destroyed until the wrapper lets go
        *outp = reinterpret_cast<uint64_t>(inptr);
        return cell;
    }
    static T parse(const mxArray* cell)
    {
        auto inptr = mx_wrapper_fns<type_traits<T>::rs2_internal_t>::parse(cell);
        rs2_frame_add_ref(inptr, nullptr);
        return T(inptr);
    }
    static void destroy(const mxArray* cell)
    {
        auto inptr = mx_wrapper_fns<type_traits<T>::rs2_internal_t>::parse(cell);
        rs2_release_frame(inptr);
        mexUnlock(); // Wrapper holds a reference to the internal type, which won't get destroyed until the wrapper lets go
    }
};

// rs_processing.hpp

// rs_pipeline.hpp
