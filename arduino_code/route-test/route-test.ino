
// ============
// Global constants
// ============

// define ENABLE_BLE for ble launch and print out
#define ENABLE_BLE

// default speed for straight walk, faster speed for sharp turn
#define DEFAULT_propeller_us 1265
#define FASTER_propeller_us 1275
#define OVERCOME_STATIC_propeller_us 1400

// servo brake us, brake 1150, release 1500
#define BRAKE_us 1150

const int steer_center_us = 1430;

#define TASK_STACK_SIZE 8192

// ============
// Global variables
// ============

boolean running = false;
boolean disablePropeller = false;

// ============
// Pin definitions
// ============

#define PIN_fan 23
#define PIN_steer 19
#define PIN_brake 18
#define PIN_encoder 26
const int ir_array_count = 5;
int PIN_ir_array[] = {25, 27, 14, 39, 13};
int ir_array_values_last[ir_array_count];
int ir_array_values[ir_array_count];  // readings from ir array

#define PIN_on_off_btn 4

// ============
// Helper macros
// ============

#define _len(x) (sizeof(x) / sizeof(x[0]))

#ifdef ENABLE_BLE
#define _logf(...)           \
  Serial.print(__VA_ARGS__); \
  bleSerial.print(__VA_ARGS__);

#define _log(...)              \
  Serial.println(__VA_ARGS__); \
  bleSerial.println(__VA_ARGS__);

#else
#define _logf(...) Serial.print(__VA_ARGS__);
#define _log(...) Serial.println(__VA_ARGS__);
#endif

// ============
// Physical On/Off Button
// ============

// TODO: replace with attachInterrupt()

// on/off button with led
// led on = 1 = start race, led off = 0 = stop race
// for digitalRead, 0 = led on, 1 = led off

// add to setup():
//    setup_on_off_button();
int on_led_last = 0;
int on_led = 0;  // 1 = led on, 0 = led off
void setup_on_off_button() {
  pinMode(PIN_on_off_btn, INPUT_PULLUP);
  on_led = !digitalRead(PIN_on_off_btn);
  on_led_last = on_led;
}

int check_led() {
  on_led_last = on_led;
  on_led = !digitalRead(PIN_on_off_btn);
  if (on_led_last == 0 && on_led == 1)
    return true;
  else
    return false;
}

int check_led_off() {
  check_led();
  if (on_led_last == 1 && on_led == 0)
    return true;
  else
    return false;
}

// ============
// BLE terminal
// ============

#ifdef ENABLE_BLE
// library modified, ref: https://github.com/iot-bus/BLESerial
#include "BLESerial.h"
// add to setup():
//    bleSerial.begin("ESP32-ble-js");
BLESerial bleSerial;
#endif

// ============
// Race Start/Stop Manager
// ============

boolean use_led = false;  // indicate use ble or use led to start race

void startRace() {
  running = true;
  // after this line will enter loop() function
}

void stopRace() {
  // use abort() to force restart the whole ESP32
  running = false;
  stopServos();
  abort();
}

boolean hasStartRaceSignal() {
#ifdef ENABLE_BLE
  return bleSerial.connected() || check_led();
#else
  return check_led();
#endif
}

boolean hasStopRaceSignal() {
#ifdef ENABLE_BLE
  if (use_led) {
    return check_led_off();
  } else {
    return !bleSerial.connected();
  }
#else
  return check_led_off();
#endif
}

void waitForStart() {  // waitForBle
  // wait for ble connect or button pressed
  running = false;
  while (!hasStartRaceSignal()) {
#ifdef ENABLE_BLE
    Serial.println("waiting for ble terminal to connect...");
#else
    Serial.println("waiting for on off button to start...");
#endif
    delay(1000);
  }
  if (on_led) {
    use_led = true;
    _log("On/off button pressed!");
  }
#ifdef ENABLE_BLE
  else {
    _log("ESP32 ble terminal connected!");
  }
#endif
  _log("After one second I will start!");
  delay(1000);
  startRace();
}

// ============
// Hardwares/Servos
// ============

// do servo.attach() in setup!!
#include <ESP32Servo.h>
Servo propellerServo;
Servo steerServo;
Servo brakeServo;

