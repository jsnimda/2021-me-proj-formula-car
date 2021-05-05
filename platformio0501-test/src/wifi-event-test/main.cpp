/*

  Conclusion: 

  - just WiFi.begin(...) after accidentally disconnected
      can rarely reconnect (only success after one minute) after disconnect
      stat is always WL_NO_SSID_AVAIL

  - do WiFi.disconnect(true, ...)
      disable STA and re-enable, can work!

  - do WiFi._setStatus(WL_IDLE_STATUS);
      (???) can work!!
      set to WL_DISCONNECTED also works!!
      because of waitForConnectResult() ???

  Found the cause now:
    Calling WiFi.begin() too frequently (less than 2 seconds) causing
      esp_wifi_connect recalled before any wifi result event occurs
      -> status won't update -> keep looping and no connection

*/

#include <Arduino.h>
#include <WiFi.h>

#define mySSID "JS"
#define myPASSWORD "81288128"

#define TEST_MAX_TASKS_NUM 32

void dumpAllTaskInfo() {
  // cannot use uxTaskGetSystemState(), but seems that we can use uxTaskGetSnapshotAll()!!
  // ref: https://github.com/espressif/esp-idf/blob/master/components/freertos/test/test_tasks_snapshot.c

  TaskSnapshot_t tasks[TEST_MAX_TASKS_NUM];
  UBaseType_t tcb_sz;
  UBaseType_t task_num = uxTaskGetSnapshotAll(tasks, TEST_MAX_TASKS_NUM, &tcb_sz);

  Serial.flush();
  Serial.printf("uxTaskGetNumberOfTasks: %d\r\n", uxTaskGetNumberOfTasks());
  Serial.printf("task_num: %d\r\n", task_num);

  for (int i = 0; i < task_num; i++) {
    TaskHandle_t tskHandle = tasks[i].pxTCB;
    Serial.printf("Task %d            : %s\r\n", i, pcTaskGetTaskName(tskHandle));
    Serial.printf("  uxTaskPriorityGet: %d\r\n", uxTaskPriorityGet(tskHandle));
    int eState = eTaskGetState(tskHandle);
    const char* stateText = ((const char*[]){
        "Running",
        "Ready",
        "Blocked",
        "Suspended",
        "Deleted",
    })[eState];
    Serial.printf("  eTaskGetState    : %d - %s\r\n", eState, stateText);
    Serial.printf("  xTaskGetAffinity : %d\r\n", xTaskGetAffinity(tskHandle));
    // uxTaskGetTaskNumber not available
    // Serial.printf("  uxTaskGetTaskNumber: %d\r\n", uxTaskGetTaskNumber(tskHandle));
  }
}

void eventA(WiFiEvent_t event) {  // delay() will block event thread
  Serial.flush();

  Serial.print("onWifiEvent core: ");
  Serial.println(xPortGetCoreID());
  Serial.print("uxTaskPriorityGet: ");
  Serial.println(uxTaskPriorityGet(NULL));
  Serial.print("pcTaskGetTaskName: ");
  Serial.println(pcTaskGetTaskName(NULL));

  Serial.printf("event a: %d ...\r\n", event);
  Serial.printf("stat: %d\r\n", WiFi.status());
  delay(5000);
  Serial.printf("event a: %d end\r\n", event);
}

void eventB(WiFiEvent_t event) {
  Serial.flush();
  Serial.printf("event b: %d ...\r\n", event);
  Serial.printf("stat: %d\r\n", WiFi.status());
  delay(5000);
  Serial.printf("event b: %d end\r\n", event);
}

void setup() {
  Serial.begin(115200);
  delay(10);

  WiFi.onEvent(eventA);  // WiFi.status() is set before event call
  WiFi.onEvent(eventB);
}

bool j = false;

void loop() {
  if (!WiFi.isConnected()) {
    Serial.flush();
    Serial.println("WiFi.begin...");

    WiFi.begin(mySSID, myPASSWORD);

    Serial.flush();
    Serial.println("WiFi.begin end");
  }

  delay(15000);
  if (!j) {
    j = true;
    dumpAllTaskInfo();
  }
}