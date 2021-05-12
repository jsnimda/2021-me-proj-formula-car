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
#include "Common.h"

// whatsoever the print rate should not exceed the maximum data rate
// as it is async it will cause data lost (by overwrite)
#define SERIAL_MSS (1024 * 2)
#define WIFI_MSS 1436
#define BUF_SIZE_FACTOR 4

// add this in setup():
void asyncIOSetup();
void loga(String s);  // call AsyncSerial.printWithLock(s) if async

typedef void (*AssDataHandler)(uint8_t* data, size_t len);
typedef void (*AssConnectHandler)(AsyncClient*);

// ============
// AsyncStream
// ============

class AsyncStream : public Print {
 public:
  AssDataHandler _onData_cb = NULL;
  portMUX_TYPE _buf_mux = portMUX_INITIALIZER_UNLOCKED;
  CircularBuffer _buf;
  size_t _mss;

  AsyncStream(size_t mss) : _buf(mss * BUF_SIZE_FACTOR), _mss(mss) {}
  NONCOPYABLE(AsyncStream);

  virtual void onData(AssDataHandler cb) {
    _onData_cb = cb;
  }
  size_t write(uint8_t c) override {
    return write(&c, 1);
  }
  virtual size_t write(const uint8_t* buffer, size_t size) override {
    size_t s = _buf.write(buffer, size);
    if (_buf.length() >= _mss) {
      notifyTx();
    }
    return s;
  }

  virtual void notifyTx() = 0;

  // ============
  // Lock
  // ============

  void lockBuf() {
    portENTER_CRITICAL(&_buf_mux);
  }
  void unlockBuf() {
    portEXIT_CRITICAL(&_buf_mux);
  }
  size_t lockedPrint(String s) {
    lockBuf();
    size_t len = print(s);
    unlockBuf();
    return len;
  }
  size_t lockedWrite(const uint8_t* buffer, size_t size) {
    lockBuf();
    size_t len = write(buffer, size);
    unlockBuf();
    return len;
  }
};

// ============
// AsyncSocketSerial
// ============

class AsyncSocketSerial : public AsyncStream {
 public:
  TaskHandle_t tskTx = NULL;
  TaskHandle_t tskRx = NULL;
  const int port;  // server port, read only
  AsyncServer server;
  portMUX_TYPE _pClient_mux = portMUX_INITIALIZER_UNLOCKED;
  AsyncClient* pClient = NULL;
  AssConnectHandler _onConnect_cb = NULL;
  AssConnectHandler _onDisconnect_cb = NULL;

  AsyncSocketSerial(int port);
  ~AsyncSocketSerial();

  void onData(AssDataHandler cb) override;
  void notifyTx() override;

  void _client_setup(AsyncClient* pc);

  void begin() {
    server.begin();
  }

  void onConnect(AssConnectHandler cb) {
    _onConnect_cb = cb;
  }
  void onDisconnect(AssConnectHandler cb) {
    _onDisconnect_cb = cb;
  }
};

// ============
// AsyncHardwareSerial
// ============

class AsyncHardwareSerial : public AsyncStream {
 public:
  TaskHandle_t tskTx = NULL;
  TaskHandle_t tskRx = NULL;
  HardwareSerial* _serial;

  AsyncHardwareSerial(HardwareSerial& s);
  ~AsyncHardwareSerial();

  void onData(AssDataHandler onData) override;
  void notifyTx();
};

#if CONFIG_USE_ASYNC_SERIAL
extern AsyncHardwareSerial AsyncSerial;
#endif

// ============
// Command Processor
// ============

#endif  // AsyncIO_h
