#include "types.h"

#include <fstream>
#include <iostream>

INITIALIZE_EASYLOGGINGPP

namespace rsimpl 
{
    class logger_type 
    {
        rs_log_severity minimum_log_severity = RS_LOG_SEVERITY_NONE;
        rs_log_severity minimum_console_severity = RS_LOG_SEVERITY_NONE;
        rs_log_severity minimum_file_severity = RS_LOG_SEVERITY_NONE;
        rs_log_severity minimum_callback_severity = RS_LOG_SEVERITY_NONE;

        std::mutex log_mutex;
        std::ofstream log_file;
        log_callback_ptr callback;

        std::string filename;

    public:
        static el::Level severity_to_level(rs_log_severity severity)
        {
            switch (severity)
            {
            case RS_LOG_SEVERITY_DEBUG: return el::Level::Debug;
            case RS_LOG_SEVERITY_INFO: return el::Level::Info;
            case RS_LOG_SEVERITY_WARN: return el::Level::Warning;
            case RS_LOG_SEVERITY_ERROR: return el::Level::Error;
            case RS_LOG_SEVERITY_FATAL: return el::Level::Fatal;
            default: return el::Level::Unknown;
            }
        }

        void configure() const
        {
            el::Configurations defaultConf;
            defaultConf.setToDefault();
            // To set GLOBAL configurations you may use

            defaultConf.setGlobally(el::ConfigurationType::ToFile, "false");
            defaultConf.setGlobally(el::ConfigurationType::ToStandardOutput, "false");
            defaultConf.setGlobally(el::ConfigurationType::Filename, filename);
            defaultConf.setGlobally(el::ConfigurationType::MaxLogFileSize, "2097152");
            defaultConf.setGlobally(el::ConfigurationType::LogFlushThreshold, "10");
            defaultConf.setGlobally(el::ConfigurationType::Format, " %datetime{%d/%M %H:%m:%s,%g} %level [%thread] (%fbase:%line) %msg");

            for (int i = minimum_console_severity; i < RS_LOG_SEVERITY_COUNT; i++)
            {
                defaultConf.set(severity_to_level(static_cast<rs_log_severity>(i)), 
                    el::ConfigurationType::ToStandardOutput, "true");
            }

            for (int i = minimum_file_severity; i < RS_LOG_SEVERITY_COUNT; i++)
            {
                defaultConf.set(severity_to_level(static_cast<rs_log_severity>(i)),
                    el::ConfigurationType::ToFile, "true");
            }

            el::Loggers::reconfigureLogger("librealsense", defaultConf);
        }

        logger_type() : callback(nullptr, [](rs_log_callback*) {})
        {
            auto t = time(nullptr); char buffer[20] = {}; const tm* time = localtime(&t);
            if (nullptr != time)
                strftime(buffer, sizeof(buffer), "%Y-%m-%d-%H_%M_%S", time);
            filename = to_string() << buffer << ".log";

            configure();
        }

        void log_to_console(rs_log_severity min_severity)
        {
            minimum_console_severity = min_severity;
            configure();
        }

        void log_to_file(rs_log_severity min_severity, const char * file_path)
        {
            minimum_file_severity = min_severity;
            if (file_path) filename = file_path;
            configure();
        }
    };

    static logger_type logger;
}

void rsimpl::log_to_console(rs_log_severity min_severity)
{
    logger.log_to_console(min_severity);
}

void rsimpl::log_to_file(rs_log_severity min_severity, const char * file_path)
{
    logger.log_to_file(min_severity, file_path);
}
