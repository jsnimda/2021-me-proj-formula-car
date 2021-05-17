#include "debug.h"

#include "Common.h"
#include "esp_panic.h"
#include "esp_task_wdt.h"

// ============
// print backtrace
// ============

/*

// ref: https://github.com/pycom/pycom-esp-idf/blob/idf_v3.2/components/esp32/panic.c
// see line 162
// line 457 doBacktrace()

  see esp_core_dump_write(...)

  XtExcFrame* f = (XtExcFrame*)task_snaphort->stack_start;
  ...stack_start = (uint32_t)tasks[i].pxTopOfStack;
  (volatile StackType_t *pxTopOfStack) is first member of TCB_t

  TCB_t* is TaskHandle_t
  TCB_t can be accessed as StaticTask_t

  pc: function pointer
  a1: sp

*/

namespace {

inline __attribute__((always_inline)) uint32_t disp_pc(uint32_t pc) {
  if (pc & 0x80000000) {
    pc = (pc & 0x3fffffff) | 0x40000000;
  }
  return pc;
}

}  // namespace

void print_backtrace() {
  TaskHandle_t tsk_handle = xTaskGetCurrentTaskHandle();
  StaticTask_t* p_tcb = (StaticTask_t*)tsk_handle;
  // XtExcFrame* frame = (XtExcFrame*)(p_tcb->pxDummy1);

  // not sure how stack frame works, so here just search for the match function address

  uint32_t addr = (intptr_t)__builtin_return_address(0) - 3;
  uint32_t pc = 0, sp = (uint32_t)(p_tcb->pxDummy1) - 0x10 * 0x50;
  uint32_t _sp = sp;

  uint32_t* entry_pairs = new uint32_t[100 * 2];  // pc:sp
  // ^^ has delete on line 103
  size_t ind = 0;
  for (int i = 0; i < 0x100; i++) {
    pc = *((uint32_t*)(sp - 0x10));
    if (disp_pc(pc - 3) == addr) {
      break;
    }
    sp += 0x10;
  }
  if (disp_pc(pc - 3) == addr) {
    for (int i = 0; i < 100; i++) {
      uint32_t psp = sp;
      if (!esp_stack_ptr_is_sane(sp)) {
        break;
      }
      sp = *((uint32_t*)(sp - 0x10 + 4));
      // putEntry(pc - 3, sp) here
      if (i != 0) {  // first and second i is the same pc
        entry_pairs[ind] = pc - 3;
        entry_pairs[ind + 1] = sp;
        ind += 2;
      }
      pc = *((uint32_t*)(psp - 0x10));
      if (pc < 0x40000000) {
        break;
      }
    }
  }

  // we have get all the data we need, now print to screen

  String s = stringf(
      "print_backtrace() was called at PC 0x%08x on core %d\r\n", addr, xPortGetCoreID());

  s += "\r\nBacktrace:";
  if (ind > 0) {
    for (int k = 0; k < ind; k += 2) {
      s += stringf(" 0x%08x:0x%08x", disp_pc(entry_pairs[k]), entry_pairs[k + 1]);
    }
  } else {
    s += " ???";
    s += stringf("\r\n_sp: %08x", _sp);
    s += stringf("\r\nsp: %08x", sp);
    s += stringf("\r\np_tcb->pxDummy1: %08x", (uint32_t)(p_tcb->pxDummy1));
  }
  s += "\r\n";

  loga(s);

  delete[] entry_pairs;
}

// ============
// task info
// ============

