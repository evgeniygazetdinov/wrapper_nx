/* exception.c — кастомный обработчик исключений libnx для логирования падения.
 * При краше в MainThread (или где угодно) вызывается __libnx_exception_handler;
 * в лог пишутся: тип исключения, PC (сбойная инструкция), FAR (адрес доступа при data abort), ESR. */

#include <switch.h>
#include <switch/arm/thread_context.h>
#include <stdio.h>

#include "util.h"
#include "so_util.h"

static const char *exception_desc_str(unsigned desc) {
  switch (desc) {
    case 0x100: return "InstructionAbort";
    case 0x101: return "Other";
    case 0x102: return "MisalignedPC";
    case 0x103: return "MisalignedSP";
    case 0x104: return "Trap";
    case 0x106: return "SError";
    case 0x301: return "BadSVC";
    default:    return "?";
  }
}

/* ESR: EC bits [31:26]; Data Abort EL0 = 0x24/0x25, Instruction Abort EL0 = 0x20/0x21. */
static void log_esr(unsigned esr) {
  unsigned ec = esr >> 26;
  unsigned iss = esr & 0x1ffffff;
  debugPrintf("[fatal] ESR ec=0x%02x iss=0x%05x", ec, iss);
  if (ec == 0x24 || ec == 0x25) debugPrintf(" (DataAbort)");
  else if (ec == 0x20 || ec == 0x21) debugPrintf(" (InstructionAbort)");
  debugPrintf("\n");
}

void __libnx_exception_handler(ThreadExceptionDump *ctx) {
  uintptr_t base = so_get_load_virtbase();
  /* Адрес этого обработчика — код NRO; по нему видно, в том ли мы модуле что и PC. */
  uintptr_t nro_marker = (uintptr_t)&__libnx_exception_handler;

  debugPrintf("\n[fatal] *** EXCEPTION ***\n");
  debugPrintf("[fatal] error_desc=0x%x (%s)\n", ctx->error_desc, exception_desc_str(ctx->error_desc));
  debugPrintf("[fatal] PC (faulting instruction) = 0x%lx\n", (unsigned long)ctx->pc.x);
  debugPrintf("[fatal] FAR (fault address)        = 0x%lx\n", (unsigned long)ctx->far.x);
  debugPrintf("[fatal] NRO: адрес кода обёртки (exception_handler) = 0x%lx — если PC рядом, краш в нашем NRO\n",
              (unsigned long)nro_marker);
  /* LR = адрес возврата; если контекст корректен, это код до падения. Смещения от приближённой базы NRO — для Ghidra. */
  {
    uintptr_t nro_base_approx = nro_marker & ~(uintptr_t)0xFFFF;
    uintptr_t lr_off = (uintptr_t)ctx->lr.x - nro_base_approx;
    uintptr_t far_off = (uintptr_t)ctx->far.x - nro_base_approx;
    const uintptr_t nro_size_max = 0x800000u; /* типичный размер NRO до 8 MB */
    int lr_in_nro = (ctx->lr.x >= nro_base_approx && lr_off <= nro_size_max);
    int far_in_nro = (ctx->far.x >= nro_base_approx && far_off <= nro_size_max);
    debugPrintf("[fatal] NRO_base_approx=0x%lx (Image Base в Ghidra для project.elf)\n", (unsigned long)nro_base_approx);
    if (lr_in_nro || far_in_nro)
      debugPrintf("[fatal] LR_offset=0x%lx  FAR_offset=0x%lx — в Ghidra перейди по адресу (base+offset) или по смещению\n",
                  (unsigned long)lr_off, (unsigned long)far_off);
    else
      debugPrintf("[fatal] LR/FAR вне NRO (контекст повреждён или краш в другом модуле). FAR=0x%lx — ищи в .so/библиотеках по этому адресу\n",
                  (unsigned long)ctx->far.x);
  }
  if (base != 0) {
    uintptr_t pc_off = (uintptr_t)ctx->pc.x - base;
    uintptr_t far_off = (uintptr_t)ctx->far.x - base;
    /* Смещение > 256 MB или переполнение — PC/FAR не в .so (NRO, heap, другая библиотека). */
    const uintptr_t so_size_max = 0x10000000u;
    int in_so = (ctx->pc.x >= base && pc_off <= so_size_max && ctx->far.x >= base && far_off <= so_size_max);
    if (in_so)
      debugPrintf("[fatal] (load_virtbase=0x%lx) PC_offset=0x%lx FAR_offset=0x%lx (для Ghidra .so)\n",
                  (unsigned long)base, (unsigned long)pc_off, (unsigned long)far_off);
    else
      debugPrintf("[fatal] PC/FAR не в .so (base=0x%lx) — ищи по абсолютному адресу (NRO/библиотека/heap)\n",
                  (unsigned long)base);
  }
  debugPrintf("[fatal] SP  = 0x%lx  LR = 0x%lx\n", (unsigned long)ctx->sp.x, (unsigned long)ctx->lr.x);
  log_esr(ctx->esr);
  debugPrintf("[fatal] *** end dump ***\n\n");
  fflush(stdout);
}
