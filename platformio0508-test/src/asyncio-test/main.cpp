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
  Serial.printf("%s:\r\n", d.name);
  Serial.printf("  min: %.6f ms\r\n", d.min_ms);
  Serial.printf("  max: %.6f ms\r\n", d.max_ms);
  Serial.printf("  avg: %.6f ms\r\n", d.total_ms / d.entries_count);
  Serial.printf("  count: %d\r\n", d.entries_count);
}

perfCreate(asyncPrint);
perfCreate(serialPrint);

#define text_to_test "Hello!"

void loop() {
  perfDeclare();
  delay(1000);

  for (int i = 0; i < 10; i++) {
    perfStart();
    AsyncSerial.println(text_to_test);
    perfEnd(asyncPrint);
  }
  delay(1000);
  for (int i = 0; i < 10; i++) {
    perfStart();
    Serial.println(text_to_test);
    perfEnd(serialPrint);
  }
  delay(1000);
  Serial.println();
  printPerfInfo(asyncPrintPerfData);
  printPerfInfo(serialPrintPerfData);

  perfClear(asyncPrint);
  perfClear(serialPrint);

  delay(10 * 1000);
}