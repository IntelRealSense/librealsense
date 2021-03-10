// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.


#pragma once
#include "context.h"
#include "core/extension.h"
#include "device.h"

#include <type_traits>
#include <iostream>

struct rs2_raw_data_buffer
{
    std::vector<uint8_t> buffer;
};

typedef struct rs2_error rs2_error;

struct rs2_notification
{
    rs2_notification(const librealsense::notification* notification)
        :_notification(notification) {}

    const librealsense::notification* _notification;
};

struct rs2_device
{
    std::shared_ptr<librealsense::context> ctx;
    std::shared_ptr<librealsense::device_info> info;
    std::shared_ptr<librealsense::device_interface> device;
};

rs2_error * rs2_create_error(const char* what, const char* name, const char* args, rs2_exception_type type);

namespace librealsense
{
    // Facilities for streaming function arguments

    // First, we define a generic parameter streaming
    // Assumes T is streamable, reasonable for C API parameters
    template<class T, bool S>
    struct arg_streamer
    {
        void stream_arg(std::ostream & out, const T& val, bool last)
        {
            out << ':' << val << (last ? "" : ", ");
        }
    };

    // Next we define type trait for testing if *t for some T* is streamable
    template<typename T>
    class is_streamable
    {
        template <typename S>
        static auto test(const S* t) -> decltype(std::cout << **t);
        static auto test(...)->std::false_type;
    public:
        enum { value = !std::is_same<decltype(test((T*)0)), std::false_type>::value };
    };

    // Using above trait, we can now specialize our streamer
    // for streamable pointers:
    template<class T>
    struct arg_streamer<T*, true>
    {
        void stream_arg(std::ostream & out, T* val, bool last)
        {
            out << ':'; // If pointer not null, stream its content
            if (val) out << *val;
            else out << "nullptr";
            out << (last ? "" : ", ");
        }
    };

    // .. and for not streamable pointers
    template<class T>
    struct arg_streamer<T*, false>
    {
        void stream_arg(std::ostream & out, T* val, bool last)
        {
            out << ':'; // If pointer is not null, stream the pointer
            if (val) out << (int*)val; // Go through (int*) to avoid dumping the content of char*
            else out << "nullptr";
            out << (last ? "" : ", ");
        }
    };

    // This facility allows for translation of exceptions to rs2_error structs at the API boundary
    template<class T> void stream_args(std::ostream & out, const char * names, const T & last)
    {
        out << names;
        arg_streamer<T, is_streamable<T>::value> s;
        s.stream_arg(out, last, true);
    }
    template<class T, class... U> void stream_args(std::ostream & out, const char * names, const T & first, const U &... rest)
    {
        while (*names && *names != ',') out << *names++;
        arg_streamer<T, is_streamable<T>::value> s;
        s.stream_arg(out, first, false);
        while (*names && (*names == ',' || isspace(*names))) ++names;
        stream_args(out, names, rest...);
    }



    static void translate_exception(const char * name, std::string args, rs2_error ** error)
    {
        try { throw; }
        catch (const librealsense_exception& e) { if (error) *error = rs2_create_error(e.what(), name, args.c_str(), e.get_exception_type() ); }
        catch (const std::exception& e) { if (error) *error = rs2_create_error(e.what(), name, args.c_str(), RS2_EXCEPTION_TYPE_COUNT); }
        catch (...) { if (error) *error = rs2_create_error("unknown error", name, args.c_str(), RS2_EXCEPTION_TYPE_COUNT); }
    }

#ifdef TRACE_API
    // API objects repository holds all live objects
    // created from API calls and not released yet.
    // This is useful for two tasks:
    // 1. More easily follow API calls in logs
    // 2. See a snapshot of all not deallocated objects
    class api_objects
    {
    public:
        // Define singleton
        static api_objects& instance() {
            static api_objects instance;
            return instance;
        }

        // Place new object into the registry and give it a name
        // according to the class type and the index of the instance
        // to be presented in the log instead of the object memory address
        std::string register_new_object(const std::string& type, const std::string& address)
        {
            std::lock_guard<std::mutex> lock(_m);
            return internal_register(type, address);
        }

