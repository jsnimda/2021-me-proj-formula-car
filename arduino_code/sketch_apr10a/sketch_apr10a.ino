#include <ESP32Servo.h>

void setup() {
 Serial.begin(115200);
 pinMode(13,INPUT); pinMode(12,INPUT);pinMode(4,INPUT);pinMode(5,INPUT);pinMode(6,INPUT);
}

void loop() {
  delay(1000);
  int TCRTa =digitalRead(13);
  int TCRTb =digitalRead(12);
  int TCRTc =digitalRead(4);
  int TCRTd =digitalRead(5);
  int TCRTe =digitalRead(6);

  Serial.println(TCRTa);
  Serial.println(TCRTb);
  Serial.println(TCRTc);
  Serial.println(TCRTd);
  Serial.println(TCRTe);
 //int TCRT[5];
 //for(int n = 0;n < 4;n++){
  //TCRT[n]=digitalRead(n+2);
  //Serial.println(TCRT[n]);
 //}
}
