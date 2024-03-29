
// ============
// Monitor variables
// ============

boolean running = false;

#define _len(x) (sizeof(x) / sizeof(x[0]))

// ============
// Race Management
// ============


// ============
// BLE terminal
// ============

#define _DEV_BLE_LOG
#ifdef _DEV_BLE_LOG

//library: https://github.com/iot-bus/BLESerial
#include "BLESerial.h"

#define _logf(...)              \
  bleSerial.print(__VA_ARGS__); \
  Serial.print(__VA_ARGS__);
#define _log(...)                 \
  bleSerial.println(__VA_ARGS__); \
  Serial.println(__VA_ARGS__);

BLESerial bleSerial;
void waitForBle() {
  running = false;
  while (!bleSerial.connected()) {
    Serial.println("waiting for ble terminal to connect...");
    delay(1000);
  }
  _log("ESP32 ble terminal connected!");
  _log("After one second I will start!");
  running = true;
  delay(1000);
  start();
}
// add this line in setup():
//    xTaskCreate(vTaskCheckBle, "vTaskCheckBle", 1000, NULL, 1, NULL);
void vTaskCheckBle(void* pvParameters) {
  for (;;) {
    if (running) {
      if (!bleSerial.connected()) {
        running = false;
        reset();
        waitForBle();
      }
      delay(1);
    } else {
      delay(1000);
    }
  }
}

#else
#define _logf(...) Serial.print(__VA_ARGS__);
#define _log(...) Serial.println(__VA_ARGS__);
#endif

// ============
// Pin definitions
// ============

#define PIN_fan 23
#define PIN_steer 19
#define PIN_brake 18
#define PIN_encoder 26
const int ir_array_count = 5;
// int PIN_ir_array[] = {13, 39, 14, 27, 25};
int PIN_ir_array[] = {25, 27, 14, 39, 13};
int ir_array_values[ir_array_count];

// ============
// Encoder
// ============

int encoderCount = 0;
int encoderInterval = 0;
double distanceTranvelled_cm = 0;
double wheelDiameter_cm = 6.25;
int countPerRevolution = 12;

// add this line in setup():
//    xTaskCreate(vTaskEncoder, "vTaskEncoder", 1000, NULL, 1, NULL);
void vTaskEncoder(void* pvParameters) {
  int last = 1;
  unsigned long long lastTime = 0;
  for (;;) {

    if (running) {
      if (!bleSerial.connected()) {
        running = false;
        reset();
        waitForBle();
      }
    } 
  


    int current = digitalRead(PIN_encoder);
    if (current != last) {
      encoderCount++;
      unsigned long long currentTime = millis();
      encoderInterval = currentTime - lastTime;
      lastTime = currentTime;
      updateEncoderDistance();
    }
    last = current;


    for (int i = 0; i < ir_array_count; i++) {
      ir_array_values[i] = digitalRead(PIN_ir_array[i]);
    }


    delay(1);
  }
}
void updateEncoderDistance() {
  distanceTranvelled_cm = encoderCount * wheelDiameter_cm * PI / countPerRevolution;
}

// ============
// Status logger
// ============
// char logBuffer[50];
// sprintf(buffer, "the current value is %d", i++);

// add this line in setup():
//    xTaskCreate(vTaskStatusLogger, "vTaskStatusLogger", 5000, NULL, 1, NULL);
void vTaskStatusLogger(void* pvParameters) {
  for (;;) {
    if (running) {
      // _logf("ir array:");
      // for (int i = 0; i < ir_array_count; i++) {
      //   _logf(" ");
      //   _logf(ir_array_values[i]);
      // }
      // _log();

      // _logf("count: ");
      // _log(encoderCount);

      // _logf("dis: ");
      // _log(distanceTranvelled_cm, 2);
    }
    delay(150);
  }
}

// ============
// Hardwares/Servos
// ============

// do servo.attach() in setup!!
#include <ESP32Servo.h>
Servo propellerServo;
Servo steerServo;
Servo brakeServo;

int steer_center_us = 1430;

// ============
// IR Array
// ============

// // add this line in setup():
// //    xTaskCreate(vTaskIrArray, "vTaskIrArray", 5000, NULL, 1, NULL);
// void vTaskIrArray(void* pvParameters) {
//   for (;;) {
//     for (int i = 0; i < ir_array_count; i++) {
//       ir_array_values[i] = digitalRead(PIN_ir_array[i]);
//     }
//     delay(1);
//   }
// }

// ============
// Route-test
// ============

enum EnterDirection {
  EnterLeft = -250,
  EnterRight = 250,
};
class EnterLineFollowManager {
};
enum Movement {
  Straight,
  Left,
  Right,
  Stop,
  Brake,
  Brake_And_Go,
  Brake_And_Stop,
  EnterLineFollow_1,
  EnterLineFollow_2,
  EnterLineFollow_3,
  Left_until_middle,
  UTurn,
  UTurnEnd,
};

