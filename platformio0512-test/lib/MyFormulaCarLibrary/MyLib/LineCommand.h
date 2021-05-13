#ifndef LineCommand_h
#define LineCommand_h

#define MAX_COMMAND_LENGTH 1024

#include <Arduino.h>

#include "CircularBuffer.h"
#include "Common.h"

typedef void (*LineHandler)(Line& c);

// ============
// Command Processor
// ============

class LineProcessor {
 public:
  CircularBuffer* const _buf;  // monitoring buffer
  int scanned = 0;
  size_t ind;

  // CR, LF or CRLF
  uint8_t last = '\0';

  LineProcessor(CircularBuffer* buf) : _buf(buf) {
    ind = _buf->_start;
  }

  virtual void onLine(String s) = 0;

  bool inside() {  // return if ind inside buffer
    CircularBuffer& buf = *_buf;
    if (buf._len == buf._cap) return true;
    if (buf._start <= buf._end) return ind >= buf._start && ind < buf._end;
    // buf._start > buf._end
    return ind >= buf._start || ind < buf._end;
  }
  void inc() {
    scanned++;
    ind++;
    if (ind >= _buf->_cap) ind = 0;
  }
  uint8_t get() {
    return _buf->_buf[ind];
  }

  void notify() {
    int len = tryGetLine();
    if (len > 0) {
      uint8_t* data = new uint8_t[len + 1];
      _buf->read(data, len);
      data[len] = '\0';
      onLine((char*)data);
      delete data;
      scanned = 0;
      ind = _buf->_start;
    }
  }

  int tryGetLine() {
    while (inside() && scanned < MAX_COMMAND_LENGTH) {
      uint8_t c = get();
      uint8_t d = last;
      last = c;
      inc();
      if (c == '\r') {
        return scanned;
      } else if (c == '\n') {
        if (d != '\r') {
          return scanned;
        }  // skip the \n after \r as it is already processed
      }
    }
    if (scanned >= MAX_COMMAND_LENGTH) {
      return scanned;
    }
    return -1;
  }
};

class CommandProcessor : public LineProcessor {
 public:
  std::vector<LineHandler> routes;

  CommandProcessor(CircularBuffer* buf) : LineProcessor(buf) {}

  void use(LineHandler r) {
    routes.push_back(r);
  }

  void onLine(String s) {
    Line c(s);
    for (int i = 0; i < routes.size(); i++) {
      LineHandler r = routes[i];
      r(c);
      if (c.handled || c.response.length()) return;
    }
    if (c.response.length()) {
      onResponse(c.response);
    }
  }

  virtual void onResponse(String s) {
    // logc(s);
  }
};

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
    handled = false;
    response = "";
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
  void set() {
    handled = true;
  }
  bool setOn(const String& s) {
    if (on(s)) {
      set();
      return true;
    } else {
      return false;
    }
  }
};

// ============
// command set
// ============

void basic_route(Line& c) {
  if (c.on("syntax") && c.on("test")) {
    int ind = c.ind;
    if (c.on("int") && c.onInt()) {
      c.setResponse(stringf("ACK int: %ld", c.rint));
    }
    c.sub(ind);
    if (c.on("double") && c.onDouble()) {
      c.setResponse(stringf("ACK double: %f", c.rdouble));
    }
    c.sub(ind);
  }
}

#endif  // LineCommand_h
