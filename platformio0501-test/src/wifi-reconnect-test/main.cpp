/*

  Conclusion: 

  - just WiFi.begin(...) after accidentally disconnected
      can rarely reconnect (only success after one minute) after disconnect
      stat is always WL_NO_SSID_AVAIL

  - do WiFi.disconnect(true, ...)
      disable STA and re-enable, can work!

  - do WiFi._setStatus(WL_IDLE_STATUS);
      (???) can work!!
      set to WL_DISCONNECTED also works!!
      because of waitForConnectResult() ???

  Found the cause now:
    Calling WiFi.begin() too frequently (less than 2 seconds) causing
      esp_wifi_connect recalled before any wifi result event occurs
      -> status won't update -> keep looping and no connection

*/

#include <Arduino.h>
#include <WiFi.h>

#include "WiFiConnection.h"

String statToStr(int stat) {
  if (stat == 255) return "WL_NO_SHIELD";
  if (stat >= 0 && stat <= 6)
    return ((const char*[]){"WL_IDLE_STATUS",
                            "WL_NO_SSID_AVAIL",
                            "WL_SCAN_COMPLETED",
                            "WL_CONNECTED",
                            "WL_CONNECT_FAILED",
                            "WL_CONNECTION_LOST",
                            "WL_DISCONNECTED"})[stat];
  return "Unknown";
}

void printWifiStat() {
  int stat = WiFi.status();
  Serial.flush();
  Serial.print("stat: ");
  Serial.print(stat);
  Serial.print(" - ");
  Serial.print(statToStr(stat));
  Serial.println();
}

void winfo() {
  Serial.flush();
  Serial.println("== printDiag ==");
  WiFi.printDiag(Serial);
  Serial.println("===============");
}

void setup() {
  Serial.begin(115200);
  delay(10);

  log_v("test v");
  log_d("test d");
  log_i("test i");
  log_w("test w");
  log_e("test e");
  
  printWifiStat();
  Serial.println("wifiConnectionSetup...");
  wifiConnectionSetup();
  Serial.println("wifiConnectionSetup end");
  printWifiStat();
}

void loop() {
  delay(1000);
  printWifiStat();
}