// ============
// Encoder, IR Arrays
// ============

int encoderCount = 0;
int encoderInterval = 0;
double distanceTranvelled_cm = 0;
double wheelDiameter_cm = 6.25;
int countPerRevolution = 12;

// add this line in setup():
//    xTaskCreate(vTaskSensorLoop, "vTaskSensorLoop", TASK_STACK_SIZE, NULL, 1, NULL);
void vTaskSensorLoop(void* pvParameters) {
  int last = 1;
  unsigned long long lastTime = 0;
  for (;;) {
    // check ble disconnected or led turned off
    if (running) {
      if (hasStopRaceSignal()) {
        stopRace();
      }
    }

    // encoder
    int current = digitalRead(PIN_encoder);
    if (current != last) {
      encoderCount++;
      unsigned long long currentTime = millis();
      encoderInterval = currentTime - lastTime;
      lastTime = currentTime;
      updateEncoderDistance();
    }
    last = current;

    // ir array
    for (int i = 0; i < ir_array_count; i++) {
      ir_array_values_last[i] = ir_array_values[i];
    }
    for (int i = 0; i < ir_array_count; i++) {
      ir_array_values[i] = digitalRead(PIN_ir_array[i]);
    }
    analyzeIR();

    delay(1);
  }
}
void updateEncoderDistance() {
  distanceTranvelled_cm = encoderCount * wheelDiameter_cm * PI / countPerRevolution;
}

// ir

/*
find line, line is 1 to 2 ir thick: -2, -1, 0, 1, 2
is not line: -30  (more than 1 to 2 ir thick)
blank: -40
line left from left: -20
line left from right: 20
*/
int where_is_line_last = -30;
int where_is_line = -30;
int cross_count = 0;
int go_off = -30;

void setWhere(int where) {
  if (where != where_is_line) {
    if (where_is_line >= -20) where_is_line_last = where_is_line;
    where_is_line = where;
  }
  if (where >= -2 && where <= 2) {
    go_off = -30;
  }
}

void findWhere() {
  if (countLine(ir_array_values) == 0 && countLine(ir_array_values_last) <= 2) {
    if (ir_array_values_last[0] == 1 || ir_array_values_last[1] == 1) {
      setWhere(-20);
      go_off = -20;
    }
    if (ir_array_values_last[4] == 1 || ir_array_values_last[3] == 1) {
      setWhere(20);
      go_off = 20;
    }
    return;
  }
  if (countLine(ir_array_values) == 0) {
    setWhere(-40);
    return;
  }
  if (countLine(ir_array_values) == 1) {
    if (ir_array_values[0] == 1) setWhere(-2);
    if (ir_array_values[1] == 1) setWhere(-1);
    if (ir_array_values[2] == 1) setWhere(0);
    if (ir_array_values[3] == 1) setWhere(1);
    if (ir_array_values[4] == 1) setWhere(2);
    return;
  }
  if (countLine(ir_array_values) == 2) {
    if (ir_array_values[0] == 1 && ir_array_values[1] == 1) setWhere(-2);
    if (ir_array_values[1] == 1 && ir_array_values[2] == 1) setWhere(-1);
    if (ir_array_values[2] == 1 && ir_array_values[3] == 1) setWhere(1);
    if (ir_array_values[3] == 1 && ir_array_values[4] == 1) setWhere(2);
    return;
  }
  setWhere(-30);
}

void analyzeIR() {
  findWhere();
  checkCross();
}

void checkCross() {
  if (countLine(ir_array_values) == 5 && /*countLine(ir_array_values_last) != 0 &&*/ countLine(ir_array_values_last) != 5) {
    cross_count++;
    _logf("cross_count: ");
    _log(cross_count);
  }
}

int countLine() {
  return countLine(ir_array_values);
}

int countLine(int ir[]) {
  int c = 0;
  for (int i = 0; i < ir_array_count; i++) {
    if (ir[i] == 1) {
      c++;
    }
  }
  return c;
}

boolean hasLine() {
  return countLine() > 0;
}

// ============
// Status logger
// ============
// char logBuffer[50];
// sprintf(buffer, "the current value is %d", i++);

