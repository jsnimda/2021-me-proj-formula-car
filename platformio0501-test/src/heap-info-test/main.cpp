/*

  Conslusion:
    Psram always 0
    esp_get_free_internal_heap_size() equals to esp_get_free_heap_size()
    ESP.getXXX() and esp_get_XXX() is not the same

    there are two types of memory:
      MALLOC_CAP_8BIT  (DRAM) = MALLOC_CAP_DEFAULT = esp_get_free_heap_size()
      MALLOC_CAP_32BIT (DRAM+IRAM) = MALLOC_CAP_INTERNAL = ESP.getFreeHeap()

    heap size (will vary):
      MALLOC_CAP_32BIT: about 356544 / 356548
      MALLOC_CAP_8BIT: about 287896

    ref: https://esp32.com/viewtopic.php?t=3802
    64kB of IRAM for CPU caches

*/

#include <Arduino.h>
#include <AsyncTCP.h>
#include <WiFi.h>

#include "WiFiConnection.h"

AsyncServer server(23);      // notice!! args is different
AsyncClient* pServerClient;  // only allow one client at a time

void ser_setup() {
  //check if there are any new clients
  server.onClient([](void*, AsyncClient* client) {}, NULL);
}

void setup() {
  Serial.begin(115200);
  wifiConnectionSetup();
  ser_setup();
  server.begin();
  server.setNoDelay(true);
}

#define dump(x)          \
  Serial.flush();        \
  Serial.print(#x ": "); \
  Serial.println(x);

void info_1() {
  // system_get_free_heap_size = xPortGetFreeHeapSize = esp_get_free_heap_size = esp_get_free_internal_heap_size
  dump(system_get_free_heap_size());
  dump(xPortGetFreeHeapSize());
  dump(esp_get_free_heap_size());
  dump(esp_get_minimum_free_heap_size());
  dump(esp_get_free_internal_heap_size());

  Serial.println();
  dump(ESP.getFreeHeap());  // heap_caps_get_free_size(MALLOC_CAP_INTERNAL)
  dump(ESP.getFreePsram());
  dump(ESP.getFreeSketchSpace());
  dump(ESP.getHeapSize());
  dump(ESP.getPsramSize());
  dump(ESP.getMaxAllocHeap());
  dump(ESP.getMaxAllocPsram());
  dump(ESP.getMinFreeHeap());
  dump(ESP.getMinFreePsram());

  Serial.println();
  dump(heap_caps_get_free_size(MALLOC_CAP_DEFAULT));

  Serial.println("============");
}

void info_2() {
  dump(esp_get_free_heap_size());
  dump(esp_get_minimum_free_heap_size());

  Serial.println();
  dump(ESP.getFreeHeap());  // heap_caps_get_free_size(MALLOC_CAP_INTERNAL)
  dump(ESP.getHeapSize());
  dump(ESP.getMaxAllocHeap());
  dump(ESP.getMinFreeHeap());

  Serial.println();
  dump(heap_caps_get_free_size(MALLOC_CAP_DEFAULT));   // = esp_get_free_heap_size()
  dump(heap_caps_get_free_size(MALLOC_CAP_INTERNAL));  // = ESP.getFreeHeap()
  dump(heap_caps_get_free_size(MALLOC_CAP_SPIRAM));    // = 0
  dump(heap_caps_get_free_size(MALLOC_CAP_INVALID));   // = 0
  Serial.println();
  dump(heap_caps_get_free_size(MALLOC_CAP_EXEC));
  dump(heap_caps_get_free_size(MALLOC_CAP_32BIT));  // = MALLOC_CAP_INTERNAL
  dump(heap_caps_get_free_size(MALLOC_CAP_8BIT));   // = MALLOC_CAP_DEFAULT
  dump(heap_caps_get_free_size(MALLOC_CAP_DMA));    // = MALLOC_CAP_DEFAULT
  dump(heap_caps_get_free_size(MALLOC_CAP_PID2));   // all 0
  dump(heap_caps_get_free_size(MALLOC_CAP_PID3));   // all 0
  dump(heap_caps_get_free_size(MALLOC_CAP_PID4));   // all 0
  dump(heap_caps_get_free_size(MALLOC_CAP_PID5));   // all 0
  dump(heap_caps_get_free_size(MALLOC_CAP_PID6));   // all 0
  dump(heap_caps_get_free_size(MALLOC_CAP_PID7));   // all 0
  Serial.println("============");
}

uint32_t getHeapSize(uint32_t MALLOC_CAP) {
  multi_heap_info_t info;
  heap_caps_get_info(&info, MALLOC_CAP);
  return info.total_free_bytes + info.total_allocated_bytes;
}

uint32_t getTotalBlocks(uint32_t MALLOC_CAP) {
  multi_heap_info_t info;
  heap_caps_get_info(&info, MALLOC_CAP);
  return info.total_blocks;
}

void info_3() {
  dump(heap_caps_get_free_size(MALLOC_CAP_32BIT));
  dump(heap_caps_get_minimum_free_size(MALLOC_CAP_32BIT));
  dump(heap_caps_get_largest_free_block(MALLOC_CAP_32BIT));
  dump(getHeapSize(MALLOC_CAP_32BIT));
  dump(getTotalBlocks(MALLOC_CAP_32BIT));
  Serial.println();
  dump(heap_caps_get_free_size(MALLOC_CAP_8BIT));  // remaining ram
  dump(heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT));
  dump(heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
  dump(getHeapSize(MALLOC_CAP_8BIT));  // total ram
  dump(getTotalBlocks(MALLOC_CAP_8BIT));
  Serial.println("============");
}

void loop() {
  Serial.println();
  info_3();
  delay(10 * 1000);
}
