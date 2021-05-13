#include "WiFiConnection.h"

#if defined(USE_WIFI_MULTI) && USE_WIFI_MULTI
#define CPP_ENABLE_WIFI_MULTI
#endif

// ============
// Includes
// ============

#include <ESPmDNS.h>
#include <WiFi.h>

#ifdef CPP_ENABLE_WIFI_MULTI
#include <WiFiMulti.h>
static WiFiMulti wifiMulti;
#endif

// ============
// Main
// ============

namespace {

void log(String s) {
  loga(s + "\r\n");
}

bool firstConnect = true;
// EventGroupHandle_t wifiConnectionEventGroup;
TaskHandle_t wifiConnectionTaskHandle = NULL;
portMUX_TYPE wifiConnectionTaskMux = portMUX_INITIALIZER_UNLOCKED;

void wifiConnectionTask(void* args) {
  if (firstConnect) {
    firstConnect = false;
    log("Connecting Wifi...");
  } else {
    log("WiFi disconnected. Trying to reconnect...");
  }
  bool flag = true;
  while (flag) {
    WiFi._setStatus(WL_DISCONNECTED);  // the init status as STA_START (wifi init)
    WiFi.begin(mySSID, myPASSWORD);

    ulTaskNotifyTake(pdFALSE, 60 * 1000 / portTICK_PERIOD_MS);

    portENTER_CRITICAL(&wifiConnectionTaskMux);
    if (WiFi.status() == WL_CONNECTED) {
      wifiConnectionTaskHandle = NULL;
      flag = false;
    } else {
      log("WiFi connection failed. Trying to reconnect...");
    }
    portEXIT_CRITICAL(&wifiConnectionTaskMux);
  }
  // handle already set to NULL, delete self
  String s;
  s = s + "\r\n" +
      "WiFi connected\r\n" +
      stringf("IP address: %s\r\n", WiFi.localIP().toString().c_str());
  loga(s);
  vTaskDelete(NULL);
}

void asyncWifiConnect() {
  portENTER_CRITICAL(&wifiConnectionTaskMux);
  if (wifiConnectionTaskHandle == NULL) {
    xTaskCreatePinnedToCore(wifiConnectionTask, "wifiConnectionTask", 8192, NULL, 10, &wifiConnectionTaskHandle, 0);
  } else {
    xTaskNotifyGive(wifiConnectionTaskHandle);
  }
  portEXIT_CRITICAL(&wifiConnectionTaskMux);
}

void onWifiEvent(WiFiEvent_t event) {
  if (event == SYSTEM_EVENT_STA_DISCONNECTED) {
    // connection fail or disconnected
    asyncWifiConnect();
  } else if (WiFi.status() == WL_CONNECTED) {
    portENTER_CRITICAL(&wifiConnectionTaskMux);
    if (wifiConnectionTaskHandle) xTaskNotifyGive(wifiConnectionTaskHandle);
    portEXIT_CRITICAL(&wifiConnectionTaskMux);
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