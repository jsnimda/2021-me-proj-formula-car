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

  void lock() {
    portENTER_CRITICAL(&_buf_mux);
  }
  void unlock() {
    portEXIT_CRITICAL(&_buf_mux);
  }
  size_t lockedPrint(String s) {
    lock();
    size_t len = print(s);
    unlock();
    return len;
  }
  size_t lockedWrite(const uint8_t* buffer, size_t size) {
    lock();
    size_t len = write(buffer, size);
    unlock();
    return len;
  }
};

// ============
// BaseAsync
// ============

class BaseAsync : public AsyncStream {
 public:
  TaskHandle_t tskTx = NULL;
  TaskHandle_t tskRx = NULL;
  bool _del = false;
  portMUX_TYPE _del_mux = portMUX_INITIALIZER_UNLOCKED;
  int _del_counter = 1;  // increase this for every task

  BaseAsync(size_t mss) : AsyncStream(mss) {}

  void onData(AssDataHandler cb) override {
    AsyncStream::onData(cb);
    if (!tskRx) {
      _create_tskRx();
    }
  }

  void notifyTx() override {
    // ref: https://www.freertos.org/RTOS_Task_Notification_As_Binary_Semaphore.html
    if (!tskTx) return;
    xTaskNotifyGive(tskTx);
  }

  virtual void _create_tskRx() = 0;

  int _dec_del_counter() {
    portENTER_CRITICAL(&_del_mux);
    _del_counter--;
    int r = _del_counter;
    portEXIT_CRITICAL(&_del_mux);
    return r;
  }
  int _inc_del_counter() {
    portENTER_CRITICAL(&_del_mux);
    _del_counter++;
    int r = _del_counter;
    portEXIT_CRITICAL(&_del_mux);
    return r;
  }
  virtual void destroy() {
    if (_del) return;
    _del = true;
    if (_dec_del_counter() == 0) {
      delete this;
    }
  }

 protected:
  // heap only
  virtual ~BaseAsync() {}
};

// ============
// AsyncSocketSerial
// ============

class AsyncSocketSerial : public virtual BaseAsync {
 public:
  const int port;  // server port, read only
  AsyncServer server;
  portMUX_TYPE _pClient_mux = portMUX_INITIALIZER_UNLOCKED;
  AsyncClient* pClient = NULL;
  AssConnectHandler _onConnect_cb = NULL;
  AssConnectHandler _onDisconnect_cb = NULL;
  portMUX_TYPE _rx_buf_mux = portMUX_INITIALIZER_UNLOCKED;
  CircularBuffer* _p_rx_buf = NULL;

  AsyncSocketSerial(int port);

  void _create_tskRx() override;
  void _client_setup(AsyncClient* pc);
  void _notify_Rx(uint8_t* data, size_t len);

  virtual void destroy() override;

  AsyncClient* _set_pClient(AsyncClient* pc) {  // return old client
    AsyncClient* old_pClient = NULL;
    portENTER_CRITICAL(&_pClient_mux);
    old_pClient = pClient;
    pClient = pc;
    portEXIT_CRITICAL(&_pClient_mux);
    _client_setup(pc);
    return old_pClient;
  }
  void begin() {
    server.begin();
  }
  void onConnect(AssConnectHandler cb) {
    _onConnect_cb = cb;
  }
  void onDisconnect(AssConnectHandler cb) {
    _onDisconnect_cb = cb;
  }

  void lockRx() {
    portENTER_CRITICAL(&_rx_buf_mux);
  }
  void unlockRx() {
    portEXIT_CRITICAL(&_rx_buf_mux);
  }

  // heap only
 private:
  ~AsyncSocketSerial() override;
};

// ============
// AsyncHardwareSerial
// ============

class AsyncHardwareSerial : public BaseAsync {
 public:
  HardwareSerial* _serial;

  AsyncHardwareSerial(HardwareSerial& s);

  void _create_tskRx() override;

  // heap ony
 private:
  ~AsyncHardwareSerial() override;
};

#if CONFIG_USE_ASYNC_SERIAL
extern AsyncHardwareSerial AsyncSerial;
#endif

// ============
// Command Processor
// ============

#endif  // AsyncIO_h
