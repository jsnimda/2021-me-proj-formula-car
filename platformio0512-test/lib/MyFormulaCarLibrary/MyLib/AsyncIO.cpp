#include "AsyncIO.h"

#define TSK_SOCKET_TX_PRIORITY 5
#define TSK_SOCKET_RX_PRIORITY 5
#define TSK_SERIAL_TX_PRIORITY 4
#define TSK_SERIAL_RX_PRIORITY 4
#define TSK_SOCKET_TX_STACK 4096
#define TSK_SOCKET_RX_STACK 4096
#define TSK_SERIAL_TX_STACK 4096
#define TSK_SERIAL_RX_STACK 4096
#define SERIAL_TX_DELAY 20
// 115200 bps * 20 ms = 288 bytes < RxBufferSize
#define SERIAL_RX_DELAY 20

// ============
// Extern
// ============

#if CONFIG_USE_ASYNC_SERIAL
AsyncHardwareSerial AsyncSerial(Serial);
#endif

// ============
// Main
// ============

#define TX_HALF_FULL_BIT BIT0
#define ACK_TX_HALF_FULL_BIT BIT1

namespace {

void socketTskTx(void* ps) {
  // AsyncSocketSerial* pSocket = (AsyncSocketSerial*)ps;
  for (;;) {
    // TODO
    delay(3600 * 1000);
  }
}
void socketTskRx(void* ps) {
  // AsyncSocketSerial* pSocket = (AsyncSocketSerial*)ps;
  for (;;) {
    // TODO
    delay(3600 * 1000);
  }
}
void serialTskTx(void* ps) {
  AsyncHardwareSerial* pSerial = (AsyncHardwareSerial*)ps;
  auto& buf = pSerial->_buf;
  // for any buf operations should lock the buf mux (lockBuf/unlockBuf)
  auto& serial = *(pSerial->_serial);
  for (;;) {
    while (buf.length()) {
      pSerial->lockBuf();  // lock mux
      size_t len = buf.length();
      uint8_t* data = new uint8_t[len];
      buf.read(data, len);
      pSerial->unlockBuf();  // unlock mux

      serial.write(data, len);
      delete[] data;
    }
    ulTaskNotifyTake(pdFALSE, SERIAL_TX_DELAY / portTICK_PERIOD_MS);
  }
}
void serialTskRx(void* ps) {
  AsyncHardwareSerial* pSerial = (AsyncHardwareSerial*)ps;
  auto& serial = *(pSerial->_serial);
  for (;;) {
    if (pSerial->_onData_cb) {
      size_t len;
      if ((len = serial.available())) {
        uint8_t* data = new uint8_t[len];
        serial.read(data, len);
        pSerial->_onData_cb(data, len);
        delete[] data;
      }
    }
    delay(SERIAL_RX_DELAY);
  }
}

}  // namespace

// ============
// AsyncSocketSerial
// ============

