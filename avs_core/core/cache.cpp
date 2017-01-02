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
#include "LruCache.h"
#include <cassert>
#include <cstdio>

#ifdef X86_32
#include <mmintrin.h>
#endif


extern const AVSFunction Cache_filters[] = {
  { "Cache", BUILTIN_FUNC_PREFIX, "c", Cache::Create },
  { "InternalCache", BUILTIN_FUNC_PREFIX, "c", Cache::Create },
  { 0 }
};


struct CachePimpl
{
  PClip child; 
  VideoInfo vi;

  // Video cache
  std::shared_ptr<LruCache<size_t, PVideoFrame> > VideoCache;

  // Audio cache
  CachePolicyHint AudioPolicy;
  char* AudioCache;
  size_t SampleSize;
  size_t MaxSampleCount;

  CachePimpl(const PClip& _child) :
    child(_child),
    vi(_child->GetVideoInfo()),
    VideoCache(std::make_shared<LruCache<size_t, PVideoFrame> >(0)),
    AudioPolicy(CACHE_AUDIO),
    AudioCache(NULL),
    SampleSize(0),
    MaxSampleCount(0)
  {
    SampleSize = vi.BytesPerAudioSample();
  }
  ~CachePimpl()
  {
    if (AudioCache)
      free(AudioCache);
    AudioCache = NULL;
  }
};


Cache::Cache(const PClip& _child, IScriptEnvironment* env) :
  Env(env),
  _pimpl(NULL)
{
  _pimpl = new CachePimpl(_child);
  env->ManageCache(MC_RegisterCache, reinterpret_cast<void*>(this));
  _RPT5(0, "Cache::Cache registered. cache_id=%p child=%p w=%d h=%d VideoCacheSize=%Iu\n", (void *)this, (void *)_child, _pimpl->vi.width, _pimpl->vi.height, _pimpl->VideoCache->size()); // P.F.
}

Cache::~Cache()
{
  _RPT5(0, "Cache::Cache unregister. cache_id=%p child=%p w=%d h=%d VideoCacheSize=%Iu\n", (void *)this, (void *)_pimpl->child, _pimpl->vi.width, _pimpl->vi.height, _pimpl->VideoCache->size()); // P.F.
  Env->ManageCache(MC_UnRegisterCache, reinterpret_cast<void*>(this));
  delete _pimpl;
}

