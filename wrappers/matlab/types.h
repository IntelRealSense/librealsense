#pragma once
#include "librealsense2/rs.hpp"
#include <memory>
#include <array>

// C API
template<> mxArray* MatlabParamParser::mx_wrapper_fns<rs2_extrinsics>::wrap(rs2_extrinsics&& val)
{
    const char* fnames[] = { "rotation", "translation" };
    mxArray* cell = mxCreateStructMatrix(1, 1, 2, fnames);
    mxSetField(cell, 0, "rotation", MatlabParamParser::wrap_array(val.rotation, 9));
    mxSetField(cell, 0, "translation", MatlabParamParser::wrap_array(val.translation, 3));
    return cell;
}
template<> mxArray* MatlabParamParser::mx_wrapper_fns<rs2_intrinsics>::wrap(rs2_intrinsics&& val)
{
    const char* fnames[] = { "width", "height", "ppx", "ppy", "fx", "fy", "model", "coeffs" };
    mxArray* cell = mxCreateStructMatrix(1, 1, 8, fnames);
    mxSetField(cell, 0, "width", MatlabParamParser::wrap(std::move(val.width)));
    mxSetField(cell, 0, "height", MatlabParamParser::wrap(std::move(val.height)));
    mxSetField(cell, 0, "ppx", MatlabParamParser::wrap(std::move(val.ppx)));
    mxSetField(cell, 0, "ppy", MatlabParamParser::wrap(std::move(val.ppy)));
    mxSetField(cell, 0, "fx", MatlabParamParser::wrap(std::move(val.fx)));
    mxSetField(cell, 0, "fy", MatlabParamParser::wrap(std::move(val.fy)));
    mxSetField(cell, 0, "model", MatlabParamParser::wrap(std::move(val.model)));
    mxSetField(cell, 0, "coeffs", MatlabParamParser::wrap_array(val.coeffs, 5));
    return cell;
}
template<> mxArray* MatlabParamParser::mx_wrapper_fns<rs2_motion_device_intrinsic>::wrap(rs2_motion_device_intrinsic&& val)
{
    const char* fnames[] = { "data", "noise_variances", "bias_variances"};
    mxArray* cell = mxCreateStructMatrix(1, 1, 3, fnames);
    mxSetField(cell, 0, "noise_variances", MatlabParamParser::wrap_array(val.noise_variances, 3));
    mxSetField(cell, 0, "bias_variances", MatlabParamParser::wrap_array(val.bias_variances, 3));

    // have to do data field manually for now because of multidimensional array. hope to extend make_array to cover this case
    using data_wrapper_t = mx_wrapper<std::remove_all_extents<decltype(rs2_motion_device_intrinsic::data)>::type>;
    mxArray* data_cell = mxCreateNumericMatrix(3, 4, data_wrapper_t::value::value, mxREAL);
    auto data_ptr = static_cast<double*>(mxGetData(data_cell));
    for (int y = 0; y < 3; ++y) for (int x = 0; x < 4; ++x) data_ptr[y + 3*x] = val.data[y][x];
    mxSetField(cell, 0, "data", data_cell);
    return cell;
}
template<> mxArray* MatlabParamParser::mx_wrapper_fns<rs2_vector>::wrap(rs2_vector&& val)
{
    using wrapper_t = mx_wrapper<float>;
    auto cells = mxCreateNumericMatrix(1, 3, wrapper_t::value::value, mxREAL);
    auto ptr = static_cast<typename wrapper_t::type*>(mxGetData(cells));
    ptr[0] = wrapper_t::type(val.x);
    ptr[1] = wrapper_t::type(val.y);
    ptr[2] = wrapper_t::type(val.z);
    return cells;
}
template<> mxArray* MatlabParamParser::mx_wrapper_fns<rs2_quaternion>::wrap(rs2_quaternion&& val)
{
    using wrapper_t = mx_wrapper<decltype(rs2_quaternion::x)>;
    auto cells = mxCreateNumericMatrix(1, 4, wrapper_t::value::value, mxREAL);
    auto ptr = static_cast<typename wrapper_t::type*>(mxGetData(cells));
    ptr[0] = wrapper_t::type(val.x);
    ptr[1] = wrapper_t::type(val.y);
    ptr[2] = wrapper_t::type(val.z);
    ptr[3] = wrapper_t::type(val.w);
    return cells;
}
template<> mxArray* MatlabParamParser::mx_wrapper_fns<rs2_pose>::wrap(rs2_pose&& val)
{
    const char* fnames[] = { "translation", "velocity", "acceleration", "rotation", "angular_velocity", "angular_acceleration", "tracker_confidence", "mapper_confidence" };
    mxArray* cell = mxCreateStructMatrix(1, 1, 8, fnames);
    mxSetField(cell, 0, "translation", MatlabParamParser::wrap(std::move(val.translation)));
    mxSetField(cell, 0, "velocity", MatlabParamParser::wrap(std::move(val.velocity)));
    mxSetField(cell, 0, "acceleration", MatlabParamParser::wrap(std::move(val.acceleration)));
    mxSetField(cell, 0, "rotation", MatlabParamParser::wrap(std::move(val.rotation)));
    mxSetField(cell, 0, "angular_velocity", MatlabParamParser::wrap(std::move(val.angular_velocity)));
    mxSetField(cell, 0, "angular_acceleration", MatlabParamParser::wrap(std::move(val.angular_acceleration)));
    mxSetField(cell, 0, "tracker_confidence", MatlabParamParser::wrap(std::move(val.tracker_confidence)));
    mxSetField(cell, 0, "mapper_confidence", MatlabParamParser::wrap(std::move(val.mapper_confidence)));
    return cell;
}