void logIR() {
  _logf("ir array:");
  for (int i = 0; i < ir_array_count; i++) {
    _logf(" ");
    _logf(ir_array_values[i]);
  }
  _log();
}
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
      _logf("where_is_line_last: ");
      _log(where_is_line_last);
      _logf("where_is_line: ");
      _log(where_is_line);
      _logf("cross_count: ");
      _log(cross_count);
      _log("============");
    }
    delay(200);
  }
}

// ============
// Route-test
// ============

enum Movement {
  LaunchPropeller_us,
  vResetDisplacement,
  Straight_cm,
  Left_cm,
  Right_cm,
  vBrake_And_Go,
  vBrake_And_Stop,
  UTurn_cm,
  TimedLeft_1,
  EnterLineFollow_1_cm,
  EnterLineFollow_2_cm,
  TimedLeft_2,
  vEnterLineFollow_3,
};
typedef struct {
  Movement movement;
  double value;
} SegmentAction;

SegmentAction segmentActions[] = {
    {LaunchPropeller_us, DEFAULT_propeller_us},
    {vResetDisplacement, 0},

    {Straight_cm, 30},
    {Right_cm, 25.934},
    {Straight_cm, 42.1 - 10},
    {EnterLineFollow_1_cm, 45.305 - 15 + 5},
    {TimedLeft_1, 0},
    {Straight_cm, 10},
    {EnterLineFollow_2_cm, 38.401 + 15},
    {vBrake_And_Go, 0},
    {LaunchPropeller_us, FASTER_propeller_us},
    {Straight_cm, 2 + 6},
    {UTurn_cm, 110},
    {LaunchPropeller_us, DEFAULT_propeller_us},
    {TimedLeft_2, 0},
    {vEnterLineFollow_3, 0},
    {vBrake_And_Stop, 0},
};

enum SegmentState {
  PendingStart,
  WaitingForTarget,
  PendingNext,
};

int currentSegmentIndex = 0;
SegmentState currentSegmentState = PendingStart;
// double seg_offset = 0;

void waitUntilDistance(double targetDistance) {
  while (distanceTranvelled_cm < targetDistance) {
    delay(1);
  }
}
void waitUntilSegmentDistance() {
  waitUntilDistance(distanceTranvelled_cm + segmentActions[currentSegmentIndex].value);
  gotoNextSegment();
}

void waitUntilSegmentTime() {
  delay(segmentActions[currentSegmentIndex].value);
  gotoNextSegment();
}

void gotoNextSegment() {
  currentSegmentState = PendingNext;
}

