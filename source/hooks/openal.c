/* openal.c -- OpenAL hooks and patches
 *
 * Copyright (C) 2021 fgsfds, Andy Nguyen
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define AL_ALEXT_PROTOTYPES
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#include <AL/efx.h>

#include "../util.h"
#include "../so_util.h"

static ALCcontext *al_ctx = NULL;
static ALCdevice *al_dev = NULL;

ALCcontext *alcCreateContextHook(ALCdevice *dev, const ALCint *unused) {
  // override 22050hz with 44100hz in case someone wants high quality sounds
  const ALCint attr[] = { ALC_FREQUENCY, 44100, 0 };
  al_ctx = alcCreateContext(dev, attr); // capture context for later deinit
  return al_ctx;
}

ALCdevice *alcOpenDeviceHook(const char *name) {
  // capture device pointer for later deinit
  al_dev = alcOpenDevice(name);
  return al_dev;
}

void patch_openal(void) {
  // used for openal - Windows API critical section functions
  // Try both correct spelling and typo variant
  // Use so_find_addr_safe to avoid fatal_error if symbol doesn't exist
  extern uintptr_t so_find_addr_safe(const char *symbol);
  uintptr_t addr;
  
  addr = so_find_addr_safe("InitializeCriticalSection");
  if (addr != 0)
    hook_arm64(addr, (uintptr_t)ret0);
  
  addr = so_find_addr_safe("InitalizeCriticalSection");  // Typo variant
  if (addr != 0)
    hook_arm64(addr, (uintptr_t)ret0);
  // openal API — all hooks use so_find_addr_safe (game may not import every symbol)
#define HOOK_OPENAL_SAFE(sym, fn) do { addr = so_find_addr_safe(sym); if (addr) hook_arm64(addr, (uintptr_t)(fn)); } while(0)
  HOOK_OPENAL_SAFE("alAuxiliaryEffectSlotf", alAuxiliaryEffectSlotf);
  HOOK_OPENAL_SAFE("alAuxiliaryEffectSlotfv", alAuxiliaryEffectSlotfv);
  HOOK_OPENAL_SAFE("alAuxiliaryEffectSloti", alAuxiliaryEffectSloti);
  HOOK_OPENAL_SAFE("alAuxiliaryEffectSlotiv", alAuxiliaryEffectSlotiv);
  HOOK_OPENAL_SAFE("alBuffer3f", alBuffer3f);
  HOOK_OPENAL_SAFE("alBuffer3i", alBuffer3i);
  HOOK_OPENAL_SAFE("alBufferData", alBufferData);
  HOOK_OPENAL_SAFE("alBufferf", alBufferf);
  HOOK_OPENAL_SAFE("alBufferfv", alBufferfv);
  HOOK_OPENAL_SAFE("alBufferi", alBufferi);
  HOOK_OPENAL_SAFE("alBufferiv", alBufferiv);
  HOOK_OPENAL_SAFE("alDeleteAuxiliaryEffectSlots", alDeleteAuxiliaryEffectSlots);
  HOOK_OPENAL_SAFE("alDeleteBuffers", alDeleteBuffers);
  HOOK_OPENAL_SAFE("alDeleteEffects", alDeleteEffects);
  HOOK_OPENAL_SAFE("alDeleteFilters", alDeleteFilters);
  HOOK_OPENAL_SAFE("alDeleteSources", alDeleteSources);
  HOOK_OPENAL_SAFE("alDisable", alDisable);
  HOOK_OPENAL_SAFE("alDistanceModel", alDistanceModel);
  HOOK_OPENAL_SAFE("alDopplerFactor", alDopplerFactor);
  HOOK_OPENAL_SAFE("alDopplerVelocity", alDopplerVelocity);
  HOOK_OPENAL_SAFE("alEffectf", alEffectf);
  HOOK_OPENAL_SAFE("alEffectfv", alEffectfv);
  HOOK_OPENAL_SAFE("alEffecti", alEffecti);
  HOOK_OPENAL_SAFE("alEffectiv", alEffectiv);
  HOOK_OPENAL_SAFE("alEnable", alEnable);
  HOOK_OPENAL_SAFE("alFilterf", alFilterf);
  HOOK_OPENAL_SAFE("alFilterfv", alFilterfv);
  HOOK_OPENAL_SAFE("alFilteri", alFilteri);
  HOOK_OPENAL_SAFE("alFilteriv", alFilteriv);
  HOOK_OPENAL_SAFE("alGenAuxiliaryEffectSlots", alGenAuxiliaryEffectSlots);
  HOOK_OPENAL_SAFE("alGenBuffers", alGenBuffers);
  HOOK_OPENAL_SAFE("alGenEffects", alGenEffects);
  HOOK_OPENAL_SAFE("alGenFilters", alGenFilters);
  HOOK_OPENAL_SAFE("alGenSources", alGenSources);
  HOOK_OPENAL_SAFE("alGetAuxiliaryEffectSlotf", alGetAuxiliaryEffectSlotf);
  HOOK_OPENAL_SAFE("alGetAuxiliaryEffectSlotfv", alGetAuxiliaryEffectSlotfv);
  HOOK_OPENAL_SAFE("alGetAuxiliaryEffectSloti", alGetAuxiliaryEffectSloti);
  HOOK_OPENAL_SAFE("alGetAuxiliaryEffectSlotiv", alGetAuxiliaryEffectSlotiv);
  HOOK_OPENAL_SAFE("alGetBoolean", alGetBoolean);
  HOOK_OPENAL_SAFE("alGetBooleanv", alGetBooleanv);
  HOOK_OPENAL_SAFE("alGetBuffer3f", alGetBuffer3f);
  HOOK_OPENAL_SAFE("alGetBuffer3i", alGetBuffer3i);
  HOOK_OPENAL_SAFE("alGetBufferf", alGetBufferf);
  HOOK_OPENAL_SAFE("alGetBufferfv", alGetBufferfv);
  HOOK_OPENAL_SAFE("alGetBufferi", alGetBufferi);
  HOOK_OPENAL_SAFE("alGetBufferiv", alGetBufferiv);
  HOOK_OPENAL_SAFE("alGetDouble", alGetDouble);
  HOOK_OPENAL_SAFE("alGetDoublev", alGetDoublev);
  HOOK_OPENAL_SAFE("alGetEffectf", alGetEffectf);
  HOOK_OPENAL_SAFE("alGetEffectfv", alGetEffectfv);
  HOOK_OPENAL_SAFE("alGetEffecti", alGetEffecti);
  HOOK_OPENAL_SAFE("alGetEffectiv", alGetEffectiv);
  HOOK_OPENAL_SAFE("alGetEnumValue", alGetEnumValue);
  HOOK_OPENAL_SAFE("alGetError", alGetError);
  HOOK_OPENAL_SAFE("alGetFilterf", alGetFilterf);
  HOOK_OPENAL_SAFE("alGetFilterfv", alGetFilterfv);
  HOOK_OPENAL_SAFE("alGetFilteri", alGetFilteri);
  HOOK_OPENAL_SAFE("alGetFilteriv", alGetFilteriv);
  HOOK_OPENAL_SAFE("alGetFloat", alGetFloat);
  HOOK_OPENAL_SAFE("alGetFloatv", alGetFloatv);
  HOOK_OPENAL_SAFE("alGetInteger", alGetInteger);
  HOOK_OPENAL_SAFE("alGetIntegerv", alGetIntegerv);
  HOOK_OPENAL_SAFE("alGetListener3f", alGetListener3f);
  HOOK_OPENAL_SAFE("alGetListener3i", alGetListener3i);
  HOOK_OPENAL_SAFE("alGetListenerf", alGetListenerf);
  HOOK_OPENAL_SAFE("alGetListenerfv", alGetListenerfv);
  HOOK_OPENAL_SAFE("alGetListeneri", alGetListeneri);
  HOOK_OPENAL_SAFE("alGetListeneriv", alGetListeneriv);
  HOOK_OPENAL_SAFE("alGetProcAddress", alGetProcAddress);
  HOOK_OPENAL_SAFE("alGetSource3f", alGetSource3f);
  HOOK_OPENAL_SAFE("alGetSource3i", alGetSource3i);
  HOOK_OPENAL_SAFE("alGetSourcef", alGetSourcef);
  HOOK_OPENAL_SAFE("alGetSourcefv", alGetSourcefv);
  HOOK_OPENAL_SAFE("alGetSourcei", alGetSourcei);
  HOOK_OPENAL_SAFE("alGetSourceiv", alGetSourceiv);
  HOOK_OPENAL_SAFE("alGetString", alGetString);
  HOOK_OPENAL_SAFE("alIsAuxiliaryEffectSlot", alIsAuxiliaryEffectSlot);
  HOOK_OPENAL_SAFE("alIsBuffer", alIsBuffer);
  HOOK_OPENAL_SAFE("alIsEffect", alIsEffect);
  HOOK_OPENAL_SAFE("alIsEnabled", alIsEnabled);
  HOOK_OPENAL_SAFE("alIsExtensionPresent", alIsExtensionPresent);
  HOOK_OPENAL_SAFE("alIsFilter", alIsFilter);
  HOOK_OPENAL_SAFE("alIsSource", alIsSource);
  HOOK_OPENAL_SAFE("alListener3f", alListener3f);
  HOOK_OPENAL_SAFE("alListener3i", alListener3i);
  HOOK_OPENAL_SAFE("alListenerf", alListenerf);
  HOOK_OPENAL_SAFE("alListenerfv", alListenerfv);
  HOOK_OPENAL_SAFE("alListeneri", alListeneri);
  HOOK_OPENAL_SAFE("alListeneriv", alListeneriv);
  HOOK_OPENAL_SAFE("alSource3f", alSource3f);
  HOOK_OPENAL_SAFE("alSource3i", alSource3i);
  HOOK_OPENAL_SAFE("alSourcePause", alSourcePause);
  HOOK_OPENAL_SAFE("alSourcePausev", alSourcePausev);
  HOOK_OPENAL_SAFE("alSourcePlay", alSourcePlay);
  HOOK_OPENAL_SAFE("alSourcePlayv", alSourcePlayv);
  HOOK_OPENAL_SAFE("alSourceQueueBuffers", alSourceQueueBuffers);
  HOOK_OPENAL_SAFE("alSourceRewind", alSourceRewind);
  HOOK_OPENAL_SAFE("alSourceRewindv", alSourceRewindv);
  HOOK_OPENAL_SAFE("alSourceStop", alSourceStop);
  HOOK_OPENAL_SAFE("alSourceStopv", alSourceStopv);
  HOOK_OPENAL_SAFE("alSourceUnqueueBuffers", alSourceUnqueueBuffers);
  HOOK_OPENAL_SAFE("alSourcef", alSourcef);
  HOOK_OPENAL_SAFE("alSourcefv", alSourcefv);
  HOOK_OPENAL_SAFE("alSourcei", alSourcei);
  HOOK_OPENAL_SAFE("alSourceiv", alSourceiv);
  HOOK_OPENAL_SAFE("alSpeedOfSound", alSpeedOfSound);
  HOOK_OPENAL_SAFE("al_print", ret0);
  HOOK_OPENAL_SAFE("alcCaptureCloseDevice", alcCaptureCloseDevice);
  HOOK_OPENAL_SAFE("alcCaptureOpenDevice", alcCaptureOpenDevice);
  HOOK_OPENAL_SAFE("alcCaptureSamples", alcCaptureSamples);
  HOOK_OPENAL_SAFE("alcCaptureStart", alcCaptureStart);
  HOOK_OPENAL_SAFE("alcCaptureStop", alcCaptureStop);
  HOOK_OPENAL_SAFE("alcCloseDevice", alcCloseDevice);
  HOOK_OPENAL_SAFE("alcCreateContext", alcCreateContextHook);
  HOOK_OPENAL_SAFE("alcDestroyContext", alcDestroyContext);
  HOOK_OPENAL_SAFE("alcGetContextsDevice", alcGetContextsDevice);
  HOOK_OPENAL_SAFE("alcGetCurrentContext", alcGetCurrentContext);
  HOOK_OPENAL_SAFE("alcGetEnumValue", alcGetEnumValue);
  HOOK_OPENAL_SAFE("alcGetError", alcGetError);
  HOOK_OPENAL_SAFE("alcGetIntegerv", alcGetIntegerv);
  HOOK_OPENAL_SAFE("alcGetProcAddress", alcGetProcAddress);
  HOOK_OPENAL_SAFE("alcGetString", alcGetString);
  HOOK_OPENAL_SAFE("alcGetThreadContext", alcGetThreadContext);
  HOOK_OPENAL_SAFE("alcIsExtensionPresent", alcIsExtensionPresent);
  HOOK_OPENAL_SAFE("alcMakeContextCurrent", alcMakeContextCurrent);
  HOOK_OPENAL_SAFE("alcOpenDevice", alcOpenDeviceHook);
  HOOK_OPENAL_SAFE("alcProcessContext", alcProcessContext);
  HOOK_OPENAL_SAFE("alcSetThreadContext", alcSetThreadContext);
  HOOK_OPENAL_SAFE("alcSuspendContext", alcSuspendContext);
#undef HOOK_OPENAL_SAFE
}

void deinit_openal(void) {
  if (al_dev) {
    if (al_ctx) {
      alcMakeContextCurrent(NULL);
      alcDestroyContext(al_ctx);
    }
    alcCloseDevice(al_dev);
  }
}
