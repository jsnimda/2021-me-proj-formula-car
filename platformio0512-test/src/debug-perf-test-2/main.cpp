#include <Arduino.h>

#include "DebugPerf.h"
#include "MyLibCommon.h"

void setup() {
  Serial.begin(115200);
  Serial.println("Hello~");
}

void printPerfInfo(PerfData &d) {
  Serial.print(d.dumpAndClear());
}

PerfData perf_name(d1);
PerfData perf_name(d2);
PerfData perf_name(d3);

#define text_to_test "Hello!"

void loop() {
  perf_rawtype a;  //, b, c;
  delay(1000);

  for (int i = 0; i < 1000; i++) {
    perf_start(a);
    perf_end(a, d1);
  }
  Serial.println("=== empty ===");
  printPerfInfo(d1);

  for (int i = 0; i < 1000; i++) {
    perf_start(a);
    uint8_t *hello = new uint8_t[1500 * 2];
    perf_end(a, d1);
    delete hello;
  }
  Serial.println("=== new ===");
  printPerfInfo(d1);

  for (int i = 0; i < 1000; i++) {
    perf_start(a);
    uint8_t *hello = new uint8_t[1500 * 2];
    delete hello;
    perf_end(a, d1);
  }
  Serial.println("=== new/delete ===");
  printPerfInfo(d1);

  const int bufsize = 1500 * 2;
  uint8_t *buf1 = new uint8_t[bufsize];
  uint8_t *buf2 = new uint8_t[bufsize];
  for (int k = 0; k < bufsize; k++) {
    buf1[k] = esp_random();
    buf2[k] = esp_random();
  }

  for (int i = 0; i < 1000; i++) {
    perf_start(a);
    esp_random();
    perf_end(a, d1);
  }
  Serial.println("=== esp_random() ===");
  printPerfInfo(d1);

  for (int i = 0; i < 1000; i++) {
    perf_start(a);
    memcpy(buf1, buf2, bufsize);
    perf_end(a, d1);
  }
  Serial.println("=== memcpy() ===");
  printPerfInfo(d1);

  for (int i = 0; i < 1000; i++) {
    perf_start(a);
    memcpy(buf1, buf2, 0);
    perf_end(a, d1);
  }
  Serial.println("=== memcpy 0 ===");
  printPerfInfo(d1);

  for (int i = 0; i < 1000; i++) {
    perf_start(a);
    memcpy(buf1, buf2, 1);
    perf_end(a, d1);
  }
  Serial.println("=== memcpy 1 ===");
  printPerfInfo(d1);

  for (int i = 0; i < 1000; i++) {
    perf_start(a);
    memcpy(buf1, buf2, bufsize / 2);
    perf_end(a, d1);
  }
  Serial.println("=== memcpy (size / 2) ===");
  printPerfInfo(d1);

  for (int k = 0; k < bufsize; k++) {
    buf1[k] = esp_random();
    buf2[k] = esp_random();
  }

  for (int i = 0; i < 1000; i++) {
    perf_start(a);
    for (int k = 0; k < bufsize; k++) {
      buf1[k] = buf2[k];
    }
    perf_end(a, d1);
  }
  Serial.println("=== for loop copy ===");
  printPerfInfo(d1);

  for (int i = 0; i < 100; i++) {
    perf_start(a);
    for (int k = 0; k < bufsize; k++) {
      buf1[k] = esp_random();
      buf2[k] = esp_random();
    }
    perf_end(a, d1);
  }
  Serial.println("=== for loop set random ===");
  printPerfInfo(d1);

  for (int i = 0; i < 1000; i++) {
    perf_start(a);
    memset(buf1, 'A', bufsize);
    perf_end(a, d1);
  }
  Serial.println("=== memset 'A' ===");
  printPerfInfo(d1);

  for (int i = 0; i < 1000; i++) {
    perf_start(a);
    memset(buf1, esp_random(), bufsize);
    perf_end(a, d1);
  }
  Serial.println("=== memset esp_random() ===");
  printPerfInfo(d1);

  for (int i = 0; i < 1000; i++) {
    perf_start(a);
    esp_random();
    memset(buf1, 'A', bufsize);
    perf_end(a, d1);
  }
  Serial.println("=== memset 'A'/esp_random() ===");
  printPerfInfo(d1);

  Serial.println("============");
  delay(10 * 1000);
}

