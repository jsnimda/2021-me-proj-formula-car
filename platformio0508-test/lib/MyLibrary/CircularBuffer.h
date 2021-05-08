#ifndef CircularBuffer_h
#define CircularBuffer_h

#include <stddef.h>
#include <stdint.h>

#include "Print.h"

template <size_t BUF_SIZE>
class CircularBuffer_T : public Print {
  enum { cap = BUF_SIZE };  // unnamed enum for constant without allocating memory

 public:
  uint8_t _buf[BUF_SIZE];
  size_t _len = 0;
  size_t _start = 0;    // inclusive index
  size_t _end = 0;      // exclusive index
  size_t overflow = 0;  // buffer overflowed count, be sure to clear this after checking

  CircularBuffer_T(){};

  size_t capacity() const {
    return cap;
  }
  size_t remaining() const {
    return cap - _len;
  }
  size_t length() const {
    return _len;
  }

  size_t write(uint8_t c) override {
    return write(&c, 1);
  }
  size_t write(const uint8_t *buffer, size_t size) override {
    size_t rem = remaining();
    if (size > rem) {
      overflow += rem - size;
      size = rem;
    }

    if (size == 0) return 0;

    // check need split if size > cap - _end
    size_t a = size > cap - _end ? cap - _end : size;
    memcpy(_buf + _end, buffer, a);
    memcpy(_buf, buffer + a, size - a);
    _len += size;
    _end = _start + _len;
    if (_end >= cap) _end -= cap;

    return size;
  }

  size_t read(uint8_t &c) {
    return read(&c, 1);
  }
  size_t read(uint8_t *buffer, size_t size) {  // retrieve
    if (size > _len) {
      size = _len;
    }

    if (size == 0) return 0;

    // check need split if size > cap - _start
    size_t a = size > cap - _start ? cap - _start : size;
    memcpy(buffer, _buf + _start, a);
    memcpy(buffer + a, _buf, size - a);
    _len -= size;
    _start += size;
    if (_start >= cap) _start -= cap;

    return size;
  }
};

#endif  // CircularBuffer_h
