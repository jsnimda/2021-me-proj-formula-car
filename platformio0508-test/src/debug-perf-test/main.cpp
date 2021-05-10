#include <Arduino.h>

#include "DebugPerf.h"
#include "MyLibCommon.h"

void setup() {
  Serial.begin(115200);
  Serial.println("Hello~");
}

void printPerfInfo(PerfData& d) {
  Serial.print(d.dumpAndClear());
}

PerfData perf_name(d1);
PerfData perf_name(d2);
PerfData perf_name(d3);

#define text_to_test "Hello!"

void loop() {
  perf_rawtype a, b, c;
  delay(1000);

  for (int i = 0; i < 1000; i++) {
    perf_start(a);
    perf_end(a, d1);
  }
  Serial.println("=== empty ===");
  printPerfInfo(d1);

  for (int i = 0; i < 1000; i++) {
    perf_start(a);
    delay(1);
    perf_end(a, d1);
  }
  Serial.println("=== delay(1) ===");
  printPerfInfo(d1);

  for (int i = 0; i < 1000; i++) {
    perf_start(a);
    perf_start(b);
    perf_start(c);
    perf_end(c);
    perf_end(b);
    perf_end(a);
    perf_add(a, d1);
    perf_add(b, d2);
    perf_add(c, d3);
  }
  Serial.println("=== nesting test ===");
  printPerfInfo(d1);
  printPerfInfo(d2);
  printPerfInfo(d3);

  for (int i = 0; i < 1000; i++) {
    perf_start(a);
    perf_start(b);
    perf_start(c);
    perf_end(c, d3);
    perf_end(b, d2);
    perf_end(a, d1);
  }
  Serial.println("=== nesting test 2 ===");
  printPerfInfo(d1);
  printPerfInfo(d2);
  printPerfInfo(d3);

  Serial.println("============");
  delay(10 * 1000);
}

