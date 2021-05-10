/*

  Non blocking data write and event based data read
  Let another core to handle the job for you

  Usage:


*/

#ifndef AsyncIO_h
#define AsyncIO_h

#include <Arduino.h>

// Arduino.h must include first

#include <AsyncTCP.h>

#include "CircularBuffer.h"
#include "MyLibCommon.h"

#define SERIAL_MSS 1024
#define WIFI_MSS 1436

// add this in setup():
void asyncIOSetup();
void logdSerial(String s);  // call AsyncSerial.printWithLock(s)

// ============
// AsyncStream
// ============

typedef void (*AsyncStreamDataHandler)(uint8_t *data, size_t len);

class DataReader {
 public:
  virtual void onData(uint8_t *data, size_t len) = 0;
};

template <size_t MSS>
class AsyncStream_T : public Print {
 public:
  bool notifying = false;
  AsyncStreamDataHandler _onData_cb = NULL;
  portMUX_TYPE _buf_mux = portMUX_INITIALIZER_UNLOCKED;
  CircularBuffer_T<MSS * 2> _buf;
  virtual void attachReader(AsyncStreamDataHandler onData) {
    _onData_cb = onData;
  }
  virtual size_t write(const uint8_t *buffer, size_t size) override {
    size_t s = _buf.write(buffer, size);
    if (!notifying && _buf.length() >= MSS) {
      notifying = true;
      notifyTx();
    }
    return s;
  }
  size_t write(uint8_t c) override {
    return write(&c, 1);
  }

  virtual void notifyTx() = 0;

  void lockBufTx() {
    portENTER_CRITICAL(&_buf_mux);
  }
  void unlockBufTx() {
    portEXIT_CRITICAL(&_buf_mux);
  }

  void printWithLock(String s) {
    lockBufTx();
    print(s);
    unlockBufTx();
  }

  AsyncStream_T() = default;
  AsyncStream_T(const AsyncStream_T &) = delete;
  AsyncStream_T &operator=(const AsyncStream_T &) = delete;
};

// TODO
class AsyncSocketSerial : public AsyncStream_T<WIFI_MSS> {
 public:
  void notifyTx();

  AsyncServer server;
  portMUX_TYPE _pClient_mux = portMUX_INITIALIZER_UNLOCKED;
  AsyncClient *pClient = NULL;
  AsyncSocketSerial(int port) : server(port) {
    server.setNoDelay(true);

    server.onClient([&](void *, AsyncClient *pc) {
      if (!pc) return;
      portENTER_CRITICAL(&_pClient_mux);
      if (pClient) {
        if (pClient->connected()) {
          pClient->write("\r\nDevice is connected on another client.\r\n");
        }
        delete pClient;  // new from AsyncTCP.cpp:1297
      }
      pClient = pc;
      portEXIT_CRITICAL(&_pClient_mux);
      clientSetup(pc);
      AsyncClient &serverClient = *pc;
      String s = stringf("New client: %s\r\n", serverClient.remoteIP().toString().c_str()) +
                 stringf("  %s:%d -> %s:%d\r\n",
                         serverClient.remoteIP().toString().c_str(),
                         serverClient.remotePort(),
                         serverClient.localIP().toString().c_str(),
                         serverClient.localPort());
      logdSerial(s);
    },
                    NULL);
  }
  void clientSetup(AsyncClient *pc) {
    if (!pc) return;
    AsyncClient &serverClient = *pc;
    serverClient.onData([&](void *, AsyncClient *, void *data, size_t len) {
      if (_onData_cb) {
        _onData_cb((uint8_t *)data, len);
      }
    },
                        NULL);
  }
  void begin() {
    server.begin();
  }
};

// TODO: support custom serial
class AsyncSerialClass : public AsyncStream_T<SERIAL_MSS> {
 public:
  void attachReader(AsyncStreamDataHandler onData) override;
  void notifyTx();
};

extern AsyncSerialClass AsyncSerial;

// ============
// Command Processor
// ============

#endif  // AsyncIO_h
