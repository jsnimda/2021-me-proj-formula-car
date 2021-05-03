
// Optional library for OTA update, WiFi serial monitor, etc

#ifndef AirConn_h
#define AirConn_h

/*
Core 0: WiFi, logging, etc
Core 1: Controller, Sensors, loop(), etc
*/

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <WiFiMulti.h>

#define WIFI_MTU_SIZE 1500

#include "CircularBuffer.h"
#include "myssid.h"

typedef CircularBuffer_T<WIFI_MTU_SIZE * 2> WirelessSerialBuffer_t;
extern WirelessSerialBuffer_t WirelessSerialBuffer;




// ============
// Functions
// ============
void airConnSetup() {

}




#endif // AirConn_h