void handleRouteTest() {
  if (currentSegmentIndex >= _len(segmentActions)) {  // we have run all the segments
    return;
  }
  while (currentSegmentState == PendingStart || currentSegmentState == PendingNext) {
    if (!running) return;
    if (currentSegmentState == PendingStart) {
      currentSegmentState = WaitingForTarget;
      handleMovement();
    } else if (currentSegmentState == PendingNext) {
      currentSegmentIndex++;
      currentSegmentState = PendingStart;
    }
  }
}
void handleMovement() {  // remember gotoNextSegment or waitUntilXXX
  _logf("next movement: ");
  _log(currentSegmentIndex);
  if (currentSegmentIndex >= _len(segmentActions)) {
    stopServos();
    return;
  }
  switch (segmentActions[currentSegmentIndex].movement) {
    case Straight_cm:
      steerServo.writeMicroseconds(steer_center_us);  // 1430, +-500 = 46.5 deg
      waitUntilSegmentDistance();
      break;
    case Left_cm:
      steerServo.writeMicroseconds(steer_center_us - 500);  // 1430, +-500 = 46.5 deg
      waitUntilSegmentDistance();
      break;
    case Right_cm:
      steerServo.writeMicroseconds(steer_center_us + 500);  // 1430, +-500 = 46.5 deg
      waitUntilSegmentDistance();
      break;
    case vBrake_And_Go:  // u turn afterwards
      doBrake();
      gotoNextSegment();
      break;
    case UTurn_cm:
      steerServo.writeMicroseconds(steer_center_us + 550);  // 1430, +-500 = 46.5 deg
      waitUntilSegmentDistance();
      break;
    case vBrake_And_Stop:
      doBrake();
      running = false;
      gotoNextSegment();
      break;
    case TimedLeft_1:
      steerServo.writeMicroseconds(steer_center_us - 500);  // 1430, +-500 = 46.5 deg
      delay(20);
      steerServo.writeMicroseconds(steer_center_us - 500);  // 1430, +-500 = 46.5 deg
      delay(20);
      steerServo.writeMicroseconds(steer_center_us - 500);  // 1430, +-500 = 46.5 deg
      delay(20);
      steerServo.writeMicroseconds(steer_center_us - 500);  // 1430, +-500 = 46.5 deg
      delay(20);
      steerServo.writeMicroseconds(steer_center_us - 500);  // 1430, +-500 = 46.5 deg
      delay(20);
      steerServo.writeMicroseconds(steer_center_us - 500);  // 1430, +-500 = 46.5 deg
      delay(375);
      gotoNextSegment();
      break;
    case TimedLeft_2:
      steerServo.writeMicroseconds(steer_center_us - 550);  // 1430, +-500 = 46.5 deg
      delay(550);
      gotoNextSegment();
      break;
    case EnterLineFollow_1_cm:
      doEnterLineFollow_1(segmentActions[currentSegmentIndex].value);
      break;
    case EnterLineFollow_2_cm:
      doEnterLineFollow_2(segmentActions[currentSegmentIndex].value);
      break;
    case vEnterLineFollow_3:
      doEnterLineFollow_3();
      break;
  }
}

// ============
// Line Follow Manager
// ============

int getDir() {
  if (where_is_line >= -20) return where_is_line;
  if (go_off == -20 || go_off == 20) return go_off;
  return 0;
}

#define deflect_1 50
#define deflect_2 80
#define deflect_3 100

void doSimpleLineFollow(double travelDis_cm) {
  go_off = -30;
  _log("doSimpleLineFollow");
  int init_pos = distanceTranvelled_cm;
  while (distanceTranvelled_cm - init_pos < travelDis_cm) {
    int dir = getDir();
    int us = steer_center_us;
    switch (dir) {
      case -20:
        us = steer_center_us - deflect_3;
        break;
      case -2:
        us = steer_center_us - deflect_2;
        break;
      case -1:
        us = steer_center_us - deflect_1;
        break;
      case 0:
        us = steer_center_us;
        break;
      case 1:
        us = steer_center_us + deflect_1;
        break;
      case 2:
        us = steer_center_us + deflect_2;
        break;
      case 20:
        us = steer_center_us + deflect_3;
        break;
    }
    steerServo.writeMicroseconds(us);

    // if (ir_array_values[4] == 1) logIR();

    delay(1);
  }
  _log("doSimpleLineFollow end");
}

void doSimpleLineFollow2() {
  go_off = -30;
  _log("doSimpleLineFollow2");
  int init_pos = distanceTranvelled_cm;
  int target = 999999999;
  while (distanceTranvelled_cm < target) {
    if (target == 999999999 && cross_count == 2) {
      target = distanceTranvelled_cm + 15;
    }
    int dir = getDir();
    int us = steer_center_us;
    switch (dir) {
      case -20:
        us = steer_center_us - deflect_3;
        break;
      case -2:
        us = steer_center_us - deflect_2;
        break;
      case -1:
        us = steer_center_us - deflect_1;
        break;
      case 0:
        us = steer_center_us;
        break;
      case 1:
        us = steer_center_us + deflect_1;
        break;
      case 2:
        us = steer_center_us + deflect_2;
        break;
      case 20:
        us = steer_center_us + deflect_3;
        break;
    }
    steerServo.writeMicroseconds(us);

    // if (ir_array_values[4] == 1) logIR();

    delay(1);
  }
  _log("doSimpleLineFollow2 end");
}

