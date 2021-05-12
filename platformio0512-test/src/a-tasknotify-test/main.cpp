/*

  ulTaskNotifyTake return notify value before decrease
    i.e. 0 means timeout

    pdFALSE notify value decrease by 1
    pdTRUE  notify value set to 0

  xTaskNotifyGive notify value increase by 1

  xTaskNotifyGive time 
    first max 0.008 ms
    max 0.003 ms

  ulTaskNotifyTake response time
    b to a + a to b
      0.03 ms

*/

#include <Arduino.h>

#include "MyLib/DebugPerf.h"

int a = 0;

TaskHandle_t tskA_handle;
TaskHandle_t tskB_handle;

// PerfData perf_name(aData);
PerfData perf_name(bData);

void taskA(void*) {
  delay(1000);
  for (;;) {
    uint32_t res = ulTaskNotifyTake(pdFALSE, 10 * 1000);
    xTaskNotifyGive(tskB_handle);
    Serial.print("Hello ");
    Serial.println(a++);
    Serial.print("    ");
    Serial.println(res);
    delay(1000);
  }
}

void taskB(void*) {
  delay(5000);
  perf_rawtype b;
  for (;;) {
    perf_start(b);
    xTaskNotifyGive(tskA_handle);
    ulTaskNotifyTake(pdFALSE, 10 * 1000);
    perf_end(b, bData);
    delay(3 * 1000);
    Serial.println("============");
    // Serial.print(aData.dump());
    Serial.print(bData.dump());
    Serial.println("============");
  }
}

void setup() {
  Serial.begin(115200);
  xTaskCreatePinnedToCore(taskA, "taskA", 4096, NULL, 5, &tskA_handle, 0);
  xTaskCreatePinnedToCore(taskB, "taskB", 4096, NULL, 5, &tskB_handle, 1);

}

void loop() {
  delay(3600 * 1000);
  return;
}