
#include <Arduino.h>

#include "AirConn.h"

AsyncSocketSerial* pTcpSerial = new AsyncSocketSerial(23);  // remember to begin()
AsyncSocketSerial& tcpSerial = *pTcpSerial;
SocketCommandProcessor cmd = pTcpSerial;

void setup() {
  Serial.begin(115200);
  airConnSetup();

  cmd.use(basic_route);
  cmd.use([](Line& c) {
    if (c.on("haha")) {
      c.setResponse("goodbye!");
    }
  });
  cmd.attach();

  tcpSerial.begin();
}

void loop() {
  delay(3000);
}