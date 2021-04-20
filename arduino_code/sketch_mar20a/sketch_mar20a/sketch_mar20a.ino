#include <ESP32Servo.h>

void setup()
{
 Serial.begin(115200);
 Servo.attach(14);
 Servo.write(0);
}

int val =0;
double a=0.5;

void loop()
 {
  delay(100);
  int v = analogRead(36);
  val =(int)(val*a+v*(1-a));
  Serial.println(val);
  int angle = val * 180/4095;
  Servo.write(angle);
 }
