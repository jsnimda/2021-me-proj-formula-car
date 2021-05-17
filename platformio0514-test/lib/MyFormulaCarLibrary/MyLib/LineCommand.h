#ifndef LineCommand_h
#define LineCommand_h

#define MAX_COMMAND_LENGTH 1024

#include <Arduino.h>

#include <functional>

#include "CircularBuffer.h"
#include "Common.h"

class Line;

typedef std::function<void(Line& c)> LineHandler;
typedef std::function<void(String s)> ResponseCallback;

// ============
// Line
// ============

class Line {
 public:
  const String _s;
  String str;
  int ind = 0;
  bool handled = false;
  String response = "";

  long rint;
  double rdouble;

  Line(String s) : _s((s.trim(), s)), str(s) {}
  NONCOPYABLE(Line);

  void reset() {
    str = _s;
    ind = 0;
  }

  bool hasResult() {
    return handled || response.length();
  }

  void sub(int i) {
    ind = i;
    str = _s.substring(i);
  }
  void drop(int d) {  // drop and trim space
    while (d < str.length() && isspace(str[d])) {
      d++;
    }
    ind += d;
    str = str.substring(d);
  }
  bool on(const String& s) {
    if (str.startsWith(s) &&
        (str.length() == s.length() || isspace(str[s.length()]))) {
      drop(s.length());
      return true;
    } else {
      return false;
    }
  }
  bool onInt(long* p = NULL) {
    if (str.length() == 0) return false;
    const char* cstr = str.c_str();
    char* end = NULL;
    long a = strtol(cstr, &end, 10);
    if (a == 0 && end <= cstr) {
      // invalid 0
      return false;
    }
    if (p) *p = a;
    rint = a;
    drop(end - cstr);
    return true;
  }
  bool onDouble(double* p = NULL) {
    if (str.length() == 0) return false;
    const char* cstr = str.c_str();
    char* end = NULL;
    double d = strtod(cstr, &end);
    if (d == 0.0 && end <= cstr) {
      // invalid 0
      return false;
    }
    if (p) *p = d;
    rdouble = d;
    drop(end - cstr);
    return true;
  }
  void setResponse(const String& s) {
    set();
    response = s;
  }
  void ack() {
    setResponse("ACK: " + _s);
  }
  void set() {
    handled = true;
  }
  // bool setOn(const String& s) {
  //   if (on(s)) {
  //     set();
  //     return true;
  //   } else {
  //     return false;
  //   }
  // }
};

// ============
// Command Processor
// ============

class LineProcessor {
 public:
  with_lock_ptr<CircularBuffer>& buf;  // monitoring buffer
  int scanned = 0;
  size_t ind = 0;

  // CR, LF or CRLF
  uint8_t last = '\0';

  LineProcessor(with_lock_ptr<CircularBuffer>& b) : buf(b) {}
  NONCOPYABLE(LineProcessor);

  virtual void onLine(String s) = 0;  // child method

  void checkNewLine() {  // call this to check new line
    size_t len;
    uint8_t* data;
    while (_find_line(data, len)) {
      onLine((char*)data);
      delete[] data;
    }
  }

  void reset() {  // this will also clear last
    scanned = 0;
    ind = buf._ptr->_start;  // no lock here
    last = '\0';
  }

  bool _inside() {  // return if ind inside buffer
    if (buf->_len == 0) return false;
    if (buf->_len == buf->_cap) return true;
    if (buf->_start <= buf->_end) return ind >= buf->_start && ind < buf->_end;
    // buf._start > buf._end
    return ind >= buf->_start || ind < buf->_end;
  }

  void _inc() {
    scanned++;
    ind++;
    if (ind >= buf->_cap) ind = 0;
  }

  void _read(uint8_t*& data, size_t& len) {  // len = scanned
    data = new uint8_t[scanned + 1];
    len = buf->read(data, scanned);
    data[len] = '\0';
    scanned = 0;
    ind = buf->_start;
  }

  bool _find_line(uint8_t*& data, size_t& len) {  // return len > 0
    data = NULL;
    len = 0;
    buf.lock();
    while (_inside()) {
      uint8_t c = buf->_buf[ind];
      uint8_t d = last;
      last = c;
      _inc();

      if (scanned >= MAX_COMMAND_LENGTH ||
          (c == '\r') ||
          (c == '\n' && d != '\r')) {  // skip the \n after \r as it is already processed
        _read(data, len);
        buf.unlock();
        return true;
      }
    }
    buf.unlock();
    return false;
  }
};

class CommandProcessor : public LineProcessor {
 public:
  std::vector<LineHandler> routes;
  ResponseCallback onResponse_cb = NULL;

  CommandProcessor(with_lock_ptr<CircularBuffer>& b) : LineProcessor(b) {}

  void use(LineHandler r) {
    routes.push_back(r);
  }

  void onLine(String s) override {
    Line c(s);
    for (int i = 0; i < routes.size(); i++) {
      LineHandler r = routes[i];
      r(c);
      if (c.hasResult()) break;
      c.reset();
    }
    if (onResponse_cb) {
      if (c.response.length()) {
        onResponse_cb(c.response);
      } else if (!c.handled) {
        onResponse_cb("No such command: " + c._s);
      }
    }
  }

  void onResponse(ResponseCallback cb) {
    onResponse_cb = cb;
  }
};

#endif  // LineCommand_h
