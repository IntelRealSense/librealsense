// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

#include "types.h"

namespace librealsense
{
#if BUILD_EASYLOGGINGPP
    struct log_message
    {
        el::LogMessage const& el_msg;
        std::string built_msg;

        log_message( el::LogMessage const& el_msg ) : el_msg( el_msg ) {}
    };

    template<char const * NAME>
    class logger_type
    {
        rs2_log_severity minimum_log_severity = RS2_LOG_SEVERITY_NONE;
        rs2_log_severity minimum_console_severity = RS2_LOG_SEVERITY_NONE;
        rs2_log_severity minimum_file_severity = RS2_LOG_SEVERITY_NONE;

        std::mutex log_mutex;
        std::ofstream log_file;
        std::vector< std::string > callback_dispatchers;

        std::string filename;
        const std::string log_id = NAME;

    public:
        static el::Level severity_to_level(rs2_log_severity severity)
        {
            switch (severity)
            {
            case RS2_LOG_SEVERITY_DEBUG: return el::Level::Debug;
            case RS2_LOG_SEVERITY_INFO: return el::Level::Info;
            case RS2_LOG_SEVERITY_WARN: return el::Level::Warning;
            case RS2_LOG_SEVERITY_ERROR: return el::Level::Error;
            case RS2_LOG_SEVERITY_FATAL: return el::Level::Fatal;
            default: return el::Level::Unknown;
            }
        }

        static rs2_log_severity level_to_severity( el::Level level )
        {
            switch( level )
            {
            case el::Level::Debug:   return RS2_LOG_SEVERITY_DEBUG;  // LOG(DEBUG)    - useful for developers to debug application
            case el::Level::Trace:   return RS2_LOG_SEVERITY_DEBUG;  // LOG(TRACE)    - useful to back-trace certain events
            case el::Level::Info:    return RS2_LOG_SEVERITY_INFO;   // LOG(INFO)     - show current progress of application
            case el::Level::Verbose: return RS2_LOG_SEVERITY_INFO;   // LOG(VERBOSE)  - can be highly useful but varies with verbose logging level
            case el::Level::Warning: return RS2_LOG_SEVERITY_WARN;   // LOG(WARNING)  - potentially harmful situtaions
            case el::Level::Error:   return RS2_LOG_SEVERITY_ERROR;  // LOG(ERROR)    - errors in application but application will keep running
            case el::Level::Fatal:   return RS2_LOG_SEVERITY_FATAL;  // LOG(FATAL)    - severe errors that will presumably abort application
            default: 
                // Unknown/Global, or otherwise
                // Global should never occur (no CGLOBAL macro); same for Unknown...
                return RS2_LOG_SEVERITY_ERROR;  // treat as error, since we don't know what it is...
            }
        }

        void open() const
        {
            el::Configurations defaultConf;
            defaultConf.setToDefault();
            // To set GLOBAL configurations you may use

            defaultConf.setGlobally(el::ConfigurationType::ToFile, "false");
            defaultConf.setGlobally(el::ConfigurationType::ToStandardOutput, "false");
            defaultConf.setGlobally(el::ConfigurationType::LogFlushThreshold, "10");
            defaultConf.setGlobally(el::ConfigurationType::Format, " %datetime{%d/%M %H:%m:%s,%g} %level [%thread] (%fbase:%line) %msg");

            for (int i = minimum_console_severity; i < RS2_LOG_SEVERITY_NONE; i++)
            {
                defaultConf.set(severity_to_level(static_cast<rs2_log_severity>(i)),
                    el::ConfigurationType::ToStandardOutput, "true");
            }

            // NOTE: you can only log to one file, so successive calls to log_to_file
            // will override one another!
            defaultConf.setGlobally( el::ConfigurationType::Filename, filename );
            for (int i = minimum_file_severity; i < RS2_LOG_SEVERITY_NONE; i++)
            {
                defaultConf.set(severity_to_level(static_cast<rs2_log_severity>(i)),
                    el::ConfigurationType::ToFile, "true");
            }

            el::Loggers::reconfigureLogger(log_id, defaultConf);
        }