PVideoFrame __stdcall Cache::GetFrame(int n, IScriptEnvironment* env)
{
  // Protect plugins that cannot handle out-of-bounds frame indices
  n = clamp(n, 0, GetVideoInfo().num_frames-1);

  if (_pimpl->VideoCache->requested_capacity() > _pimpl->VideoCache->capacity())
    env->ManageCache(MC_NodAndExpandCache, reinterpret_cast<void*>(this));
  else
    env->ManageCache(MC_NodCache, reinterpret_cast<void*>(this));

  PVideoFrame result;
  LruCache<size_t, PVideoFrame>::handle cache_handle;
  
#ifdef _DEBUG	
  std::chrono::time_point<std::chrono::high_resolution_clock> t_start, t_end; 
  t_start = std::chrono::high_resolution_clock::now(); // t_start starts in the constructor. Used in logging

  LruLookupResult LruLookupRes = _pimpl->VideoCache->lookup(n, &cache_handle, true);
  switch (LruLookupRes)
#else
  switch(_pimpl->VideoCache->lookup(n, &cache_handle, true))
#endif
  {
  case LRU_LOOKUP_NOT_FOUND:
    {
      try
      {
        //cache_handle.first->value = _pimpl->child->GetFrame(n, env);
        result = _pimpl->child->GetFrame(n, env); // P.F. fill result immediately
        cache_handle.first->value = result; // not after commit!
  #ifdef X86_32
        _mm_empty();
  #endif
        _pimpl->VideoCache->commit_value(&cache_handle);
      }
      catch (...)
      {
        _pimpl->VideoCache->rollback(&cache_handle);
        throw;
      }
#ifdef _DEBUG	
#define SLOW_READOUT_TEST
  #ifdef SLOW_READOUT_TEST
      // at threadcount==8, when the _RPT1 debug line (or similar time-consuming command) is present here
      // random frame==NULL return -> 0xC0000005
      // !!! some process during the next 1/10000 seconds is overwriting the content of this cache handle (frame) with NULL!
      // 1/10000 sec delay, but a simple _RPT debug line is enough, albeit the corruption occurs more rarely
      std::chrono::time_point<std::chrono::high_resolution_clock> t_start2, t_end2;
      std::chrono::duration<double> elapsed_seconds;
      t_start2 = std::chrono::high_resolution_clock::now();
      do {
        t_end2 = std::chrono::high_resolution_clock::now();
        elapsed_seconds = t_end2 - t_start2;
      } while (elapsed_seconds.count() < 1.0 / 10000.0);
      // end of delay
      //assert(NULL != cache_handle.first->value); // and now it's NULL!!!
      assert(NULL != result); // but previously saved value is NOT NULL!!!
      // P.F. debug lines to be removed when the NULL pointer frame problem is surely solved
  #endif
      t_end = std::chrono::high_resolution_clock::now();
      elapsed_seconds = t_end - t_start;
      std::string name = FuncName;
      char buf[256];
      if (NULL == cache_handle.first->value) {
          _snprintf(buf, 255, "Cache::GetFrame LRU_LOOKUP_NOT_FOUND: HEY! got nulled! [%s] n=%6d child=%p frame=%p framebefore=%p SeekTimeWithGetFrame:%f\n", name.c_str(), n, (void *)_pimpl->child, (void *)cache_handle.first->value, (void *)result, elapsed_seconds.count()); // P.F.
          _RPT0(0, buf);
      } else {
          _snprintf(buf, 255, "Cache::GetFrame LRU_LOOKUP_NOT_FOUND: [%s] n=%6d child=%p frame=%p framebefore=%p videoCacheSize=%zu SeekTimeWithGetFrame:%f\n", name.c_str(), n, (void *)_pimpl->child, (void *)cache_handle.first->value, (void *)result, _pimpl->VideoCache->size(), elapsed_seconds.count()); // P.F.
          _RPT0(0, buf);
      }
#endif
      // result = cache_handle.first->value; not here! 
      // its content may change after commit when the last lock is released 
      // (cache is being restructured by other threads, new frames, etc...)
      break;
    }
  case LRU_LOOKUP_FOUND_AND_READY:
    {
      result = cache_handle.first->value;
#ifdef _DEBUG	
      t_end = std::chrono::high_resolution_clock::now();
      std::chrono::duration<double> elapsed_seconds = t_end - t_start;
      std::string name = FuncName;
      char buf[256];
      _snprintf(buf, 255, "Cache::GetFrame LRU_LOOKUP_FOUND_AND_READY: [%s] n=%6d child=%p frame=%p vfb=%p videoCacheSize=%zu SeekTime            :%f\n", name.c_str(), n, (void *)_pimpl->child, (void *)result, (void *)result->GetFrameBuffer(), _pimpl->VideoCache->size(), elapsed_seconds.count());
      _RPT0(0, buf);
      assert(result != NULL);
#endif
      break;
    }
  case LRU_LOOKUP_NO_CACHE:
    {
      result = _pimpl->child->GetFrame(n, env);
#ifdef _DEBUG	
      t_end = std::chrono::high_resolution_clock::now();
      std::chrono::duration<double> elapsed_seconds = t_end - t_start;
      std::string name = FuncName;
      char buf[256];
      _snprintf(buf, 255, "Cache::GetFrame LRU_LOOKUP_NO_CACHE: [%s] n=%6d child=%p frame=%p vfb=%p videoCacheSize=%zu SeekTime            :%f\n", name.c_str(), n, (void *)_pimpl->child, (void *)result, (void *)result->GetFrameBuffer(), _pimpl->VideoCache->size(), elapsed_seconds.count()); // P.F.
      _RPT0(0, buf);
#endif
      break;
    }
  case LRU_LOOKUP_FOUND_BUT_NOTAVAIL:    // Fall-through intentional
  default:
    {
      assert(0);
      break;
    }
  }

  return result;
}

void Cache::FillAudioZeros(void* buf, int start_offset, int count) {
    const int bps = _pimpl->vi.BytesPerAudioSample();
    unsigned char* byte_buf = (unsigned char*)buf;
    memset(byte_buf + start_offset * bps, 0, count * bps);
}

void __stdcall Cache::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env)
{
    if (count <= 0)
        return;

    // -----------------------------------------------------------
    //          Enforce audio bounds
    // -----------------------------------------------------------

    VideoInfo * vi = &(_pimpl->vi);

    if ((!vi->HasAudio()) || (start + count <= 0) || (start >= vi->num_audio_samples)) {
        // Completely skip.
        FillAudioZeros(buf, 0, (int)count);
        count = 0;
        return;
    }

    if (start < 0) {  // Partial initial skip
        FillAudioZeros(buf, 0, (int)-start);  // Fill all samples before 0 with silence.
        count += start;  // Subtract start bytes from count.
        buf = ((BYTE*)buf) - (int)(start*vi->BytesPerAudioSample());
        start = 0;
    }

    if (start + count > vi->num_audio_samples) {  // Partial ending skip
        FillAudioZeros(buf, (int)(vi->num_audio_samples - start), (int)(count - (vi->num_audio_samples - start)));  // Fill end samples
        count = (vi->num_audio_samples - start);
    }

    // -----------------------------------------------------------
    //          Caching
    // -----------------------------------------------------------
    
    // TODO: implement audio cache


    _pimpl->child->GetAudio(buf, start, count, env);
}

