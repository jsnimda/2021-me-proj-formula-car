
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
  });

  server.begin();
}

void loop() {
  delay(3000);
}