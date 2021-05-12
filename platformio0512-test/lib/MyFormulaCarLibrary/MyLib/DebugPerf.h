/*

  Usage:

  Global:
    PerfData perf_name(#perf_data_name);  // Declare data object

  Local:
    perf_rawtype #loc_1, #loc_2, #loc_3;  // Declare local variable for timing use

    perf_start(#loc_1);                   // Start timing

    perf_end(#loc_1);                     // End timing
    perf_end(#loc_1, #perf_data_name);    // End timing + Add measurement to perf data

    perf_add(#loc_1, #perf_data_name);    // Add measurement to perf data

*/

#ifndef DebugPerf_h
#define DebugPerf_h

#include <Arduino.h>
#include <limits.h>

#include "CommonIncludes.h"

extern const double CCountPerMillisecond;

#define PERF_USE_MICROS 0

#ifndef PERF_USE_MICROS
#define PERF_USE_MICROS 0
#endif

#if PERF_USE_MICROS
// #define perfNow_ms_double() (micros() / 1000.0)
#define _perf_now_raw() micros()
#define _perf_raw_type unsigned long
#define _perf_raw_per_ms_double 1000.0
#else  // only for time interval less than 17 (= 2^32 / 240e6) seconds
#define _perf_now_raw() portGET_RUN_TIME_COUNTER_VALUE()
#define _perf_raw_type unsigned
#define _perf_raw_per_ms_double CCountPerMillisecond
#endif

#define _perf_raw_to_ms_double(x) (x / _perf_raw_per_ms_double)

class PerfData {
 public:
  const char* name;
  uint64_t min_raw;
  uint64_t max_raw;
  uint64_t total_raw;
  unsigned int entries_count;
  double min_ms() {
    return _perf_raw_to_ms_double(min_raw);
  }
  double max_ms() {
    return _perf_raw_to_ms_double(max_raw);
  }
  double total_ms() {
    return _perf_raw_to_ms_double(total_raw);
  }
  double avg_ms() {
    return total_ms() / entries_count;
  }
  PerfData() : PerfData("no name") {}
  PerfData(const char* name) : name(name) {
    clear();
  }
  inline __attribute__((always_inline)) void add_entry(uint64_t raw) {
    min_raw = min(min_raw, raw);
    max_raw = max(max_raw, raw);
    total_raw += raw;
    entries_count++;
  }
  void clear() {
    min_raw = UINT64_MAX;
    max_raw = 0;
    total_raw = 0;
    entries_count = 0;
  }
  String dump() {
    return stringf("%s:\r\n", name) +
           stringf("  min: %.6f ms (%llu)\r\n", min_ms(), min_raw) +
           stringf("  max: %.6f ms (%llu)\r\n", max_ms(), max_raw) +
           stringf("  avg: %.6f ms\r\n", avg_ms()) +
           stringf("  count: %d\r\n", entries_count);
  }
  String dumpAndClear() {
    String s = dump();
    clear();
    return s;
  }
};

#define perf_name(perf_data_name) perf_data_name(#perf_data_name)
#define perf_rawtype _perf_raw_type
#define perf_start(loc) loc = _perf_now_raw()
#define _perf_end1(loc) loc = _perf_now_raw() - loc
#define perf_add(loc, perfData) perfData.add_entry(loc)
#define _perf_end2(loc, perfData) \
  _perf_end1(loc);                \
  perf_add(loc, perfData);
#define perf_clear(perfData) perfData.clear()

// ref: https://stackoverflow.com/questions/11761703/overloading-macro-on-number-of-arguments
#define GET_MACRO12(_1, _2, NAME, ...) NAME
#define perf_end(...)                              \
  GET_MACRO12(__VA_ARGS__, _perf_end2, _perf_end1) \
  (__VA_ARGS__)

#endif  // DebugPerf_h
