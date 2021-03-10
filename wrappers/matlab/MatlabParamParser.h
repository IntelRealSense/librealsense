#pragma once
#include "mex.h"
#include "matrix.h"
#include <string>
#include <memory>
#include <vector>
#include <array>
#include <type_traits>

namespace std
{
template <bool B> using bool_constant = integral_constant<bool, B>;
}

template <typename T> using is_basic_type = std::bool_constant<std::is_arithmetic<T>::value || std::is_pointer<T>::value || std::is_enum<T>::value>;
template <typename T> struct is_array_type : std::false_type {};
template <typename T> struct is_array_type<std::vector<T>> : std::true_type { using inner_t = T; };

// TODO: consider using nested impl/detail namespace
namespace MatlabParamParser
{
    // in charge of which overloads to use
    
    // helper information for overloaded functions
    template <typename T, typename voider = void> struct mx_wrapper_fns
    {
        static T parse(const mxArray* cell);
        static mxArray* wrap(T&& var);
        static void destroy(const mxArray* cell);
    };
    template <typename T, typename = void> struct mx_wrapper;
    // enums are exposed as 64bit integers, using the underlying type to determine signedness
    template <typename T> struct mx_wrapper<T, typename std::enable_if<std::is_enum<T>::value>::type>
    {
    private:
        using signed_t = std::integral_constant<mxClassID, mxINT64_CLASS>;
        using unsigned_t = std::integral_constant<mxClassID, mxUINT64_CLASS>;
        using value_t = typename std::conditional<bool(std::is_signed<typename std::underlying_type<T>::type>::value), signed_t, unsigned_t>::type;
    public:
//        static const mxClassID value = /*value_t::value*/ signed_t::value;
        using value = signed_t;
        using type = typename std::conditional<std::is_same<value_t, signed_t>::value, int64_t, uint64_t>::type;
    };
    // pointers are cast to uint64_t because matlab doesn't have a concept of pointers
    template <typename T> struct mx_wrapper<T, typename std::enable_if<std::is_pointer<T>::value>::type>
    {
 //       static const mxClassID value = mxUINT64_CLASS;
        using value = std::integral_constant<mxClassID, mxUINT64_CLASS>;
        using type = uint64_t;
    };
    // bools are exposed as matlab's native logical type
    template <> struct mx_wrapper<bool, void>
    {
//        static const mxClassID value = mxLOGICAL_CLASS;
        using value = std::integral_constant<mxClassID, mxLOGICAL_CLASS>;
        using type = mxLogical;
    };
    // floating points are exposed as matlab's native double type
    template <typename T> struct mx_wrapper<T, typename std::enable_if<std::is_floating_point<T>::value>::type>
    {
//        static const mxClassID value = mxDOUBLE_CLASS;
        using value = std::integral_constant<mxClassID, mxDOUBLE_CLASS>;
        using type = double;
    };
    // integral types are exposed like enums
    template <typename T> struct mx_wrapper<T, typename std::enable_if<std::is_integral<T>::value>::type>
    {
    private:
        using signed_t = std::integral_constant<mxClassID, mxINT64_CLASS>;
        using unsigned_t = std::integral_constant<mxClassID, mxUINT64_CLASS>;
        using value_t = typename std::conditional<std::is_signed<T>::value, signed_t, unsigned_t>::type;
    public:
//        static const mxClassID value = value_t::value;
        using value = value_t;
        using type = typename std::conditional<std::is_same<value_t, signed_t>::value, int64_t, uint64_t>::type;
    };
    // uint8_t, uint16_t, uint32_t, uint64_t, int8_t, int16_t, int32_t, and int64_t
    // are exposed as the relevant native Matlab type
    template <> struct mx_wrapper<uint8_t> {
        using value = std::integral_constant<mxClassID, mxUINT8_CLASS>;
        using type = uint8_t;
    };
    template <> struct mx_wrapper<uint16_t> {
        using value = std::integral_constant<mxClassID, mxUINT16_CLASS>;
        using type = uint16_t;
    };
    template <> struct mx_wrapper<uint32_t> {
        using value = std::integral_constant<mxClassID, mxUINT32_CLASS>;
        using type = uint32_t;
    };
    template <> struct mx_wrapper<uint64_t> {
        using value = std::integral_constant<mxClassID, mxUINT64_CLASS>;
        using type = uint64_t;
    };
    template <> struct mx_wrapper<int8_t> {
        using value = std::integral_constant<mxClassID, mxINT8_CLASS>;
        using type = int8_t;
    };
    template <> struct mx_wrapper<int16_t> {
        using value = std::integral_constant<mxClassID, mxINT16_CLASS>;
        using type = int16_t;
    };
    template <> struct mx_wrapper<int32_t> {
        using value = std::integral_constant<mxClassID, mxINT32_CLASS>;
        using type = int32_t;
    };
    template <> struct mx_wrapper<int64_t> {
        using value = std::integral_constant<mxClassID, mxINT64_CLASS>;
        using type = int64_t;
    };
    // by default non-basic types are wrapped as pointers
    template <typename T> struct mx_wrapper<T, typename std::enable_if<!is_basic_type<T>::value>::type> : mx_wrapper<void*> {};
    template <typename T, typename = void> struct type_traits {
        // KEEP THESE COMMENTED. They are only here to show the signature
        // should you choose to declare these functions
        // static rs2_internal_t* to_internal(T&& val);
        // static T from_internal(rs2_internal_t* ptr);
    };
    struct traits_trampoline {
    private:
        template <typename T> struct detector {
            struct fallback { int to_internal, from_internal, use_cells; };
            struct derived : type_traits<T>, fallback {};
            template <typename U, U> struct checker;
            typedef char ao1[1];
            typedef char ao2[2];
            template <typename U> static ao1& check_to(checker<int fallback::*, &U::to_internal> *);
            template <typename U> static ao2& check_to(...);
            template <typename U> static ao1& check_from(checker<int fallback::*, &U::from_internal> *);
            template <typename U> static ao2& check_from(...);
            template <typename U> static ao1& check_cells(checker<int fallback::*, &U::use_cells> *);
            template <typename U> static ao2& check_cells(...);
//            template <typename, typename = void> struct use_cells_t : false_type {};
//            template <typename U> struct use_cells_t<U, typename std::enable_if<sizeof(check_cells<U>(0)) == 2>::type>
//                : T::use_cells {};
            
            enum { has_to = sizeof(check_to<derived>(0)) == 2 };
            enum { has_from = sizeof(check_from<derived>(0)) == 2 };
//            enum { use_cells = use_cells_t<derived>::value };
            enum { use_cells = sizeof(check_cells<derived>(0)) == 2 };
        };
        template <typename T> using internal_t = typename type_traits<T>::rs2_internal_t;
    public:
        // selected if type_traits<T>::to_internal exists
        template <typename T> static typename std::enable_if<detector<T>::has_to, internal_t<T>*>::type
            to_internal(T&& val) { return type_traits<T>::to_internal(std::move(val)); }
        // selected if it doesnt
        template <typename T> static typename std::enable_if<!detector<T>::has_to, internal_t<T>*>::type
            to_internal(T&& val) { mexLock(); return new internal_t<T>(val); }