const VideoInfo& __stdcall Cache::GetVideoInfo()
{
  return _pimpl->vi;
}

bool __stdcall Cache::GetParity(int n)
{
  return _pimpl->child->GetParity(n);
}

int __stdcall Cache::SetCacheHints(int cachehints, int frame_range)
{
  _RPT3(0, "Cache::SetCacheHints called. cache=%p hint=%d frame_range=%d\n", (void *)this, cachehints, frame_range); // P.F.
  switch(cachehints)
  {
    /*********************************************
        MISC
    *********************************************/

    // By returning IS_CACHE_ANS to IS_CACHE_REQ, we tell the caller we are a cache
    case CACHE_IS_CACHE_REQ:
      return CACHE_IS_CACHE_ANS;

    case CACHE_GET_POLICY: // Get the current policy.
      return CACHE_GENERIC;

    case CACHE_DONT_CACHE_ME:
      return 1;

    case CACHE_GET_MTMODE:
      return MT_NICE_FILTER;

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
    { // This is not atomic, but frankly, we don't care
      size_t min, max;
      _pimpl->VideoCache->limits(&min, &max);
      min = frame_range;
      _pimpl->VideoCache->set_limits(min, max);
      break;
    }

    case CACHE_SET_MAX_CAPACITY:
    { // This is not atomic, but frankly, we don't care
      size_t min, max;
      _pimpl->VideoCache->limits(&min, &max);
      max = frame_range;
      _pimpl->VideoCache->set_limits(min, max);
      _RPT3(0, "Cache::SetCacheHints CACHE_SET_MAX_CAPACITY cache=%p hint=%d frame_range=%d\n", (void *)this, cachehints, frame_range); // P.F.
      break;
    }

    case CACHE_GET_MIN_CAPACITY:
    {
      size_t min, max;
      _pimpl->VideoCache->limits(&min, &max);
      return (int)min;
    }

    case CACHE_GET_MAX_CAPACITY:
    {
      size_t min, max;
      _pimpl->VideoCache->limits(&min, &max);
      return (int)max;
    }

    case CACHE_GET_SIZE:
      return (int)_pimpl->VideoCache->size();
      
    case CACHE_GET_REQUESTED_CAP:
      return (int)_pimpl->VideoCache->requested_capacity();

    case CACHE_GET_CAPACITY:
      return (int)_pimpl->VideoCache->capacity();

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
      if (!_pimpl->vi.HasAudio())
        break;

      // Range means for audio.
      // 0 == Create a default buffer (256kb).
      // Positive. Allocate X bytes for cache.
      if (frame_range == 0) {
        if (_pimpl->AudioPolicy != CACHE_AUDIO_NONE)   // We already have a policy - no need for a default one.
          break;

        frame_range=256*1024;
      }

      if (frame_range/_pimpl->SampleSize > _pimpl->MaxSampleCount) { // Only make bigger
        char * NewAudioCache = (char*)realloc(_pimpl->AudioCache, frame_range);
        if (NewAudioCache == NULL)
        {
          throw std::bad_alloc();
        }
        _pimpl->AudioCache = NewAudioCache;

        _pimpl->MaxSampleCount = frame_range/_pimpl->SampleSize;
      }

      _pimpl->AudioPolicy = (CachePolicyHint)cachehints;
      break;

    case CACHE_AUDIO_NONE:
    case CACHE_AUDIO_NOTHING:
      free(_pimpl->AudioCache);
      _pimpl->AudioCache = NULL;
      _pimpl->MaxSampleCount = 0;
      _pimpl->AudioPolicy = (CachePolicyHint)cachehints;
      break;

    case CACHE_GET_AUDIO_POLICY: // Get the current audio policy.
      return _pimpl->AudioPolicy;

    case CACHE_GET_AUDIO_SIZE: // Get the current audio cache size.
      return (int)(_pimpl->SampleSize * _pimpl->MaxSampleCount);

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
  return ((p->GetVersion() >= 5) && (p->SetCacheHints(CACHE_IS_CACHE_REQ, 0) == CACHE_IS_CACHE_ANS));
}