        // Given a list of parameters in form of "param:val,param:val"
        // This function will replace all val that have alternative names in this repository
        // with their names
        // the function is used by the log with param = function input param name from the prototype,
        // and val = the address of the param.
        // the function will change the val field from memory address to instance class and index name
        std::string augment_params(std::string p)
        {
            std::lock_guard<std::mutex> lock(_m);
            std::string acc = "";
            std::string res = "";
            std::string param = "";
            p += ",";
            bool is_value = false;
            for (auto i = 0; i < p.size(); i++)
            {
                if (p[i] == ':')
                {
                    param = acc;
                    acc = "";
                    is_value = true;
                }
                else if (is_value)
                {
                    if (p[i] == ',')
                    {
                        auto it = _names.find(acc);
                        if (it != _names.end()) acc = it->second;
                        else
                        {
                            // Heuristic: Assume things that are the same length
                            // as pointers are in-fact pointers
                            std::stringstream ss; ss << (int*)0;
                            if (acc.size() == ss.str().size())
                            {
                                acc = internal_register(param, acc);
                            }
                        }
                        res += param + ":" + acc;
                        if (i != p.size() - 1) res += ",";
                        acc = "";
                        is_value = false;
                    }
                    else acc += p[i];
                }
                else acc += p[i];
            }
            return res;
        }

        // Mark that a certain object is de-allocated
        void remove_object(const std::string& name)
        {
            std::lock_guard<std::mutex> lock(_m);
            auto it = _names.find(name);
            if (it != _names.end())
                _names.erase(it);
        }
    private:
        std::string internal_register(const std::string& type, const std::string& address)
        {
            auto it = _counters.find(type);
            if (it == _counters.end()) _counters[type] = 0;
            _counters[type]++;
            std::stringstream ss;
            ss << type << _counters[type];
            _names[address] = ss.str();
            return _names[address];
        }

        std::mutex _m;
        std::map<std::string, std::string> _names;
        std::map<std::string, int> _counters;
    };

    // This class wraps every API call and reports on it once its done
    class api_logger
    {
    public:
        api_logger(std::string function)
            : _function(std::move(function)), _result(""), _param_str(""), _type(""),
              _params([]() { return std::string{}; })
        {
            _start = std::chrono::high_resolution_clock::now();
            LOG_DEBUG("/* Begin " << _function << " */");

            // Define the returning "type" as the last word in function name
            std::size_t found = _function.find_last_of("_");
            _type = _function.substr(found + 1);

            check_if_deleter("rs2_delete");
            check_if_deleter("rs2_release");
        }

        // if the calling function is going to release an API object - remove it from the tracing list
        void check_if_deleter(const char* prefix)
        {
            auto deleter_pos = _function.find(prefix);
            if (deleter_pos == 0)
            {
                _is_deleter = true;
            }
        }

        void set_return_value(std::string result)
        {
            _result = std::move(result);
        }

        void set_params(std::function<std::string()> params)
        {
            _params = std::move(params);
        }

        std::string get_params() { return _param_str = _params(); }

        void report_error() { _error = true; }
        void report_pointer_return_type() { _returns_pointer = true; }

        ~api_logger()
        {
            auto d = std::chrono::high_resolution_clock::now() - _start;
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(d).count();
            std::stringstream ss;
            std::string prefix = "";
            if (_param_str == "") _param_str = _params();

            auto&& objs = api_objects::instance();
            if (_returns_pointer)
            {
                prefix = objs.register_new_object(_type, _result) + " = ";
                _result = "";
            }
            if (_is_deleter) objs.remove_object(_result);
            if (_param_str != "") _param_str = objs.augment_params(_param_str);

            ss << prefix << _function << "(" << _param_str << ")";
            if (_result != "") ss << " returned " << _result;
            ss << ";";
            if (ms > 0) ss << " /* Took " << ms << "ms */";
            if (_error) LOG_ERROR(ss.str());
            else LOG_INFO(ss.str());
        }
    private:
        std::string _function, _result, _type, _param_str;
        std::function<std::string()> _params;
        bool _returns_pointer = false;
        bool _is_deleter = false;
        bool _error = false;
        std::chrono::high_resolution_clock::time_point _start;
    };

    // This dummy helper function lets us fetch return type from lambda
    // this is used for result_printer, to be able to be used
    // similarly for functions with and without return parameter
    template<typename F, typename R>
    R fetch_return_type(F& f, R(F::*mf)() const);