// double segments[] = {30, 26.934, 47.1, 10.989, 70.305, 10.699,               37.417, 26.452, 73.401};
// Movement segMovements[] = {Straight, Right, Straight, Left, Straight, Left, Straight, Right, Straight};
double segments[] = {30, 25.934, 42.1 - 10, 45.305 - 5, 23.699 - 5, 10, 38.401 + 20, 0,0, 100, 0, 20, 0, 100, 0};
Movement segMovements[] = {Straight, Right, Straight, EnterLineFollow_1, Left, Straight, EnterLineFollow_2, Brake_And_Stop, Brake_And_Go, UTurn, UTurnEnd, Straight, Left_until_middle, EnterLineFollow_3, Brake};

int currentSegmentIndex = 0;
double seg_offset = 0;
void handleRouteTest() {
  if (currentSegmentIndex >= min(_len(segments), _len(segMovements))) {
    return;
  }
  if ((distanceTranvelled_cm - seg_offset) >= segments[currentSegmentIndex]) {
    seg_offset += segments[currentSegmentIndex];  // will vary to acc segments when line following
    currentSegmentIndex++;
    handleMovement();
  }
}
void handleMovement() {
  _logf("next movement: ");
  _log(currentSegmentIndex);
  if (currentSegmentIndex >= min(_len(segments), _len(segMovements))) {
    stopServos();
    return;
  }
  switch (segMovements[currentSegmentIndex]) {
    case Straight:
      steerServo.writeMicroseconds(steer_center_us);  // 1430, +-500 = 46.5 deg
      break;
    case Left:
      steerServo.writeMicroseconds(steer_center_us - 500);  // 1430, +-500 = 46.5 deg
      break;
    case Right:
      steerServo.writeMicroseconds(steer_center_us + 500);  // 1430, +-500 = 46.5 deg
      break;
    case Stop:
      stopServos();
      break;
    case Brake:
      doBrake();
      break;
    case Brake_And_Go:  // u turn afterwards
      doBrake();
      seg_offset = distanceTranvelled_cm;
      if (running) restartPropeller(1280);
      break;
    case UTurn:
      steerServo.writeMicroseconds(steer_center_us + 550);  // 1430, +-500 = 46.5 deg
      break;
    case UTurnEnd:
      if (running)   propellerServo.writeMicroseconds(1270);
      break;
    case Brake_And_Stop:
      doBrake();
      running = false;
      break;
    case Left_until_middle:
    _log("Left_until_middle");
      steerServo.writeMicroseconds(steer_center_us - 500);
      delay(200);
      while (ir_array_values[2] != 1) {
        dirLine();
        delay(1);
      }
      steerServo.writeMicroseconds(steer_center_us);
      // seg_offset = distanceTranvelled_cm;  // will vary to acc segments when line following
      // currentSegmentIndex++;
      // handleMovement();
      dirLine();
      doEnterLineFollow_3();
      break;
    case EnterLineFollow_1:
      doEnterLineFollow_1(segments[currentSegmentIndex]);
      break;
    case EnterLineFollow_2:
      doEnterLineFollow_2(segments[currentSegmentIndex]);
      break;
    case EnterLineFollow_3:
      doEnterLineFollow_3();
      break;
  }
}
void stopServos() {
  propellerServo.writeMicroseconds(1000);
  steerServo.writeMicroseconds(1430);  // 1430, +-500 = 46.5 deg
  brakeServo.writeMicroseconds(1500);
}

// ============
// Line Follow Manager
// ============

boolean hasLine() {
  for (int i = 0; i < ir_array_count; i++) {
    if (ir_array_values[i] == 1) {
      return true;
    }
  }
  return false;
}

int countLine() {
  int c = 0;
  for (int i = 0; i < ir_array_count; i++) {
    if (ir_array_values[i] == 1) {
      c++;
    }
  }
  return c;
}

int lastDir = 0;

int dirLine() {
  int c = countLine();
  if (c == 0) return lastDir;
  if (c != 1) return 0;
  if (ir_array_values[0] == 1 || ir_array_values[1] == 1) 
    lastDir = -1;
  else if (ir_array_values[3] == 1 || ir_array_values[4] == 1)
    lastDir = 1;
  else lastDir = 0;
  return lastDir;
}
int angle_Cal;

