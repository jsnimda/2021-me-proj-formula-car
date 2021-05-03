
// Optional library for OTA update, WiFi serial monitor, etc

#ifndef AirDeploy_h
#define AirDeploy_h

/*
Core 0: WiFi, logging, etc
Core 1: Controller, Sensors, loop(), etc
*/

#define WIFI_MTU_SIZE 1500

#include "CircularBuffer.h"
#include <WiFi.h>

typedef CircularBuffer_T<WIFI_MTU_SIZE * 2> WirelessSerialBuffer_t;
WirelessSerialBuffer_t WirelessSerialBuffer;




// ============
// Functions
// ============
// void airconnSetup();




#endif  // AirDeploy_h