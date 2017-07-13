// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.


#pragma once
#include "context.h"
#include "core/extension.h"
#include "device.h"

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


    void notifications_proccessor::raise_notification(const notification n)
    {
        _dispatcher.invoke([this, n](dispatcher::cancellable_timer ct)
                           {
                               std::lock_guard<std::mutex> lock(_callback_mutex);
                               rs2_notification noti(&n);
                               if (_callback)_callback->on_notification(&noti);
                               else
                               {
    #ifdef DEBUG

    #endif // !DEBUG

                               }
                           });
    }


    #define NOEXCEPT_RETURN(R, ...) catch(...) { std::ostringstream ss; librealsense::stream_args(ss, #__VA_ARGS__, __VA_ARGS__); rs2_error* e; librealsense::translate_exception(__FUNCTION__, ss.str(), &e); LOG_WARNING(rs2_get_error_message(e)); rs2_free_error(e); return R; }
    #define HANDLE_EXCEPTIONS_AND_RETURN(R, ...) catch(...) { std::ostringstream ss; librealsense::stream_args(ss, #__VA_ARGS__, __VA_ARGS__); librealsense::translate_exception(__FUNCTION__, ss.str(), error); return R; }
    #define VALIDATE_NOT_NULL(ARG) if(!(ARG)) throw std::runtime_error("null pointer passed for argument \"" #ARG "\"");
    #define VALIDATE_ENUM(ARG) if(!librealsense::is_valid(ARG)) { std::ostringstream ss; ss << "invalid enum value for argument \"" #ARG "\""; throw librealsense::invalid_value_exception(ss.str()); }
    #define VALIDATE_RANGE(ARG, MIN, MAX) if((ARG) < (MIN) || (ARG) > (MAX)) { std::ostringstream ss; ss << "out of range value for argument \"" #ARG "\""; throw librealsense::invalid_value_exception(ss.str()); }
    #define VALIDATE_LE(ARG, MAX) if((ARG) > (MAX)) { std::ostringstream ss; ss << "out of range value for argument \"" #ARG "\""; throw std::runtime_error(ss.str()); }
    //#define VALIDATE_NATIVE_STREAM(ARG) VALIDATE_ENUM(ARG); if(ARG >= RS2_STREAM_NATIVE_COUNT) { std::ostringstream ss; ss << "argument \"" #ARG "\" must be a native stream"; throw rsimpl2::wrong_value_exception(ss.str()); }
    #define VALIDATE_INTERFACE_NO_THROW(X, T) (dynamic_cast<T*>(&(*X)) || dynamic_cast<librealsense::extension_interface*>(&(*X)))
    #define VALIDATE_INTERFACE(X,T)                                                                \
        ([&]() {                                                                                   \
            auto p = dynamic_cast<T*>(&(*X));                                                       \
            if (!dynamic_cast<T*>(&(*X)))                                                           \
            {                                                                                      \
                auto ext = dynamic_cast<librealsense::extension_interface*>(&(*X));                      \
                if (!ext)                                                                          \
                    throw std::runtime_error("Object does not support \"" #T "\" interface! " );   \
                else                                                                               \
                    return dynamic_cast<T*>(ext);                                                  \
            }                                                                                      \
            return p;                                                                              \
        })()

}
