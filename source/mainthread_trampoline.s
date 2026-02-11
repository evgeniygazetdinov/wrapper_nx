/* Трамплин для inline-хуков MainThread: сохраняет регистры, вызывает C-лог, восстанавливает, ret */
.arch armv8-a
.global mainthread_trampoline_asm
.type mainthread_trampoline_asm, %function

mainthread_trampoline_asm:
  /* BTI c (0xd503241f): разрешить косвенный BR сюда; без этого переход из .so в NRO может падать */
  .inst 0xd503241f
  /* Сохраняем x0-x18, x30 (lr). 20 * 8 = 160 байт, выравнивание 16 */
  sub  sp, sp, #160
  stp  x0,  x1,  [sp, #0]
  stp  x2,  x3,  [sp, #16]
  stp  x4,  x5,  [sp, #32]
  stp  x6,  x7,  [sp, #48]
  stp  x8,  x9,  [sp, #64]
  stp  x10, x11, [sp, #80]
  stp  x12, x13, [sp, #96]
  stp  x14, x15, [sp, #112]
  stp  x16, x17, [sp, #128]
  str  x18,      [sp, #144]
  str  x30,      [sp, #152]

  mov  x0, x30
  bl   mainthread_log_trampoline_c

  ldp  x0,  x1,  [sp, #0]
  ldp  x2,  x3,  [sp, #16]
  ldp  x4,  x5,  [sp, #32]
  ldp  x6,  x7,  [sp, #48]
  ldp  x8,  x9,  [sp, #64]
  ldp  x10, x11, [sp, #80]
  ldp  x12, x13, [sp, #96]
  ldp  x14, x15, [sp, #112]
  ldp  x16, x17, [sp, #128]
  ldr  x18,      [sp, #144]
  ldr  x30,      [sp, #152]
  add  sp, sp, #160
  ret

/* --- Wrap-хуки: вход → оригинальная инструкция → выход (чтобы по "выход" понимать, что инструкция отработала) --- */
.extern mainthread_wrap_enter_c
.extern mainthread_wrap_exit_c

.macro MAINTHREAD_WRAP_STUB n
  .align 4
  .global mainthread_wrap_stub_\n
  .type   mainthread_wrap_stub_\n, %function
mainthread_wrap_stub_\n:
  .inst 0xd503241f
  sub   sp, sp, #160
  stp   x0,  x1,  [sp, #0]
  stp   x2,  x3,  [sp, #16]
  stp   x4,  x5,  [sp, #32]
  stp   x6,  x7,  [sp, #48]
  stp   x8,  x9,  [sp, #64]
  stp   x10, x11, [sp, #80]
  stp   x12, x13, [sp, #96]
  stp   x14, x15, [sp, #112]
  stp   x16, x17, [sp, #128]
  str   x18,      [sp, #144]
  str   x30,      [sp, #152]
  mov   x0, x30
  bl    mainthread_wrap_enter_c
  ldp   x0,  x1,  [sp, #0]
  ldp   x2,  x3,  [sp, #16]
  ldp   x4,  x5,  [sp, #32]
  ldp   x6,  x7,  [sp, #48]
  ldp   x8,  x9,  [sp, #64]
  ldp   x10, x11, [sp, #80]
  ldp   x12, x13, [sp, #96]
  ldp   x14, x15, [sp, #112]
  ldp   x16, x17, [sp, #128]
  ldr   x18,      [sp, #144]
  ldr   x30,      [sp, #152]
  str   x30,      [sp, #152]
  .global mainthread_wrap_insn_\n
mainthread_wrap_insn_\n:
  .inst  0xd503201f
  ldr   x30,      [sp, #152]
  mov   x0, x30
  bl    mainthread_wrap_exit_c
  ldp   x0,  x1,  [sp, #0]
  ldp   x2,  x3,  [sp, #16]
  ldp   x4,  x5,  [sp, #32]
  ldp   x6,  x7,  [sp, #48]
  ldp   x8,  x9,  [sp, #64]
  ldp   x10, x11, [sp, #80]
  ldp   x12, x13, [sp, #96]
  ldp   x14, x15, [sp, #112]
  ldp   x16, x17, [sp, #128]
  ldr   x18,      [sp, #144]
  ldr   x30,      [sp, #152]
  add   sp, sp, #160
  ret
.endm

MAINTHREAD_WRAP_STUB 0
MAINTHREAD_WRAP_STUB 1
MAINTHREAD_WRAP_STUB 2
MAINTHREAD_WRAP_STUB 3
MAINTHREAD_WRAP_STUB 4
MAINTHREAD_WRAP_STUB 5
MAINTHREAD_WRAP_STUB 6
MAINTHREAD_WRAP_STUB 7
