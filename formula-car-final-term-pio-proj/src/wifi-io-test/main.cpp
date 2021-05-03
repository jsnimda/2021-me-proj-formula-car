#include <Arduino.h>

#include <CircularBuffer.h>

typedef CircularBuffer_T<WIFI_MTU_SIZE * 2> CircularBuffer;
CircularBuffer test;

void setup() {
  // put your setup code here, to run once:
  unsigned char buf[10] = "4567";
  test.write(buf, 4);
  test.read(buf, 4);
}

void loop() {
  // put your main code here, to run repeatedly:
  // Serial.write
}