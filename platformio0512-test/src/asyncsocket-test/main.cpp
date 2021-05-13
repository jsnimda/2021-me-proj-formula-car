/*

  AsyncClient.write(...) time test 
  loop: for (int i = 0; i < 100; i++)

  with interval between (delay(50))
    short text (len = 6) (max 5.5 ms) (avg 1.9 ms)
    long text(len = 368) (max 4.43 ms) (avg 1.9 ms)
    long text(len = 1472) (... about the same) count: 200
    long text(len = 2944) (... about the same) count: 200

  min interval between (delay(1))
    short text (len = 6) (max 4.24 ms) (avg 1.16 ms) count: 11
    long text(len = 368) (max 5.61 ms) (avg 0.45 ms) count: 45
    long text(len = 1472) (max 6.68 ms) (avg 0.23 ms) count: 143
    long text(len = 2944) (max 6.01 ms) (avg 0.209 ms) count: 200

  (vary very much)

  normal ping 2 ms to 20 ms
  ping while sending ~50 ms

*/

#include <Arduino.h>

#include "AirConn.h"

AsyncSocketSerial* pTcpSerial = new AsyncSocketSerial(23);  // remember to begin()
AsyncSocketSerial& tcpSerial = *pTcpSerial;

#define text_to_test "This is a macro that calls the xQueueGenericReceive() function.Receive an item from a queue. The item is received by copy so a buffer of adequate size must be provided. The number of bytes copied into the buffer was defined when the queue was created.This function must not be used in an interrupt service routine. See xQueueReceiveFromISR for an alternative that can."

#define t4 text_to_test text_to_test text_to_test text_to_test
#define t8 t4 t4

// #define text_to_test "Hello~"

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
  for (int i = 0; i < 2000; i++) {
    tcpSerial.lockedPrint(t8 "\r\n");
    delay(1);
  }
  Serial.println("============");
  Serial.print(pd_wifiWrite.dump());
  delay(3000);
}