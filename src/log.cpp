#include "types.h"

#include <fstream>

#if BUILD_EASYLOGGINGPP
INITIALIZE_EASYLOGGINGPP

namespace librealsense
{
    class logger_type
    {
        rs2_log_severity minimum_log_severity = RS2_LOG_SEVERITY_NONE;
        rs2_log_severity minimum_console_severity = RS2_LOG_SEVERITY_NONE;
        rs2_log_severity minimum_file_severity = RS2_LOG_SEVERITY_NONE;
        rs2_log_severity minimum_callback_severity = RS2_LOG_SEVERITY_NONE;

        std::mutex log_mutex;
        std::ofstream log_file;
        log_callback_ptr callback;

        std::string filename;
        const std::string log_id = "librealsense";

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

        void open() const
        {
            el::Configurations defaultConf;
            defaultConf.setToDefault();
            // To set GLOBAL configurations you may use

            defaultConf.setGlobally(el::ConfigurationType::ToFile, "false");
            defaultConf.setGlobally(el::ConfigurationType::ToStandardOutput, "false");
            defaultConf.setGlobally(el::ConfigurationType::MaxLogFileSize, "2097152");
            defaultConf.setGlobally(el::ConfigurationType::LogFlushThreshold, "10");
            defaultConf.setGlobally(el::ConfigurationType::Format, " %datetime{%d/%M %H:%m:%s,%g} %level [%thread] (%fbase:%line) %msg");

            for (int i = minimum_console_severity; i < RS2_LOG_SEVERITY_NONE; i++)
            {
                defaultConf.set(severity_to_level(static_cast<rs2_log_severity>(i)),
                    el::ConfigurationType::ToStandardOutput, "true");
            }

            for (int i = minimum_file_severity; i < RS2_LOG_SEVERITY_NONE; i++)
            {
                defaultConf.setGlobally(el::ConfigurationType::Filename, filename);
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
            : callback(nullptr, [](rs2_log_callback*) {}),
              filename(to_string() << datetime_string() << ".log")
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
    };

    static logger_type logger;
}

void librealsense::log_to_console(rs2_log_severity min_severity)
{
    logger.log_to_console(min_severity);
}

void librealsense::log_to_file(rs2_log_severity min_severity, const char * file_path)
{
    logger.log_to_file(min_severity, file_path);
}

#else // BUILD_EASYLOGGINGPP

void librealsense::log_to_console(rs2_log_severity min_severity)
{
}

void librealsense::log_to_file(rs2_log_severity min_severity, const char * file_path)
{
}

#endif // BUILD_EASYLOGGINGPP