        // selected if type_traits<T>::from_internal exists
        template <typename T> static typename std::enable_if<detector<T>::has_from, T>::type
            from_internal(internal_t<T>* ptr) { return type_traits<T>::from_internal(ptr); }
        // selected if it doesnt
        template <typename T> static typename std::enable_if<!detector<T>::has_from, T>::type
            from_internal(internal_t<T>* ptr) { return T(*ptr); }
        template <typename T> using use_cells = std::integral_constant<bool, detector<T>::use_cells>;
    };

    // TODO: try/catch->err msg?
    template <typename T> static T parse(const mxArray* cell) { return mx_wrapper_fns<T>::parse(cell); }
    template <typename T> static mxArray* wrap(T&& var) { return mx_wrapper_fns<T>::wrap(std::move(var)); };
    template <typename T> static void destroy(const mxArray* cell) { return mx_wrapper_fns<T>::destroy(cell); }

    template <typename T> static typename std::enable_if<!is_basic_type<T>::value, std::vector<T>>::type parse_array(const mxArray* cells);
    template <typename T> static typename std::enable_if<is_basic_type<T>::value, std::vector<T>>::type parse_array(const mxArray* cells);

    template <typename T> static typename std::enable_if<!is_basic_type<T>::value && !traits_trampoline::use_cells<T>::value, mxArray*>::type wrap_array(const T* var, size_t length);
    template <typename T> static typename std::enable_if<!is_basic_type<T>::value && traits_trampoline::use_cells<T>::value, mxArray*>::type wrap_array(const T* var, size_t length);
    template <typename T> static typename std::enable_if<is_basic_type<T>::value, mxArray*>::type wrap_array(const T* var, size_t length);
};