void doEnterLineFollow_1(double travelDis_cm) {
  // _log("doEnterLineFollow_1");
  steerServo.writeMicroseconds(steer_center_us);
  while (running) {
    if (hasLine()) {
      // _log("hasLine");
      delay(100);
      cross_count = 0;
      steerServo.writeMicroseconds(steer_center_us - 500);
      delay(475);
      // logIR();
      doSimpleLineFollow(travelDis_cm);
      // int init_pos = distanceTranvelled_cm;
      // steerServo.writeMicroseconds(steer_center_us);
      // while (distanceTranvelled_cm - init_pos < travelDis_cm) {
      //   delay(1);
      // }

      gotoNextSegment();

      // doBrake();
      // running = false;
      return;
    }
    delay(1);
  }
}

void doEnterLineFollow_2(double travelDis_cm) {
  // _log("doEnterLineFollow_2");
  steerServo.writeMicroseconds(steer_center_us);
  while (running) {
    if (hasLine()) {
      // _log("hasLine");
      delay(50);
      cross_count = 0;
      steerServo.writeMicroseconds(steer_center_us + 500);
      delay(550);
      doSimpleLineFollow2();
      // int init_pos = distanceTranvelled_cm;
      // steerServo.writeMicroseconds(steer_center_us);
      // while (distanceTranvelled_cm - init_pos < travelDis_cm) {
      //   delay(1);
      // }

      gotoNextSegment();

      // doBrake();
      // running = false;
      return;
    }
    delay(1);
  }
}

void doEnterLineFollow_3() {
  // _log("doEnterLineFollow_1");
  steerServo.writeMicroseconds(steer_center_us);
  while (running) {
    if (true) {
      // _log("hasLine");
      cross_count = 0;
      steerServo.writeMicroseconds(steer_center_us - 500);
      delay(500 + 150);
      steerServo.writeMicroseconds(steer_center_us);
      delay(100);
      // logIR();
      doSimpleLineFollow(115 + 15 + 15);
      // int init_pos = distanceTranvelled_cm;
      // steerServo.writeMicroseconds(steer_center_us);
      // while (distanceTranvelled_cm - init_pos < travelDis_cm) {
      //   delay(1);
      // }

      gotoNextSegment();

      // doBrake();
      // running = false;
      return;
    }
    delay(1);
  }
}

// ============
// Basic Control Functions
// ============

void stopServos() {
  propellerServo.writeMicroseconds(1000);
  steerServo.writeMicroseconds(steer_center_us);  // 1430, +-500 = 46.5 deg
  brakeServo.writeMicroseconds(1500);
}

void launchPropeller() {
  launchPropeller(DEFAULT_propeller_us);
}
void launchPropeller(int continuous_us) {
  if (disablePropeller) return;
  if (!running) return;
  propellerServo.writeMicroseconds(OVERCOME_STATIC_propeller_us);
  delay(500);
  propellerServo.writeMicroseconds(continuous_us);
}

void resetDisplacement() {
  encoderCount = 0;
  distanceTranvelled_cm = 0;

  currentSegmentIndex = 0;
  seg_offset = 0;
}

void doBrake() {
  //ref stopServos
  propellerServo.writeMicroseconds(1000);
  steerServo.writeMicroseconds(steer_center_us);
  brakeServo.writeMicroseconds(BRAKE_us);
  delay(1000);
  stopServos();
  delay(3000);
}

// ============
// setup() and loop()
// ============

void setup() {
  Serial.begin(115200);

  // setup servos, encoder, ir array
  propellerServo.attach(PIN_fan, 500, 2500);
  steerServo.attach(PIN_steer, 500, 2500);
  brakeServo.attach(PIN_brake, 500, 2500);
  stopServos();

  pinMode(PIN_encoder, INPUT);
  for (int i = 0; i < ir_array_count; i++) {
    pinMode(PIN_ir_array[i], INPUT);
  }

  setup_on_off_button();

#ifdef ENABLE_BLE
  bleSerial.begin("ESP32-ble-js");
#endif

  xTaskCreate(vTaskSensorLoop, "vTaskSensorLoop", TASK_STACK_SIZE, NULL, 1, NULL);
  // xTaskCreate(vTaskStatusLogger, "vTaskStatusLogger", 5000, NULL, 1, NULL);

  waitForStart();
}

void loop() {
  if (running) {
    handleRouteTest();
  }
  delay(1);
}
