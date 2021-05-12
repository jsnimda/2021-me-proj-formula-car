#ifndef Common_h
#define Common_h

#ifndef CONFIG_USE_ASYNC_SERIAL
#define CONFIG_USE_ASYNC_SERIAL 0
#endif

#include <Arduino.h>

// ============
// debug
// ============

void loga(String s);

// ============
// useful functions
// ============

inline String stringf(const char* format, ...) __attribute__((format(printf, 1, 2)));
inline String stringf(const char* format, ...) {
  // ref: see Print::printf(...)
  char loc_buf[64];
  char* temp = loc_buf;
  va_list arg;
  va_list copy;
  va_start(arg, format);
  va_copy(copy, arg);
  int len = vsnprintf(temp, sizeof(loc_buf), format, copy);
  va_end(copy);
  if (len < 0) {
    va_end(arg);
    return "";
  };
  if (len >= sizeof(loc_buf)) {
    temp = (char*)malloc(len + 1);
    if (temp == NULL) {
      va_end(arg);
      return "";
    }
    len = vsnprintf(temp, len + 1, format, arg);
  }
  va_end(arg);
  String s = temp;
  if (temp != loc_buf) {
    free(temp);
  }
  return s;
}

// ============
// useful macros
// ============

// Rule of three
#define NONCOPYABLE(class_name)           \
  class_name(const class_name&) = delete; \
  class_name& operator=(const class_name&) = delete

#endif  // Common_h
