#include "AsyncIO.h"

#define TSK_SOCKET_TX_PRIORITY 5
#define TSK_SOCKET_RX_PRIORITY 5
#define TSK_SERIAL_TX_PRIORITY 4
#define TSK_SERIAL_RX_PRIORITY 4
#define TSK_SOCKET_TX_STACK 4096
#define TSK_SOCKET_RX_STACK 4096
#define TSK_SERIAL_TX_STACK 4096
#define TSK_SERIAL_RX_STACK 4096
#define SOCKET_TX_DELAY 20
#define SOCKET_RX_DELAY (3600 * 1000)
#define SERIAL_TX_DELAY 20
// 115200 bps * 20 ms = 288 bytes < RxBufferSize
#define SERIAL_RX_DELAY 20

// ============
// Extern
// ============

#if CONFIG_USE_ASYNC_SERIAL
AsyncHardwareSerial AsyncSerial(Serial);
#endif
#if CONFIG_DEBUG_PERF
PerfData pd_wifiWrite("wifiWrite");
#else
#define perf_loc(...)
#define perf_start(loc)
#define perf_add(loc, perfData)
#define perf_end(...)
#endif

// ============
// Tasks
// ============

namespace {

void socketTskTx(void* ps) {
  // TODO: resolve race condition when client is deleted
  AsyncSocketSerial* pSocket = (AsyncSocketSerial*)ps;
  pSocket->_inc_del_counter();
  auto& buf = pSocket->_buf;
  perf_loc(_a);
  while (!pSocket->_del) {
    AsyncClient* pc = pSocket->pClient;
    if (pc) {
      while (pc == pSocket->pClient && !pSocket->_del) {
        while (buf.length()) {
          pSocket->lock();  // lock mux
          size_t len = min(buf.length(), (size_t)WIFI_MSS);
          uint8_t* data = new uint8_t[len];
          buf.read(data, len);
          pSocket->unlock();  // unlock mux

          perf_start(_a);
          pc->write((const char*)data, len);
          perf_end(_a, pd_wifiWrite);
          delete[] data;
        }
        ulTaskNotifyTake(pdFALSE, SOCKET_TX_DELAY / portTICK_PERIOD_MS);
      }
    }
    ulTaskNotifyTake(pdFALSE, SOCKET_TX_DELAY / portTICK_PERIOD_MS);
  }
  pSocket->_dec_del_counter();
  vTaskDelete(NULL);
}
void socketTskRx(void* ps) {
  AsyncSocketSerial* pSocket = (AsyncSocketSerial*)ps;
  pSocket->_inc_del_counter();
  while (!pSocket->_del) {
    if (pSocket->_onData_cb) {
      auto& rx_buf = *(pSocket->_p_rx_buf);
      if (pSocket->_dump_rx) {
        while (rx_buf.length()) {
          pSocket->lockRx();
          size_t len = min(rx_buf.length(), (size_t)WIFI_MSS);
          uint8_t* data = new uint8_t[len];
          rx_buf.read(data, len);
          pSocket->unlockRx();

          pSocket->_onData_cb(data, len);
        }
      } else {
        if (rx_buf.length()) {
          pSocket->_onData_cb(NULL, rx_buf.length());  // no dump, notify only
        }
      }
    }
    ulTaskNotifyTake(pdFALSE, SOCKET_RX_DELAY / portTICK_PERIOD_MS);
  }
  pSocket->_dec_del_counter();
  vTaskDelete(NULL);
}
void serialTskTx(void* ps) {
  AsyncHardwareSerial* pSerial = (AsyncHardwareSerial*)ps;
  pSerial->_inc_del_counter();
  auto& buf = pSerial->_buf;
  // for any buf operations should lock the buf mux (lockBuf/unlockBuf)
  auto& serial = *(pSerial->_serial);
  while (!pSerial->_del) {
    while (buf.length()) {
      pSerial->lock();  // lock mux
      size_t len = buf.length();
      uint8_t* data = new uint8_t[len];
      buf.read(data, len);
      pSerial->unlock();  // unlock mux

      serial.flush();
      serial.write(data, len);
      delete[] data;
    }
    ulTaskNotifyTake(pdFALSE, SERIAL_TX_DELAY / portTICK_PERIOD_MS);
  }
  pSerial->_dec_del_counter();
  vTaskDelete(NULL);
}
void serialTskRx(void* ps) {
  AsyncHardwareSerial* pSerial = (AsyncHardwareSerial*)ps;
  pSerial->_inc_del_counter();
  auto& serial = *(pSerial->_serial);
  while (!pSerial->_del) {
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
  pSerial->_dec_del_counter();
  vTaskDelete(NULL);
}

}  // namespace

// ============
// AsyncSocketSerial
// ============

AsyncSocketSerial::AsyncSocketSerial(int port)
    : BaseAsync(WIFI_MSS), port(port), server(port) {
  server.setNoDelay(true);

  server.onClient([&](void*, AsyncClient* pc) {
    if (!pc) return;
    AsyncClient* old_pClient = _set_pClient(pc);
    if (old_pClient) {
      if (old_pClient->connected()) {
        old_pClient->write("\r\nDevice is connected on another client.\r\n");
      }
      delete old_pClient;  // new from AsyncTCP.cpp:1297
    }

    notifyTx();  // check and send tx buffer

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

  // _create_tskTx
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
void AsyncSocketSerial::_create_tskRx() {
  _p_rx_buf = new CircularBuffer(_buf._cap);
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
void AsyncSocketSerial::_client_setup(AsyncClient* pc) {
  if (!pc) return;
  AsyncClient& serverClient = *pc;
  serverClient.onData([&](void*, AsyncClient*, void* data, size_t len) {
    _notify_Rx((uint8_t*)data, len);
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
void AsyncSocketSerial::_notify_Rx(uint8_t* data, size_t len) {
  if (_p_rx_buf) {
    lockRx();
    _p_rx_buf->write(data, len);
    unlockRx();
  }
  if (tskRx) xTaskNotifyGive(tskRx);
}
void AsyncSocketSerial::destroy() {
  if (_del) return;
  BaseAsync::destroy();
  AsyncClient* old_pClient = _set_pClient(NULL);
  if (old_pClient) {
    if (old_pClient->connected()) {
      old_pClient->write("\r\nServer closed.\r\n");
    }
    delete old_pClient;  // new from AsyncTCP.cpp:1297
  }
  server.end();  // end server and client avoid accidentally calling _notify_Rx by onData
}
AsyncSocketSerial::~AsyncSocketSerial() {
  AsyncClient* old_pClient = _set_pClient(NULL);
  if (old_pClient) {
    if (old_pClient->connected()) {
      old_pClient->write("\r\nServer closed.\r\n");
    }
    delete old_pClient;  // new from AsyncTCP.cpp:1297
  }
  if (_p_rx_buf) {
    lockRx();
    delete _p_rx_buf;
    _p_rx_buf = NULL;
    unlockRx();
  }
}

// ============
// AsyncHardwareSerial
// ============

AsyncHardwareSerial::AsyncHardwareSerial(HardwareSerial& s) : BaseAsync(SERIAL_MSS), _serial(&s) {
  // _create_tskTx
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
void AsyncHardwareSerial::_create_tskRx() {
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
AsyncHardwareSerial::~AsyncHardwareSerial() {}

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
