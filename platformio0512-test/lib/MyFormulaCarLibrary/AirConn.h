
// Optional library for OTA update, WiFi serial monitor, etc

#ifndef AirConn_h
#define AirConn_h

/*
Core 0: WiFi, logging, etc
Core 1: Controller, Sensors, loop(), etc
*/

#ifndef USE_OTA
#define USE_OTA 1
#endif
#ifndef USE_WIFI
#define USE_WIFI 1
#endif
#ifndef CONFIG_USE_ASYNC_SERIAL
#define CONFIG_USE_ASYNC_SERIAL 0
#endif
#ifndef CONFIG_DEBUG_PERF
#define CONFIG_DEBUG_PERF 1
#endif

/*

  Dependency
  |-- <myssid.h>
  |-- <Common>
  |-- <CircularBuffer>
  |   |-- <Common>
  |-- <DebugPerf>
  |   |-- <Common>
  |-- <WiFiConnection>
  |   |-- <myssid.h>
  |   |-- <Common>
  |-- <AsyncIO>
  |   |-- <Common>
  |   |-- <CircularBuffer>

*/

#include <Arduino.h>
#include <ArduinoOTA.h>

#include "MyLib/Common.h"
#include "MyLib/DebugPerf.h"
//
#include "MyLib/AsyncIO.h"
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
