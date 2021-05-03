#ifndef CircularBuffer_h
#define CircularBuffer_h

#include <stddef.h>
#include <stdint.h>

#include "Print.h"

#define WIFI_MTU_SIZE 1500

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

    // case 1: right remaining > 0 (start <= end < capacity)
    //         rrem = capacity - end
    // split if size > rrem

    // case 2: no right remaining (end <= start < capacity)
    // no need to split

    // case 3: len == capacity (start == end)
    // should already be returned (should not reach here)

    // if size > cap - _end
    // implies case 1 need split
    // no need check start <= end < capacity
    // if (size > cap - _end) {  // need split
    //   int rrem = cap - _end;
    //   memcpy(_buf + _end, buffer, rrem);  // sizeof(uint8_t) always 1
    //   memcpy(_buf, buffer + rrem, size - rrem);
    // } else {  // no need split
    //   memcpy(_buf + _end, buffer, size);
    // }

    // Flash size 201680 -> 201676
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

    // case 1: _buf is splited (start + len > cap)
    // (end == 0 is not considered splited)
    // split if size > cap - start

    // case 2: continuous _buf (start + len <= cap)

    // if (size > cap - _start) {
    //   int rsize = cap - _start;
    //   memcpy(buffer, _buf + _start, rsize);
    //   memcpy(buffer + rsize, _buf, size - rsize);
    // } else {
    //   memcpy(buffer, _buf + _start, size);
    // }

    // Flash size 201676 -> 201652
    size_t a = size > cap - _start ? cap - _start : size;
    memcpy(buffer, _buf + _start, a);
    memcpy(buffer + a, _buf, size - a);
    _len -= size;
    _start += size;
    if (_start >= cap) _start -= cap;

    return size;
  }
};

// typedef CircularBuffer_T<WIFI_MTU_SIZE * 2> CircularBuffer;

#endif  // CircularBuffer_h
