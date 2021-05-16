#include "AsyncIO.h"

#define TSK_SOCKET_TX_PRIORITY 5
#define TSK_SOCKET_RX_PRIORITY 5
#define TSK_SERIAL_TX_PRIORITY 4
#define TSK_SERIAL_RX_PRIORITY 4
#define TSK_SOCKET_TX_STACK 4096
#define TSK_SOCKET_RX_STACK 4096
#define TSK_SERIAL_TX_STACK 4096
#define TSK_SERIAL_RX_STACK 4096
#define SOCKET_TX_DELAY 20
#define SOCKET_RX_DELAY (3600 * 1000)
#define SERIAL_TX_DELAY 20
// 115200 bps * 20 ms = 288 bytes < RxBufferSize
#define SERIAL_RX_DELAY 20

// ============
// Extern
// ============

#if CONFIG_DEBUG_PERF
PerfData pd_wifiWrite("wifiWrite");
PerfData pd_wifi_onData_cb("wifi_onData_cb");
#else
#define perf_loc(...)
#define perf_start(loc)
#define perf_add(loc, perfData)
#define perf_end(...)
#endif

std::vector<SocketServerResource*> console_sockets;  // for logb

// ============
// Tasks
// ============

namespace {

// todo dynamic maximum len
inline size_t read_buffer(with_lock_ptr<CircularBuffer>& buf, uint8_t*& data, size_t& len) {
  buf.lock();  // lock mux
  len = min(buf->length(), (size_t)WIFI_MSS);
  data = new uint8_t[len];
  size_t r = buf->read(data, len);
  buf.unlock();  // unlock mux
  return r;
}

void socketTskTx(void* p) {
  SocketServerResource* ps = static_cast<SocketServerResource*>(p);
  auto& buf = ps->_buf_tx;
  auto& client = ps->client;
  perf_loc(_a);

  while (!ps->del) {
    // todo, _do_clear_vector()
    AsyncClient* pc = client._ptr;
    if (pc) {
      while (client == pc && !ps->del) {
        while (buf._ptr->length()) {
          size_t len;
          uint8_t* data;
          read_buffer(buf, data, len);

          perf_start(_a);
          pc->write((const char*)data, len);
          perf_end(_a, pd_wifiWrite);
          delete[] data;
        }
        ulTaskNotifyTake(pdFALSE, SOCKET_TX_DELAY / portTICK_PERIOD_MS);
      }
    }
    ulTaskNotifyTake(pdFALSE, SOCKET_TX_DELAY / portTICK_PERIOD_MS);
  }
  ps->release();
  vTaskDelete(NULL);
}

void socketTskRx(void* p) {
  SocketServerResource* ps = static_cast<SocketServerResource*>(p);
  auto& buf = ps->_buf_rx;
  perf_loc(_a);

  while (!ps->del) {
    if (buf._ptr->length()) {
      if (ps->rx_notify_only) {
        ps->_onData_cb(ps, NULL, 0);  // no copy, notify only
      } else {
        while (buf._ptr->length()) {
          size_t len;
          uint8_t* data;
          read_buffer(buf, data, len);

          perf_start(_a);
          ps->_onData_cb(ps, data, len);
          perf_end(_a, pd_wifi_onData_cb);
          delete[] data;
        }
      }
    }
    ulTaskNotifyTake(pdFALSE, SOCKET_RX_DELAY / portTICK_PERIOD_MS);
  }
  ps->release();
  vTaskDelete(NULL);
}

}  // namespace

// ============
// SocketServerResource
// ============

namespace {

inline void attachClientEvent(SocketServerResource* ps, AsyncClient* pc) {
  if (!pc) return;
  AsyncClient& c = *pc;
  if (ps->hasRx()) {  // don't attach if rx not exist
    c.onData([ps](void*, AsyncClient*, void* data, size_t len) {
      ps->_buf_rx.lock();
      ps->_buf_rx->write((uint8_t*)data, len);
      ps->_buf_rx.unlock();
      ps->notifyRx();
    });
  }
  c.onDisconnect([ps](void*, AsyncClient* pc) {
    if (!pc) return;
    if (ps->client == pc) {
      ps->client.lock();
      if (ps->client == pc) {
        ps->client = NULL;
      }
      ps->client.unlock();
    }
    if (ps->_onDisconnect_cb) ps->_onDisconnect_cb(ps, pc);

    ps->_delete_pc(pc);  // new from AsyncTCP.cpp:1297
  });
}

}  // namespace

SocketServerResource::SocketServerResource(int port)
    : port(port), server(port) {
  _name = stringf("Port%d", port);
  _mss = WIFI_MSS;
  server.setNoDelay(true);

  AcConnectHandler cb = [this](void*, AsyncClient* pc) {
    if (!pc) return;
    AsyncClient* old_pc = client.locked_set_ptr(pc);

    attachClientEvent(this, pc);

    AsyncClient& c = *pc;
    String s =
        stringf("New client: %s\r\n", c.remoteIP().toString().c_str()) +
        stringf("  %s:%d -> %s:%d\r\n",
                c.remoteIP().toString().c_str(),
                c.remotePort(),
                c.localIP().toString().c_str(),
                c.localPort());
    loga(s);

    if (hasTx()) notifyTx();

    if (old_pc) {
      if (old_pc->connected()) {
        old_pc->write("\r\nDevice is connected on another client.\r\n");
        old_pc->close();
      }
      _delete_pc(old_pc);  // new from AsyncTCP.cpp:1297
    }

    if (_onConnect_cb) _onConnect_cb(this, pc);
  };
  server.onClient(cb, NULL);
}

void SocketServerResource::begin() {
  if (use_tx) {
    _write_init();
    _create_tsk_tx(socketTskTx, TSK_SOCKET_TX_STACK, TSK_SOCKET_TX_PRIORITY);
  }
  if (use_rx) {
    _read_init();
    _create_tsk_rx(socketTskRx, TSK_SOCKET_RX_STACK, TSK_SOCKET_RX_PRIORITY);
  }
  server.begin();
}

// ============
// Export
// ============

void asyncIOSetup() {
  Serial.setRxBufferSize(1024);
}

void loga(String s) {
  Serial.flush();
  Serial.print(s);
}
void logb(String s) {
  for (int i = 0; i < console_sockets.size(); i++) {
    auto& buf = console_sockets[i]->_buf_tx;
    buf.lock();
    buf->write((const uint8_t*)s.c_str(), s.length());
    buf.unlock();
  }
}
void logc(String s) {
  loga(s);
  logb(s);
}
#undef loge
void loge(String s, const char* file_name, size_t line, const char* function) {
  s = stringf(ARDUHAL_LOG_COLOR_E "[E][%s:%u] %s(): %s" ARDUHAL_LOG_RESET_COLOR "\r\n",
              file_name, line, function, s.c_str());
  loga(s);
}
