/*

  Non blocking data write and event based data read
  Let another core to handle the job for you

  Usage:


*/

#ifndef AsyncIO_h
#define AsyncIO_h

#include <Arduino.h>

//

#include <AsyncTCP.h>

// add this in setup():
void asyncIOSetup();

// ============
// AsyncStream
// ============

typedef void (*AsyncStreamDataHandler)(uint8_t *data, size_t len);

class DataReader {
 public:
  virtual void onData(uint8_t *data, size_t len) = 0;
};

class AsyncStream : public Print {
 public:
  AsyncStreamDataHandler _onData_cb = NULL;
  virtual void attachReader(AsyncStreamDataHandler onData) {
    _onData_cb = onData;
  }
  virtual size_t write(const uint8_t *buffer, size_t size) override = 0;
  size_t write(uint8_t c) override {
    return write(&c, 1);
  }
};

// TODO
class AsyncSocketSerial : public AsyncStream {
 public:
  size_t write(const uint8_t *buffer, size_t size) override;
};

// TODO: support custom serial
class AsyncHardwareSerial : public AsyncStream {
 public:
  HardwareSerial *ps;
  AsyncHardwareSerial(HardwareSerial &s) : ps(&s) {}
  void attachReader(AsyncStreamDataHandler onData) override;
  size_t write(const uint8_t *buffer, size_t size) override;
};

extern AsyncHardwareSerial AsyncSerial;

// ============
// Command Processor
// ============

#endif  // AsyncIO_h
