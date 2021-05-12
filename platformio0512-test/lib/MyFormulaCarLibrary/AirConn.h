
// Optional library for OTA update, WiFi serial monitor, etc

#ifndef AirConn_h
#define AirConn_h

/*
Core 0: WiFi, logging, etc
Core 1: Controller, Sensors, loop(), etc
*/

#include <Arduino.h>
#include <ArduinoOTA.h>

#include "MyLib/Common.h"
//
#include "MyLib/AsyncIO.h"
#include "MyLib/DebugPerf.h"
#include "MyLib/WiFiConnection.h"

// ============
// Functions
// ============
inline void airConnSetup() {
}

#endif  // AirConn_h
