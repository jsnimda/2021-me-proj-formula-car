#include "debug.h"

#include "Common.h"
#include "esp_panic.h"

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
  uint32_t pc = 0, sp = (uint32_t)(p_tcb->pxDummy1);
  uint32_t _sp = sp;

  uint32_t* entry_pairs = new uint32_t[100 * 2];  // pc:sp
  size_t ind = 0;
  for (int i = 0; i < 1000; i++) {
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
      entry_pairs[ind] = pc - 3;
      entry_pairs[ind + 1] = sp;
      ind += 2;
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
    s += stringf("\r\np_tcb->pxDummy1: %08x", p_tcb->pxDummy1);
  }
  s += "\r\n";

  loga(s);
}
