/* mainthread_hooks.c -- inline-хуки внутри MainThread по смещениям из Ghidra
 *
 * Два режима:
 * 1) mainthread_add_inline_hook(offset, msg) — одна инструкция заменяется на BL к логгеру;
 *    оригинал не выполняется, только лог "дошли сюда" (для бинарного поиска).
 * 2) mainthread_add_inline_hook_wrap(offset, msg_enter, msg_exit) — лог "вход", выполнение
 *    оригинальной инструкции, лог "выход"; по отсутствию "выход" видно, что инструкция упала.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <switch.h>
#include "config.h"
#include "util.h"
#include "so_util.h"

#define PAGE_SIZE 0x1000

/* Мост: страница в virtmem (как .so), в ней stub "LDR x16,#8; BR x16; .quad trampoline".
 * Патч в .so переходит сюда, затем stub переходит в NRO-трамплин. Обходим возможный запрет BR из .so в NRO. */
#define BRIDGE_SLOT_SIZE 16
#define BRIDGE_SLOTS_MAX (PAGE_SIZE / BRIDGE_SLOT_SIZE)

static uintptr_t s_bridge_virt = 0;
static void *s_bridge_src = NULL;
static VirtmemReservation *s_bridge_rv = NULL;
static int s_bridge_slot_count = 0;

static void bridge_write_slot_at(uintptr_t base, int slot_index, uintptr_t trampoline_addr) {
  uint32_t *p = (uint32_t *)(base + (uintptr_t)(slot_index * BRIDGE_SLOT_SIZE));
  p[0] = 0x58000050u;
  p[1] = 0xd61f0220u;
  *(uint64_t *)(p + 2) = trampoline_addr;
}

static uintptr_t get_bridge_page_for_trampoline(uintptr_t trampoline_addr) {
  if (s_bridge_virt != 0)
    return s_bridge_virt;
  virtmemLock();
  void *v = virtmemFindCodeMemory(PAGE_SIZE, PAGE_SIZE);
  if (v) s_bridge_rv = virtmemAddReservation(v, PAGE_SIZE);
  virtmemUnlock();
  if (!v || !s_bridge_rv) return 0;
  s_bridge_virt = (uintptr_t)v;
  s_bridge_src = malloc(PAGE_SIZE);
  if (!s_bridge_src) return 0;
  memset(s_bridge_src, 0, PAGE_SIZE);
  bridge_write_slot_at((uintptr_t)s_bridge_src, 0, trampoline_addr);
  s_bridge_slot_count = 1;
  Result rc = svcMapProcessCodeMemory(envGetOwnProcessHandle(), (u64)s_bridge_virt, (u64)s_bridge_src, PAGE_SIZE);
  if (R_FAILED(rc)) { s_bridge_virt = 0; return 0; }
  rc = svcSetProcessMemoryPermission(envGetOwnProcessHandle(), (u64)s_bridge_virt, PAGE_SIZE, Perm_Rx);
  if (R_FAILED(rc)) { s_bridge_virt = 0; return 0; }
  return s_bridge_virt;
}

/* Выделить слот на bridge-странице под BR из .so → stub (NRO). Без моста переход .so→NRO падает. */
static uintptr_t get_bridge_slot_for_stub(uintptr_t stub_addr) {
  if (s_bridge_virt == 0 || s_bridge_src == NULL) {
    virtmemLock();
    void *v = virtmemFindCodeMemory(PAGE_SIZE, PAGE_SIZE);
    if (v) s_bridge_rv = virtmemAddReservation(v, PAGE_SIZE);
    virtmemUnlock();
    if (!v || !s_bridge_rv) return stub_addr;
    s_bridge_virt = (uintptr_t)v;
    s_bridge_src = malloc(PAGE_SIZE);
    if (!s_bridge_src) return stub_addr;
    memset(s_bridge_src, 0, PAGE_SIZE);
    bridge_write_slot_at((uintptr_t)s_bridge_src, 0, stub_addr);
    s_bridge_slot_count = 1;
    Result rc = svcMapProcessCodeMemory(envGetOwnProcessHandle(), (u64)s_bridge_virt, (u64)s_bridge_src, PAGE_SIZE);
    if (R_FAILED(rc)) return stub_addr;
    rc = svcSetProcessMemoryPermission(envGetOwnProcessHandle(), (u64)s_bridge_virt, PAGE_SIZE, Perm_Rx);
    if (R_FAILED(rc)) return stub_addr;
    return s_bridge_virt;
  }
  if (s_bridge_slot_count >= BRIDGE_SLOTS_MAX)
    return stub_addr;
  int slot = s_bridge_slot_count++;
  Result rc = svcSetProcessMemoryPermission(envGetOwnProcessHandle(), (u64)s_bridge_virt, PAGE_SIZE, Perm_Rw);
  if (R_FAILED(rc)) return stub_addr;
  bridge_write_slot_at(s_bridge_virt, slot, stub_addr);
  svcSetProcessMemoryPermission(envGetOwnProcessHandle(), (u64)s_bridge_virt, PAGE_SIZE, Perm_Rx);
  return s_bridge_virt + (uintptr_t)(slot * BRIDGE_SLOT_SIZE);
}

