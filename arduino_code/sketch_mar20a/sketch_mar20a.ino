
boolean stopped = false;

#define _DEV_BLE_DEBUG 1
#if _DEV_BLE_DEBUG
#include "BLESerial.h"
#include "esp_task_wdt.h"

//library: https://github.com/iot-bus/BLESerial

BLESerial bleSerial;
#define _logf(x)      \
  bleSerial.print(x); \
  Serial.print(x);
#define _log(x)         \
  bleSerial.println(x); \
  Serial.println(x);

void hard_restart() {
  esp_task_wdt_init(1,true);
  esp_task_wdt_add(NULL);
  while(true);
}

void checkBle() {
  if (!bleSerial.connected()) {
    safetyStop();
    stopped = true;
    hard_restart();
  }
}
void waitForBle() {
  stopped = true;
  bleSerial.begin("ESP32-ble-js");
  while (!bleSerial.connected()) {
    Serial.println("waiting for ble terminal to connect...");
    delay(1000);
  }
  _log("ESP32 ble terminal connected!");
  _log("After one second I will start!");
  delay(1000);
  stopped = false;
}
#else
#define _logf(x) Serial.print(x);
#define _log(x) Serial.println(x);
#endif

// main

#include <ESP32Servo.h>

Servo propellerServo;
Servo SteeringServo;
Servo LeftBrakeServo;
Servo RightBrakeServo;

int ir_sensor_pins[] = {13, 12, 14, 27, 25};

void safetyStop() {
  propellerServo.writeMicroseconds(1000);
  SteeringServo.write(90);
}

void setup() {
  Serial.begin(115200);
  propellerServo.attach(16, 1000, 2000);
  propellerServo.writeMicroseconds(1000);
  SteeringServo.attach(17, 500, 2500);
  SteeringServo.write(90);
  // LeftBrakeServo.attach(36);
  // RightBrakeServo.attach(39);
  // LeftBrakeServo.write(0);
  // RightBrakeServo.write(0);
  pinMode(26, INPUT);
  pinMode(13, INPUT);
  pinMode(12, INPUT);
  pinMode(14, INPUT);
  pinMode(27, INPUT);
  pinMode(25, INPUT);

#if _DEV_BLE_DEBUG
  stopped = true;  // use ble to start
  waitForBle();
#endif
}

int val = 0;
int car_position = 0;
int Round = 0;
double a = 0.5;

void loop() {
#if _DEV_BLE_DEBUG
  checkBle();
#endif
  if (!stopped) {
    _loop();
  } else {
    delay(1000);
  }
}

void _loop() {
  int TCRT[4];
  int intEncoder = digitalRead(17);
  propellerServo.writeMicroseconds(1300);
  for (int n = 0; n < 5; n++) {
    TCRT[n] = digitalRead(ir_sensor_pins[n]);
    _log(TCRT[n]);
  }
  if (intEncoder == 1)
    Round++;
  _logf("Round:");
  _log(Round);
  car_position = Encoder(Round);
  Turn(TCRT, car_position);
  delay(1000);
  if (car_position == 5 || car_position == 7)
    Brake();
}

void Turn(int Senser[], int The_Car_position) {
  if (The_Car_position == 3 || The_Car_position == 5)
    if (Senser[0] == 0 && Senser[0] == Senser[1] == Senser[2] == Senser[3] == Senser[4]) {
      if (The_Car_position == 1 || The_Car_position == 6)
        SteeringServo.write(45);
      else if (The_Car_position == 3 || The_Car_position == 5)
        SteeringServo.write(315);
      else  // if(The_Car_position!=3)
        SteeringServo.write(0);
    }  //else if (Senser[0] == 1 || Senser[1] == 1 || Senser[2] == 1 || Senser[3] == 1 || Senser[4] == 1) {
       //Track(Senser);
       //}
}

/*void Track(int TrackSenser[]) {
  int Trackangle = 20;
  if (TrackSenser[0] == 1 && TrackSenser[0] == TrackSenser[1] == TrackSenser[2] == TrackSenser[3] == TrackSenser[4])
    SteeringServo.write(45);
  if (TrackSenser[0] == 1 && TrackSenser[1] == 1)
    SteeringServo.write(Trackangle);
  else if (TrackSenser[1] == 1 && TrackSenser[2] == 1)
    SteeringServo.write(Trackangle - 10);
  else if (TrackSenser[4] == 1 && TrackSenser[3] == 1)
    SteeringServo.write(360 - Trackangle);
  else if (TrackSenser[3] == 1 && TrackSenser[2] == 1)
    SteeringServo.write(360 - Trackangle - 10);
}*/

int Encoder(int Round) {
  int a = 0;
  if (Round >= 1 && Round < 3)
    a = 1;
  else if (Round >= 3 && Round < 7)
    a = 2;
  else if (Round >= 7 && Round < 9)
    a = 3;
  else if (Round >= 9 && Round < 16)
    a = 4;
  else if (Round >= 16 && Round < 18)
    a = 5;
  else if (Round >= 18 && Round < 30)
    a = 6;
  else if (Round >= 30)
    a = 7;
  return a;
}

void Brake() {
  LeftBrakeServo.write(90);
  RightBrakeServo.write(90);
  delay(1000);
  delay(1000);
  delay(1000);
  delay(1000);
  LeftBrakeServo.write(0);
  RightBrakeServo.write(0);
}