/*

Result 2

new    0.01 ms
delete 0.015 ms
memcpy 0.008 ms
memset 0.004 ms
for loop copy
       0.05 ms

10:46:34.937 > === empty ===
10:46:34.940 > d1:
10:46:34.940 >   min: 0.000046 ms
10:46:34.942 >   max: 0.000063 ms
10:46:34.943 >   avg: 0.000046 ms
10:46:34.945 >   count: 1000
10:46:34.968 > === new ===
10:46:34.971 > d1:
10:46:34.971 >   min: 0.022221 ms
10:46:34.973 >   max: 0.031008 ms
10:46:34.974 >   avg: 0.022507 ms
10:46:34.977 >   count: 1000
10:46:34.999 > === new/delete ===
10:46:35.003 > d1:
10:46:35.003 >   min: 0.027254 ms
10:46:35.005 >   max: 0.036029 ms
10:46:35.006 >   avg: 0.027568 ms
10:46:35.007 >   count: 1000
10:46:35.010 > === esp_random() ===
10:46:35.011 > d1:
10:46:35.011 >   min: 0.000604 ms
10:46:35.013 >   max: 0.000763 ms
10:46:35.016 >   avg: 0.000610 ms
10:46:35.017 >   count: 1000
10:46:35.019 > === memcpy() ===
10:46:35.021 > d1:
10:46:35.023 >   min: 0.007987 ms
10:46:35.025 >   max: 0.015562 ms
10:46:35.025 >   avg: 0.008042 ms
10:46:35.028 >   count: 1000
10:46:35.029 > === memcpy 0 ===
10:46:35.030 > d1:
10:46:35.030 >   min: 0.000046 ms
10:46:35.033 >   max: 0.000046 ms
10:46:35.034 >   avg: 0.000046 ms
10:46:35.037 >   count: 1000
10:46:35.037 > === memcpy 1 ===
10:46:35.038 > d1:
10:46:35.040 >   min: 0.000058 ms
10:46:35.042 >   max: 0.000075 ms
10:46:35.043 >   avg: 0.000058 ms
10:46:35.045 >   count: 1000
10:46:35.045 > === memcpy (size / 2) ===
10:46:35.049 > d1:
10:46:35.049 >   min: 0.004100 ms
10:46:35.052 >   max: 0.004167 ms
10:46:35.053 >   avg: 0.004101 ms
10:46:35.054 >   count: 1000
10:46:35.100 > === for loop copy ===
10:46:35.104 > d1:
10:46:35.104 >   min: 0.050075 ms
10:46:35.106 >   max: 0.057446 ms
10:46:35.108 >   avg: 0.050457 ms
10:46:35.110 >   count: 1000
10:46:35.453 > === for loop set random ===
10:46:35.457 > d1:
10:46:35.458 >   min: 3.516421 ms
10:46:35.458 >   max: 3.527492 ms
10:46:35.460 >   avg: 3.522023 ms
10:46:35.463 >   count: 100
10:46:35.465 > === memset 'A' ===
10:46:35.466 > d1:
10:46:35.466 >   min: 0.004125 ms
10:46:35.468 >   max: 0.011696 ms
10:46:35.470 >   avg: 0.004155 ms
10:46:35.471 >   count: 1000
10:46:35.472 > === memset esp_random() ===
10:46:35.475 > d1:
10:46:35.476 >   min: 0.004679 ms
10:46:35.477 >   max: 0.012254 ms
10:46:35.478 >   avg: 0.004728 ms
10:46:35.482 >   count: 1000
10:46:35.482 > === memset 'A'/esp_random() ===
10:46:35.484 > d1:
10:46:35.484 >   min: 0.004675 ms
10:46:35.487 >   max: 0.012250 ms
10:46:35.488 >   avg: 0.004716 ms
10:46:35.491 >   count: 1000
10:46:35.492 > ============

*/