void doEnterLineFollow_1(double travelDis_cm) {
  _log("doEnterLineFollow_1");
  angle_Cal=200;
  steerServo.writeMicroseconds(steer_center_us);
  while (running) {
    if (hasLine()) {
      _log("hasLine");
      delay(50);
      steerServo.writeMicroseconds(steer_center_us - 500);
      delay(450);
      int init_pos = distanceTranvelled_cm;
      steerServo.writeMicroseconds(steer_center_us);
      while (distanceTranvelled_cm - init_pos < travelDis_cm) {Calibration(angle_Cal);delay(1);}
      seg_offset = distanceTranvelled_cm;  // will vary to acc segments when line following
      currentSegmentIndex++;
      handleMovement();

      // doBrake();
      // running = false;
      return;
    }
    delay(1);
  }
}

void doEnterLineFollow_2(double travelDis_cm) {
  _log("doEnterLineFollow_2");
  angle_Cal=-400;
  steerServo.writeMicroseconds(steer_center_us);
  while (running) {
    if (hasLine()) {
      _log("hasLine");
      steerServo.writeMicroseconds(steer_center_us + 500);
      delay(500);
      int init_pos = distanceTranvelled_cm;
      steerServo.writeMicroseconds(steer_center_us);
      while (distanceTranvelled_cm - init_pos < travelDis_cm) {Calibration(angle_Cal);delay(1);}
      seg_offset = distanceTranvelled_cm;  // will vary to acc segments when line following
      currentSegmentIndex++;
      handleMovement();

      // doBrake();
      // running = false;
      return;
    }
    delay(1);
  }
}

void doEnterLineFollow_3() { // simpleLineFollow
  _log("doEnterLineFollow_3");
  double target = distanceTranvelled_cm + 80;
  steerServo.writeMicroseconds(steer_center_us);
  while (running && distanceTranvelled_cm < target) {
    if (distanceTranvelled_cm >= target) {
      // seg_offset = distanceTranvelled_cm;  // will vary to acc segments when line following
      // currentSegmentIndex++;
      // handleMovement();

      doBrake();
      running = false;
      return;
    }
    int dd = dirLine();
    if (dd == 0) steerServo.writeMicroseconds(steer_center_us);
    if (dd == -1) steerServo.writeMicroseconds(steer_center_us - 200);
    if (dd == 1) steerServo.writeMicroseconds(steer_center_us + 200);

    delay(1);
  }
}

void Calibration(int angle_in_Cal)
{
  int Line=dirLine();
  if(Line!=0)
    while(angle_in_Cal>0){      
      steerServo.writeMicroseconds(steer_center_us + angle_in_Cal/(-2));
      angle_in_Cal/=(-2);
    } 
}
// ============
// Main
// ============

void reset() {  // please follow setup
  propellerServo.writeMicroseconds(1000);
  steerServo.writeMicroseconds(1430);  // 1430, +-500 = 46.5 deg
  brakeServo.writeMicroseconds(1500);

  encoderCount = 0;
  distanceTranvelled_cm = 0;

  currentSegmentIndex = 0;
  seg_offset = 0;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  propellerServo.attach(PIN_fan, 500, 2500);
  steerServo.attach(PIN_steer, 500, 2500);
  brakeServo.attach(PIN_brake, 500, 2500);
  reset();

  pinMode(PIN_encoder, INPUT);
  for (int i = 0; i < ir_array_count; i++) {
    pinMode(PIN_ir_array[i], INPUT);
  }

  bleSerial.begin("ESP32-ble-js");


  xTaskCreate(vTaskEncoder, "vTaskEncoder", 1000, NULL, 1, NULL);
  // xTaskCreate(vTaskIrArray, "vTaskIrArray", 1000, NULL, 1, NULL);
  running = true;

#ifdef _DEV_BLE_LOG
  // xTaskCreate(vTaskCheckBle, "vTaskCheckBle", 1000, NULL, 1, NULL);
  // xTaskCreate(vTaskStatusLogger, "vTaskStatusLogger", 5000, NULL, 1, NULL);
  waitForBle();
#endif
}

void doBrake() {
  //ref stopServos
  propellerServo.writeMicroseconds(1000);
  brakeServo.writeMicroseconds(1150);
  delay(1000);
  stopServos();
  delay(2000);
}
// dis_test
// void dis_test() {
//   if (distanceTranvelled_cm >= 300) {
//     doBrake();
//     running = false;
//   }
// }

void restartPropeller() {
  restartPropeller(1270);
}
void restartPropeller(int b) {
  if (!running) return;
  propellerServo.writeMicroseconds(1400);
  delay(500);
  propellerServo.writeMicroseconds(b);
}

void start() {
  restartPropeller();
  encoderCount = 0;
  distanceTranvelled_cm = 0;
}

void loop() {
  // put your main code here, to run repeatedly:
  if (running) {
    handleRouteTest();
    // dis_test();
  }
  delay(1);
}
