#pragma once
#include "mex.h"
#include "matrix.h"
#include <string>
#include <memory>
#include <type_traits>

template <typename T> using is_basic_type = std::bool_constant<std::is_arithmetic<T>::value || std::is_pointer<T>::value || std::is_enum<T>::value>;
// TODO: consider turning MatlabParamParser into a namespace. Might help make lots of things cleaner. can keep things hidden via nested namespace MatlabParamParser::detail

class MatlabParamParser
{
private:
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
        static const mxClassID value = value_t::value;
        using type = typename std::conditional<std::is_same<value_t, signed_t>::value, int64_t, uint64_t>::type;
    };
    // pointers are cast to uint64_t because matlab doesn't have a concept of pointers
    template <typename T> struct mx_wrapper<T, typename std::enable_if<std::is_pointer<T>::value>::type>
    {
        static const mxClassID value = mxUINT64_CLASS;
        using type = uint64_t;
    };
    // bools are exposed as matlab's native logical type
    template <> struct mx_wrapper<bool, void>
    {
        static const mxClassID value = mxLOGICAL_CLASS;
        using type = mxLogical;
    };
    // floating points are exposed as matlab's native double type
    template <typename T> struct mx_wrapper<T, typename std::enable_if<std::is_floating_point<T>::value>::type>
    {
        static const mxClassID value = mxDOUBLE_CLASS;
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
        static const mxClassID value = value_t::value;
        using type = typename std::conditional<std::is_same<value_t, signed_t>::value, int64_t, uint64_t>::type;
    };
    // by default non-basic types are wrapped as pointers
    template <typename T> struct mx_wrapper<T, typename std::enable_if<!is_basic_type<T>::value>::type> : mx_wrapper<int*> {};

    template <typename T, typename = void> struct type_traits {
        // KEEP THESE COMMENTED. They are only here to show the signature should you choose
        // to declare these functions
        // static rs2_internal_t* to_internal(T&& val);
        // static T from_internal(rs2_internal_t* ptr);
    };

    struct traits_trampoline {

        // used for SFINAE of which version of to_internal and from_internal to use
        struct general_ {};
        struct special_ : general_ {};
        template<typename> struct int_ { typedef int type; };

        template <typename T> using internal_t = typename type_traits<T>::rs2_internal_t;

        // above sub-structs and these two functions is what allows you to specify the functions to_internal or from_internal
        // in type_traits<T> to define custom conversions from the internal type to T and vice verse instead of a direct construction
        template <typename T, typename int_<decltype(type_traits<T>::to_internal)>::type = 0>
        static typename internal_t<T>* to_internal(T&& val, special_) { return type_traits<T>::to_internal(val); }
        template <typename T, typename int_<decltype(type_traits<T>::from_internal)>::type = 0>
        static T from_internal(typename internal_t<T>* ptr, special_) { return type_traits<T>::from_internal(ptr); }
        
        // these two handle the default direct construction case
        // wrapper creates new object on the heap, so don't allow the wrapper to unload before the object is destroyed.
        // librealsense types are sent to matlab using a pointer to the internal type.
        template <typename T> static typename internal_t<T>* to_internal(T&& val, general_) { mexLock(); return new internal_t<T>(val); }
        template <typename T> static T from_internal(typename internal_t<T>* ptr, general_) { return T(*ptr); }
    };
public:
    MatlabParamParser() {};
    ~MatlabParamParser() {};

    // TODO: try/catch->err msg?
    template <typename T> static T parse(const mxArray* cell) { return mx_wrapper_fns<T>::parse(cell); }
    template <typename T> static mxArray* wrap(T&& var) { return mx_wrapper_fns<T>::wrap(std::move(var)); };
    template <typename T> static void destroy(const mxArray* cell) { return mx_wrapper_fns<T>::destroy(cell); }

    template <typename T> static mxArray* wrap_array(const T* var, /*size_t ndims,*/ size_t *dims, bool contiguous = true);
};

#include "rs2_type_traits.h"

