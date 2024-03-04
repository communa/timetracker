#include <stdio.h>
#include <stdarg.h>

#include "uiohook_logger.h"
#include "uiohook.h"

/* enum: LOG_LEVEL_DEBUG = 1, LOG_LEVEL_INFO, LOG_LEVEL_WARN, LOG_LEVEL_ERROR */
int uiohookLoggerLevel = LOG_LEVEL_ERROR; /* set the value to 0 to disable logging completely */

void uiohook_logger(int level, const char *format, ...)
{
    if (uiohookLoggerLevel && level >= uiohookLoggerLevel) {
        va_list args;
        va_start(args, format);
        vfprintf(level == LOG_LEVEL_INFO ? stdout : stderr, format, args);
        va_end(args);
    }
}
