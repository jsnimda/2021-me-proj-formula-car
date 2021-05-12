#ifndef CircularBuffer_h
#define CircularBuffer_h

#include "Common.h"

enum eOverflowAction {
  eOverwrite,
  eDiscard,
};

class CircularBuffer {
 public:
  uint8_t* const _buf;
  const size_t _cap;
  size_t _len = 0;
  size_t _start = 0;  // inclusive index
  size_t _end = 0;    // exclusive index
  eOverflowAction _act;
  size_t overflow = 0;  // buffer overflowed count, be sure to clear this after checking
  size_t highMark = 0;  // high water mark, debug only

  CircularBuffer(size_t capacity, eOverflowAction overflowAction = eOverwrite)
      : _buf(new uint8_t[capacity]), _cap(capacity), _act(overflowAction) {}
  ~CircularBuffer() {
    delete[] _buf;
  }
  NONCOPYABLE(CircularBuffer);

  size_t capacity() const {
    return _cap;
  }
  size_t remaining() const {
    return _cap - _len;
  }
  size_t length() const {
    return _len;
  }

  size_t write(const uint8_t* buffer, size_t size) {
    size_t rem = remaining();
    if (size > rem) {
      overflow += size - rem;
      if (_act == eDiscard) {
        size = rem;
      } else {  // BUFFER_OVERWRITE or default
        if (size > _cap) {
          buffer += size - _cap;  // only save last (size) bytes to buffer
          size = _cap;
        }
        // size >= rem here (size == rem if size > cap && rem == cap)
        drop(size - rem);  // give up the space in buffer
      }
    }

    if (size == 0) return 0;

    // check need split if size > cap - _end
    size_t a = size > _cap - _end ? _cap - _end : size;
    memcpy(_buf + _end, buffer, a);
    memcpy(_buf, buffer + a, size - a);
    _len += size;
    _end = _start + _len;
    if (_end >= _cap) {
      _end -= _cap;
      highMark = max(highMark, (size_t)_cap);
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
      if (_start >= _cap) _start -= _cap;
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

  size_t read(uint8_t& c) {
    return read(&c, 1);
  }
  size_t read(uint8_t* buffer, size_t size) {  // retrieve
    if (size > _len) {
      size = _len;
    }

    if (size == 0) return 0;

    // check need split if size > cap - _start
    size_t a = size > _cap - _start ? _cap - _start : size;
    memcpy(buffer, _buf + _start, a);
    memcpy(buffer + a, _buf, size - a);
    _drop_no_check(size);

    return size;
  }
};

#endif  // CircularBuffer_h
