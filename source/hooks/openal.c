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
  // openal API — EFX symbols use so_find_addr_safe (game may not import them)
#define HOOK_OPENAL_SAFE(sym, fn) do { addr = so_find_addr_safe(sym); if (addr) hook_arm64(addr, (uintptr_t)(fn)); } while(0)
#define HOOK_OPENAL(sym, fn) hook_arm64(so_find_addr(sym), (uintptr_t)(fn))
  HOOK_OPENAL_SAFE("alAuxiliaryEffectSlotf", alAuxiliaryEffectSlotf);
  HOOK_OPENAL_SAFE("alAuxiliaryEffectSlotfv", alAuxiliaryEffectSlotfv);
  HOOK_OPENAL_SAFE("alAuxiliaryEffectSloti", alAuxiliaryEffectSloti);
  HOOK_OPENAL_SAFE("alAuxiliaryEffectSlotiv", alAuxiliaryEffectSlotiv);
  HOOK_OPENAL("alBuffer3f", alBuffer3f);
  HOOK_OPENAL("alBuffer3i", alBuffer3i);
  HOOK_OPENAL("alBufferData", alBufferData);
  HOOK_OPENAL("alBufferf", alBufferf);
  HOOK_OPENAL("alBufferfv", alBufferfv);
  HOOK_OPENAL("alBufferi", alBufferi);
  HOOK_OPENAL("alBufferiv", alBufferiv);
  HOOK_OPENAL_SAFE("alDeleteAuxiliaryEffectSlots", alDeleteAuxiliaryEffectSlots);
  HOOK_OPENAL("alDeleteBuffers", alDeleteBuffers);
  HOOK_OPENAL_SAFE("alDeleteEffects", alDeleteEffects);
  HOOK_OPENAL_SAFE("alDeleteFilters", alDeleteFilters);
  HOOK_OPENAL("alDeleteSources", alDeleteSources);
  HOOK_OPENAL("alDisable", alDisable);
  HOOK_OPENAL("alDistanceModel", alDistanceModel);
  HOOK_OPENAL("alDopplerFactor", alDopplerFactor);
  HOOK_OPENAL("alDopplerVelocity", alDopplerVelocity);
  HOOK_OPENAL_SAFE("alEffectf", alEffectf);
  HOOK_OPENAL_SAFE("alEffectfv", alEffectfv);
  HOOK_OPENAL_SAFE("alEffecti", alEffecti);
  HOOK_OPENAL_SAFE("alEffectiv", alEffectiv);
  HOOK_OPENAL("alEnable", alEnable);
  HOOK_OPENAL_SAFE("alFilterf", alFilterf);
  HOOK_OPENAL_SAFE("alFilterfv", alFilterfv);
  HOOK_OPENAL_SAFE("alFilteri", alFilteri);
  HOOK_OPENAL_SAFE("alFilteriv", alFilteriv);
  HOOK_OPENAL_SAFE("alGenAuxiliaryEffectSlots", alGenAuxiliaryEffectSlots);
  HOOK_OPENAL("alGenBuffers", alGenBuffers);
  HOOK_OPENAL_SAFE("alGenEffects", alGenEffects);
  HOOK_OPENAL_SAFE("alGenFilters", alGenFilters);
  HOOK_OPENAL("alGenSources", alGenSources);
  HOOK_OPENAL_SAFE("alGetAuxiliaryEffectSlotf", alGetAuxiliaryEffectSlotf);
  HOOK_OPENAL_SAFE("alGetAuxiliaryEffectSlotfv", alGetAuxiliaryEffectSlotfv);
  HOOK_OPENAL_SAFE("alGetAuxiliaryEffectSloti", alGetAuxiliaryEffectSloti);
  HOOK_OPENAL_SAFE("alGetAuxiliaryEffectSlotiv", alGetAuxiliaryEffectSlotiv);
  HOOK_OPENAL("alGetBoolean", alGetBoolean);
  HOOK_OPENAL("alGetBooleanv", alGetBooleanv);
  HOOK_OPENAL("alGetBuffer3f", alGetBuffer3f);
  HOOK_OPENAL("alGetBuffer3i", alGetBuffer3i);
  HOOK_OPENAL("alGetBufferf", alGetBufferf);
  HOOK_OPENAL("alGetBufferfv", alGetBufferfv);
  HOOK_OPENAL("alGetBufferi", alGetBufferi);
  HOOK_OPENAL("alGetBufferiv", alGetBufferiv);
  HOOK_OPENAL("alGetDouble", alGetDouble);
  HOOK_OPENAL("alGetDoublev", alGetDoublev);
  HOOK_OPENAL_SAFE("alGetEffectf", alGetEffectf);
  HOOK_OPENAL_SAFE("alGetEffectfv", alGetEffectfv);
  HOOK_OPENAL_SAFE("alGetEffecti", alGetEffecti);
  HOOK_OPENAL_SAFE("alGetEffectiv", alGetEffectiv);
  HOOK_OPENAL("alGetEnumValue", alGetEnumValue);
  HOOK_OPENAL("alGetError", alGetError);
  HOOK_OPENAL_SAFE("alGetFilterf", alGetFilterf);
  HOOK_OPENAL_SAFE("alGetFilterfv", alGetFilterfv);
  HOOK_OPENAL_SAFE("alGetFilteri", alGetFilteri);
  HOOK_OPENAL_SAFE("alGetFilteriv", alGetFilteriv);
  HOOK_OPENAL("alGetFloat", alGetFloat);
  HOOK_OPENAL("alGetFloatv", alGetFloatv);
  HOOK_OPENAL("alGetInteger", alGetInteger);
  HOOK_OPENAL("alGetIntegerv", alGetIntegerv);
  HOOK_OPENAL("alGetListener3f", alGetListener3f);
  HOOK_OPENAL("alGetListener3i", alGetListener3i);
  HOOK_OPENAL("alGetListenerf", alGetListenerf);
  HOOK_OPENAL("alGetListenerfv", alGetListenerfv);
  HOOK_OPENAL("alGetListeneri", alGetListeneri);
  HOOK_OPENAL("alGetListeneriv", alGetListeneriv);
  HOOK_OPENAL("alGetProcAddress", alGetProcAddress);
  HOOK_OPENAL("alGetSource3f", alGetSource3f);
  HOOK_OPENAL("alGetSource3i", alGetSource3i);
  HOOK_OPENAL("alGetSourcef", alGetSourcef);
  HOOK_OPENAL("alGetSourcefv", alGetSourcefv);
  HOOK_OPENAL("alGetSourcei", alGetSourcei);
  HOOK_OPENAL("alGetSourceiv", alGetSourceiv);
  HOOK_OPENAL("alGetString", alGetString);
  HOOK_OPENAL_SAFE("alIsAuxiliaryEffectSlot", alIsAuxiliaryEffectSlot);
  HOOK_OPENAL("alIsBuffer", alIsBuffer);
  HOOK_OPENAL_SAFE("alIsEffect", alIsEffect);
  HOOK_OPENAL("alIsEnabled", alIsEnabled);
  HOOK_OPENAL("alIsExtensionPresent", alIsExtensionPresent);
  HOOK_OPENAL_SAFE("alIsFilter", alIsFilter);
  HOOK_OPENAL("alIsSource", alIsSource);
  HOOK_OPENAL("alListener3f", alListener3f);
  HOOK_OPENAL("alListener3i", alListener3i);
  HOOK_OPENAL("alListenerf", alListenerf);
  HOOK_OPENAL("alListenerfv", alListenerfv);
  HOOK_OPENAL("alListeneri", alListeneri);
  HOOK_OPENAL("alListeneriv", alListeneriv);
  HOOK_OPENAL("alSource3f", alSource3f);
  HOOK_OPENAL("alSource3i", alSource3i);
  HOOK_OPENAL("alSourcePause", alSourcePause);
  HOOK_OPENAL("alSourcePausev", alSourcePausev);
  HOOK_OPENAL("alSourcePlay", alSourcePlay);
  HOOK_OPENAL("alSourcePlayv", alSourcePlayv);
  HOOK_OPENAL("alSourceQueueBuffers", alSourceQueueBuffers);
  HOOK_OPENAL("alSourceRewind", alSourceRewind);
  HOOK_OPENAL("alSourceRewindv", alSourceRewindv);
  HOOK_OPENAL("alSourceStop", alSourceStop);
  HOOK_OPENAL("alSourceStopv", alSourceStopv);
  HOOK_OPENAL("alSourceUnqueueBuffers", alSourceUnqueueBuffers);
  HOOK_OPENAL("alSourcef", alSourcef);
  HOOK_OPENAL("alSourcefv", alSourcefv);
  HOOK_OPENAL("alSourcei", alSourcei);
  HOOK_OPENAL("alSourceiv", alSourceiv);
  HOOK_OPENAL("alSpeedOfSound", alSpeedOfSound);
  HOOK_OPENAL("al_print", ret0);
  HOOK_OPENAL("alcCaptureCloseDevice", alcCaptureCloseDevice);
  HOOK_OPENAL("alcCaptureOpenDevice", alcCaptureOpenDevice);
  HOOK_OPENAL("alcCaptureSamples", alcCaptureSamples);
  HOOK_OPENAL("alcCaptureStart", alcCaptureStart);
  HOOK_OPENAL("alcCaptureStop", alcCaptureStop);
  HOOK_OPENAL("alcCloseDevice", alcCloseDevice);
  HOOK_OPENAL("alcCreateContext", alcCreateContextHook);
  HOOK_OPENAL("alcDestroyContext", alcDestroyContext);
  HOOK_OPENAL("alcGetContextsDevice", alcGetContextsDevice);
  HOOK_OPENAL("alcGetCurrentContext", alcGetCurrentContext);
  HOOK_OPENAL("alcGetEnumValue", alcGetEnumValue);
  HOOK_OPENAL("alcGetError", alcGetError);
  HOOK_OPENAL("alcGetIntegerv", alcGetIntegerv);
  HOOK_OPENAL("alcGetProcAddress", alcGetProcAddress);
  HOOK_OPENAL("alcGetString", alcGetString);
  HOOK_OPENAL("alcGetThreadContext", alcGetThreadContext);
  HOOK_OPENAL("alcIsExtensionPresent", alcIsExtensionPresent);
  HOOK_OPENAL("alcMakeContextCurrent", alcMakeContextCurrent);
  HOOK_OPENAL("alcOpenDevice", alcOpenDeviceHook);
  HOOK_OPENAL("alcProcessContext", alcProcessContext);
  HOOK_OPENAL("alcSetThreadContext", alcSetThreadContext);
  HOOK_OPENAL("alcSuspendContext", alcSuspendContext);
#undef HOOK_OPENAL_SAFE
#undef HOOK_OPENAL
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
