
#include <Arduino.h>

#include "AirConn.h"

CommandServer server(23);

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
  });

  server.begin();
}

void loop() {
  delay(10 * 1000);
  // server.println("test no locking");
}