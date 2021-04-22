
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
  // esp_task_wdt_init(1, true);
  // esp_task_wdt_add(NULL);
  // while (true)
  //   ;
  pinMode(18, OUTPUT);
  digitalWrite(18, LOW);
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

int ir_sensor_pins[] = {13, 39, 14, 27, 25};

void safetyStop() {
  propellerServo.writeMicroseconds(1000);
  SteeringServo.writeMicroseconds(900);
}

void vTaskEncoder(void* pvParameters) {
  portTickType xLastWakeTime;
  const portTickType xFrequency = pdMS_TO_TICKS(20);
  for (;;) {
    checkEncoder();
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

void setup() {
  Serial.begin(115200);
  propellerServo.attach(23, 1000, 2000);
  propellerServo.writeMicroseconds(1000);
  SteeringServo.attach(19, 500, 2500);
  SteeringServo.writeMicroseconds(1430);
  // LeftBrakeServo.attach(36);
  // RightBrakeServo.attach(39);
  // LeftBrakeServo.write(0);
  // RightBrakeServo.write(0);
  pinMode(26, INPUT);
  pinMode(13, INPUT);
  pinMode(39, INPUT);
  pinMode(14, INPUT);
  pinMode(27, INPUT);
  pinMode(25, INPUT);
  
  xTaskCreate(vTaskEncoder, "vTaskEncoder", 1000, NULL, 1, NULL);

#if _DEV_BLE_DEBUG
  stopped = true;  // use ble to start
  waitForBle();
#endif
}

int val = 0;
int car_position = 0;
int Round = 0;
double a = 0.5;
int CheckRound=0;
int ReturnRound=0;
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

int checkEncoder() {
  static int last = 1;
  int intEncoder = digitalRead(26);
  if (last != 0 && intEncoder == 0)
    CheckRound++;
  if(CheckRound==10){
    ReturnRound++;   
    CheckRound=0; 
  }
  last = intEncoder;
  return ReturnRound;
}

void _loop() {
  int TCRT[5];
  propellerServo.writeMicroseconds(1270);
  for (int n = 0; n < 5; n++) {
    TCRT[n] = digitalRead(ir_sensor_pins[n]);
    //_log(TCRT[n]);
  }
  Round = checkEncoder();
  car_position = Encoder(Round);
  _logf("car_position:");
  _log(car_position);
  Turn(TCRT, car_position);
  delay(100);
  if (car_position == 5 || car_position == 10)
    Brake();
}

void Turn(int Senser[5], int The_Car_position) {
  /*for (int n = 0; n < 5; n++){
    _log(Senser[n]);
    }*/
  if (Senser[1] == 0 && Senser[2] == 0 && Senser[3] == 0 ) {
    if (The_Car_position == 1 || The_Car_position == 8)
      SteeringServo.writeMicroseconds(1930);
    else if (The_Car_position == 2 || The_Car_position == 5)
      SteeringServo.writeMicroseconds(930);
    else  // if(The_Car_position!=3)
      SteeringServo.writeMicroseconds(1430);
  }  
    //else if (Senser[0] == 1 || Senser[1] == 1 || Senser[2] == 1 || Senser[3] == 1 || Senser[4] == 1) {
    // if(The_Car_position >= 4 && The_Car_position <=5 || The_Car_position > 6 && The_Car_position <= 9)
    //  Track(Senser);
     //}
}



int Encoder(int Round) {
  int a = 0;
  if (Round >= 0 && Round <1)//First Left Turn
    a = 1;
  else if (Round >= 1 && Round < 2)//First Right Turn
    a = 2;
  else if (Round >= 3 && Round < 4)//First Track Line
    a = 3;
  else if (Round >= 5 && Round < 6)//Second Right Turn
    a = 4;
  else if (Round >= 6 && Round < 8)//Second Track line & Second Left Turn
    a = 5;
  else if (Round >= 8 && Round < 11)//Tracking Second line
    a = 6;
  else if (Round >= 11&& Round < 14)//First Stop & Left Turn to Second Line
    a = 7;
   else if (Round >= 14&& Round < 16)//Tracking the Second To the Final area
    a = 8;
   else if (Round >= 16)
    a = 9;
  return a;
}

void Brake() {
  propellerServo.writeMicroseconds(0);
  delay(1000);
  delay(1000);
  delay(1000);
  delay(1000);
}

// void Track(int TrackSenser[]) {
//   int Trackangle = 200;
//   if (TrackSenser[0] == 1 && TrackSenser[0] == 0&&TrackSenser[1] == 0&& TrackSenser[2] == 0&& TrackSenser[3] == 0&&TrackSenser[4] ==0)
//     SteeringServo.writeMicroseconds(1430);
//   if (TrackSenser[0] == 1 && TrackSenser[1] == 1)
//     SteeringServo.writeMicroseconds(1430-Trackangle);
//   else if (TrackSenser[1] == 1 && TrackSenser[2] == 1)
//     SteeringServo.writeMicroseconds(1430 -Trackangle/2);
//   else if (TrackSenser[4] == 1 && TrackSenser[3] == 1)
//     SteeringServo.writeMicroseconds(1430 + Trackangle);
//   else if (TrackSenser[3] == 1 && TrackSenser[2] == 1)
//     SteeringServo.writeMicroseconds(1430 + Trackangle/2);
// }
