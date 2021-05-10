#ifndef DebugPerf_h
#define DebugPerf_h

#include <Arduino.h>
#include <float.h>

#include "MyLibCommon.h"

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
  double min_ms;
  double max_ms;
  double total_ms;
  unsigned int entries_count;
  PerfData() : PerfData("no name") {}
  PerfData(const char* name) : name(name) {
    clear();
  }
  void add_entry(double ms) {
    min_ms = min(min_ms, ms);
    max_ms = max(max_ms, ms);
    total_ms += ms;
    entries_count++;
  }
  void clear() {
    min_ms = DBL_MAX;
    max_ms = 0;
    total_ms = 0;
    entries_count = 0;
  }
  String dump() {
    return stringf("%s:\r\n", name) +
           stringf("  min: %.6f ms\r\n", min_ms) +
           stringf("  max: %.6f ms\r\n", max_ms) +
           stringf("  avg: %.6f ms\r\n", total_ms / entries_count) +
           stringf("  count: %d\r\n", entries_count);
  }
  String dumpAndClear() {
    String s = dump();
    clear();
    return s;
  }
};

#define perf_name(x) x(#x)
#define perf_rawtype _perf_raw_type
#define perf_start(x) x = _perf_now_raw()
#define _perf_end1(x) x = _perf_now_raw() - x
#define perf_add(x, perfData) perfData.add_entry(_perf_raw_to_ms_double(x))
#define _perf_end2(x, perfData) \
  _perf_end1(x);                \
  perf_add(x, perfData);
#define perf_clear(perfData) perfData.clear()

// ref: https://stackoverflow.com/questions/11761703/overloading-macro-on-number-of-arguments
#define GET_MACRO12(_1, _2, NAME, ...) NAME
#define perf_end(...)                              \
  GET_MACRO12(__VA_ARGS__, _perf_end2, _perf_end1) \
  (__VA_ARGS__)

#endif  // DebugPerf_h