    // Result printer class will capture lambda result value
    // and pass it as string to the api logger
    template<class T>
    class result_printer
    {
    public:
        result_printer(api_logger* logger) : _logger(logger) {}

        template<class F>
        T invoke(F func)
        {
            _res = func(); // Invoke lambda and save result for later
            // I assume this will not have any side-effects, since it is applied
            // to types that are about to be passed from C API
            return _res;
        }

        ~result_printer()
        {
            std::stringstream ss; ss << _res;
            _logger->set_return_value(ss.str());
        }
    private:
        T _res;
        api_logger* _logger;
    };

    // Result printer class for T* should not dump memory content
    template<class T>
    class result_printer<T*>
    {
    public:
        result_printer(api_logger* logger) : _logger(logger) {}

        template<class F>
        T* invoke(F func)
        {
            _res = func(); // Invoke lambda and save result for later
                           // I assume this will not have any side-effects, since it is applied
                           // to types that are about to be passed from C API
            return _res;
        }

        ~result_printer()
        {
            std::stringstream ss; ss << (int*)_res; // Force case to int* to avoid char* and such
            // being dumped to log
            _logger->report_pointer_return_type();
            _logger->set_return_value(ss.str());
        }
    private:
        T* _res;
        api_logger* _logger;
    };

    // To work-around the fact that void can't be "stringified"
    // we define an alternative printer just for void returning lambdas
    template<>
    class result_printer<void>
    {
    public:
        result_printer(api_logger* logger) {}
        template<class F>
        void invoke(F func) { func(); }
    };

// Begin API macro defines new API logger and redirects function body into a lambda
// The extra brace ensures api_logger will die after everything else
#define BEGIN_API_CALL { api_logger __api_logger(__FUNCTION__); {\
auto func = [&](){

// The various return macros finish the lambda and invoke it using the type printer
// practically capturing function return value
// In addition, error flag and function parameters are captured into the API logger
#define NOEXCEPT_RETURN(R, ...) };\
result_printer<decltype(fetch_return_type(func, &decltype(func)::operator()))> __p(&__api_logger);\
__api_logger.set_params([&](){ std::ostringstream ss; librealsense::stream_args(ss, #__VA_ARGS__, __VA_ARGS__); return ss.str(); });\
try {\
return __p.invoke(func);\
} catch(...) {\
rs2_error* e; librealsense::translate_exception(__FUNCTION__, __api_logger.get_params(), &e);\
LOG_WARNING(rs2_get_error_message(e)); rs2_free_error(e); __api_logger.report_error(); return R; } } }

#define HANDLE_EXCEPTIONS_AND_RETURN(R, ...) };\
result_printer<decltype(fetch_return_type(func, &decltype(func)::operator()))> __p(&__api_logger);\
__api_logger.set_params([&](){ std::ostringstream ss; librealsense::stream_args(ss, #__VA_ARGS__, __VA_ARGS__); return ss.str(); });\
try {\
return __p.invoke(func);\
} catch(...) {\
librealsense::translate_exception(__FUNCTION__, __api_logger.get_params(), error); __api_logger.report_error(); return R; } } }

#define NOARGS_HANDLE_EXCEPTIONS_AND_RETURN(R, ...) };\
result_printer<decltype(fetch_return_type(func, &decltype(func)::operator()))> __p(&__api_logger);\
try {\
return __p.invoke(func);\
} catch(...) { librealsense::translate_exception(__FUNCTION__, "", error); __api_logger.report_error(); return R; } } }

#else // No API tracing:

#define BEGIN_API_CALL try
#define NOEXCEPT_RETURN(R, ...) catch(...) { std::ostringstream ss; librealsense::stream_args(ss, #__VA_ARGS__, __VA_ARGS__); rs2_error* e; librealsense::translate_exception(__FUNCTION__, ss.str(), &e); LOG_WARNING(rs2_get_error_message(e)); rs2_free_error(e); return R; }
#define HANDLE_EXCEPTIONS_AND_RETURN(R, ...) catch(...) { std::ostringstream ss; librealsense::stream_args(ss, #__VA_ARGS__, __VA_ARGS__); librealsense::translate_exception(__FUNCTION__, ss.str(), error); return R; }
#define NOARGS_HANDLE_EXCEPTIONS_AND_RETURN(R) catch(...) { librealsense::translate_exception(__FUNCTION__, "", error); return R; }

#endif


    #define VALIDATE_NOT_NULL(ARG) if(!(ARG)) throw std::runtime_error("null pointer passed for argument \"" #ARG "\"");
    #define VALIDATE_ENUM(ARG) if(!librealsense::is_valid(ARG)) { std::ostringstream ss; ss << "invalid enum value for argument \"" #ARG "\""; throw librealsense::invalid_value_exception(ss.str()); }
    #define VALIDATE_OPTION(OBJ, OPT_ID) if(!OBJ->options->supports_option(OPT_ID)) { std::ostringstream ss; ss << "object doesn't support option #" << std::to_string(OPT_ID); throw librealsense::invalid_value_exception(ss.str()); }
    #define VALIDATE_RANGE(ARG, MIN, MAX) if((ARG) < (MIN) || (ARG) > (MAX)) { std::ostringstream ss; ss << "out of range value for argument \"" #ARG "\""; throw librealsense::invalid_value_exception(ss.str()); }
    #define VALIDATE_LE(ARG, MAX) if((ARG) > (MAX)) { std::ostringstream ss; ss << "out of range value for argument \"" #ARG "\""; throw std::runtime_error(ss.str()); }
    #define VALIDATE_INTERFACE_NO_THROW(X, T)                                                   \
    ([&]() -> T* {                                                                              \
        T* p = dynamic_cast<T*>(&(*X));                                                         \
        if (p == nullptr)                                                                       \
        {                                                                                       \
            auto ext = dynamic_cast<librealsense::extendable_interface*>(&(*X));                \
            if (ext == nullptr) return nullptr;                                                 \
            else                                                                                \
            {                                                                                   \
                if(!ext->extend_to(TypeToExtension<T>::value, (void**)&p))                      \
                    return nullptr;                                                             \
                return p;                                                                       \
            }                                                                                   \
        }                                                                                       \
        return p;                                                                               \
    })()

    #define VALIDATE_INTERFACE(X,T)                                                             \
        ([&]() -> T* {                                                                          \
            T* p = VALIDATE_INTERFACE_NO_THROW(X,T);                                            \
            if(p == nullptr)                                                                    \
                throw std::runtime_error("Object does not support \"" #T "\" interface! " );    \
            return p;                                                                           \
        })()
}

inline int lrs_major(int version)
{
    return version / 10000;
}
inline int lrs_minor(int version)
{
    return (version % 10000) / 100;
}
inline int lrs_patch(int version)
{
    return (version % 100);
}

inline std::string api_version_to_string(int version)
{
    if (lrs_major(version) == 0) return librealsense::to_string() << version;
    return librealsense::to_string() << lrs_major(version) << "." << lrs_minor(version) << "." << lrs_patch(version);
}

inline void report_version_mismatch(int runtime, int compiletime)
{
    throw librealsense::invalid_value_exception(librealsense::to_string() << "API version mismatch: librealsense.so was compiled with API version "
        << api_version_to_string(runtime) << " but the application was compiled with "
        << api_version_to_string(compiletime) << "! Make sure correct version of the library is installed (make install)");
}

inline void verify_version_compatibility(int api_version)
{
    rs2_error* error = nullptr;
    auto runtime_api_version = rs2_get_api_version(&error);
    if (error)
        throw librealsense::invalid_value_exception(rs2_get_error_message(error));

    if ((runtime_api_version < 10) || (api_version < 10))
    {
        // when dealing with version < 1.0.0 that were still using single number for API version, require exact match
        if (api_version != runtime_api_version)
            report_version_mismatch(runtime_api_version, api_version);
    }
    else if ((lrs_major(runtime_api_version) == 1 && lrs_minor(runtime_api_version) <= 9)
        || (lrs_major(api_version) == 1 && lrs_minor(api_version) <= 9))
    {
        // when dealing with version < 1.10.0, API breaking changes are still possible without minor version change, require exact match
        if (api_version != runtime_api_version)
            report_version_mismatch(runtime_api_version, api_version);
    }
    else
    {
        // starting with 1.10.0, versions with same patch are compatible
        // Incompatible versions differ on major, or with the executable's minor bigger than the library's one.
        if ((lrs_major(api_version) != lrs_major(runtime_api_version))
            || (lrs_minor(api_version) > lrs_minor(runtime_api_version)))
            report_version_mismatch(runtime_api_version, api_version);
    }
}
