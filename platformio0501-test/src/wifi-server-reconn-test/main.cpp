/*

  Conclusion:
    no need special reconnection on WiFiServer
    once wifi is up, server is up

*/

#include <Arduino.h>
#include <WiFi.h>

#include "WiFiConnection.h"

WiFiServer server(23, /* max_clients */ 1);
WiFiClient serverClient;  // only allow one client at a time

#define LARGE_BUFFER_SIZE 8192
uint8_t large_buffer[LARGE_BUFFER_SIZE];

void setup() {
  Serial.begin(115200);
  wifiConnectionSetup();

  while (!WiFi.isConnected()) {
    delay(1000);
  }

  server.begin();
  server.setNoDelay(true);

  Serial.print("Ready! Use 'telnet ");
  Serial.print(WiFi.localIP());
  Serial.println(" 23' to connect");
}

void loop() {
  // ref: WiFiTelnetToSerial.ino
  if (WiFi.isConnected()) {
    //check if there are any new clients
    if (server.hasClient()) {
      if (serverClient.connected()) {
        serverClient.write("\r\nDevice is connected on another client.\r\n");
        serverClient.stop();
      }
      serverClient = server.available();
      if (!serverClient.connected()) {
        Serial.println("available broken");
      } else {
        Serial.print("New client: ");
        Serial.println(serverClient.remoteIP());
        Serial.printf("  %s:%d -> %s:%d\r\n",
                      serverClient.remoteIP().toString().c_str(),
                      serverClient.remotePort(),
                      serverClient.localIP().toString().c_str(),
                      serverClient.localPort());
      }
    }

    //check clients for data
    if (serverClient.connected() && serverClient.available()) {
      int len;
      while ((len = serverClient.available())) {
        len = min(len, LARGE_BUFFER_SIZE);
        // use native read(...) instead of readBytes
        int res = serverClient.read(large_buffer, len);
        if (res > 0) Serial.write(large_buffer, res);
        // todo else log read error
        // typical error: [E][WiFiClient.cpp:439] read(): fail on fd 55, errno: 113, "Software caused connection abort"
      }
    }

    //check UART for data
    // do not consume serial if client is not connected
    if (serverClient.connected() && Serial.available()) {
      int total_len = 0;
      int len;
      while ((len = Serial.available())) {
        // use native read(...) instead of readBytes
        len = min(len, LARGE_BUFFER_SIZE - total_len);
        int res = Serial.read(large_buffer + total_len, len);
        if (res > 0) total_len += res;
        if (total_len >= LARGE_BUFFER_SIZE) {
          serverClient.write(large_buffer, total_len);
          total_len = 0;
        }
      }
      if (total_len) {
        serverClient.write(large_buffer, total_len);
      }
    }
  } else {
    Serial.println("WiFi not connected!");
    if (serverClient.connected()) serverClient.stop();
    delay(1000);
  }
}