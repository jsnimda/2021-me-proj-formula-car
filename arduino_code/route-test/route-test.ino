
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
  // launchPropeller();
  resetDisplacement();
  running = true;
  launchPropeller();
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
  Straight,
  Left,
  Left_ms_1,
  Left_ms_2,
  Right,
  Stop,
  Brake,
  Brake_And_Go,
  Brake_And_Stop,
  EnterLineFollow_1,
  EnterLineFollow_2,
  EnterLineFollow_3,
  UTurn,
  UTurnEnd,
};

double segments[] = {
    30,               //Straight
    25.934,           //Right
    42.1 - 10,        //Straight
    45.305 - 15 + 5,  //EnterLineFollow_1
    0,                //Left ms
    10,               //Straight
    38.401 + 15,      //EnterLineFollow_2
    0,                //Brake_And_Go
    2 + 6,            // Straight
    110,              //UTurn
    0,                //UTurnEnd
    0,                //Left_ms_2
    0,                //EnterLineFollow_3
    0};               //Brake
Movement segMovements[] = {
    Straight,
    Right,
    Straight,
    EnterLineFollow_1,
    Left_ms_1,
    Straight,
    EnterLineFollow_2,
    Brake_And_Go,
    Straight,
    UTurn,
    UTurnEnd,
    Left_ms_2,
    EnterLineFollow_3,
    Brake_And_Stop,
    Brake};

int currentSegmentIndex = 0;
double seg_offset = 0;

void gotoNextSegment() {
  seg_offset = distanceTranvelled_cm;
  currentSegmentIndex++;
  handleMovement();
}

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
    case Left_ms_1:
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
    case Left_ms_2:
      steerServo.writeMicroseconds(steer_center_us - 550);  // 1430, +-500 = 46.5 deg
      delay(550);
      // for (int i = 0; i < 300; i++) {
      //   delay(1);
      //   if (hasLine()) break;
      // }
      gotoNextSegment();
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
      if (running) launchPropeller(FASTER_propeller_us);
      break;
    case UTurn:
      steerServo.writeMicroseconds(steer_center_us + 550);  // 1430, +-500 = 46.5 deg
      break;
    case UTurnEnd:
      if (disablePropeller) return;
      if (running) propellerServo.writeMicroseconds(DEFAULT_propeller_us);
      break;
    case Brake_And_Stop:
      doBrake();
      running = false;
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
// Main
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

  // setup on off button
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