        void open_def() const
        {
            el::Configurations defaultConf;
            defaultConf.setToDefault();
            // To set GLOBAL configurations you may use

            defaultConf.setGlobally(el::ConfigurationType::ToFile, "false");
            defaultConf.setGlobally(el::ConfigurationType::ToStandardOutput, "false");

            el::Loggers::reconfigureLogger(log_id, defaultConf);
        }


        logger_type()
            : filename(to_string() << datetime_string() << ".log")
        {
            rs2_log_severity severity;
            if (try_get_log_severity(severity))
            {
                log_to_file(severity, filename.c_str());
            }
            else
            {
                open_def();
            }
        }

        static bool try_get_log_severity(rs2_log_severity& severity)
        {
            static const char* severity_var_name = "LRS_LOG_LEVEL";
            auto content = getenv(severity_var_name);

            if (content)
            {
                std::string content_str(content);
                std::transform(content_str.begin(), content_str.end(), content_str.begin(), ::tolower);

                for (uint32_t i = 0; i < RS2_LOG_SEVERITY_COUNT; i++)
                {
                    auto current = (rs2_log_severity)i;
                    std::string name = librealsense::get_string(current);
                    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
                    if (content_str == name)
                    {
                        severity = current;
                        return true;
                    }
                }
            }

            return false;
        }

        void log_to_console(rs2_log_severity min_severity)
        {
            minimum_console_severity = min_severity;
            open();
        }

        void log_to_file(rs2_log_severity min_severity, const char * file_path)
        {
            if (!try_get_log_severity(minimum_file_severity))
                minimum_file_severity = min_severity;

            if (file_path)
                filename = file_path;

            open();
        }

    protected:
        // Create a dispatch sink which will get any messages logged to EasyLogging, which will then
        // post the messages on the viewer's notification window.
        class elpp_dispatcher : public el::LogDispatchCallback
        {
        public:
            log_callback_ptr callback;
            rs2_log_severity min_severity = RS2_LOG_SEVERITY_NONE;

        protected:
            void handle( el::LogDispatchData const* data ) noexcept override
            {
                // NOTE: once a callback is set, it gets ALL messages -- even from other (non-"librealsense") loggers!
                el::LogMessage const& msg = *data->logMessage();
                auto severity = level_to_severity( msg.level() );
                if( callback && severity >= min_severity )
                {
                    log_message msg_wrapper( msg );
                    callback->on_log( severity, reinterpret_cast<rs2_log_message const&>(msg_wrapper) );  // noexcept!
                }
            }
        };

    public:
        void remove_callbacks()
        {
            for( auto const& dispatch : callback_dispatchers )
                el::Helpers::uninstallLogDispatchCallback< elpp_dispatcher >( dispatch );
            callback_dispatchers.clear();
        }

        void log_to_callback( rs2_log_severity min_severity, log_callback_ptr callback )
        {
            open();
            
            try_get_log_severity( min_severity );
            if( callback  &&  min_severity != RS2_LOG_SEVERITY_NONE )
            {
                // Each callback we install must have a unique name
                std::ostringstream ss;
                ss << "elpp_dispatcher_" << callback_dispatchers.size();
                std::string dispatch_name = ss.str();

                // Record all the dispatchers we have so we can erase them
                callback_dispatchers.push_back( dispatch_name );

                // The only way to install the callback in ELPP is with a default ctor...
                el::Helpers::installLogDispatchCallback< elpp_dispatcher >( dispatch_name );
                auto dispatcher = el::Helpers::logDispatchCallback< elpp_dispatcher >( dispatch_name );
                dispatcher->callback = callback;
                dispatcher->min_severity = min_severity;
                
                // Remove the default logger (which will log to standard out/err) or it'll still be active
                //el::Helpers::uninstallLogDispatchCallback< el::base::DefaultLogDispatchCallback >( "DefaultLogDispatchCallback" );
            }
        }
    };
#endif //BUILD_EASYLOGGINGPP
}