/*

Change add_entry from double ms to raw
Do raw to ms (double) transformation only in dump

11:33:25.295 > === empty ===
11:33:25.297 > d1:
11:33:25.297 >   min: 0.000046 ms (11)
11:33:25.300 >   max: 0.000046 ms (11)
11:33:25.302 >   avg: 0.000046 ms
11:33:25.305 >   count: 1000
11:33:26.295 > === delay(1) ===
11:33:26.298 > d1:
11:33:26.298 >   min: 0.999792 ms (239950)
11:33:26.301 >   max: 1.463071 ms (351137)
11:33:26.303 >   avg: 1.000255 ms     
11:33:26.306 >   count: 1000
11:33:26.307 > === nesting test ===   
11:33:26.309 > d1:
11:33:26.310 >   min: 0.000237 ms (57)
11:33:26.311 >   max: 0.000237 ms (57)
11:33:26.313 >   avg: 0.000237 ms     
11:33:26.315 >   count: 1000
11:33:26.317 > d2:
11:33:26.318 >   min: 0.000142 ms (34)
11:33:26.319 >   max: 0.000142 ms (34)
11:33:26.321 >   avg: 0.000142 ms     
11:33:26.324 >   count: 1000
11:33:26.324 > d3:
11:33:26.324 >   min: 0.000050 ms (12)
11:33:26.327 >   max: 0.000050 ms (12)
11:33:26.330 >   avg: 0.000050 ms
11:33:26.331 >   count: 1000
11:33:26.332 > === nesting test 2 ===
11:33:26.334 > d1:
11:33:26.334 >   min: 0.000538 ms (129)
11:33:26.337 >   max: 0.000575 ms (138)
11:33:26.339 >   avg: 0.000538 ms
11:33:26.341 >   count: 1000
11:33:26.343 > d2:
11:33:26.343 >   min: 0.000296 ms (71)
11:33:26.346 >   max: 0.000325 ms (78)
11:33:26.348 >   avg: 0.000296 ms
11:33:26.349 >   count: 1000
11:33:26.351 > d3:
11:33:26.351 >   min: 0.000050 ms (12)
11:33:26.353 >   max: 0.000063 ms (15)
11:33:26.354 >   avg: 0.000050 ms
11:33:26.356 >   count: 1000
11:33:26.359 > ============

*/
/*

nesting test 2

PERF_USE_MICROS 0

10:48:51.311 > === nesting test ===
10:48:51.314 > d1:
10:48:51.314 >   min: 0.000233 ms
10:48:51.316 >   max: 0.000308 ms
10:48:51.318 >   avg: 0.000233 ms
10:48:51.320 >   count: 1000
10:48:51.321 > d2:
10:48:51.321 >   min: 0.000142 ms
10:48:51.323 >   max: 0.000183 ms
10:48:51.324 >   avg: 0.000142 ms
10:48:51.326 >   count: 1000
10:48:51.328 > d3:
10:48:51.328 >   min: 0.000046 ms
10:48:51.330 >   max: 0.000063 ms
10:48:51.331 >   avg: 0.000046 ms
10:48:51.332 >   count: 1000
10:48:51.335 > === nesting test 2 ===
10:48:51.337 > d1:
10:48:51.337 >   min: 0.007004 ms
10:48:51.338 >   max: 0.014858 ms
10:48:51.341 >   avg: 0.007207 ms
10:48:51.342 >   count: 1000
10:48:51.344 > d2:
10:48:51.344 >   min: 0.003512 ms
10:48:51.345 >   max: 0.011108 ms
10:48:51.348 >   avg: 0.003547 ms
10:48:51.350 >   count: 1000
10:48:51.351 > d3:
10:48:51.351 >   min: 0.000046 ms
10:48:51.353 >   max: 0.000063 ms
10:48:51.355 >   avg: 0.000046 ms
10:48:51.356 >   count: 1000
10:48:51.358 > ============

*/
/*

Result 1

PERF_USE_MICROS 0

21:42:04.019 > === empty ===
21:42:04.022 > d1:
21:42:04.023 >   min: 0.000050 ms
21:42:04.025 >   max: 0.000071 ms
21:42:04.026 >   avg: 0.000050 ms
21:42:04.028 >   count: 1000
21:42:05.020 > === delay(1) ===  
21:42:05.022 > d1:
21:42:05.022 >   min: 0.996246 ms
21:42:05.024 >   max: 1.129446 ms
21:42:05.027 >   avg: 0.996542 ms
21:42:05.028 >   count: 1000       
21:42:05.031 > === nesting test ===
21:42:05.034 > d1:
21:42:05.035 >   min: 0.000250 ms  
21:42:05.037 >   max: 0.000333 ms
21:42:05.038 >   avg: 0.000250 ms
21:42:05.039 >   count: 1000
21:42:05.040 > d2:
21:42:05.042 >   min: 0.000142 ms
21:42:05.043 >   max: 0.000192 ms
21:42:05.045 >   avg: 0.000142 ms
21:42:05.047 >   count: 1000
21:42:05.048 > d3:
21:42:05.050 >   min: 0.000046 ms
21:42:05.050 >   max: 0.000063 ms
21:42:05.052 >   avg: 0.000046 ms
21:42:05.054 >   count: 1000
21:42:05.055 > ============

PERF_USE_MICROS 1

21:43:07.390 > === empty ===     
21:43:07.393 > d1:
21:43:07.394 >   min: 0.000000 ms
21:43:07.395 >   max: 0.009000 ms
21:43:07.397 >   avg: 0.000985 ms
21:43:07.399 >   count: 1000
21:43:08.390 > === delay(1) ===
21:43:08.392 > d1:
21:43:08.394 >   min: 0.798000 ms
21:43:08.395 >   max: 0.996000 ms
21:43:08.397 >   avg: 0.995802 ms
21:43:08.399 >   count: 1000
21:43:08.405 > === nesting test ===
21:43:08.408 > d1:
21:43:08.408 >   min: 0.003000 ms
21:43:08.412 >   max: 0.012000 ms
21:43:08.413 >   avg: 0.003868 ms
21:43:08.413 >   count: 1000
21:43:08.415 > d2:
21:43:08.415 >   min: 0.002000 ms
21:43:08.418 >   max: 0.010000 ms
21:43:08.420 >   avg: 0.002139 ms
21:43:08.422 >   count: 1000
21:43:08.423 > d3:
21:43:08.423 >   min: 0.000000 ms
21:43:08.424 >   max: 0.001000 ms
21:43:08.427 >   avg: 0.000938 ms
21:43:08.428 >   count: 1000
21:43:08.430 > ============

*/