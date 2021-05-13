/*

*/

#include <Arduino.h>

#include "AirConn.h"

AsyncSocketSerial* pTcpSerial = new AsyncSocketSerial(23);  // remember to begin()
AsyncSocketSerial& tcpSerial = *pTcpSerial;

void setup() {
  Serial.begin(115200);
  airConnSetup();
  tcpSerial.onData([](uint8_t* data, size_t len) {
    // Serial.write(data, len);
    loga(stringf("len: %d\r\n", len));
  });
  tcpSerial.begin();
}

void loop() {
  tcpSerial.lockedPrint("Hello\r\n");
  delay(3000);
}