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

#include <WiFi.h>

#define mySSID "JS"
#define myPASSWORD "81288128"

void eventA(WiFiEvent_t event) {  // delay() will block event thread
  Serial.flush();
  Serial.printf("event a: %d ...\r\n", event);
  Serial.printf("stat: %d\r\n", WiFi.status());
  delay(5000);
  Serial.printf("event a: %d end\r\n", event);
}

void eventB(WiFiEvent_t event) {
  Serial.flush();
  Serial.printf("event b: %d ...\r\n", event);
  Serial.printf("stat: %d\r\n", WiFi.status());
  delay(5000);
  Serial.printf("event b: %d end\r\n", event);
}

void setup() {
  Serial.begin(115200);
  delay(10);

  WiFi.onEvent(eventA);  // WiFi.status() is set before event call
  WiFi.onEvent(eventB);
}

void loop() {
  if (!WiFi.isConnected()) {
    Serial.flush();
    Serial.println("WiFi.begin...");

    WiFi.begin(mySSID, myPASSWORD);

    Serial.flush();
    Serial.println("WiFi.begin end");
  }

  delay(15000);
}