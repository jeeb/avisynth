// Avisynth v2.6.  Copyright 2002-2009 Ben Rudiak-Gould et al.
// http://www.avisynth.org

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA, or visit
// http://www.gnu.org/copyleft/gpl.html .
//
// Linking Avisynth statically or dynamically with other modules is making a
// combined work based on Avisynth.  Thus, the terms and conditions of the GNU
// General Public License cover the whole combination.
//
// As a special exception, the copyright holders of Avisynth give you
// permission to link Avisynth with independent modules that communicate with
// Avisynth solely through the interfaces defined in avisynth.h, regardless of the license
// terms of these independent modules, and to copy and distribute the
// resulting combined work under terms of your choice, provided that
// every copy of the combined work is accompanied by a complete copy of
// the source code of Avisynth (the version of Avisynth used to produce the
// combined work), being distributed under the terms of the GNU General
// Public License plus this exception.  An independent module is a module
// which is not derived from or based on Avisynth, such as 3rd-party filters,
// import and export plugins, or graphical user interfaces.

#include "cache.h"
#include "internal.h"
#include <cassert>

#ifdef X86_32
#include <mmintrin.h>
#endif



extern const AVSFunction Cache_filters[] = {
  { "Cache", "c", Cache::Create },
  { "InternalCache", "c", Cache::Create },
  { 0 }
};

enum 
{
  IS_CACHE_REQ = 0x8686,
  IS_CACHE_ANS = 0x6546,
};

Cache::Cache(const PClip& _child, IScriptEnvironment* env) :
  Env(env),
  child(_child),
  vi(_child->GetVideoInfo()),
  VideoCache(std::make_shared<LruCache<size_t, PVideoFrame> >(0)),
  AudioPolicy(CACHE_AUDIO)
{
  env->ManageCache(MC_RegisterCache, reinterpret_cast<void*>(this));
}

Cache::~Cache()
{
  Env->ManageCache(MC_UnRegisterCache, reinterpret_cast<void*>(this));
}

PVideoFrame __stdcall Cache::GetFrame(int n, IScriptEnvironment* env)
{
  env->ManageCache(MC_NodCache, reinterpret_cast<void*>(this));

  bool found;
  LruCache<size_t, PVideoFrame>::handle cache_handle;
  PVideoFrame* frame = VideoCache->lookup(n, &found, &cache_handle);

  if (frame != NULL)
  {
    if (VideoCache->requested_capacity() > VideoCache->capacity())
      env->ManageCache(MC_ExpandCache, reinterpret_cast<void*>(this));
    
    if (!found)
    {
      try
      {
        *frame = child->GetFrame(n, env);
  #ifdef X86_32
        _mm_empty();
  #endif
        VideoCache->commit_value(&cache_handle, frame);
      }
      catch(...)
      {
        VideoCache->rollback(&cache_handle);
        throw;
      }
    }

    return *frame;
  }
  else
  {
    return child->GetFrame(n, env);
  }
}

void __stdcall Cache::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env)
{
  // TODO: implement audio cache
  child->GetAudio(buf, start, count, env);
}

const VideoInfo& __stdcall Cache::GetVideoInfo()
{
  return vi;
}

bool __stdcall Cache::GetParity(int n)
{
  return child->GetParity(n);
}

