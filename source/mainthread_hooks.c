/* mainthread_hooks.c -- inline-хуки внутри MainThread по смещениям из Ghidra
 *
 * Два режима:
 * 1) mainthread_add_inline_hook(offset, msg) — одна инструкция заменяется на BL к логгеру;
 *    оригинал не выполняется, только лог "дошли сюда" (для бинарного поиска).
 * 2) mainthread_add_inline_hook_wrap(offset, msg_enter, msg_exit) — лог "вход", выполнение
 *    оригинальной инструкции, лог "выход"; по отсутствию "выход" видно, что инструкция упала.
 */

#include <stdint.h>
#include <string.h>
#include <switch.h>
#include "config.h"
#include "util.h"
#include "so_util.h"

#define PAGE_SIZE 0x1000

#define MAINTHREAD_HOOKS_MAX 16
#define MAINTHREAD_WRAP_MAX 8
#define MAINTHREAD_GLOBALS_MAX 8

typedef struct {
  uintptr_t addr;
  const char *msg;
} MainthreadHook;

typedef struct {
  uintptr_t patch_addr;
  const char *msg_enter;
  const char *msg_exit;
} MainthreadWrapHook;

typedef struct {
  uintptr_t offset;
  const char *name;
} MainthreadGlobal;

static MainthreadHook s_hooks[MAINTHREAD_HOOKS_MAX];
static int s_num_hooks;

static MainthreadWrapHook s_wrap_hooks[MAINTHREAD_WRAP_MAX];
static int s_num_wrap_hooks;

static MainthreadGlobal s_globals[MAINTHREAD_GLOBALS_MAX];
static int s_num_globals;

/* Точки входа и слоты для оригинальной инструкции (asm) */
extern void mainthread_wrap_stub_0(void), mainthread_wrap_stub_1(void), mainthread_wrap_stub_2(void), mainthread_wrap_stub_3(void);
extern void mainthread_wrap_stub_4(void), mainthread_wrap_stub_5(void), mainthread_wrap_stub_6(void), mainthread_wrap_stub_7(void);
extern uint32_t mainthread_wrap_insn_0, mainthread_wrap_insn_1, mainthread_wrap_insn_2, mainthread_wrap_insn_3;
extern uint32_t mainthread_wrap_insn_4, mainthread_wrap_insn_5, mainthread_wrap_insn_6, mainthread_wrap_insn_7;

static void (*const s_wrap_stubs[])(void) = {
  mainthread_wrap_stub_0, mainthread_wrap_stub_1, mainthread_wrap_stub_2, mainthread_wrap_stub_3,
  mainthread_wrap_stub_4, mainthread_wrap_stub_5, mainthread_wrap_stub_6, mainthread_wrap_stub_7,
};
static uint32_t *const s_wrap_insn_slots[] = {
  &mainthread_wrap_insn_0, &mainthread_wrap_insn_1, &mainthread_wrap_insn_2, &mainthread_wrap_insn_3,
  &mainthread_wrap_insn_4, &mainthread_wrap_insn_5, &mainthread_wrap_insn_6, &mainthread_wrap_insn_7,
};

/* Прочитать 32-битную глобальную переменную по смещению от базы .so (адрес из Ghidra при Image Base 0). */
uint32_t mainthread_read_u32(uintptr_t offset) {
  uintptr_t base = so_get_load_virtbase();
  if (base == 0) return 0;
  return *(uint32_t *)(base + offset);
}

/* Зарегистрировать глобальную переменную: при каждом срабатывании inline-хука её значение будет выведено в лог.
 * Удобно проверять условия вроде (DAT_013654fc & 1) == 0 — в логе будет значение и bit0. */
void mainthread_register_global(uintptr_t offset, const char *name) {
  if (s_num_globals >= MAINTHREAD_GLOBALS_MAX || name == NULL) return;
  s_globals[s_num_globals].offset = offset;
  s_globals[s_num_globals].name = name;
  s_num_globals++;
}

/* Вывести в лог значения всех зарегистрированных глобальных переменных. */
static void mainthread_log_registered_globals(void) {
  uintptr_t base = so_get_load_virtbase();
  if (base == 0) return;
  for (int i = 0; i < s_num_globals; i++) {
    uint32_t v = *(uint32_t *)(base + s_globals[i].offset);
    debugPrintf("  %s = 0x%x (bit0=%u) -> (%u & 1)==0 is %s\n",
      s_globals[i].name, v, v & 1u, v & 1u, (v & 1u) == 0 ? "true" : "false");
  }
}

/* Вызывается из asm-трамплина: lr = адрес возврата (patch_addr + 4) */
void mainthread_log_trampoline_c(uintptr_t lr) {
  uintptr_t patch_addr = lr - 4u;
  for (int i = 0; i < s_num_hooks; i++) {
    if (s_hooks[i].addr == patch_addr) {
      debugPrintf("[MT] %s (addr 0x%lx)\n", s_hooks[i].msg, (unsigned long)patch_addr);
      mainthread_log_registered_globals();
      return;
    }
  }
  debugPrintf("[MT] ? (addr 0x%lx)\n", (unsigned long)patch_addr);
  mainthread_log_registered_globals();
}

/* Вызывается из wrap-трамплина до выполнения оригинальной инструкции. lr = patch_addr + 4 */
void mainthread_wrap_enter_c(uintptr_t lr) {
  uintptr_t patch_addr = lr - 4u;
  for (int i = 0; i < s_num_wrap_hooks; i++) {
    if (s_wrap_hooks[i].patch_addr == patch_addr) {
      debugPrintf("[MT wrap] ENTER %s (addr 0x%lx)\n", s_wrap_hooks[i].msg_enter, (unsigned long)patch_addr);
      mainthread_log_registered_globals();
      return;
    }
  }
  debugPrintf("[MT wrap] ENTER ? (addr 0x%lx)\n", (unsigned long)patch_addr);
  mainthread_log_registered_globals();
}

