/*

  Conclusion:
    Using xQueueSend
    short text (len = 6) serial (0.01 ms) faster than async (0.05 ms)
    long text(len = 368) async (0.05 ms) faster than serial (31.08 ms)

    Using buffer
    short text (len = 6) serial (0.02 ms) faster than async (0.003 ms)
    long text(len = 368) async (0.07 ms) faster than serial (31.08 ms)
      (but async overflowed)

*/

#include <Arduino.h>

#include "AsyncIO.h"
#include "DebugPerf.h"
#include "WiFiConnection.h"

void setup() {
  Serial.begin(115200);
  Serial.println("Hello~");
  // wifiConnectionSetup();
  asyncIOSetup();

  AsyncSerial.onData([](uint8_t* data, size_t len) {
    AsyncSerial.lockBuf();
    AsyncSerial.print("ACK: ");
    AsyncSerial.write(data, len);
    AsyncSerial.println();
    AsyncSerial.unlockBuf();
  });
}

void printPerfInfo(PerfData& d) {
  Serial.flush();
  Serial.print(d.dumpAndClear());
}

PerfData perf_name(ap);
PerfData perf_name(sp);

#define text_to_test "This is a macro that calls the xQueueGenericReceive() function.Receive an item from a queue. The item is received by copy so a buffer of adequate size must be provided. The number of bytes copied into the buffer was defined when the queue was created.This function must not be used in an interrupt service routine. See xQueueReceiveFromISR for an alternative that can."

void loop() {
  perf_rawtype a;
  delay(1000);

  for (int i = 0; i < 10; i++) {
    perf_start(a);
    AsyncSerial.lockBuf();
    AsyncSerial.print(text_to_test);
    AsyncSerial.println(i);
    AsyncSerial.unlockBuf();
    perf_end(a, ap);
  }
  delay(500);
  AsyncSerial.lockBuf();
  AsyncSerial.println();
  AsyncSerial.println();
  AsyncSerial.unlockBuf();
  delay(500);
  for (int i = 0; i < 10; i++) {
    perf_start(a);
    Serial.print(text_to_test);
    Serial.println(i);
    perf_end(a, sp);
  }
  delay(1000);
  Serial.println();
  printPerfInfo(ap);
  printPerfInfo(sp);

  delay(10 * 1000);
}