// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.


#pragma once
#include "context.h"
#include "core/extension.h"
#include "device.h"

struct rs2_raw_data_buffer
{
    std::vector<uint8_t> buffer;
};

struct rs2_error
{
    std::string message;
    const char* function;
    std::string args;
    rs2_exception_type exception_type;
};

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

namespace librealsense
{
    // This facility allows for translation of exceptions to rs2_error structs at the API boundary
    template<class T> void stream_args(std::ostream & out, const char * names, const T & last) { out << names << ':' << last; }
    template<class T, class... U> void stream_args(std::ostream & out, const char * names, const T & first, const U &... rest)
    {
        while (*names && *names != ',') out << *names++;
        out << ':' << first << ", ";
        while (*names && (*names == ',' || isspace(*names))) ++names;
        stream_args(out, names, rest...);
    }

    static void translate_exception(const char * name, std::string args, rs2_error ** error)
    {
        try { throw; }
        catch (const librealsense_exception& e) { if (error) *error = new rs2_error{ e.what(), name, move(args), e.get_exception_type() }; }
        catch (const std::exception& e) { if (error) *error = new rs2_error{ e.what(), name, move(args) }; }
        catch (...) { if (error) *error = new rs2_error{ "unknown error", name, move(args) }; }
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
        
        // Place new object into the registry
        std::string register_new_object(const std::string& type, const std::string& address)
        {
            std::lock_guard<std::mutex> lock(_m);
            auto it = _counters.find(type);
            if (it == _counters.end()) _counters[type] = 0;
            _counters[type]++;
            std::stringstream ss;
            ss << type << _counters[type];
            _names[address] = ss.str();
            return _names[address];
        }
        
        // Given a list of parameters in form of "param:val,param:val"
        // This function will replace all val that have alternative names in this repository
        // with their names
        std::string augment_params(std::string p)
        {
            std::lock_guard<std::mutex> lock(_m);
            std::string acc = "";
            std::string res = "";
            p += ",";
            bool is_value = false;
            for (auto i = 0; i < p.size(); i++)
            {
                if (p[i] == ':') is_value = true;
                else if (is_value)
                {
                    if (p[i] == ',')
                    {
                        auto it = _names.find(acc);
                        if (it != _names.end()) acc = it->second;
                        res += ":";
                        res += acc;
                        if (i != p.size() - 1) res += ",";
                        acc = "";
                        is_value = false;
                    }
                    else acc += p[i];
                } else res += p[i];
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
        std::mutex _m;
        std::map<std::string, std::string> _names;
        std::map<std::string, int> _counters;
    };
    
    // This class wraps every API call and reports on it once its done
    class api_logger
    {
    public:
        api_logger(std::string function)
            : _function(std::move(function)), _result(""), _params(""), _type("")
        {
            _start = std::chrono::high_resolution_clock::now();
            LOG_DEBUG("/* Begin " << _function << " */");
            
            check_if_creator("rs2_create_");
            check_if_creator("rs2_query_");
            check_if_deleter("rs2_delete");
            check_if_deleter("rs2_release");
        }
        
        void check_if_creator(const char* prefix)
        {
            auto create_pos = _function.find(prefix);
            if (create_pos == 0)
            {
                _type = _function;
                _type.erase(0, strlen(prefix));
                _is_creator = true;
            }
        }
        
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
        
        void set_params(std::string params)
        {
            _params = std::move(params);
        }
        
        void report_error() { _error = true; }
        
        ~api_logger()
        {
            auto d = std::chrono::high_resolution_clock::now() - _start;
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(d).count();
            std::stringstream ss;
            std::string prefix = "";
            
            auto&& objs = api_objects::instance();
            if (_is_creator) {
                prefix = objs.register_new_object(_type, _result) + " = ";
                _result = "";
            }
            if (_is_deleter) objs.remove_object(_result);
            if (_params != "") _params = objs.augment_params(_params);
            
            ss << prefix << _function << "(" << _params << ")";
            if (_result != "") ss << " returned " << _result;
            ss << ";";
            if (ms > 0) ss << " /* Took " << ms << "ms */";
            if (_error) LOG_ERROR(ss.str());
            else LOG_INFO(ss.str());
        }
    private:
        std::string _function, _result, _params, _type;
        bool _is_creator = false;
        bool _is_deleter = false;
        bool _error = false;
        std::chrono::high_resolution_clock::time_point _start;
    };

    // This dummy helper function lets us fetch return type from lambda
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
// practicly capturing function return value
// In addition, error flag and function parameters are captured into the API logger
#define NOEXCEPT_RETURN(R, ...) };\
result_printer<decltype(fetch_return_type(func, &decltype(func)::operator()))> __p(&__api_logger);\
std::ostringstream ss; librealsense::stream_args(ss, #__VA_ARGS__, __VA_ARGS__); auto params = ss.str();\
__api_logger.set_params(params);\
try {\
return __p.invoke(func);\
} catch(...) {\
rs2_error* e; librealsense::translate_exception(__FUNCTION__, params, &e);\
LOG_WARNING(rs2_get_error_message(e)); rs2_free_error(e); __api_logger.set_params(params); __api_logger.report_error(); return R; } } }
    
#define HANDLE_EXCEPTIONS_AND_RETURN(R, ...) };\
result_printer<decltype(fetch_return_type(func, &decltype(func)::operator()))> __p(&__api_logger);\
std::ostringstream ss; librealsense::stream_args(ss, #__VA_ARGS__, __VA_ARGS__); auto params = ss.str();\
__api_logger.set_params(params);\
try {\
return __p.invoke(func);\
} catch(...) {\
librealsense::translate_exception(__FUNCTION__, params, error); __api_logger.set_params(params);__api_logger.report_error(); return R; } } }
    
#define NOARGS_HANDLE_EXCEPTIONS_AND_RETURN(R, ...) };\
result_printer<decltype(fetch_return_type(func, &decltype(func)::operator()))> __p(&__api_logger);\
try {\
return __p.invoke(func);\
} catch(...) { librealsense::translate_exception(__FUNCTION__, "", error); __api_logger.report_error(); return R; } } }
    
#else // No API tracing:
    
#define BEGIN_API_CALL try
#define NOEXCEPT_RETURN(R, ...) catch(...) { std::ostringstream ss; librealsense::stream_args(ss, #__VA_ARGS__, __VA_ARGS__); rs2_error* e; librealsense::translate_exception(__FUNCTION__, ss.str(), &e); LOG_WARNING(rs2_get_error_message(e)); rs2_free_error(e); return R; }
#define HANDLE_EXCEPTIONS_AND_RETURN(R, ...) catch(...) { std::ostringstream ss; librealsense::stream_args(ss, #__VA_ARGS__, __VA_ARGS__); librealsense::translate_exception(__FUNCTION__, ss.str(), error); return R; }
#define NOARGS_HANDLE_EXCEPTIONS_AND_RETURN(R, ...) catch(...) { librealsense::translate_exception(__FUNCTION__, "", error); return R; }
    
#endif


    #define VALIDATE_NOT_NULL(ARG) if(!(ARG)) throw std::runtime_error("null pointer passed for argument \"" #ARG "\"");
    #define VALIDATE_ENUM(ARG) if(!librealsense::is_valid(ARG)) { std::ostringstream ss; ss << "invalid enum value for argument \"" #ARG "\""; throw librealsense::invalid_value_exception(ss.str()); }
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
