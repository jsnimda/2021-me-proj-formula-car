
#include <Arduino.h>

#include "AirConn.h"

CommandServer server(23);

void outTestTask(void* args) {
  server.lockedPrint("hello\r\n");
  delay(1000);
  server.lockedPrint("hello\r\n");
  delay(1000);
  server.lockedPrint("hello\r\n");
  delay(1000);
  server.lockedPrint("a");
  delay(1000);
  server.lockedPrint("b");
  delay(1000);
  server.lockedPrint("c");
  delay(1000);
  server.lockedPrint("\r\n");
  delay(1000);
  server.lockedPrint("\r\n");
  delay(1000);
  server.lockedPrint("x");
  delay(1000);
  server.lockedPrint("y");
  delay(1000);
  server.lockedPrint("z");
  delay(1000);
  vTaskDelete(NULL);
}

void setup() {
  Serial.begin(115200);
  airConnSetup();

  server.use(core_r);
  server.use([](Line& c) {
    if (c.on("haha")) {
      c.setResponse("goodbye!");
    }
    if (c.on("h2")) {
      print_backtrace();
    }
    if (c.on("h3")) {
      print_backtrace();
      Serial.flush();
      delay(500);
      abort();
    }
    if (c.on("out")) {
      c.ack();
      xTaskCreatePinnedToCore(
        outTestTask, 
        "outTestTask",
        4096,
        NULL,
        4,
        NULL,
        0);
    }
  });

  server.begin();
}

void loop() {
  delay(10 * 1000);
  // server.println("test no locking");
}