#include "rs2_type_traits.h"

// for basic types (aka arithmetic, pointer, and enum types)
template <typename T> struct MatlabParamParser::mx_wrapper_fns<T, typename std::enable_if<is_basic_type<T>::value && !extra_checks<T>::value && !is_array_type<T>::value>::type>
{
    // Use full width types when converting integers.
    // Always using full width makes some things easier, and doing it this way
    // still allows us to use the mx_wrapper<T> class to send more exact types
    // for frame data arrays
    using type = typename std::conditional<std::is_integral<T>::value, typename std::conditional<std::is_signed<T>::value, int64_t, uint64_t>::type, T>::type;
    using wrapper = mx_wrapper<type>;
    static T parse(const mxArray* cell)
    {
        // obtain pointer to data, cast to proper matlab type
        auto *p = static_cast<typename wrapper::type*>(mxGetData(cell));
        // dereference pointer and cast to correct C++ type
        return T(*p);
    }
    static mxArray* wrap(T&& var)
    {
        // request 1x1 matlab matrix of correct type
        mxArray* cell = mxCreateNumericMatrix(1, 1, wrapper::value::value, mxREAL);
        // access matrix's data as pointer to correct C++ type
        auto *outp = static_cast<typename wrapper::type*>(mxGetData(cell));
        // cast object to correct C++ type and then store it in the matrix
        *outp = typename wrapper::type(var);
        return cell;
    }
    static void destroy(const mxArray* cell)
    {
        static_assert(!is_basic_type<T>::value, "Trying to destroy basic type. This shouldn't happen.");
        static_assert(is_basic_type<T>::value, "Non-basic type ended up in basic type's destroy function. How?");
    }
};

// default for non-basic types (eg classes)
template<typename T> struct MatlabParamParser::mx_wrapper_fns<T, typename std::enable_if<!is_basic_type<T>::value && !extra_checks<T>::value && !is_array_type<T>::value>::type>
{
    // librealsense types are sent to matlab using a pointer to the internal type.
    // to get it back from matlab we first parse that pointer and then reconstruct the C++ wrapper
    static T parse(const mxArray* cell)
    {
        using internal_t = typename type_traits<T>::rs2_internal_t;
        return traits_trampoline::from_internal<T>(mx_wrapper_fns<internal_t*>::parse(cell));
    }

    static mxArray* wrap(T&& var)
    {
        using internal_t = typename type_traits<T>::rs2_internal_t;
        return mx_wrapper_fns<internal_t*>::wrap(traits_trampoline::to_internal<T>(std::move(var)));
    }
    
    static void destroy(const mxArray* cell)
    {
        using internal_t = typename type_traits<T>::rs2_internal_t;
        // get pointer to the internal type we put on the heap
        auto ptr = mx_wrapper_fns<internal_t*>::parse(cell);
        delete ptr;
        // signal to matlab that the wrapper owns one fewer objects
        mexUnlock();
    }
};

// simple helper overload to refer std::array and std::vector to wrap_array
template<typename T> struct MatlabParamParser::mx_wrapper_fns<T, typename std::enable_if<is_array_type<T>::value>::type>
{
    static T parse(const mxArray* cell)
    {
        return MatlabParamParser::parse_array<typename is_array_type<T>::inner_t>(cell);
    }
    static mxArray* wrap(T&& var)
    {
        return MatlabParamParser::wrap_array(var.data(), var.size());
    }
};

// overload for wrapping C-strings. TODO: do we need special parsing too?
template<> mxArray* MatlabParamParser::mx_wrapper_fns<const char *>::wrap(const char*&& str)
{
    return mxCreateString(str);
}

