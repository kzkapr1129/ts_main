#include "Logger.h"

#include <stdarg.h>

void log(const char* format, ...) {
  va_list arg;

  va_start(arg, format);
  int len = vsnprintf(NULL, 0, format, arg);
  char* str = (char*)alloca(len + 1);
  if (str) {
    vsnprintf(str, len + 1, format, arg);
  }
  va_end(arg);

  Serial.println(str);
}
