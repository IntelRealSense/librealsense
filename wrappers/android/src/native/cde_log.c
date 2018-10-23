// License: Apache 2.0. See LICENSE file in root directory.
//
// a common/simple log mechanism for troubleshooting Linux kernel 
// issue, Android source code issue, Linux source code issue


#include "cde_log.h"

#ifndef __KERNEL__
#include <string.h>
#else
#include <linux/printk.h>
#endif

#define NONE                 "\e[0m"
#define BLACK                "\e[0;30m"
#define L_BLACK              "\e[1;30m"
#define RED                  "\e[0;31m"
#define L_RED                "\e[1;31m"
#define GREEN                "\e[0;32m"
#define L_GREEN              "\e[1;32m"
#define BROWN                "\e[0;33m"
#define L_YELLOW             "\e[1;33m"
#define BLUE                 "\e[0;34m"
#define L_BLUE               "\e[1;34m"
#define PURPLE               "\e[0;35m"
#define L_PURPLE             "\e[1;35m"
#define CYAN                 "\e[0;36m"
#define L_CYAN               "\e[1;36m"
#define WHITE                "\e[0;37m"
#define L_WHITE              "\e[1;37m"

#define BOLD                 "\e[1m"
#define UNDERLINE            "\e[4m"
#define BLINK                "\e[5m"
#define REVERSE              "\e[7m"
#define HIDE                 "\e[8m"
#define CLEAR                "\e[2J"
#define CLRLINE              "\r\e[K" //or "\e[1K\r"



#define LOG_BUF_LEN 4096
static char logBuf[LOG_BUF_LEN];

void  LOG_PRI_IMPL(const char *file, const char *func, unsigned int line,  int priority, const char *tag, const char *format,  ...) {

    memset(logBuf, 0, LOG_BUF_LEN);

    va_list va;
    va_start(va, format);
    int len = 0;

    const char *color = L_RED;

    switch (priority) {
        case ANDROID_LOG_VERBOSE:
            color = L_RED;
            break;
        case ANDROID_LOG_DEBUG:
            color = L_YELLOW;
            break;
        case ANDROID_LOG_INFO:
            color = L_WHITE;
            break;
        case ANDROID_LOG_WARN:
            color = L_GREEN;
            break;
        default:
            color = L_RED;
            break;
    }
#ifndef __KERNEL__
    //len = snprintf(logBuf, LOG_BUF_LEN, "%s[%s,%s,%d]: ", color, file, func, line);
    len = snprintf(logBuf, LOG_BUF_LEN, "%s[%s,%d]: ", color, func, line);
#else
    len = snprintf(logBuf, LOG_BUF_LEN, "[%s,%s,%d]: ", file, func, line);
#endif

    vsnprintf(logBuf + len, LOG_BUF_LEN - len, format, va);

#ifndef __KERNEL__
    #ifdef __ANDROID__
    __android_log_print(priority, tag, "%s", logBuf);
    __android_log_print(priority, tag, NONE);
    #else
    printf("%s%s", logBuf, NONE);
    #endif
#else
    printk(KERN_INFO "%s", logBuf);
#endif

    va_end(va);
}