template<> std::string MatlabParamParser::mx_wrapper_fns<std::string>::parse(const mxArray* cell)
{
    auto str = mxArrayToString(cell);
    auto ret = std::string(str);
    mxFree(str);
    return ret;
}
template<> mxArray* MatlabParamParser::mx_wrapper_fns<std::string>::wrap(std::string&& str)
{
    return mx_wrapper_fns<const char*>::wrap(str.c_str());
}

template<> struct MatlabParamParser::mx_wrapper_fns<std::chrono::nanoseconds>
{
    static std::chrono::nanoseconds parse(const mxArray* cell)
    {
        auto ptr = static_cast<double*>(mxGetData(cell));
        return std::chrono::nanoseconds{ static_cast<long long>(*ptr * 1e6) }; // convert from milliseconds, smallest time unit that's easy to work with in matlab
    }
    static mxArray* wrap(std::chrono::nanoseconds&& dur)
    {
        auto cell = mxCreateNumericMatrix(1, 1, mxDOUBLE_CLASS, mxREAL);
        auto ptr = static_cast<double*>(mxGetData(cell));
        *ptr = dur.count() / 1.e6; // convert to milliseconds, smallest time unit that's easy to work with in matlab
        return cell;
    }
};

template <typename T> static typename std::enable_if<!is_basic_type<T>::value, std::vector<T>>::type MatlabParamParser::parse_array(const mxArray* cells)
{
    using wrapper_t = mx_wrapper<T>;
    using internal_t = typename type_traits<T>::rs2_internal_t;

    std::vector<T> ret;
    auto length = mxGetNumberOfElements(cells);
    ret.reserve(length);
    auto ptr = static_cast<typename wrapper_t::type*>(mxGetData(cells)); // array of uint64_t's
    for (int i = 0; i < length; ++i) {
        ret.push_back(traits_trampoline::from_internal<T>(reinterpret_cast<internal_t*>(ptr[i])));
    }
    return ret;
}
template <typename T> static typename std::enable_if<is_basic_type<T>::value, std::vector<T>>::type MatlabParamParser::parse_array(const mxArray* cells)
{
    using wrapper_t = mx_wrapper<T>;

    std::vector<T> ret;
    auto length = mxGetNumberOfElements(cells);
    ret.reserve(length);
    auto ptr = static_cast<typename wrapper_t::type*>(mxGetData(cells)); // array of uint64_t's
    for (int i = 0; i < length; ++i) {
        ret.push_back(ptr[i]);
    }
    return ret;
}
template <typename T> static typename std::enable_if<!is_basic_type<T>::value && !MatlabParamParser::traits_trampoline::use_cells<T>::value, mxArray*>::type
MatlabParamParser::wrap_array(const T* var, size_t length)
{
    auto cells = mxCreateNumericMatrix(1, length, MatlabParamParser::mx_wrapper<T>::value::value, mxREAL);
    auto ptr = static_cast<typename mx_wrapper<T>::type*>(mxGetData(cells));
    for (int x = 0; x < length; ++x)
        ptr[x] = reinterpret_cast<typename mx_wrapper<T>::type>(traits_trampoline::to_internal<T>(T(var[x])));
    
    return cells;
}

template <typename T> static typename std::enable_if<!is_basic_type<T>::value && MatlabParamParser::traits_trampoline::use_cells<T>::value, mxArray*>::type
MatlabParamParser::wrap_array(const T* var, size_t length)
{
    auto cells = mxCreateCellMatrix(1, length);
    for (int x = 0; x < length; ++x)
        mxSetCell(cells, x, wrap(T(var[x])));

    return cells;
}

template <typename T> static typename std::enable_if<is_basic_type<T>::value, mxArray*>::type MatlabParamParser::wrap_array(const T* var, size_t length)
{
    auto cells = mxCreateNumericMatrix(1, length, MatlabParamParser::mx_wrapper<T>::value::value, mxREAL);
    auto ptr = static_cast<typename mx_wrapper<T>::type*>(mxGetData(cells));
    for (int x = 0; x < length; ++x)
        ptr[x] = typename mx_wrapper<T>::type(var[x]);

    return cells;
}
#include "types.h"
