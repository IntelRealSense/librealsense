#include "types.h"

#include <fstream>
#include <iostream>
#include <iomanip>
#include <ctime>

int rsimpl::minimum_log_severity = static_cast<int>(rsimpl::log_severity::info);

struct logger
{
    std::ofstream out;

    logger() : out("librealsense-log.txt", std::ofstream::app)
    {
        std::time_t t = std::time(nullptr); char buffer[20];
	std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
        out << "\n" << buffer << " PROGRAM START" << std::endl;
    }

    ~logger()
    {
        std::time_t t = std::time(nullptr); char buffer[20];
	std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
        out << buffer << " PROGRAM STOP" << std::endl;    
    }
};

void rsimpl::log(log_severity severity, const std::string & message)
{
    if(static_cast<int>(severity) < minimum_log_severity) return;

    static logger the_logger;

    std::time_t t = std::time(nullptr); char buffer[20];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&t));

    switch(severity)
    {
    case log_severity::debug:   the_logger.out << buffer << " DEBUG: " << message << std::endl; break;
    case log_severity::info:    the_logger.out << buffer << " INFO:  " << message << std::endl; break;
    case log_severity::warning: the_logger.out << buffer << " WARN:  " << message << std::endl; break;
    case log_severity::error:   the_logger.out << buffer << " ERROR: " << message << std::endl; break;
    case log_severity::fatal:   the_logger.out << buffer << " FATAL: " << message << std::endl; break;
    }

    switch(severity)
    {
    case log_severity::debug:   std::cout << "[debug] "   << message << std::endl; break;
    case log_severity::info:    std::cout << "[info] "    << message << std::endl; break;
    case log_severity::warning: std::cout << "[warning] " << message << std::endl; break;
    case log_severity::error:   std::cout << "[error] "   << message << std::endl; break;
    case log_severity::fatal:   std::cout << "[fatal] "   << message << std::endl; break;
    }
}
