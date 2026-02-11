#ifndef __HOOKS_H__
#define __HOOKS_H__

void patch_opengl(void);
void patch_openal(void);
void patch_game(void);
void patch_io(void);
/** Установить TLS (fake_tls) в TPIDR_EL0. Вызывать перед MainThread(), т.к. инициализация .so может сбросить его. */
void game_ensure_tls(void);

void deinit_opengl(void);
void deinit_openal(void);

#endif