std::vector<debug_task_info> dump_task_info() {
  TaskHandle_t current_task = xTaskGetCurrentTaskHandle();
  UBaseType_t n = uxTaskGetNumberOfTasks();
  TaskSnapshot_t* snapshots = new TaskSnapshot_t[n * 2];
  // ^^ has delete on line 137
  UBaseType_t tcb_sz;
  n = uxTaskGetSnapshotAll(snapshots, n * 2, &tcb_sz);

  std::vector<debug_task_info> res;
  for (int i = 0; i < n; i++) {
    TaskHandle_t tskHandle = snapshots[i].pxTCB;
    debug_task_info info{
        .tsk = tskHandle,
        .name = pcTaskGetTaskName(tskHandle),
        .priority = uxTaskPriorityGet(tskHandle),
        .state = eTaskGetState(tskHandle),
        .core_id = xTaskGetAffinity(tskHandle),
        .endOfStack = (uint32_t)(snapshots[i].pxEndOfStack),
        .topOfStack = (uint32_t)(snapshots[i].pxTopOfStack),
        .highWaterMark = (uint32_t)uxTaskGetStackHighWaterMark(tskHandle),
        .stackStart = (uint32_t)pxTaskGetStackStart(tskHandle),
        .wdt_status = esp_task_wdt_status(tskHandle),
        .isCurrentTask = tskHandle == current_task,
    };
    res.push_back(info);
  }

  delete[] snapshots;

  return res;
}

/*

*********************************************************************
Task            Prio  State    Core    StackEnd    StackTop  wdt
                                       HighMark  StackStart
*********************************************************************
1234567890123456  99  Suspended   1  0x7fffffff  0x7fffffff   0   (*)
                                     0x7fffffff  0x7fffffff
*********************************************************************

*********************************************************************
Task            Prio  State  Core  STKSZ    TOP   HIGH    %  wdt
*********************************************************************
1234567890123456  99  Suspe     1  16384  16384  16384   99   0   (*)
*********************************************************************

*/

static const char* eStateText[] = {
    "Running",
    "Ready",
    "Blocked",
    "Suspended",
    "Deleted",
};

String taskInfoRawAll() {
  std::vector<debug_task_info> r = dump_task_info();
  String hr =
      "*********************************************************************";
  String br = "\r\n";
  String title =
      "Task            Prio  State    Core    StackEnd    StackTop  wdt\r\n"
      "                                       HighMark  StackStart";
  String s = hr + br + title + br + hr + br;
  for (int i = 0; i < r.size(); i++) {
    debug_task_info info = r[i];
    char c = (info.wdt_status == ESP_OK) ? '1' : (info.wdt_status == ESP_ERR_NOT_FOUND ? '0' : '?');
    s += stringf("%-16s  %2d  %-9s  %2d  0x%08x  0x%08x   %c   %s\r\n",
                 info.name,
                 info.priority,
                 eStateText[info.state],
                 info.core_id,
                 info.endOfStack,
                 info.topOfStack,
                 c,
                 info.isCurrentTask ? "(*)" : "");
    s += stringf("                                     0x%08x  0x%08x\r\n",
                 info.highWaterMark,
                 info.stackStart);
  }
  s += hr;
  return s;
}

String taskInfoAll() {
  std::vector<debug_task_info> r = dump_task_info();
  String hr =
      "*********************************************************************";
  String br = "\r\n";
  String title =
      "Task            Prio  State  Core  STKSZ    TOP   HIGH    %  wdt";
  String s = hr + br + title + br + hr + br;
  for (int i = 0; i < r.size(); i++) {
    debug_task_info info = r[i];
    char c = (info.wdt_status == ESP_OK) ? '1' : (info.wdt_status == ESP_ERR_NOT_FOUND ? '0' : '?');
    size_t stksz = info.endOfStack - info.stackStart;
    size_t top = info.endOfStack - info.topOfStack;
    size_t high = stksz - info.highWaterMark;
    size_t pct = high * 100 / stksz;
    s += stringf("%-16s  %2d  %-5.5s  %4d %6d %6d %6d  %3d   %c   %s\r\n",
                 info.name,
                 info.priority,
                 eStateText[info.state],
                 info.core_id,
                 stksz,
                 top,
                 high,
                 pct,
                 c,
                 info.isCurrentTask ? "(*)" : "");
  }
  s += hr;
  return s;
}