// rs_types.hpp
template<> mxArray* MatlabParamParser::mx_wrapper_fns<rs2::option_range>::wrap(rs2::option_range&& val)
{
    const char* fnames[] = { "min", "max", "step", "def" };
    mxArray* cell = mxCreateStructMatrix(1, 1, 4, fnames);
    mxSetField(cell, 0, "min", MatlabParamParser::wrap(std::move(val.min)));
    mxSetField(cell, 0, "max", MatlabParamParser::wrap(std::move(val.max)));
    mxSetField(cell, 0, "step", MatlabParamParser::wrap(std::move(val.step)));
    mxSetField(cell, 0, "def", MatlabParamParser::wrap(std::move(val.def)));
    return cell;
}
template<> struct MatlabParamParser::mx_wrapper_fns<rs2::region_of_interest>
{
    static rs2::region_of_interest parse(const mxArray* cell)
    {
        rs2::region_of_interest ret;
        ret.min_x = MatlabParamParser::parse<int>(mxGetField(cell, 0, "min_x"));
        ret.min_y = MatlabParamParser::parse<int>(mxGetField(cell, 0, "min_y"));
        ret.max_x = MatlabParamParser::parse<int>(mxGetField(cell, 0, "max_x"));
        ret.max_y = MatlabParamParser::parse<int>(mxGetField(cell, 0, "max_y"));
        return ret;
    }
    static mxArray* wrap(rs2::region_of_interest&& val)
    {
        const char* fnames[] = { "min_x", "min_y", "max_x", "max_y" };
        mxArray* cell = mxCreateStructMatrix(1, 1, 4, fnames);
        mxSetField(cell, 0, "min_x", MatlabParamParser::wrap(std::move(val.min_x)));
        mxSetField(cell, 0, "min_y", MatlabParamParser::wrap(std::move(val.min_y)));
        mxSetField(cell, 0, "max_x", MatlabParamParser::wrap(std::move(val.max_x)));
        mxSetField(cell, 0, "max_y", MatlabParamParser::wrap(std::move(val.max_y)));
        return cell;
    }
};

// rs_context.hpp

// rs_record_playback.hpp

// rs_device.hpp
template<> mxArray* MatlabParamParser::mx_wrapper_fns<rs2::device_list>::wrap(rs2::device_list&& var)
{
    // Device list is sent as a native array of (ptr, id) pairs to preserve lazy instantiation of devices
    size_t len = var.size();

    mxArray* vec = mxCreateCellMatrix(1, len);
    for (uint32_t i = 0; i < len; ++i)
    {
        using dl_wrap_t = mx_wrapper<rs2::device_list>;
        using idx_wrap_t = mx_wrapper<decltype(i)>;
        auto cells = mxCreateCellMatrix(1, 2);
        auto dl_cell = mxCreateNumericMatrix(1, 1, dl_wrap_t::value::value, mxREAL);
        mexLock(); // lock once for each created pointer
        *static_cast<dl_wrap_t::type*>(mxGetData(dl_cell)) = reinterpret_cast<dl_wrap_t::type>(new type_traits<rs2::device_list>::rs2_internal_t(var));
        auto idx_cell = mxCreateNumericMatrix(1, 1, idx_wrap_t::value::value, mxREAL);
        *static_cast<idx_wrap_t::type*>(mxGetData(idx_cell)) = static_cast<idx_wrap_t::type>(i);
        mxSetCell(cells, 0, dl_cell);
        mxSetCell(cells, 1, idx_cell);
        mxSetCell(vec, i, cells);
    }

    return vec;
}

// rs_sensor.hpp

