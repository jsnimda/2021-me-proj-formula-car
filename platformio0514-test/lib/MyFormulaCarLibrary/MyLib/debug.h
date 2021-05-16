#ifndef debug_h
#define debug_h

// ref: https://github.com/pycom/pycom-esp-idf/blob/idf_v3.2/components/esp32/panic.c
// see line 162
void print_backtrace() __attribute__((noinline));

#endif  // debug_h