int __stdcall Cache::SetCacheHints(int cachehints, int frame_range)
{
  switch(cachehints)
  {
    /*********************************************
        MISC
    *********************************************/

    // By returning IS_CACHE_ANS to IS_CACHE_REQ, we tell the caller we are a cache
    case IS_CACHE_REQ:
      return IS_CACHE_ANS;

    case CACHE_GET_POLICY: // Get the current policy.
      return CACHE_GENERIC;

    case CACHE_DONT_CACHE_ME:
      return 1;

    /*********************************************
        AVS 2.5 TRANSLATION
    *********************************************/

    // Ignore 2.5 CACHE_NOTHING requests
    case CACHE_25_NOTHING:
      break;

    // Map 2.5 CACHE_RANGE calls to CACHE_WINDOW
    // force minimum range to 2
    case CACHE_25_RANGE:
      if (frame_range < 2) frame_range = 2;
      SetCacheHints(CACHE_WINDOW, frame_range);
      break;

    // Map 2.5 CACHE_ALL calls to CACHE_GENERIC
    case CACHE_25_ALL:
      SetCacheHints(CACHE_GENERIC, frame_range);
      break;

    // Map 2.5 CACHE_AUDIO calls to CACHE_AUDIO
    case CACHE_25_AUDIO:
      SetCacheHints(CACHE_AUDIO, frame_range);
      break;

    // Map 2.5 CACHE_AUDIO_NONE calls to CACHE_AUDIO_NONE
    case CACHE_25_AUDIO_NONE:
      SetCacheHints(CACHE_AUDIO_NONE, 0);
      break;

    // Map 2.5 CACHE_AUDIO_AUTO calls to CACHE_AUDIO_AUTO
    case CACHE_25_AUDIO_AUTO:
      SetCacheHints(CACHE_AUDIO_AUTO, frame_range);
      break;

    /*********************************************
        VIDEO
    *********************************************/

    case CACHE_SET_MIN_CAPACITY:
    { // This is not atomic, but rankly, we don't care
      size_t min, max;
      VideoCache->limits(&min, &max);
      min = frame_range;
      VideoCache->set_limits(min, max);
      break;
    }

    case CACHE_SET_MAX_CAPACITY:
    { // This is not atomic, but rankly, we don't care
      size_t min, max;
      VideoCache->limits(&min, &max);
      max = frame_range;
      VideoCache->set_limits(min, max);
      break;
    }

    case CACHE_GET_MIN_CAPACITY:
    {
      size_t min, max;
      VideoCache->limits(&min, &max);
      return min;
    }

    case CACHE_GET_MAX_CAPACITY:
    {
      size_t min, max;
      VideoCache->limits(&min, &max);
      return max;
    }

    case CACHE_GET_SIZE:
      return VideoCache->size();
      
    case CACHE_GET_REQUESTED_CAP:
      return VideoCache->requested_capacity();

    case CACHE_GET_CAPACITY:
      return VideoCache->capacity();

    case CACHE_GET_WINDOW: // Get the current window h_span.
    case CACHE_GET_RANGE: // Get the current generic frame range.
      return 2;
      break;

    case CACHE_GENERIC:
    case CACHE_FORCE_GENERIC:
    case CACHE_NOTHING:
    case CACHE_WINDOW:
    case CACHE_PREFETCH_FRAME:          // Queue request to prefetch frame N.
    case CACHE_PREFETCH_GO:             // Action video prefetches.
      break;

    /*********************************************
        TODO AUDIO
    *********************************************/

    case CACHE_AUDIO:
    case CACHE_AUDIO_AUTO:
      if (!vi.HasAudio())
        break;

      // Range means for audio.
      // 0 == Create a default buffer (256kb).
      // Positive. Allocate X bytes for cache.
      if (frame_range == 0) {
        if (AudioPolicy != CACHE_AUDIO_NONE)   // We already have a policy - no need for a default one.
          break;

        frame_range=256*1024;
      }

      if (frame_range/SampleSize > MaxSampleCount) { // Only make bigger
        char * NewAudioCache = (char*)realloc(AudioCache, frame_range);
        if (NewAudioCache == NULL)
        {
          throw std::bad_alloc();
        }
        AudioCache = NewAudioCache;

        MaxSampleCount = frame_range/SampleSize;
      }

      AudioPolicy = (CachePolicyHint)cachehints;
      break;

    case CACHE_AUDIO_NONE:
    case CACHE_AUDIO_NOTHING:
      free(AudioCache);
      AudioCache = NULL;
      MaxSampleCount = 0;
      AudioPolicy = (CachePolicyHint)cachehints;
      break;

    case CACHE_GET_AUDIO_POLICY: // Get the current audio policy.
      return AudioPolicy;

    case CACHE_GET_AUDIO_SIZE: // Get the current audio cache size.
      return SampleSize * MaxSampleCount;

    case CACHE_PREFETCH_AUDIO_BEGIN:    // Begin queue request to prefetch audio (take critical section).
    case CACHE_PREFETCH_AUDIO_STARTLO:  // Set low 32 bits of start.
    case CACHE_PREFETCH_AUDIO_STARTHI:  // Set high 32 bits of start.
    case CACHE_PREFETCH_AUDIO_COUNT:    // Set low 32 bits of length.
    case CACHE_PREFETCH_AUDIO_COMMIT:   // Enqueue request transaction to prefetch audio (release critical section).
    case CACHE_PREFETCH_AUDIO_GO:       // Action audio prefetch
      break;

    default:
      return 0;
  }

  return 0;
}

AVSValue __cdecl Cache::Create(AVSValue args, void*, IScriptEnvironment* env)
{
  PClip p = 0;
  if (args.IsClip())
  {
    p = args.AsClip();
  }
  else if (args.IsArray() && args[0].IsClip())
  {
    p = args[0].AsClip();
  }

  if (p)  // If the child is a clip
  {
    if ( (p->GetVersion() >= 5)
      && (p->SetCacheHints(CACHE_DONT_CACHE_ME, 0) != 0) )
    {
      // Don't create cache instance if the child doesn't want to be cached
      return p; /* This is op, not args! */
    }
    else
    {
      return new Cache(p, env);
    }
  }
  else
  {
    return args;
  }
}

bool __stdcall Cache::IsCache(const PClip& p)
{
  if ((p->GetVersion() >= 5) && (p->SetCacheHints(IS_CACHE_REQ, 0) == IS_CACHE_ANS))
    return true;
  else
    return false;
}
