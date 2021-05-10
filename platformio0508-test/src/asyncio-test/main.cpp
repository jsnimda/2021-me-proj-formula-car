/*

  Conclusion:
    Using xQueueSend
    short text (len = 6) serial (0.01 ms) faster than async (0.05 ms)
    long text(len = 368) async (0.05 ms) faster than serial (31.08 ms)

*/

#include <Arduino.h>

#include "AsyncIO.h"
#include "DebugPerf.h"
#include "WiFiConnection.h"

void setup() {
  Serial.begin(115200);
  Serial.println("Hello~");
  wifiConnectionSetup();
  asyncIOSetup();

  AsyncSerial.attachReader([](uint8_t* data, size_t len) {
    AsyncSerial.print("ACK: ");
    AsyncSerial.write(data, len);
    AsyncSerial.println();
  });
}

void printPerfInfo(PerfData& d) {
  Serial.flush();
  Serial.print(d.dumpAndClear());
}

PerfData perf_name(ap);
PerfData perf_name(sp);

#define text_to_test "Hello!"

void loop() {
  perf_rawtype a;
  delay(1000);

  for (int i = 0; i < 10; i++) {
    perf_start(a);
    AsyncSerial.println(text_to_test);
    perf_end(a, ap);
  }
  delay(1000);
  for (int i = 0; i < 10; i++) {
    perf_start(a);
    Serial.println(text_to_test);
    perf_end(a, sp);
  }
  delay(1000);
  Serial.println();
  printPerfInfo(ap);
  printPerfInfo(sp);

  delay(10 * 1000);
}