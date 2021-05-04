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

void printStat(int stat) {
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

  winfo();
  printStat(WiFi.status());
  delay(1000);
  WiFi.begin(mySSID, myPASSWORD);
  Serial.flush();
  Serial.println("Connecting Wifi...");
  winfo();
  printStat(WiFi.status());
  delay(1000);
  winfo();

  Serial.println("Wait for wifi result...");
  WiFi.waitForConnectResult();
  Serial.flush();
  Serial.println("Wait end");
}

void loop() {
  delay(1000);
  if (!WiFi.isConnected()) winfo();
  Serial.flush();
  Serial.print("wifi connected: ");
  Serial.println(WiFi.isConnected());
  if (!WiFi.isConnected()) {
    Serial.flush();
    printStat(WiFi.status());
    Serial.println("WiFi not connected!");

    // WiFi.disconnect(true, false);
    WiFi._setStatus(WL_CONNECTION_LOST);

    WiFi.begin(mySSID, myPASSWORD);
    Serial.flush();
    Serial.println("Connecting Wifi...");

    Serial.println("Wait for wifi result...");
    WiFi.waitForConnectResult();
    delay(2000);
    Serial.flush();
    Serial.println("Wait end");

    printStat(WiFi.status());
    winfo();
  } else {
    delay(10 * 1000);
  }
}