#define MAINTHREAD_HOOKS_MAX 16
#define MAINTHREAD_WRAP_MAX 32
#define MAINTHREAD_GLOBALS_MAX 8
/* Long-form патч занимает 16 байт; трейс каждые 16 байт. entry_offset задаётся из main (MainThread - load_virtbase). */
#define MAINTHREAD_TRACE_RANGE         0x80u
#define MAINTHREAD_TRACE_STEP          16u
#define MAINTHREAD_TRACE_COUNT         (MAINTHREAD_TRACE_RANGE / MAINTHREAD_TRACE_STEP)

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
extern void mainthread_wrap_stub_8(void), mainthread_wrap_stub_9(void), mainthread_wrap_stub_10(void), mainthread_wrap_stub_11(void);
extern void mainthread_wrap_stub_12(void), mainthread_wrap_stub_13(void), mainthread_wrap_stub_14(void), mainthread_wrap_stub_15(void);
extern void mainthread_wrap_stub_16(void), mainthread_wrap_stub_17(void), mainthread_wrap_stub_18(void), mainthread_wrap_stub_19(void);
extern void mainthread_wrap_stub_20(void), mainthread_wrap_stub_21(void), mainthread_wrap_stub_22(void), mainthread_wrap_stub_23(void);
extern void mainthread_wrap_stub_24(void), mainthread_wrap_stub_25(void), mainthread_wrap_stub_26(void), mainthread_wrap_stub_27(void);
extern void mainthread_wrap_stub_28(void), mainthread_wrap_stub_29(void), mainthread_wrap_stub_30(void), mainthread_wrap_stub_31(void);
extern uint32_t mainthread_wrap_insn_0, mainthread_wrap_insn_1, mainthread_wrap_insn_2, mainthread_wrap_insn_3;
extern uint32_t mainthread_wrap_insn_4, mainthread_wrap_insn_5, mainthread_wrap_insn_6, mainthread_wrap_insn_7;
extern uint32_t mainthread_wrap_insn_8, mainthread_wrap_insn_9, mainthread_wrap_insn_10, mainthread_wrap_insn_11;
extern uint32_t mainthread_wrap_insn_12, mainthread_wrap_insn_13, mainthread_wrap_insn_14, mainthread_wrap_insn_15;
extern uint32_t mainthread_wrap_insn_16, mainthread_wrap_insn_17, mainthread_wrap_insn_18, mainthread_wrap_insn_19;
extern uint32_t mainthread_wrap_insn_20, mainthread_wrap_insn_21, mainthread_wrap_insn_22, mainthread_wrap_insn_23;
extern uint32_t mainthread_wrap_insn_24, mainthread_wrap_insn_25, mainthread_wrap_insn_26, mainthread_wrap_insn_27;
extern uint32_t mainthread_wrap_insn_28, mainthread_wrap_insn_29, mainthread_wrap_insn_30, mainthread_wrap_insn_31;

