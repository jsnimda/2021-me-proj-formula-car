/*

  Non blocking data write and event based data read
  Let another core to handle the job for you

  Usage:


*/

#ifndef AsyncIO_h
#define AsyncIO_h

#include <Arduino.h>

#include <functional>

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

// std::function can handle captured lambda no problem

// ============
// resource class
// ============

class BaseAsyncResource : public BaseResource {
 public:
  typedef std::function<void(BaseAsyncResource*, uint8_t* data, size_t len)> DataHandler;

  TaskHandle_t _tskTx = NULL;
  TaskHandle_t _tskRx = NULL;
  String _name;  // use for task name
  DataHandler _onData_cb = NULL;
  size_t _mss;
  with_lock_ptr<CircularBuffer> _buf_tx;  // should coexist with _tskTx
  with_lock_ptr<CircularBuffer> _buf_rx;  // should coexist with _tskRx

  void _write_init() {
    _buf_tx = new CircularBuffer(_mss * BUF_SIZE_FACTOR);
  }
  void _read_init() {  // typically called when _onData_cb is set
    _buf_rx = new CircularBuffer(_mss * BUF_SIZE_FACTOR);
  }
  void _create_tsk_tx(TaskFunction_t func, uint32_t stack, UBaseType_t priority) {
    inc_del_counter();
    xTaskCreatePinnedToCore(
        func, ("Tx" + _name).c_str(), stack, this, priority, &_tskTx, 0);
  }
  void _create_tsk_rx(TaskFunction_t func, uint32_t stack, UBaseType_t priority) {
    inc_del_counter();
    xTaskCreatePinnedToCore(
        func, ("Rx" + _name).c_str(), stack, this, priority, &_tskRx, 0);
  }

  void notifyTx() {
    if (_tskTx) {
      xTaskNotifyGive(_tskTx);
    } else {
      loge(stringf("Tx%s not exist", _name.c_str()));
    }
  }
  void notifyRx() {
    if (_tskRx) {
      xTaskNotifyGive(_tskRx);
    } else {
      loge(stringf("Rx%s not exist", _name.c_str()));
    }
  }

  bool hasTx() {
    return _buf_tx && _tskTx;
  }
  bool hasRx() {
    return _buf_rx && _tskRx;
  }

  size_t write(const uint8_t* buffer, size_t size) {
    size_t s = _buf_tx->write(buffer, size);
    if (_buf_tx->length() >= _mss) {
      notifyTx();
    }
    return s;
  }

  virtual void begin() {
    // do write or read init here
  }

 protected:
  ~BaseAsyncResource() {
    _buf_tx.delete_ptr();
    _buf_rx.delete_ptr();
  }
};

class SocketServerResource : public BaseAsyncResource {
 public:
  typedef std::function<void(SocketServerResource*, AsyncClient*)> ConnectHandler;
  ConnectHandler _onConnect_cb = NULL;
  ConnectHandler _onDisconnect_cb = NULL;
  const int port;  // server port, read only
  AsyncServer server;
  with_lock_ptr<AsyncClient> client;
  with_lock<std::vector<AsyncClient*>> _pc_to_be_freed;  // use delete to free
  bool use_tx = false;
  bool use_rx = false;
  bool rx_notify_only = false;  // do not copy uint8_t* to on data cb

  SocketServerResource(int port);

  void begin() override;

  void _delete_pc(AsyncClient* pc) {
    if (hasTx() || hasRx()) {
      _pc_to_be_freed.lock();
      _pc_to_be_freed->push_back(pc);  // delete pointer in task
      _pc_to_be_freed.unlock();
    } else {
      delete pc;
    }
  }
  void _do_clear_vector() {
    _pc_to_be_freed.lock();
    for (int i = 0; i < _pc_to_be_freed->size(); i++) {
      AsyncClient* pc = (*_pc_to_be_freed)[i];
      delete pc;
    }
    _pc_to_be_freed->clear();
    _pc_to_be_freed.unlock();
  }

  void release() override {
    if (!del) {  // first release
      del = true;
      AsyncClient* old_pc = client.locked_set_ptr(NULL);
      if (old_pc) {
        if (old_pc->connected()) {
          old_pc->write("\r\nServer closed.\r\n");
        }
        _delete_pc(old_pc);  // new from AsyncTCP.cpp:1297
      }
      server.end();
    }
    BaseAsyncResource::release();
  }

 protected:
  ~SocketServerResource() {
    client.delete_ptr();
    _do_clear_vector();
  }
};

// ============
// server class
// ============

// template <typename T>  // T : SocketServerResource
class BaseSocketServer : public Print {
 public:
  using DataHandler = BaseAsyncResource::DataHandler;
  using ConnectHandler = SocketServerResource::ConnectHandler;

  SocketServerResource* resource;

  BaseSocketServer(int port) : resource(new SocketServerResource(port)) {}
  virtual ~BaseSocketServer() {
    resource->release();
  }

  virtual void begin() {
    resource->begin();
  }

  size_t write(uint8_t c) override {
    return write(&c, 1);
  }
  size_t write(const uint8_t* buffer, size_t size) override {
    auto& buf = resource->_buf_tx;
    return buf->write(buffer, size);
  }

  void onData(DataHandler cb) {
    resource->_onData_cb = cb;
  }
  void onConnect(ConnectHandler cb) {
    resource->_onConnect_cb = cb;
  }
  void onDisconnect(ConnectHandler cb) {
    resource->_onDisconnect_cb = cb;
  }

  void lockedPrint(String s) {
    auto& buf = resource->_buf_tx;
    buf.lock();
    print(s);
    buf.unlock();
  }
  void lockedWrite(const uint8_t* data, size_t len) {
    auto& buf = resource->_buf_tx;
    buf.lock();
    write(data, len);
    buf.unlock();
  }

  // raii lock
  // TODO
};

// ============
// Extern
// ============

#if CONFIG_DEBUG_PERF
extern PerfData pd_wifiWrite;
#endif

// todo, make console_sockets thread safe
extern std::vector<SocketServerResource*> console_sockets;  // for logb

#endif  // AsyncIO_h
