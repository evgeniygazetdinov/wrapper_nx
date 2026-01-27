/* main.c
 *
 * Copyright (C) 2021 fgsfds, Andy Nguyen
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <libgen.h>
#include <switch.h>

#include "config.h"
#include "util.h"
#include "error.h"
#include "so_util.h"
#include "hooks.h"
#include "imports.h"

static void *heap_so_base = NULL;
static size_t heap_so_limit = 0;

// provide replacement heap init function to separate newlib heap from the .so
void __libnx_initheap(void) {
  void *addr;
  size_t size = 0, fake_heap_size = 0;
  size_t mem_available = 0, mem_used = 0;

  if (envHasHeapOverride()) {
    addr = envGetHeapOverrideAddr();
    size = envGetHeapOverrideSize();
  } else {
    svcGetInfo(&mem_available, InfoType_TotalMemorySize, CUR_PROCESS_HANDLE, 0);
    svcGetInfo(&mem_used, InfoType_UsedMemorySize, CUR_PROCESS_HANDLE, 0);
    if (mem_available > mem_used + 0x200000)
      size = (mem_available - mem_used - 0x200000) & ~0x1FFFFF;
    if (size == 0)
      size = 0x2000000 * 16;
    Result rc = svcSetHeapSize(&addr, size);
    if (R_FAILED(rc))
      diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_HeapAllocFailed));
  }

  // only allocate a fixed amount for the newlib heap
  extern char *fake_heap_start;
  extern char *fake_heap_end;
  fake_heap_size  = umin(size, MEMORY_MB * 1024 * 1024);
  fake_heap_start = (char *)addr;
  fake_heap_end   = (char *)addr + fake_heap_size;

  heap_so_base = (char *)addr + fake_heap_size;
  heap_so_base = (void *)ALIGN_MEM((uintptr_t)heap_so_base, 0x1000); // align to page size
  heap_so_limit = (char *)addr + size - (char *)heap_so_base;
}

static void check_data(void) {
  const char *files[] = {
    // Main data directory (bullyorig folder from data_0.zip)
    "bullyorig",
    // Essential files in bullyorig root
    "bullyorig/version.cfg",
    "bullyorig/runtime.xml",
    "bullyorig/memory.xml",
    // Essential directories
    "bullyorig/config",
    "bullyorig/models",
    "bullyorig/audio",
    "bullyorig/data",
    // Additional data from data_1.zip (bully folder)
    "bully",
    // mod file goes here
    "",
  };
  struct stat st;
  unsigned int numfiles = (sizeof(files) / sizeof(*files)) - 1;
  char cwd_buf[512];
  if (getcwd(cwd_buf, sizeof(cwd_buf)) == NULL)
    cwd_buf[0] = '\0';
  // if mod is enabled, also check for mod file
  if (config.mod_file[0])
    files[numfiles++] = config.mod_file;
  // check if all the required files are present
  for (unsigned int i = 0; i < numfiles; ++i) {
    if (stat(files[i], &st) < 0) {
      const char *cwd_str = cwd_buf[0] ? cwd_buf : "(getcwd failed)";
      debugPrintf("[check_data] FAIL %s | cwd=%s\n", files[i], cwd_str);
      fatal_error_with_cwd(cwd_str,
        "Could not find: %s\n\nCheck your data files. Set data_path in config.txt to the directory containing bullyorig, or run the .nro from that directory.\n\nExtract data_0.zip and data_1.zip from split_data_1.apk into the game directory.",
        files[i]);
      break;
    }
  }
}

static void check_syscalls(void) {
  if (!envIsSyscallHinted(0x77))
    fatal_error("svcMapProcessCodeMemory is unavailable.");
  if (!envIsSyscallHinted(0x78))
    fatal_error("svcUnmapProcessCodeMemory is unavailable.");
  if (!envIsSyscallHinted(0x73))
    fatal_error("svcSetProcessMemoryPermission is unavailable.");
  if (envGetOwnProcessHandle() == INVALID_HANDLE)
    fatal_error("Own process handle is unavailable.");
}

static void set_screen_size(int w, int h) {
  if (w <= 0 || h <= 0 || w > 1920 || h > 1080) {
    // auto; pick resolution based on docked mode
    if (appletGetOperationMode() == AppletOperationMode_Console) {
      screen_width = 1920;
      screen_height = 1080;
    } else {
      screen_width = 1280;
      screen_height = 720;
    }
  } else {
    screen_width = w;
    screen_height = h;
  }
  debugPrintf("screen mode: %dx%d\n", screen_width, screen_height);
}

int main(int argc, char **argv) {
  /* Сначала перейти в каталог с .nro, чтобы читать config.txt оттуда (на Switch cwd часто родительский) */
  if (argc > 0 && argv && argv[0] && argv[0][0]) {
    char nro_dir[512];
    strlcpy(nro_dir, argv[0], sizeof(nro_dir));
    char *dir = dirname(nro_dir);
    if (dir && dir[0] && chdir(dir) == 0) {
      /* теперь cwd = каталог с .nro, config.txt и bullyorig должны быть здесь */
    }
  }

  if (read_config(CONFIG_NAME) < 0)
    write_config(CONFIG_NAME);

  check_syscalls();

  if (config.data_path[0]) {
    if (chdir(config.data_path) != 0)
      fatal_error("Cannot chdir to data_path:\n%s\nCheck config.txt.", config.data_path);
  }
  check_data();

  // calculate actual screen size
  set_screen_size(config.screen_width, config.screen_height);

  debugPrintf("heap size = %u KB\n", MEMORY_MB * 1024);
  debugPrintf(" lib base = %p\n", heap_so_base);
  debugPrintf("  lib max = %u KB\n", heap_so_limit / 1024);

  if (so_load(SO_NAME, heap_so_base, heap_so_limit) < 0)
    fatal_error("Could not load\n%s.", SO_NAME);

  // won't save without it
  mkdir("savegames", 0777);

  update_imports();

  so_relocate();
  // Resolve imports first - this will handle InitializeCriticalSection and other Windows API functions
  so_resolve(dynlib_functions, dynlib_numfunctions, 1);

  // Apply patches after imports are resolved
  debugPrintf("[main] patch_openal\n");
  patch_openal();
  debugPrintf("[main] patch_opengl\n");
  patch_opengl();
  debugPrintf("[main] patch_game\n");
  patch_game();
  debugPrintf("[main] patches done\n");

  // can't set it in the initializer because it's not constant
  stderr_fake = stderr;

  // Set storage root path for Bully (replaces StorageRootBuffer from Max Payne)
  // CRITICAL: мы пишем по этому адресу — без символа будет краш в strcpy
  uintptr_t storage_root = so_find_addr_safe("StorageRootPath");
  if (!storage_root)
    fatal_error("StorageRootPath not found.\nCheck your .so file.");
  strcpy((char *)storage_root, ".");
  debugPrintf("[main] StorageRootPath ok\n");

  // Try to find Bully entry points
  void* (* MainThread)(void*) = (void *)so_find_addr_rx_safe("_Z10MainThreadPv");
  void (* AND_ThreadOnMain)(void) = (void *)so_find_addr_rx_safe("_Z16AND_ThreadOnMainv");
  void (* AND_GameStartupDone)(void) = (void *)so_find_addr_rx_safe("_Z19AND_GameStartupDonev");
  void (* implOnInitialSetup)(void) = (void *)so_find_addr_rx_safe("Java_com_rockstargames_oswrapper_GameNative_implOnInitialSetup");
  if (!MainThread)
    fatal_error("MainThread not found.\nCheck your .so file.");

  debugPrintf("[main] so_finalize\n");
  so_finalize();
  so_flush_caches();

  so_execute_init_array();

  so_free_temp();

  // Initialize Android thread on main thread
  if (AND_ThreadOnMain) {
    debugPrintf("Calling AND_ThreadOnMain()\n");
    AND_ThreadOnMain();
  }

  // Try JNI initial setup first (if available)
  if (implOnInitialSetup) {
    debugPrintf("Calling implOnInitialSetup()\n");
    implOnInitialSetup();
  }

  // Start main game thread
  if (MainThread) {
    debugPrintf("Starting MainThread()\n");
    // MainThread might be blocking, so we call it directly
    // If it blocks, we might need to run it in a separate thread
    MainThread(NULL);
  } else {
    fatal_error("Could not find\nMainThread function.\nCheck your .so file.");
  }

  // Signal game startup done (if available)
  if (AND_GameStartupDone) {
    debugPrintf("Calling AND_GameStartupDone()\n");
    AND_GameStartupDone();
  }

  return 0;
}