static void (*const s_wrap_stubs[])(void) = {
  mainthread_wrap_stub_0, mainthread_wrap_stub_1, mainthread_wrap_stub_2, mainthread_wrap_stub_3,
  mainthread_wrap_stub_4, mainthread_wrap_stub_5, mainthread_wrap_stub_6, mainthread_wrap_stub_7,
  mainthread_wrap_stub_8, mainthread_wrap_stub_9, mainthread_wrap_stub_10, mainthread_wrap_stub_11,
  mainthread_wrap_stub_12, mainthread_wrap_stub_13, mainthread_wrap_stub_14, mainthread_wrap_stub_15,
  mainthread_wrap_stub_16, mainthread_wrap_stub_17, mainthread_wrap_stub_18, mainthread_wrap_stub_19,
  mainthread_wrap_stub_20, mainthread_wrap_stub_21, mainthread_wrap_stub_22, mainthread_wrap_stub_23,
  mainthread_wrap_stub_24, mainthread_wrap_stub_25, mainthread_wrap_stub_26, mainthread_wrap_stub_27,
  mainthread_wrap_stub_28, mainthread_wrap_stub_29, mainthread_wrap_stub_30, mainthread_wrap_stub_31,
};
static uint32_t *const s_wrap_insn_slots[] = {
  &mainthread_wrap_insn_0, &mainthread_wrap_insn_1, &mainthread_wrap_insn_2, &mainthread_wrap_insn_3,
  &mainthread_wrap_insn_4, &mainthread_wrap_insn_5, &mainthread_wrap_insn_6, &mainthread_wrap_insn_7,
  &mainthread_wrap_insn_8, &mainthread_wrap_insn_9, &mainthread_wrap_insn_10, &mainthread_wrap_insn_11,
  &mainthread_wrap_insn_12, &mainthread_wrap_insn_13, &mainthread_wrap_insn_14, &mainthread_wrap_insn_15,
  &mainthread_wrap_insn_16, &mainthread_wrap_insn_17, &mainthread_wrap_insn_18, &mainthread_wrap_insn_19,
  &mainthread_wrap_insn_20, &mainthread_wrap_insn_21, &mainthread_wrap_insn_22, &mainthread_wrap_insn_23,
  &mainthread_wrap_insn_24, &mainthread_wrap_insn_25, &mainthread_wrap_insn_26, &mainthread_wrap_insn_27,
  &mainthread_wrap_insn_28, &mainthread_wrap_insn_29, &mainthread_wrap_insn_30, &mainthread_wrap_insn_31,
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

/* Вызывается из asm-трамплина: lr = адрес возврата (patch_addr + 4); при long-form патче (BR) lr от вызывающего — поиск по addr может не сойтись */
void mainthread_log_trampoline_c(uintptr_t lr) {
  uintptr_t patch_addr = lr - 4u;
  debugPrintf("[MT] trampoline lr=0x%lx patch_addr=0x%lx\n", (unsigned long)lr, (unsigned long)patch_addr);
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
    /* Long form: BR в мост (virtmem), мост BR в NRO-трамплин — обход запрета перехода из .so в NRO */
    uintptr_t target = get_bridge_page_for_trampoline((uintptr_t)mainthread_trampoline_asm);
    if (target == 0)
      target = (uintptr_t)mainthread_trampoline_asm;
    else
      debugPrintf("[mainthread_hooks] using bridge page at 0x%lx\n", (unsigned long)target);
    uint32_t *patch = (uint32_t *)addr_w;
    patch[0] = 0x58000051u;
    patch[1] = 0xd61f0220u;
    *(uint64_t *)(patch + 2) = target;
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
    /* BR из .so в NRO падает; переходим через bridge-страницу (virtmem). */
    uintptr_t target = get_bridge_slot_for_stub((uintptr_t)stub);
    uint32_t *patch = (uint32_t *)addr_w;
    patch[0] = 0x58000051u;
    patch[1] = 0xd61f0220u;
    *(uint64_t *)(patch + 2) = target;
  }
}

void mainthread_inline_hooks_apply(void) {
  /* Вызывается после patch_game(); хуки уже добавлены через mainthread_add_inline_hook.
   * Кэши сбросятся в so_flush_caches(). */
  (void)s_num_hooks;
}

/* Регистрирует wrap-хук каждые 16 байт пролога MainThread. entry_offset = адрес MainThread - load_virtbase (из so_find_addr_rx_safe - so_get_load_virtbase). */
void mainthread_add_trace_prologue(uintptr_t entry_offset) {
  static char s_trace_msgs[MAINTHREAD_TRACE_COUNT][20];
  for (unsigned i = 0; i < MAINTHREAD_TRACE_COUNT && s_num_wrap_hooks < MAINTHREAD_WRAP_MAX; i++) {
    uintptr_t off = entry_offset + i * MAINTHREAD_TRACE_STEP;
    (void)snprintf(s_trace_msgs[i], sizeof(s_trace_msgs[i]), "0x%lx", (unsigned long)off);
    mainthread_add_inline_hook_wrap(off, s_trace_msgs[i], s_trace_msgs[i]);
  }
}
