#ifdef DEBUG
#define LOG(fmt, ...) log(fmt, __VA_ARGS__)
#define LOGIF(cond, fmt, ...) if (cond) log(fmt, __VA_ARGS__)
#else
#define LOG(fmt, ...)
#define LOGIF(cond, fmt, ...)
#endif

#ifndef __LOGGER_H__
#define __LOGGER_H__

void log(const char* format, ...);

#endif
