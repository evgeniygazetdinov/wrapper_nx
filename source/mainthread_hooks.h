#ifndef MAINTHREAD_HOOKS_H
#define MAINTHREAD_HOOKS_H

#include <stdint.h>

/* Добавить inline-хук: по смещению от базы .so (как в Ghidra при Image Base 0) пишется лог msg.
 * Одна инструкция (4 байта) в этом месте заменяется на вызов трамплина; оригинал не выполняется. */
void mainthread_add_inline_hook(uintptr_t offset, const char *msg);

/* Обёртка: лог "вход" → выполнение оригинальной инструкции → лог "выход".
 * По отсутствию "выход" в логе видно, что инструкция упала (краш внутри неё или внутри вызванной функции). */
void mainthread_add_inline_hook_wrap(uintptr_t offset, const char *msg_enter, const char *msg_exit);

/* Прочитать 32-битную глобальную переменную по смещению (адрес из Ghidra, например DAT_013654fc = 0x013654fc). */
uint32_t mainthread_read_u32(uintptr_t offset);

/* Зарегистрировать глобальную: при каждом срабатывании inline-хука её значение выводится в лог (удобно для условий вроде (DAT_013654fc & 1) == 0). */
void mainthread_register_global(uintptr_t offset, const char *name);

/* Пустая функция для явного применения (кэши сбрасываются в so_flush_caches). */
void mainthread_inline_hooks_apply(void);

#endif