// rs_frame.hpp
// TODO: Is there really not a cleaner way to deal with cloned stream_profiles?
template<typename T> struct MatlabParamParser::mx_wrapper_fns<T, typename std::enable_if<std::is_base_of<rs2::stream_profile, T>::value>::type>
{
    static T parse(const mxArray* cell)
    {
        using internal_t = typename type_traits<T>::rs2_internal_t;
        // since stream_profiles embed the raw pointer directly, we need to add on the extra level of indirection expected by from_internal.
        auto internal_p = mx_wrapper_fns<internal_t>::parse(cell);
        return traits_trampoline::from_internal<T>(&internal_p);
    }
    static mxArray* wrap(T&& val)
    {
        auto cells = mxCreateCellMatrix(1, 2);
        auto handle_cell = mxCreateNumericMatrix(1, 1, mxUINT64_CLASS, mxREAL);
        auto handle_ptr = static_cast<uint64_t*>(mxGetData(handle_cell));
        *handle_ptr = reinterpret_cast<uint64_t>(typename type_traits<T>::rs2_internal_t(val));

        auto own_cell = mxCreateNumericMatrix(1, 1, mxUINT64_CLASS, mxREAL);
        auto own_ptr = static_cast<uint64_t*>(mxGetData(own_cell));
        // if its cloned, give the wrapper ownership of the stream_profile
        if (val.is_cloned())
        {
            mexLock();
            *own_ptr = reinterpret_cast<uint64_t>(new std::shared_ptr<rs2_stream_profile>(val));
        }
        else *own_ptr = reinterpret_cast<uint64_t>(nullptr);

        mxSetCell(cells, 0, handle_cell);
        mxSetCell(cells, 1, own_cell);
        return cells;
    }
    static void destroy(const mxArray* cell)
    {
        // we parse a std::shard_ptr<rs2_stream_profile>* because that is how the ownership
        // pointer is stored, which is the one we need to destroy (assuming it exists)
        auto ptr = mx_wrapper_fns<std::shared_ptr<rs2_stream_profile>*>::parse(cell);
        if (!ptr) return; // only destroy if wrapper owns the profile
        delete ptr;
        mexUnlock();
    }
};

// rs2::frame does its own internal refcounting, so this interfaces with it properly
template<typename T> struct MatlabParamParser::mx_wrapper_fns<T, typename std::enable_if<std::is_base_of<rs2::frame, T>::value>::type>
{
    static mxArray* wrap(T&& var)
    {
        using wrapper_t = mx_wrapper<T>;
        mxArray *cell = mxCreateNumericMatrix(1, 1, wrapper_t::value::value, mxREAL);
        auto *outp = static_cast<uint64_t*>(mxGetData(cell));
        auto inptr = typename type_traits<T>::rs2_internal_t(var);
        rs2_frame_add_ref(inptr, nullptr);
        mexLock(); // Wrapper holds a reference to the internal type, which won't get destroyed until the wrapper lets go
        *outp = reinterpret_cast<uint64_t>(inptr);
        return cell;
    }
    static T parse(const mxArray* cell)
    {
        auto inptr = mx_wrapper_fns<typename type_traits<T>::rs2_internal_t>::parse(cell);
        rs2_frame_add_ref(inptr, nullptr);
        // Can the jump through rs2::frame happen automatically? e.g. is rs2::points(rs2_frame* inptr) equivalent to rs2::points(rs2::frame(rs2_frame* inptr))?
        // If not, might have to use trampolining like stream_profile
        return T(inptr);
    }
    static void destroy(const mxArray* cell)
    {
        auto inptr = mx_wrapper_fns<typename type_traits<T>::rs2_internal_t>::parse(cell);
        rs2_release_frame(inptr);
        mexUnlock(); // Wrapper holds a reference to the internal type, which won't get destroyed until the wrapper lets go
    }
};
template <> mxArray* MatlabParamParser::wrap_array<rs2::vertex>(const rs2::vertex* var, size_t length)
{
    using wrapper_t = mx_wrapper<float>;
    auto cells = mxCreateNumericMatrix(length, 3, wrapper_t::value::value, mxREAL);
    auto ptr = static_cast<typename wrapper_t::type*>(mxGetData(cells));
    for (int x = 0; x < length; ++x) {
        ptr[0 * length + x] = wrapper_t::type(var[x].x);
        ptr[1 * length + x] = wrapper_t::type(var[x].y);
        ptr[2 * length + x] = wrapper_t::type(var[x].z);
    }
    return cells;
}
template <> mxArray* MatlabParamParser::wrap_array<rs2::texture_coordinate>(const rs2::texture_coordinate* var, size_t length)
{
    using wrapper_t = mx_wrapper<float>;
    auto cells = mxCreateNumericMatrix(length, 2, wrapper_t::value::value, mxREAL);
    auto ptr = static_cast<typename wrapper_t::type*>(mxGetData(cells));
    for (int x = 0; x < length; ++x) {
        ptr[0 * length + x] = wrapper_t::type(var[x].u);
        ptr[1 * length + x] = wrapper_t::type(var[x].v);
    }
    return cells;
}

// rs_processing.hpp
//template<> rs2::process_interface* MatlabParamParser::mx_wrapper_fns<rs2::process_interface*>::parse(const mxArray* cell)
//{
//    using traits_t = type_traits<rs2::process_interface>;
//    auto ptr = static_cast<traits_t::rs2_internal_t*>(mxGetData(cell));
//    if (ptr->type == traits_t::carrier_enum::value) return reinterpret_cast<traits_t::carrier_t*>(ptr->ptr)->get();
//    mexErrMsgTxt("Error parsing argument, object is not a process_interface");
//}

// rs_pipeline.hpp
