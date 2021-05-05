#include "WiFiConnection.h"

#if defined(USE_WIFI_MULTI) && USE_WIFI_MULTI
#define CPP_ENABLE_WIFI_MULTI
#endif

// ============
// Includes
// ============

#include <ESPmDNS.h>
#include <HardwareSerial.h>
#include <WiFi.h>

#ifdef CPP_ENABLE_WIFI_MULTI
#include <WiFiMulti.h>
static WiFiMulti wifiMulti;
#endif

// ============
// Main
// ============

#define ConnectionResult_BIT BIT0
// #define Connecting_BIT BIT8
// #define Connected_BIT BIT9
// #define Disconnected_BIT BIT10

namespace {

bool firstConnect = true;
EventGroupHandle_t wifiConnectionEventGroup;
TaskHandle_t wifiConnectionTaskHandle = NULL;
portMUX_TYPE wifiConnectionTaskMux = portMUX_INITIALIZER_UNLOCKED;

void wifiConnectionTask(void *args) {
  if (firstConnect) {
    firstConnect = false;
  } else {
    Serial.flush();
    Serial.println("WiFi disconnected. Trying to reconnect...");
  }
  bool flag = true;
  while (flag) {
    xEventGroupClearBits(wifiConnectionEventGroup, 0xFF);

    Serial.flush();
    Serial.println("Connecting Wifi...");

    WiFi._setStatus(WL_DISCONNECTED);  // the init status as STA_START (wifi init)
    WiFi.begin(mySSID, myPASSWORD);

    xEventGroupWaitBits(
        wifiConnectionEventGroup,        /* The event group being tested. */
        ConnectionResult_BIT,            /* The bits within the event group to wait for. */
        pdTRUE,                          /* BIT_0 & BIT_4 should be cleared before returning. */
        pdFALSE,                         /* Don't wait for both bits, either bit will do. */
        60 * 1000 / portTICK_PERIOD_MS); /* Wait a maximum of 100ms for either bit to be set. */
    // or portMAX_DELAY

    portENTER_CRITICAL(&wifiConnectionTaskMux);
    if (WiFi.status() == WL_CONNECTED) {
      wifiConnectionTaskHandle = NULL;
      flag = false;
    } else {
      Serial.flush();
      Serial.println("Connection failed. Trying to reconnect...");
    }
    portEXIT_CRITICAL(&wifiConnectionTaskMux);
  }
  // handle already set to NULL, delete self
  Serial.flush();
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  vTaskDelete(NULL);
}

void asyncWifiConnect() {
  portENTER_CRITICAL(&wifiConnectionTaskMux);
  if (wifiConnectionTaskHandle == NULL) {
    xTaskCreatePinnedToCore(wifiConnectionTask, "wifiConnectionTask", 8192, NULL, 10, &wifiConnectionTaskHandle, 0);
  } else {
    xEventGroupSetBits(wifiConnectionEventGroup, ConnectionResult_BIT);
  }
  portEXIT_CRITICAL(&wifiConnectionTaskMux);
}

void onWifiEvent(WiFiEvent_t event) {
  if (event == SYSTEM_EVENT_STA_DISCONNECTED) {
    // connection fail or disconnected
    asyncWifiConnect();
  } else if (WiFi.status() == WL_CONNECTED) {
    xEventGroupSetBits(wifiConnectionEventGroup, ConnectionResult_BIT);
  }
}

}  // namespace

// ============
// Export
// ============

void wifiConnectionSetup() {
#ifdef CPP_ENABLE_WIFI_MULTI
  wifiMulti.addAP(mySSID, myPASSWORD);
#if defined(mySSID2) && defined(myPASSWORD2)
  wifiMulti.addAP(mySSID2, myPASSWORD2);
#endif
#if defined(mySSID3) && defined(myPASSWORD3)
  wifiMulti.addAP(mySSID3, myPASSWORD3);
#endif
#endif

  if (!wifiConnectionEventGroup) {
    wifiConnectionEventGroup = xEventGroupCreate();
  }

  MDNS.begin(myHOSTNAME);
  MDNS.enableArduino();

  WiFi.mode(WIFI_STA);
  // ref: https://www.mischianti.org/2021/03/06/esp32-practical-power-saving-manage-wifi-and-cpu-1/
  // Power consumption 80mA -> 145mA
  WiFi.setSleep(false);  // reduce ping (but increase power usage)
  WiFi.setHostname(myHOSTNAME);
  WiFi.setAutoReconnect(false);  // disable default auto connect
  WiFi.onEvent(onWifiEvent);
  asyncWifiConnect();
}