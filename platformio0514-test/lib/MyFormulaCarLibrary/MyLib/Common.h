#ifndef Common_h
#define Common_h

#include "myconfig.h"
//
#include <Arduino.h>

// ============
// useful macros
// ============

// Rule of three
#define NONCOPYABLE(class_name)           \
  class_name(const class_name&) = delete; \
  class_name& operator=(const class_name&) = delete

// ============
// debug
// ============

void loga(String s);  // serial only
void logb(String s);  // wifi only
void logc(String s);  // both
#undef loge
void loge(String s, const char* file_name, size_t line, const char* function);  // log_e to serial
#define loge(s) (loge)(s, pathToFileName(__FILE__), __LINE__, __FUNCTION__)

// ============
// useful functions
// ============

inline String stringf(const char* format, ...) __attribute__((format(printf, 1, 2)));
inline String stringf(const char* format, ...) {
  // ref: see Print::printf(...)
  char loc_buf[64];
  char* temp = loc_buf;
  va_list arg;
  va_list copy;
  va_start(arg, format);
  va_copy(copy, arg);
  int len = vsnprintf(temp, sizeof(loc_buf), format, copy);
  va_end(copy);
  if (len < 0) {
    va_end(arg);
    return "";
  };
  if (len >= sizeof(loc_buf)) {
    temp = (char*)malloc(len + 1);
    if (temp == NULL) {
      va_end(arg);
      return "";
    }
    len = vsnprintf(temp, len + 1, format, arg);
  }
  va_end(arg);
  String s = temp;
  if (temp != loc_buf) {
    free(temp);
  }
  return s;
}

// ============
// useful classes
// ============

// lock

class lock_base {
 public:
  // mux.count == 0 if unlocked
  portMUX_TYPE _mux = portMUX_INITIALIZER_UNLOCKED;
  void lock() {
    portENTER_CRITICAL(&_mux);
  }
  void unlock() {
    portEXIT_CRITICAL(&_mux);
  }

  lock_base() {}
  NONCOPYABLE(lock_base);
};

template <typename T>
class with_lock : public lock_base {
 public:
  T _data;

  template <typename... Args>
  with_lock(Args&&... args) : _data(std::forward<Args>(args)...) {}

  T& operator*() {
    if (_mux.count == 0) {
      loge(stringf("forget to lock? in %s", __PRETTY_FUNCTION__));
    }
    return _data;
  }
  T* operator->() {
    if (_mux.count == 0) {
      loge(stringf("forget to lock? in %s", __PRETTY_FUNCTION__));
    }
    return &_data;
  }
};

template <typename T>
class with_lock_ptr : public lock_base {
 public:
  T* _ptr;

  with_lock_ptr(T* p = NULL) : _ptr(p) {}

  T& operator*() {
    if (_mux.count == 0) {
      loge(stringf("forget to lock? in %s", __PRETTY_FUNCTION__));
    }
    return *_ptr;
  }
  T* operator->() {
    if (_mux.count == 0) {
      loge(stringf("forget to lock? in %s", __PRETTY_FUNCTION__));
    }
    return _ptr;
  }

  void operator=(T* p) {
    set_ptr(p);
  }

  bool operator==(T* p) {
    return _ptr == p;
  }
  bool operator!=(T* p) {
    return _ptr != p;
  }

  explicit operator bool() {
    return _ptr != NULL;
  }

  void set_ptr(T* p) {
    _ptr = p;
  }
  void delete_ptr() {
    delete _ptr;
    _ptr = NULL;
  }  // for delete[] do it your self

  T* locked_set_ptr(T* p) {  // return old value
    lock();
    T* old = _ptr;
    _ptr = p;
    unlock();
    return old;
  }
};

// lifetime related
class BaseResource {
 public:
  bool del = false;
  with_lock<int> del_counter;

  BaseResource() {
    (*del_counter) = 1;
  }
  NONCOPYABLE(BaseResource);

  virtual void release() {
    del = true;
    if (dec_del_counter() == 0) {
      delete this;
    }
  }

  int inc_del_counter() {
    del_counter.lock();
    (*del_counter)++;
    int r = (*del_counter);
    del_counter.unlock();
    return r;
  }
  int dec_del_counter() {
    del_counter.lock();
    (*del_counter)--;
    int r = (*del_counter);
    del_counter.unlock();
    return r;
  }

 protected:
  // heap only
  virtual ~BaseResource() {}
};

// ============
// debug includes
// ============

#include "DebugPerf.h"

#endif  // Common_h
