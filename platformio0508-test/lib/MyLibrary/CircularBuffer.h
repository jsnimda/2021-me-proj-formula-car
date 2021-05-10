#ifndef CircularBuffer_h
#define CircularBuffer_h

#include <stddef.h>
#include <stdint.h>

#include "Print.h"

enum BufferOverflowAction {
  BUFFER_OVERWRITE,
  WRITE_DISCARD,
};

template <size_t BUF_SIZE, BufferOverflowAction OVERFLOW_ACTION = BUFFER_OVERWRITE>
class CircularBuffer_T : public Print {
  enum { cap = BUF_SIZE };  // unnamed enum for constant without allocating memory

 public:
  uint8_t _buf[BUF_SIZE];
  size_t _len = 0;
  size_t _start = 0;    // inclusive index
  size_t _end = 0;      // exclusive index
  size_t overflow = 0;  // buffer overflowed count, be sure to clear this after checking
  size_t highMark = 0;  // high water mark, debug only

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
      overflow += size - rem;
      if (OVERFLOW_ACTION == WRITE_DISCARD) {
        size = rem;
      } else {  // BUFFER_OVERWRITE or default
        if (size > cap) {
          buffer += size - cap;  // only save last (size) bytes to buffer
          size = cap;
        }
        // size >= rem here (size == rem if size > cap && rem == cap)
        drop(size - rem);  // give up the space in buffer
      }
    }

    if (size == 0) return 0;

    // check need split if size > cap - _end
    size_t a = size > cap - _end ? cap - _end : size;
    memcpy(_buf + _end, buffer, a);
    memcpy(_buf, buffer + a, size - a);
    _len += size;
    _end = _start + _len;
    if (_end >= cap) {
      _end -= cap;
      highMark = max(highMark, (size_t)cap);
    } else {
      highMark = max(highMark, _end);
    }

    return size;
  }

  inline __attribute__((always_inline)) void _drop_no_check(size_t size) {
    _len -= size;
    if (_len == 0) {  // reset to 0
      _start = 0;
      _end = 0;
    } else {
      _start += size;
      if (_start >= cap) _start -= cap;
    }
  }
  size_t drop(size_t size) {
    if (size > _len) {
      size = _len;
    }

    if (size == 0) return 0;

    _drop_no_check(size);

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
    _drop_no_check(size);

    return size;
  }
};

#endif  // CircularBuffer_h
