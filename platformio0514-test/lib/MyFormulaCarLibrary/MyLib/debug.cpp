#include "debug.h"

#include "Common.h"
#include "esp_panic.h"

namespace {

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

*/

String s_panicPutHex(int a) {
  String s;
  int x;
  int c;
  for (x = 0; x < 8; x++) {
    c = (a >> 28) & 0xf;
    if (c < 10) {
      s += char('0' + c);
    } else {
      s += char('a' + c - 10);
    }
    a <<= 4;
  }
  return s;
}

String s_putEntry(uint32_t pc, uint32_t sp) {
  if (pc & 0x80000000) {
    pc = (pc & 0x3fffffff) | 0x40000000;
  }
  String s = (" 0x");
  s += s_panicPutHex(pc);
  s += (":0x");
  s += s_panicPutHex(sp);
  return s;
}

}  // namespace

void do_print_backtrace(XtExcFrame* frame) {
  uint32_t i = 0, pc = frame->pc, sp = frame->a1;
  String s = ("\r\nBacktrace:");
  /* Do not check sanity on first entry, PC could be smashed. */
  s += s_putEntry(pc, sp);
  pc = frame->a0;
  while (i++ < 100) {
    uint32_t psp = sp;
    if (!esp_stack_ptr_is_sane(sp) || i++ > 100) {
      break;
    }
    sp = *((uint32_t*)(sp - 0x10 + 4));
    s += s_putEntry(pc - 3, sp);  // stack frame addresses are return addresses, so subtract 3 to get the CALL address
    pc = *((uint32_t*)(psp - 0x10));
    if (pc < 0x40000000) {
      break;
    }
  }
  s += ("\r\n\r\n");

  loga(s);
}

void print_backtrace() {
  String s = stringf("print_backtrace() was called at PC 0x%08x on core %d\r\n",
                     (intptr_t)__builtin_return_address(0) - 3,
                     xPortGetCoreID());

  loga(s);

  TaskHandle_t tsk_handle = xTaskGetCurrentTaskHandle();
  StaticTask_t* p_tcb = (StaticTask_t*)tsk_handle;
  XtExcFrame* frame = (XtExcFrame*)(p_tcb->pxDummy1);

  uint32_t i = 0, pc = frame->pc, sp = frame->a1;
  s += ("\r\nBacktrace:");
  /* Do not check sanity on first entry, PC could be smashed. */
  s += s_putEntry(pc, sp);
  pc = frame->a0;
  while (i++ < 100) {
    uint32_t psp = sp;
    if (!esp_stack_ptr_is_sane(sp) || i++ > 100) {
      break;
    }
    sp = *((uint32_t*)(sp - 0x10 + 4));
    s += s_putEntry(pc - 3, sp);  // stack frame addresses are return addresses, so subtract 3 to get the CALL address
    pc = *((uint32_t*)(psp - 0x10));
    if (pc < 0x40000000) {
      break;
    }
  }
  s += ("\r\n\r\n");

  loga(s);
}
