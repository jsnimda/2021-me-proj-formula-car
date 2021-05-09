#ifndef DebugPerf_h
#define DebugPerf_h

#include <Arduino.h>
#include <float.h>

extern const double CCountPerMillisecond;

#ifdef USE_MICROS
// #define perfNow_ms_double() (micros() / 1000.0)
#define perfNow_raw() micros()
#define perfRawType unsigned long
#define perfRawToMs_double(x) (x / 1000.0)
#else  // only for time interval less than 17 (= 2^32 / 240e6) seconds
// #define perfNow_ms_double() (portGET_RUN_TIME_COUNTER_VALUE() / CCountPerMillisecond)
#define perfNow_raw() portGET_RUN_TIME_COUNTER_VALUE()
#define perfRawType unsigned
#define perfRawToMs_double(x) (x / CCountPerMillisecond)
#endif

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
};

#define perfCreate(x) PerfData x##PerfData(#x);
#define perfDeclare() perfRawType _perf_local_var;
#define perfStart() (_perf_local_var = perfNow_raw());
#define perfEnd(x)                                         \
  _perf_local_var = perfNow_raw() - _perf_local_var; \
  x##PerfData.add_entry(perfRawToMs_double(_perf_local_var));
#define perfClear(x) x##PerfData.clear();

#endif  // DebugPerf_h
