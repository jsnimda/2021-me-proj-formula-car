
// ============
// Monitor variables
// ============

boolean running = false;

#define _len(x) (sizeof(x) / sizeof(x[0]))

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
    int current = digitalRead(PIN_encoder);
    if (current != last) {
      encoderCount++;
      unsigned long long currentTime = millis();
      encoderInterval = currentTime - lastTime;
      lastTime = currentTime;
      updateEncoderDistance();
    }
    last = current;
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
      _logf("ir array:");
      for (int i = 0; i < ir_array_count; i++) {
        _logf(" ");
        _logf(ir_array_values[i]);
      }
      _log();

      _logf("count: ");
      _log(encoderCount);

      _logf("dis: ");
      _log(distanceTranvelled_cm, 2);
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

// add this line in setup():
//    xTaskCreate(vTaskIrArray, "vTaskIrArray", 5000, NULL, 1, NULL);
void vTaskIrArray(void* pvParameters) {
  for (;;) {
    for (int i = 0; i < ir_array_count; i++) {
      ir_array_values[i] = digitalRead(PIN_ir_array[i]);
    }
    delay(1);
  }
}

// ============
// Line Follow Manager
// ============

enum EnterDirection {
  EnterLeft = -250,
  EnterRight = 250,
};
class EnterLineFollowManager {
};
boolean hasLine() {
  for (int i = 0; i < ir_array_count; i++) {
    if (ir_array_values[i] == 1) {
      return true;
    }
  }
  return false;
}
void doEnterLineFollow(int direction, double travelDis_cm) {
  while (running) {
    if (hasLine()) {
      steerServo.writeMicroseconds(steer_center_us + direction);
      delay(1000);
      doBrake();
      running = false;
    }
    delay(1);
  }
}

// ============
// Route-test
// ============

enum Movement {
  Straight,
  Left,
  Right,
  Stop,
  Brake,
  EnterLineFollow_left,
  EnterLineFollow_right,
};

// double segments[] = {30, 26.934, 47.1, 10.989, 70.305, 10.699, 37.417, 26.452, 73.401};
// Movement segMovements[] = {Straight, Right, Straight, Left, Straight, Left, Straight, Right, Straight};
double segments[] = {30, 23.934, 32.1, 10.989, 70.305, 10.699, 37.417, 26.452, 73.401};
Movement segMovements[] = {Straight, Right, Straight, EnterLineFollow_left, Brake};

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
    case EnterLineFollow_left:
      doEnterLineFollow(EnterDirection::EnterLeft, segments[currentSegmentIndex]);
      break;
    case EnterLineFollow_right:
      doEnterLineFollow(EnterDirection::EnterRight, segments[currentSegmentIndex]);
      break;
  }
}
void stopServos() {
  propellerServo.writeMicroseconds(1000);
  steerServo.writeMicroseconds(1430);  // 1430, +-500 = 46.5 deg
  brakeServo.writeMicroseconds(1500);
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
  xTaskCreate(vTaskEncoder, "vTaskEncoder", 1000, NULL, 1, NULL);
  xTaskCreate(vTaskIrArray, "vTaskIrArray", 5000, NULL, 1, NULL);
  running = true;

#ifdef _DEV_BLE_LOG
  bleSerial.begin("ESP32-ble-js");
  xTaskCreate(vTaskCheckBle, "vTaskCheckBle", 1000, NULL, 1, NULL);
  xTaskCreate(vTaskStatusLogger, "vTaskStatusLogger", 5000, NULL, 1, NULL);
  waitForBle();
#endif
}

void doBrake() {
  //ref stopServos
  propellerServo.writeMicroseconds(1000);
  brakeServo.writeMicroseconds(1200);
  delay(3000);
  stopServos();
}
// dis_test
// void dis_test() {
//   if (distanceTranvelled_cm >= 300) {
//     doBrake();
//     running = false;
//   }
// }

void start() {
  propellerServo.writeMicroseconds(1270);
  delay(200);
  propellerServo.writeMicroseconds(1250);
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
