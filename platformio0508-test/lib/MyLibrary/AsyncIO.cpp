#include "AsyncIO.h"

#define TSK_SOCKET_TX_PRIORITY 5
#define TSK_SOCKET_RX_PRIORITY 5
#define TSK_SERIAL_TX_PRIORITY 4
#define TSK_SERIAL_RX_PRIORITY 4
#define TSK_SOCKET_TX_STACK 4096
#define TSK_SOCKET_RX_STACK 4096
#define TSK_SERIAL_TX_STACK 4096
#define TSK_SERIAL_RX_STACK 4096
#define SERIAL_TX_DELAY 1
// 115200 bps * 20 ms = 288 bytes < RxBufferSize
#define SERIAL_RX_DELAY 20

// TODO: reduce the use of heap (use static buffer instead of new)

// ============
// Extern
// ============

AsyncHardwareSerial AsyncSerial(Serial);

// ============
// Main
// ============

#define TX_HALF_FULL_BIT BIT0
#define ACK_TX_HALF_FULL_BIT BIT1

namespace {

// // handle data in buffer that is being to send
// TaskHandle_t taskAsyncTx_handle;
// // handle onRead redirected by another task
// TaskHandle_t taskAsyncRx_handle;
// // handle Serial.available()
// TaskHandle_t taskAsyncSerialRx_handle;

// QueueHandle_t queueTx;

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
    if (buf.length()) {
      pSerial->lockBuf();  // lock mux
      size_t len = buf.length();
      uint8_t* data = new uint8_t[len];
      buf.read(data, len);
      pSerial->unlockBuf();  // unlock mux

      serial.write(data, len);
      delete[] data;
    }
    pSerial->notifying = false;
    xEventGroupClearBits(pSerial->_tsk_event, TX_HALF_FULL_BIT);
    xEventGroupWaitBits(
        pSerial->_tsk_event, /* The event group being tested. */
        TX_HALF_FULL_BIT,    /* The bits within the event group to wait for. */
        pdTRUE,              /* BIT_0 & BIT_4 should be cleared before returning. */
        pdFALSE,             /* Don't wait for both bits, either bit will do. */
        SERIAL_TX_DELAY);    /* Wait a maximum of 100ms for either bit to be set. */
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

AsyncSocketSerial::AsyncSocketSerial(int port) : port(port), server(port) {
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
  AsyncStream_T::onData(cb);
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

AsyncHardwareSerial::AsyncHardwareSerial(HardwareSerial& s) : _serial(&s) {
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
  AsyncStream_T::onData(onData);
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
  xEventGroupSetBits(_tsk_event, TX_HALF_FULL_BIT);
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
  // if (!queueTx) {
  //   queueTx = xQueueCreate(32, sizeof(QueueTxData));
  // }
  // if (!taskAsyncTx_handle) {
  //   xTaskCreatePinnedToCore(asyncTxTask, "asyncTxTask", 8192, NULL, ASYNC_TX_TASK_PRIORITY, &taskAsyncTx_handle, 0);
  // }
  // if (!taskAsyncRx_handle) {
  //   xTaskCreatePinnedToCore(asyncRxTask, "asyncRxTask", 8192, NULL, ASYNC_RX_TASK_PRIORITY, &taskAsyncRx_handle, 0);
  // }
}

void loga(String s) {
  AsyncSerial.lockedPrint(s);
}