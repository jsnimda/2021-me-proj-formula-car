
#include "Arduino.h"

#define LED_BUILTIN 2

int i = 500;

void setup() {
  Serial.begin(115200);
}

void loop() {
  Serial.print("main: ");
  Serial.println(i);
  i++;
  delay(1000);
}