#include "AsyncIO.h"

#define ASYNC_TX_TASK_PRIORITY 5
#define ASYNC_RX_TASK_PRIORITY 5
#define ASYNC_SERIAL_RX_TASK_PRIORITY 4

// TODO: reduce the use of heap (use static buffer instead of new)

// ============
// Extern
// ============

AsyncHardwareSerial AsyncSerial(Serial);

// ============
// Main
// ============

namespace {

// handle data in buffer that is being to send
TaskHandle_t taskAsyncTx_handle;
// handle onRead redirected by another task
TaskHandle_t taskAsyncRx_handle;
// handle Serial.available()
TaskHandle_t taskAsyncSerialRx_handle;

QueueHandle_t queueTx;

enum DataType {
  TYPE_UNKNOWN,
  TYPE_TX_SERIAL,
};

typedef struct {
  uint8_t* data;
  size_t len;
  DataType type;
  void* target;
} QueueTxData;

void asyncTxTask(void* args) {
  for (;;) {
    QueueTxData txData;
    xQueueReceive(queueTx, &txData, portMAX_DELAY);
    if (txData.type == TYPE_TX_SERIAL) {
      Serial.flush();
      Serial.write(txData.data, txData.len);
    }
    delete[] txData.data;
  }
}

void asyncRxTask(void* args) {
  for (;;) {
    delay(10000);
  }
}
void asyncSerialRxTask(void* args) {
  for (;;) {
    int len;
    while ((len = Serial.available())) {
      uint8_t* data = new uint8_t[len];
      Serial.read(data, len);
      if (AsyncSerial._onData_cb) {
        AsyncSerial._onData_cb(data, len);
      }
      delete[] data;
    }
    delay(20);  // 50 Hz
    // 115200 bps * 20 ms = 288 bytes
  }
}

}  // namespace

// ============
// Export
// ============

inline void AsyncHardwareSerial::attachReader(AsyncStreamDataHandler onData) {
  AsyncStream::attachReader(onData);
  if (!taskAsyncSerialRx_handle) {
    Serial.setRxBufferSize(1024);  // default is 64 or 128
    xTaskCreatePinnedToCore(asyncSerialRxTask, "asyncSerialRxTask", 8192, NULL, ASYNC_SERIAL_RX_TASK_PRIORITY, &taskAsyncSerialRx_handle, 0);
  }
}

// TODO: resolve race condition
inline size_t AsyncHardwareSerial::write(const uint8_t* buffer, size_t size) {
  uint8_t* new_data = new uint8_t[size];  // need to delete this in asyncTxTask
  memcpy(new_data, buffer, size);
  QueueTxData txData = {
    .data = new_data,
    .len = size,
    .type = TYPE_TX_SERIAL,
    .target = &Serial,
  };
  if (xQueueSend(queueTx, &txData, 0) != pdTRUE) {
    // fail, full?
    // todo
    return 0;
  }
  return size;
}

void asyncIOSetup() {
  if (!queueTx) {
    queueTx = xQueueCreate(32, sizeof(QueueTxData));
  }
  if (!taskAsyncTx_handle) {
    xTaskCreatePinnedToCore(asyncTxTask, "asyncTxTask", 8192, NULL, ASYNC_TX_TASK_PRIORITY, &taskAsyncTx_handle, 0);
  }
  if (!taskAsyncRx_handle) {
    xTaskCreatePinnedToCore(asyncRxTask, "asyncRxTask", 8192, NULL, ASYNC_RX_TASK_PRIORITY, &taskAsyncRx_handle, 0);
  }
}