// for basic types (aka arithmetic, pointer, and enum types)
template <typename T> struct MatlabParamParser::mx_wrapper_fns<T, typename std::enable_if<is_basic_type<T>::value && !extra_checks<T>::value>::type>
{
    static T parse(const mxArray* cell)
    {
        using wrapper = mx_wrapper<T>;
        // obtain pointer to data, cast to proper matlab type
        auto *p = static_cast<typename wrapper::type*>(mxGetData(cell));
        // dereference pointer and cast to correct C++ type
        return T(*p);
    }
    static mxArray* wrap(T&& var)
    {
        // get helper info
        using wrapper = mx_wrapper<T>;

        // request 1x1 matlab matrix of correct type
        mxArray* cell = mxCreateNumericMatrix(1, 1, wrapper::value, mxREAL);
        // access matrix's data as pointer to correct C++ type
        auto *outp = static_cast<typename wrapper::type*>(mxGetData(cell));
        // cast object to correct C++ type and then store it in the matrix
        *outp = wrapper::type(var);
        return cell;
    }
    static void destroy(const mxArray* cell)
    {
        static_assert(!is_basic_type<T>::value, "Trying to destroy basic type. This shouldn't happen.");
        static_assert(is_basic_type<T>::value, "Non-basic type ended up in basic type's destroy function. How?");
    }
};

// default for non-basic types (eg classes)
template<typename T> struct MatlabParamParser::mx_wrapper_fns<T, typename std::enable_if<!is_basic_type<T>::value && !extra_checks<T>::value>::type>
{
    // librealsense types are sent to matlab using a pointer to the internal type.
    // to get it back from matlab we first parse that pointer and then reconstruct the C++ wrapper
    static T parse(const mxArray* cell)
    {
        using internal_t = typename type_traits<T>::rs2_internal_t;
        using special_t = traits_trampoline::special_;
        return traits_trampoline::from_internal<T>(mx_wrapper_fns<internal_t*>::parse(cell), special_t());
    }

    static mxArray* wrap(T&& var)
    {
        using internal_t = typename type_traits<T>::rs2_internal_t;
        using special_t = traits_trampoline::special_;
        return mx_wrapper_fns<internal_t*>::wrap(traits_trampoline::to_internal<T>(std::move(var), special_t()));
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

// overload for wrapping C-strings. TODO: do we need special parsing too?
template<> static mxArray* MatlabParamParser::mx_wrapper_fns<const char *>::wrap(const char*&& str)
{
    return mxCreateString(str);
}

template<> static std::string MatlabParamParser::mx_wrapper_fns<std::string>::parse(const mxArray* cell)
{
    auto str = mxArrayToString(cell);
    auto ret = std::string(str);
    mxFree(str);
    return ret;
}
template<> static mxArray* MatlabParamParser::mx_wrapper_fns<std::string>::wrap(std::string&& str)
{
    return mx_wrapper_fns<const char*>::wrap(str.c_str());
}

// TODO: find a way to fold this in with type_traits? should mx_wrapper and type_traits in general be folded together?
// wrap many Ts with the same class you wrap 1 T. defining this as an explicit struct allows for exceptions to be registered
// commented out for now because mx_wrapper is a private substruct of MatlabParamParser, and ty is not a substruct of MatlabParamParser
//template <typename T, typename = void> struct ty : std::integral_constant<mxClassID, MatlabParamParser::mx_wrapper<T>::value>{};

template <typename T> static mxArray* MatlabParamParser::wrap_array(const T* var, /*size_t ndims=1,*/ size_t *dims, bool contiguous)
{
//    static_assert(!std::is_same<ty<T>, std::false_type>::value, "Not a supported array type");
    auto cells = mxCreateNumericArray(/*ndims*/1, dims, /*ty<T>::value*/ MatlabParamParser::mx_wrapper<T>::value, mxREAL);
    auto ptr = static_cast<T*>(mxGetData(cells));
    // TODO: generalize to more dimensions. maybe nested helper functions?
    for (int x = 0; x < dims[0]; ++x)
        // or something...
        ptr[x] = (contiguous) ? var[x/*+y*dims[0]*/] : var[x]/*[y]*/;
    return cells;
}

#include "types.h"