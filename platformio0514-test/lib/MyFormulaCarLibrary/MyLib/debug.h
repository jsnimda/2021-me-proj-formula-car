#ifndef debug_h
#define debug_h

#include <Arduino.h>

// ref: https://github.com/pycom/pycom-esp-idf/blob/idf_v3.2/components/esp32/panic.c
// see line 162
void print_backtrace();

typedef struct {
  TaskHandle_t tsk;
  const char* name;
  UBaseType_t priority;
  eTaskState state;
  BaseType_t core_id;
  uint32_t endOfStack;
  uint32_t topOfStack;
  uint32_t highWaterMark;
  uint32_t stackStart;
  esp_err_t wdt_status;
  bool isCurrentTask;
} debug_task_info;

std::vector<debug_task_info> dump_task_info();
String taskInfoAll();
String taskInfoRawAll();

#endif  // debug_h
