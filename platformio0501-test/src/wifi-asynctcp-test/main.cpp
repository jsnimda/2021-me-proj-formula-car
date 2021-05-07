/*

  Conclusion:
    no need special reconnection on AsyncServer
    once wifi is up, server is up

  Conclusion 2:
    telnet -> esp32
      maximum data rate: each onData() have len 1436
    client info:
      MSS: 1436
      Ack timeout: 5000 ms
      tcp_sndbuf: 5744

*/

#include <Arduino.h>
#include <AsyncTCP.h>
#include <WiFi.h>

#include "WiFiConnection.h"

AsyncServer server(23);      // notice!! args is different
AsyncClient* pServerClient;  // only allow one client at a time

#define LARGE_BUFFER_SIZE 8192
uint8_t large_buffer[LARGE_BUFFER_SIZE];

void ser_setup();
void client_setup();

void setup() {
  Serial.begin(115200);
  wifiConnectionSetup();

  while (!WiFi.isConnected()) {
    delay(1000);
  }

  ser_setup();

  server.begin();
  server.setNoDelay(true);

  Serial.print("Ready! Use 'telnet ");
  Serial.print(WiFi.localIP());
  Serial.println(" 23' to connect");
}

void dump_client_info() {
  if (!pServerClient) return;
  AsyncClient& serverClient = *pServerClient;
  Serial.printf("Client info:\r\n");
  Serial.printf("  space(): %d\r\n", serverClient.space());
  Serial.printf("  getMss(): %d\r\n", serverClient.getMss());
  Serial.printf("  getRxTimeout() s: %d\r\n", serverClient.getRxTimeout());
  Serial.printf("  getAckTimeout() ms: %d\r\n", serverClient.getAckTimeout());
  Serial.printf("  getNoDelay(): %d\r\n", serverClient.getNoDelay());
  Serial.printf("  stateToString(): %s\r\n", serverClient.stateToString());
}

void ser_setup() {
  //check if there are any new clients
  server.onClient([](void*, AsyncClient* client) {
    Serial.println("server.onClient!!");
    // server.hasClient()
    if (pServerClient) {
      if (pServerClient->connected())
        pServerClient->write("\r\nDevice is connected on another client.\r\n");
      delete pServerClient;
    }
    pServerClient = client;
    AsyncClient& serverClient = *pServerClient;
    client_setup();
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
  },
                  NULL);
}

void client_setup() {
  if (!pServerClient) return;
  AsyncClient& serverClient = *pServerClient;
  //check clients for data
  serverClient.onData([](void*, AsyncClient*, void* data, size_t len) {
    // Serial.write((uint8_t*)data, len);
    Serial.print("len: ");
    Serial.println(len);
  },
                      NULL);
}

unsigned long long j = 0;

// warning: async_tcp can cause abort
// task_wdt: Task watchdog got triggered. The following tasks did not reset the watchdog in time:
// task_wdt:  - IDLE0 (CPU 0)
void loop() {
  // ref: wifi-server-reconn-test
  if (WiFi.isConnected()) {
    if (!pServerClient) return;
    AsyncClient& serverClient = *pServerClient;
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
          serverClient.write((char*)large_buffer, total_len);
          total_len = 0;
        }
      }
      if (total_len) {
        serverClient.write((char*)large_buffer, total_len);
      }
    }

    if (millis() - j > 10 * 1000) {
      j = millis();
      dump_client_info();
    }
  } else {
    Serial.println("WiFi not connected!");
    if (pServerClient && pServerClient->connected()) pServerClient->stop();
    delay(1000);
  }
}