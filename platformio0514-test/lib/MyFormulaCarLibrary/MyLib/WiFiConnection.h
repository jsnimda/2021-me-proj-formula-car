/*

  Logic:
    by default, WiFi.begin(...) will auto reconnect if ap not found
      (because of WiFi.getAutoReconnect() and line 409 in WiFiGeneric.cpp)

    Every time WiFi.begin(...) will always call esp_wifi_connect() (normally)

    ref: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/wifi.html

    once esp_wifi_connect() is called:
      if connection fail always raise -> SYSTEM_EVENT_STA_DISCONNECTED
      otherwise (connect success):
        raise -> SYSTEM_EVENT_STA_CONNECTED (WL_IDLE_STATUS)
        and then -> SYSTEM_EVENT_STA_GOT_IP (WL_CONNECTED)

  WiFiMulti:
    scan before begin(...), if no ssid found, won't call begin(...)
    there are no api to know if there are bestNetwork.ssid
    we can only increase connectTimeout to 10 000 for stability
    (SYSTEM_EVENT_SCAN_DONE is rasied during run())

  init status (as when wifi init): WL_DISCONNECTED

*/

#ifndef WiFiConnection_h
#define WiFiConnection_h

#include "myssid.h"
//
#include "Common.h"

// TODO: support WiFiMulti
// #define USE_WIFI_MULTI 1

// add this in setup():
void wifiConnectionSetup();

#endif  // WiFiConnection_h
