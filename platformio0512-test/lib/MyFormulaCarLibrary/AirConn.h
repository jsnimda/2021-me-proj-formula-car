
// Optional library for OTA update, WiFi serial monitor, etc

#ifndef AirConn_h
#define AirConn_h

/*
Core 0: WiFi, logging, etc
Core 1: Controller, Sensors, loop(), etc
*/

/*

  TODO: handle buffer overflow
    (allocate more memory or block until buffer is available)

*/

#ifndef USE_OTA
#define USE_OTA 1
#endif
#ifndef USE_WIFI
#define USE_WIFI 1
#endif
#include "myconfig.h"

/*

  Dependency
  |-- <myconfig.h>
  |-- <myssid.h>
  |-- <Common>
  |   |-- <myconfig.h>
  |   |-- <DebugPerf>
  |-- <DebugPerf>
  |   |-- <Common>
  |
  |-- <CircularBuffer>
  |   |-- <Common>
  |-- <WiFiConnection>
  |   |-- <myssid.h>
  |   |-- <Common>
  |-- <AsyncIO>
  |   |-- <Common>
  |   |-- <CircularBuffer>
  |-- <LineCommand>
  |   |-- ...

*/

#include <Arduino.h>
#include <ArduinoOTA.h>

#include "MyLib/Common.h"
//
#include "MyLib/AsyncIO.h"
#include "MyLib/LineCommand.h"
#include "MyLib/WiFiConnection.h"

// ============
// Functions
// ============
inline void airConnSetup() {
#if USE_OTA
  // todo
#endif
#if USE_WIFI
  wifiConnectionSetup();
#endif
  asyncIOSetup();
}

#endif  // AirConn_h
