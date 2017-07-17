/*********************************************************************
* Software License Agreement (BSD License)
*
*  Copyright (c) 2008, Willow Garage, Inc.
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of the Willow Garage nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*********************************************************************/

/* Author: Ryan Luna, Ioan Sucan */

#ifndef CONSOLE_BRIDGE_CONSOLE_
#define CONSOLE_BRIDGE_CONSOLE_

#include <string>

#include "exportdecl.h"

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#define CONSOLE_BRIDGE_DEPRECATED __attribute__ ((deprecated))
#elif defined(_MSC_VER)
#define CONSOLE_BRIDGE_DEPRECATED __declspec(deprecated)
#else
#pragma message("WARNING: You need to implement DEPRECATED for this compiler")
#define CONSOLE_BRIDGE_DEPRECATED
#endif

static inline void CONSOLE_BRIDGE_DEPRECATED console_bridge_deprecated() {}

/** \file console.h
    \defgroup logging Logging Macros
    \{

    \def logError(fmt, ...)
    \brief Log a formatted error string.
    \remarks This macro takes the same arguments as <a href="http://www.cplusplus.com/reference/clibrary/cstdio/printf">printf</a>.

    \def logWarn(fmt, ...)
    \brief Log a formatted warning string.
    \remarks This macro takes the same arguments as <a href="http://www.cplusplus.com/reference/clibrary/cstdio/printf">printf</a>.

    \def logInform(fmt, ...)
    \brief Log a formatted information string.
    \remarks This macro takes the same arguments as <a href="http://www.cplusplus.com/reference/clibrary/cstdio/printf">printf</a>.

    \def logDebug(fmt, ...)
    \brief Log a formatted debugging string.
    \remarks This macro takes the same arguments as <a href="http://www.cplusplus.com/reference/clibrary/cstdio/printf">printf</a>.

    \}
*/
#define CONSOLE_BRIDGE_logError(fmt, ...)  \
  console_bridge::log(__FILE__, __LINE__, console_bridge::CONSOLE_BRIDGE_LOG_ERROR, fmt, ##__VA_ARGS__)

#define CONSOLE_BRIDGE_logWarn(fmt, ...)   \
  console_bridge::log(__FILE__, __LINE__, console_bridge::CONSOLE_BRIDGE_LOG_WARN,  fmt, ##__VA_ARGS__)

#define CONSOLE_BRIDGE_logInform(fmt, ...) \
  console_bridge::log(__FILE__, __LINE__, console_bridge::CONSOLE_BRIDGE_LOG_INFO,  fmt, ##__VA_ARGS__)

#define CONSOLE_BRIDGE_logDebug(fmt, ...)  \
  console_bridge::log(__FILE__, __LINE__, console_bridge::CONSOLE_BRIDGE_LOG_DEBUG, fmt, ##__VA_ARGS__)

//#define logError(fmt, ...)  \
//  console_bridge::log_deprecated(__FILE__, __LINE__, console_bridge::CONSOLE_BRIDGE_LOG_ERROR, fmt, ##__VA_ARGS__)
//
//#define logWarn(fmt, ...)   \
//  console_bridge::log_deprecated(__FILE__, __LINE__, console_bridge::CONSOLE_BRIDGE_LOG_WARN,  fmt, ##__VA_ARGS__)
//
//#define logInform(fmt, ...) \
//  console_bridge::log_deprecated(__FILE__, __LINE__, console_bridge::CONSOLE_BRIDGE_LOG_INFO,  fmt, ##__VA_ARGS__)
//
//#define logDebug(fmt, ...)  \
//  console_bridge::log_deprecated(__FILE__, __LINE__, console_bridge::CONSOLE_BRIDGE_LOG_DEBUG, fmt, ##__VA_ARGS__)


/** \brief Message namespace. This contains classes needed to
    output error messages (or logging) from within the library.
    Message logging can be performed with \ref logging "logging macros" */
namespace console_bridge
{
/** \brief The set of priorities for message logging */
enum LogLevel
  {
    CONSOLE_BRIDGE_LOG_DEBUG = 0,
    CONSOLE_BRIDGE_LOG_INFO,
    CONSOLE_BRIDGE_LOG_WARN,
    CONSOLE_BRIDGE_LOG_ERROR,
    CONSOLE_BRIDGE_LOG_NONE
  };

/** \brief Generic class to handle output from a piece of
    code.
    
    In order to handle output from the library in different
    ways, an implementation of this class needs to be
    provided. This instance can be set with the useOutputHandler
    function. */
class OutputHandler
{
public:
  
  OutputHandler(void)
  {
  }
  
  virtual ~OutputHandler(void)
  {
  }
  
  /** \brief log a message to the output handler with the given text
      and logging level from a specific file and line number */
  virtual void log(const std::string &text, LogLevel level, const char *filename, int line) = 0;
};

/** \brief Default implementation of OutputHandler. This sends
    the information to the console. */
class OutputHandlerSTD : public OutputHandler
{
public:
  
  OutputHandlerSTD(void) : OutputHandler()
  {
  }
  
  virtual void log(const std::string &text, LogLevel level, const char *filename, int line);
  
};

/** \brief Implementation of OutputHandler that saves messages in a file. */
class OutputHandlerFile : public OutputHandler
{
public:
  
  /** \brief The name of the file in which to save the message data */
  OutputHandlerFile(const char *filename);
  
  virtual ~OutputHandlerFile(void);
  
  virtual void log(const std::string &text, LogLevel level, const char *filename, int line);
  
private:
  
  /** \brief The file to save to */
  FILE *file_;
  
};

/** \brief This function instructs ompl that no messages should be outputted. Equivalent to useOutputHandler(NULL) */
void noOutputHandler(void);

/** \brief Restore the output handler that was previously in use (if any) */
void restorePreviousOutputHandler(void);

/** \brief Specify the instance of the OutputHandler to use. By default, this is OutputHandlerSTD */
void useOutputHandler(OutputHandler *oh);

/** \brief Get the instance of the OutputHandler currently used. This is NULL in case there is no output handler. */
OutputHandler* getOutputHandler(void);

/** \brief Set the minimum level of logging data to output.  Messages
    with lower logging levels will not be recorded. */
void setLogLevel(LogLevel level);

/** \brief Retrieve the current level of logging data.  Messages
    with lower logging levels will not be recorded. */
LogLevel getLogLevel(void);

/** \brief Root level logging function.  This should not be invoked directly,
    but rather used via a \ref logging "logging macro".  Formats the message
    string given the arguments and forwards the string to the output handler */
void log(const char *file, int line, LogLevel level, const char* m, ...);

/** \brief Root level logging function.  This should not be invoked directly,
    but rather used via a \ref logging "logging macro".  Formats the message
    string given the arguments and forwards the string to the output handler */
CONSOLE_BRIDGE_DEPRECATED void log_deprecated(const char *file, int line, LogLevel level, const char* m, ...);
}


#endif