AsyncSocketSerial::AsyncSocketSerial(int port) : AsyncStream(WIFI_MSS), port(port), server(port) {
  server.setNoDelay(true);

  server.onClient([&](void*, AsyncClient* pc) {
    if (!pc) return;
    AsyncClient* old_pClient = NULL;
    portENTER_CRITICAL(&_pClient_mux);
    if (pClient) {
      old_pClient = pClient;
    }
    pClient = pc;
    portEXIT_CRITICAL(&_pClient_mux);
    if (old_pClient) {
      if (old_pClient->connected()) {
        old_pClient->write("\r\nDevice is connected on another client.\r\n");
      }
      delete old_pClient;  // new from AsyncTCP.cpp:1297
    }

    _client_setup(pc);
    AsyncClient& serverClient = *pc;
    String s = stringf("New client: %s\r\n", serverClient.remoteIP().toString().c_str()) +
               stringf("  %s:%d -> %s:%d\r\n",
                       serverClient.remoteIP().toString().c_str(),
                       serverClient.remotePort(),
                       serverClient.localIP().toString().c_str(),
                       serverClient.localPort());
    loga(s);
  },
                  NULL);

  String tskName = stringf("TxPort%d", port);
  xTaskCreatePinnedToCore(
      socketTskTx,
      tskName.c_str(),
      TSK_SOCKET_TX_STACK,
      this,
      TSK_SOCKET_TX_PRIORITY,
      &(this->tskTx),
      0);
}
void AsyncSocketSerial::onData(AssDataHandler cb) {
  AsyncStream::onData(cb);
  if (!tskRx) {
    String tskName = stringf("RxPort%d", port);
    xTaskCreatePinnedToCore(
        socketTskRx,
        tskName.c_str(),
        TSK_SOCKET_RX_STACK,
        this,
        TSK_SOCKET_RX_PRIORITY,
        &(this->tskRx),
        0);
  }
}
void AsyncSocketSerial::notifyTx() {
  // TODO
}
void AsyncSocketSerial::_client_setup(AsyncClient* pc) {
  if (!pc) return;
  AsyncClient& serverClient = *pc;
  serverClient.onData([&](void*, AsyncClient*, void* data, size_t len) {
    if (_onData_cb) {
      _onData_cb((uint8_t*)data, len);
    }
  },
                      NULL);
  serverClient.onDisconnect([&](void*, AsyncClient* pc) {
    if (!pc) return;
    if (pClient == pc) {
      portENTER_CRITICAL(&_pClient_mux);
      if (pClient == pc) {
        pClient = NULL;
      }
      portEXIT_CRITICAL(&_pClient_mux);
    }
    delete pc;  // new from AsyncTCP.cpp:1297
  });
}
AsyncSocketSerial::~AsyncSocketSerial() {
  if (tskTx) vTaskDelete(tskTx);
  if (tskRx) vTaskDelete(tskRx);
  AsyncClient* old_pClient = NULL;
  portENTER_CRITICAL(&_pClient_mux);
  if (pClient) {
    old_pClient = pClient;
  }
  portEXIT_CRITICAL(&_pClient_mux);
  if (old_pClient) {
    if (old_pClient->connected()) {
      old_pClient->write("\r\nServer closed.\r\n");
    }
    delete old_pClient;  // new from AsyncTCP.cpp:1297
  }
}

// ============
// AsyncHardwareSerial
// ============

AsyncHardwareSerial::AsyncHardwareSerial(HardwareSerial& s) : AsyncStream(SERIAL_MSS), _serial(&s) {
  String tskName = "Tx";
  if (_serial == &Serial) {
    tskName += "Serial";
  } else if (_serial == &Serial1) {
    tskName += "Serial1";
  } else if (_serial == &Serial2) {
    tskName += "Serial2";
  }
  xTaskCreatePinnedToCore(
      serialTskTx,
      tskName.c_str(),
      TSK_SERIAL_TX_STACK,
      this,
      TSK_SERIAL_TX_PRIORITY,
      &(this->tskTx),
      0);
}

void AsyncHardwareSerial::onData(AssDataHandler onData) {
  AsyncStream::onData(onData);
  if (!tskRx) {
    String tskName = "Rx";
    if (_serial == &Serial) {
      tskName += "Serial";
    } else if (_serial == &Serial1) {
      tskName += "Serial1";
    } else if (_serial == &Serial2) {
      tskName += "Serial2";
    }
    xTaskCreatePinnedToCore(
        serialTskRx,
        tskName.c_str(),
        TSK_SERIAL_RX_STACK,
        this,
        TSK_SERIAL_RX_PRIORITY,
        &(this->tskRx),
        0);
  }
}

void AsyncHardwareSerial::notifyTx() {
  // ref: https://www.freertos.org/RTOS_Task_Notification_As_Binary_Semaphore.html
  xTaskNotifyGive(tskTx);
}

AsyncHardwareSerial::~AsyncHardwareSerial() {
  if (tskTx) vTaskDelete(tskTx);
  if (tskRx) vTaskDelete(tskRx);
}

// ============
// Export
// ============

void asyncIOSetup() {
  Serial.setRxBufferSize(1024);
}

void loga(String s) {
#if CONFIG_USE_ASYNC_SERIAL
  AsyncSerial.lockedPrint(s);
#else
  Serial.flush();
  Serial.print(s);
#endif
}