/* Вызывается из wrap-трамплина после выполнения оригинальной инструкции. Если её не видно — инструкция упала. */
void mainthread_wrap_exit_c(uintptr_t lr) {
  uintptr_t patch_addr = lr - 4u;
  for (int i = 0; i < s_num_wrap_hooks; i++) {
    if (s_wrap_hooks[i].patch_addr == patch_addr) {
      debugPrintf("[MT wrap] EXIT  %s (addr 0x%lx)\n", s_wrap_hooks[i].msg_exit, (unsigned long)patch_addr);
      mainthread_log_registered_globals();
      return;
    }
  }
  debugPrintf("[MT wrap] EXIT  ? (addr 0x%lx)\n", (unsigned long)patch_addr);
  mainthread_log_registered_globals();
}

/* Трамплин: сохраняет регистры, вызывает mainthread_log_trampoline_c(lr), восстанавливает, ret */
extern void mainthread_trampoline_asm(void);

void mainthread_add_inline_hook(uintptr_t offset, const char *msg) {
  if (s_num_hooks >= MAINTHREAD_HOOKS_MAX)
    return;
  uintptr_t base_w = so_get_load_base();     /* куда писать патч (до so_finalize — буфер) */
  uintptr_t base_v = so_get_load_virtbase(); /* runtime-адрес для сравнения в трамплине */
  if (base_w == 0 || base_v == 0)
    return;
  uintptr_t addr_w = base_w + offset;
  uintptr_t addr_v = base_v + offset;
  s_hooks[s_num_hooks].addr = addr_v;
  s_hooks[s_num_hooks].msg = msg;
  s_num_hooks++;

  /* BL rel: encoding 0x94000000 | ((rel/4) & 0x03FFFFFF), rel = (trampoline - (addr+4)), signed 26 bits * 4 */
  int64_t rel = (int64_t)((char *)mainthread_trampoline_asm - (char *)(addr_v + 4));
  if (rel >= (int64_t)(-0x08000000) && rel < (int64_t)0x08000000) {
    uint32_t bl = 0x94000000u | ((uint32_t)(rel >> 2) & 0x03FFFFFFu);
    *(uint32_t *)addr_w = bl;
  } else {
    uint32_t *patch = (uint32_t *)addr_w;
    patch[0] = 0x58000051u;
    patch[1] = 0xd61f0220u;
    *(uint64_t *)(patch + 2) = (uint64_t)(uintptr_t)mainthread_trampoline_asm;
  }
}

/* Обёртка: лог "вход" → выполнение оригинальной инструкции → лог "выход". По отсутствию "выход" видно, что инструкция упала. */
void mainthread_add_inline_hook_wrap(uintptr_t offset, const char *msg_enter, const char *msg_exit) {
  if (s_num_wrap_hooks >= MAINTHREAD_WRAP_MAX || msg_enter == NULL || msg_exit == NULL)
    return;
  uintptr_t base_w = so_get_load_base();
  uintptr_t base_v = so_get_load_virtbase();
  if (base_w == 0 || base_v == 0)
    return;
  uintptr_t addr_w = base_w + offset;
  uintptr_t addr_v = base_v + offset;
  uint32_t orig_insn = *(uint32_t *)addr_w;

  int slot = s_num_wrap_hooks;

  /* Слот инструкции в .text (RX); временно делаем страницу Rw для записи оригинальной инструкции */
  {
    uintptr_t slot_addr = (uintptr_t)s_wrap_insn_slots[slot];
    uintptr_t page_addr = slot_addr & ~(uintptr_t)(PAGE_SIZE - 1);
    Result rc = svcSetProcessMemoryPermission(envGetOwnProcessHandle(), page_addr, PAGE_SIZE, Perm_Rw);
    if (R_SUCCEEDED(rc)) {
      *s_wrap_insn_slots[slot] = orig_insn;
      svcSetProcessMemoryPermission(envGetOwnProcessHandle(), page_addr, PAGE_SIZE, Perm_Rx);
    }
    /* при ошибке слот остаётся NOP — wrap всё равно установится, но выполнится NOP вместо оригинала */
  }
  s_wrap_hooks[s_num_wrap_hooks].patch_addr = addr_v;
  s_wrap_hooks[s_num_wrap_hooks].msg_enter = msg_enter;
  s_wrap_hooks[s_num_wrap_hooks].msg_exit = msg_exit;
  s_num_wrap_hooks++;

  void *stub = (void *)s_wrap_stubs[slot];
  int64_t rel = (int64_t)((char *)stub - (char *)(addr_v + 4));
  if (rel >= (int64_t)(-0x08000000) && rel < (int64_t)0x08000000) {
    uint32_t bl = 0x94000000u | ((uint32_t)(rel >> 2) & 0x03FFFFFFu);
    *(uint32_t *)addr_w = bl;
  } else {
    uint32_t *patch = (uint32_t *)addr_w;
    patch[0] = 0x58000051u;
    patch[1] = 0xd61f0220u;
    *(uint64_t *)(patch + 2) = (uint64_t)(uintptr_t)stub;
  }
}

void mainthread_inline_hooks_apply(void) {
  /* Вызывается после patch_game(); хуки уже добавлены через mainthread_add_inline_hook.
   * Кэши сбросятся в so_flush_caches(). */
  (void)s_num_hooks;
}
