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


#include "stdafx.h"

#include "cache.h"

// Global statistics counters
struct {
  long resets;
  long vfb_found;
  long vfb_modified;
  long vfb_stolen;
  long vfb_notfound;
  long vfb_never;
  long vfb_locks;
  long vfb_protects;
  char tag[72];
} g_Cache_stats = {0, 0, 0, 0, 0, 0, 0, 0, "resets, vfb_[found,modified,stolen,notfound,never,locks,protects]"};


extern const AVSFunction Cache_filters[] = {
  { "Cache", "c", Cache::Create_Cache },
  { "InternalCache", "c", Cache::Create_Cache },
  { 0 }
};


enum {CACHE_SCALE_SHIFT       =   4}; // Granularity fraction for cache size - 1/16th
enum {CACHE_SCALE_FACTOR      =   1<<CACHE_SCALE_SHIFT};
enum {MAX_CACHE_MISSES        = 100}; // Consecutive misses before a reset
enum {MAX_CACHED_VIDEO_FRAMES = 200}; // Maximum number of VFB's that are tracked
enum {MAX_CACHED_VF_SCALED    = MAX_CACHED_VIDEO_FRAMES<<CACHE_SCALE_SHIFT};
enum {MAX_CACHE_WINDOW        =  21}; // Maximum CACHE_WINDOW span i.e. number of protected VFB's


/*******************************
 *******   Cache filter   ******
 ******************************/


long Cache::Clock = 1;
long Cache::cacheDepth = 0;


Cache::Cache(PClip _child, IScriptEnvironment* env)
 : GenericVideoFilter(_child), nextCache(NULL), cache(0), priorCache(NULL), Tick(0)
{
//  InitializeCriticalSectionAndSpinCount(&cs_cache_V, 4000);
//  InitializeCriticalSectionAndSpinCount(&cs_cache_A, 4000);

  h_policy = vi.HasVideo() ? CACHE_GENERIC : CACHE_NOTHING;  // Since hints are not used per default, this is to describe the lowest default cache mode.
  h_audiopolicy = vi.HasAudio() ? CACHE_AUDIO_NONE : CACHE_AUDIO_NOTHING;  // Don't cache audio per default, auto mode.

//h_total_frames = 0; // Hint cache not init'ed
  h_span = 0;
  protectcount = 0;

  samplesize = vi.BytesPerAudioSample();

  maxsamplecount = 0;
  cache_start = 0;
  cache_count = 0;
  ac_expected_next = 0;
  ac_too_small_count = 0;
  ac_currentscore = 20;

  minframe = vi.num_frames;
  maxframe = -1;

  cache_init  = 0;
  cache_limit = CACHE_SCALE_FACTOR / 2;  // Start half way towards 1 buffer
  fault_rate = 0;
  miss_count = 0x80000000; // Hugh negative

  childcost = 0; // Unsupported by child
  childaccesscost = 0; // Unsupported by child

  // Join all the rest of the caches
  env->ManageCache(MC_RegisterCache, this);

  // For 2.6 filters ask about desired parent cache parameters
  if (child->GetVersion() <= 5) {
    childthreadmode = CACHE_THREAD_UNSAFE; // 2.5 Default
  }
  else {
    childthreadmode = CACHE_THREAD_CLASS; // 2.6 Default

    const int vmode = child->SetCacheHints(CACHE_GETCHILD_CACHE_MODE, h_policy); // Cache ask Child for desired video cache mode
    switch (vmode) {
      case 0: // Unsupported by child
        break;
      case CACHE_NOTHING:       // Do not cache video.
      case CACHE_WINDOW:        // Hard protect upto X frames within a range of X from the current frame N.
      case CACHE_GENERIC:       // LRU cache upto X frames.
      case CACHE_FORCE_GENERIC: // LRU cache upto X frames, override any previous CACHE_WINDOW.
        // Cache ask Child for desired video cache size
        SetCacheHints(vmode, child->SetCacheHints(CACHE_GETCHILD_CACHE_SIZE, h_policy == CACHE_WINDOW ? h_span : cache_init));
        break;
      default:
        env->ThrowError("Cache: Filter returned invalid response to CACHE_GETCHILD_CACHE_MODE. %d", vmode);
        break;
    }

    const int amode = child->SetCacheHints(CACHE_GETCHILD_AUDIO_MODE, h_audiopolicy); // Cache ask Child for desired audio cache mode
    switch (amode) {
      case 0: // Unsupported by child
        break;
      case CACHE_AUDIO:         // Explicitly do cache audio, X byte cache.
      case CACHE_AUDIO_NOTHING: // Explicitly do not cache audio.
      case CACHE_AUDIO_NONE:    // Audio cache off (auto mode), X byte intial cache.
      case CACHE_AUDIO_AUTO:    // Audio cache on (auto mode), X byte intial cache.
        // Cache ask Child for desired audio cache size
        SetCacheHints(amode, child->SetCacheHints(CACHE_GETCHILD_AUDIO_SIZE, maxsamplecount * samplesize));
        break;
      default:
        env->ThrowError("Cache: Filter returned invalid response to CACHE_GETCHILD_AUDIO_MODE. %d", amode);
        break;
    }

    const int cost = child->SetCacheHints(CACHE_GETCHILD_COST, 0); // Cache ask Child for estimated processing cost.
    switch (cost) {
      case 0: // Unsupported by child
        break;
      case CACHE_COST_ZERO: // Child response of zero cost (ptr arithmetic only).
      case CACHE_COST_UNIT: // Child response of unit cost (less than or equal 1 full frame blit).
      case CACHE_COST_LOW:  // Child response of light cost. (Fast)
      case CACHE_COST_MED:  // Child response of medium cost. (Real time)
      case CACHE_COST_HI:   // Child response of heavy cost. (Slow)
        childcost = cost;
        break;
      default:
        env->ThrowError("Cache: Filter returned invalid response to CACHE_GETCHILD_COST. %d", cost);
        break;
    }

    const int thread = child->SetCacheHints(CACHE_GETCHILD_THREAD_MODE, 0); // Cache ask Child for thread safetyness.
    switch (thread) {
      case 0: // Unsupported by child
        break;
      case CACHE_THREAD_UNSAFE: // Only 1 thread allowed for all instances. 2.5 filters default!
      case CACHE_THREAD_CLASS: // Only 1 thread allowed for each instance. 2.6 filters default!
      case CACHE_THREAD_SAFE: //  Allow all threads in any instance.
      case CACHE_THREAD_OWN: // Safe but limit to 1 thread, internally threaded.
        childthreadmode = thread;
        break;
      default:
        env->ThrowError("Cache: Filter returned invalid response to CACHE_GETCHILD_THREAD_MODE. %d", thread);
        break;
    }

    const int access = child->SetCacheHints(CACHE_GETCHILD_ACCESS_COST, 0); // Cache ask Child for preferred access pattern.
    switch (access) {
      case 0: // Unsupported by child
        break;
      case CACHE_ACCESS_RAND: // Filter is access order agnostic.
      case CACHE_ACCESS_SEQ0: // Filter prefers sequential access (low cost)
      case CACHE_ACCESS_SEQ1: // Filter needs sequential access (high cost)
        childaccesscost = access;
        break;
      default:
        env->ThrowError("Cache: Filter returned invalid response to CACHE_GETCHILD_ACCESS_COST. %d", access);
        break;
    }
  }
}


// Generic interface to poke all cache instances
void Cache::PokeCache(int key, int data, IScriptEnvironment* env)
{
  switch (key) {
    case PC_Nil: { // Do Nothing!
      return;
    }
    case PC_UnlockOld: { // Unlock head vfb
      // Release the head vfb if it is locked and this
      // instance is not with the current GetFrame chain.
      if (Tick != Clock) {
        if (UnlockVFB(video_frames.next)) {
          _RPT3(0, "Cache:%x: PokeCache UnlockOld vfb %x, frame %d\n",
                this, video_frames.next->vfb, video_frames.next->frame_number);
        }
      }
      break;
    }
    case PC_UnlockAll: { // Unlock all vfb's if the vfb size is big enough to satisfy
//    EnterCriticalSection(&cs_cache_V);
      for (CachedVideoFrame* i = video_frames.next; i != &video_frames; i = i->next) {
        if (i->vfb->data_size >= data) {
          if (UnlockVFB(i)) {
            _RPT3(0, "Cache:%x: PokeCache UnlockAll vfb %x, frame %d\n",
                  this, video_frames.next->vfb, video_frames.next->frame_number);
          }
        }
      }
//    LeaveCriticalSection(&cs_cache_V);
      break;
    }
    case PC_UnProtect: {   // Unprotect 1 vfb if this instance is
      if (Tick != Clock) { // not with the current GetFrame chain.
//      EnterCriticalSection(&cs_cache_V);
        for (CachedVideoFrame* i = video_frames.next; i != &video_frames; i = i->next) {
          // Unprotect the youngest because it might be the easiest
          // to regenerate from parent caches that are still current.
          // And to give it a fair chance we also promote it.
          if (i->vfb->data_size >= data) {
            if (UnProtectVFB(i)) {
              UnlockVFB(i);
              env->ManageCache(MC_PromoteVideoFrameBuffer, i->vfb);
              _RPT3(0, "Cache:%x: PokeCache UnProtect vfb %x, frame %d\n",
                    this, video_frames.next->vfb, video_frames.next->frame_number);
              break;
            }
          }
        }
//      LeaveCriticalSection(&cs_cache_V);
      }
      break;
    }
    case PC_UnProtectAll: { // Unprotect all vfb's
//    EnterCriticalSection(&cs_cache_V);
      for (CachedVideoFrame* i = video_frames.next; i != &video_frames; i = i->next) {
        if (i->vfb->data_size >= data) {
          if (UnProtectVFB(i)) {
            UnlockVFB(i);
            env->ManageCache(MC_PromoteVideoFrameBuffer, i->vfb);
            _RPT3(0, "Cache:%x: PokeCache UnProtectAll vfb %x, frame %d\n",
                  this, video_frames.next->vfb, video_frames.next->frame_number);
          }
        }
      }
//    LeaveCriticalSection(&cs_cache_V);
      break;
    }
    default:
      return;
  }
  // Poke the next Cache in the chain
  if (nextCache) nextCache->PokeCache(key, data, env);
}


/*********** V I D E O   C A C H E ************/


// Any vfb's that are still current we give back for earlier reuse
void Cache::ReturnVideoFrameBuffer(CachedVideoFrame *i, IScriptEnvironment* env)
{
    // A vfb purge has occured
    if (!i->vfb) return;

    UnProtectVFB(i);
    UnlockVFB(i);

    // if vfb is not current (i.e ours) leave it alone
    if (i->vfb->GetSequenceNumber() != i->sequence_number) return;

    // we are not active leave it alone
    if (miss_count < 0) return;

    // return vfb to vfb pool for immediate reuse
    env->ManageCache(MC_ReturnVideoFrameBuffer, i->vfb);
}


// If the cache is not being used reset it
void Cache::ResetCache(IScriptEnvironment* env)
{
  InterlockedIncrement(&g_Cache_stats.resets);
  minframe = vi.num_frames;
  maxframe = -1;
  CachedVideoFrame *i, *j;

  _RPT3(0, "Cache:%x: Cache Reset, cache_limit %d, cache_init %d\n", this, cache_limit, cache_init<<CACHE_SCALE_SHIFT);

  int count=0;
  for (i = video_frames.next; i != &video_frames; i = i->next) {
    if (++count >= cache_init) goto purge_old_frame;

    const int ifn = i->frame_number;
    if (ifn < minframe) minframe = ifn;
    if (ifn > maxframe) maxframe = ifn;
  }
  return;

purge_old_frame:

  // Truncate the tail of the chain
  video_frames.prev = i;
  j = (CachedVideoFrame*)InterlockedExchangePointer(&i->next, &video_frames);

  // Delete the excess CachedVideoFrames
  while (j != &video_frames) {
    i = j->next;
    ReturnVideoFrameBuffer(j, env); // Return old vfb to vfb pool for early reuse
    delete j;
    j = i;
  }
  cache_limit = cache_init << CACHE_SCALE_SHIFT;
}


// This seems to be an excelent place for catching out of bounds violations
// on VideoFrameBuffer's. Well at least some of them, VFB's have a small
// variable margin due to alignment constraints. Here we catch those accesses
// that violate that, for those are the ones that are going to cause problems.


PVideoFrame __stdcall Cache::childGetFrame(int n, IScriptEnvironment* env)
{
  InterlockedIncrement(&cacheDepth);
  PVideoFrame result = child->GetFrame(n, env);
  InterlockedDecrement(&cacheDepth);

  if (!result)
    env->ThrowError("Cache: Filter returned NULL PVideoFrame");

#ifdef _DEBUG
  int *p=(int *)(result->vfb->data);
  if ((p[-4] != 0xDEADBEAF)
   || (p[-3] != 0xDEADBEAF)
   || (p[-2] != 0xDEADBEAF)
   || (p[-1] != 0xDEADBEAF)) {
    env->ThrowError("Debug Cache: Write before start of VFB! Addr=%x", p);
  }
  p=(int *)(result->vfb->data + result->vfb->data_size);
  if ((p[0] != 0xDEADBEAF)
   || (p[1] != 0xDEADBEAF)
   || (p[2] != 0xDEADBEAF)
   || (p[3] != 0xDEADBEAF)) {
    env->ThrowError("Debug Cache: Write after end of VFB! Addr=%x", p);
  }
#endif
  return result;
}


// Unfortunatly the code for "return child->GetFrame(n, env);" seems highly likely
// to generate code that relies on the contents of the ebx register being preserved
// across the call. By inserting a "mov ebx,ebx" before the call the compiler can
// be convinced the ebx register has been changed and emit altermate code that is not
// dependant on the ebx registers contents.
//
// The other half of the problem is when using inline assembler, __asm, that uses the
// ebx register together with a medium amount of C++ code around it, the optimizer
// dutifully restructures the emited C++ code to remove all it's ebx references, forgets
// about the __asm ebx references, and thinks it's okay to removes the push/pop ebx
// from the entry prologue.
//
// Together they are a smoking gun!


PVideoFrame __stdcall Cache::GetFrame(int n, IScriptEnvironment* env)
{
  // :FIXME: Need a better model to find the 1st cache in the getframe chain
  // :FIXME: When a host hits us from multiple threads this is totally inadequate
  if (cacheDepth == 0) InterlockedIncrement(&Clock);
  Tick = Clock;

  n = min(vi.num_frames-1, max(0, n));  // Inserted to avoid requests beyond framerange.

  __asm {emms} // Protection from rogue filter authors

  if (h_policy == CACHE_NOTHING) { // don't want a cache. Typically filters that only ever seek forward.
    __asm mov ebx,ebx  // Hack! prevent compiler from trusting ebx contents across call
    return childGetFrame(n, env);
  }

//  EnterCriticalSection(&cs_cache_V);

  if (UnlockVFB(video_frames.next)) {  // release the head vfb if it is locked
    _RPT3(0, "Cache:%x: unlocking vfb %x for frame %d\n", this, video_frames.next->vfb, video_frames.next->frame_number);
  }

  CachedVideoFrame* cvf=0;

  // look for a cached copy of the frame
  if (n>=minframe && n<=maxframe) { // Is this frame in the range we are tracking
    miss_count = 0; // Start/Reset cache miss counter

    int c=0;
    int imaxframe = -1;
    int iminframe = vi.num_frames;
    bool found = false;

    // Scan the cache for a candidate
    for (CachedVideoFrame* i = video_frames.next; i != &video_frames; i = i->next) {
      ++c;
      const int ifn = i->frame_number;

      // Ahah! Our frame
      if (ifn == n) {
        found = true;
/*
        if( i->status & CACHE_ST_BEING_GENERATED)  {// Check if the frame is being generated
          InterlockedIncrement(&e_generated_refcount[i->e_generated_index]);
          LeaveCriticalSection(&cs_cache_V);
          WaitForSingleObject(e_generated[i->e_generated_index], INFINITE);
          InterlockedDecrement(&e_generated_refcount[i->e_generated_index]);
          InterlockedIncrement(&g_Cache_stats.vfb_found);
          LockVFB(i, env);  //  BuildVideoFrame expect the VFB to be locked
          EnterCriticalSection(&cs_cache_V);
          PVideoFrame retval=BuildVideoFrame(i, n);
          InterlockedDecrement(&retval->refcount);
          LeaveCriticalSection(&cs_cache_V);
          return retval;
        }
*/
        if (LockVFB(i, env)) {  // LockVFB will only lock the frame if the sequence number hasn't change
          // And it hasn't been screwed, Excellent!
          InterlockedIncrement(&g_Cache_stats.vfb_found);
          PVideoFrame retval = BuildVideoFrame(i, n); // Success! // 2.60
          InterlockedDecrement(&retval->refcount); // 2.60
//          LeaveCriticalSection(&cs_cache_V);
          return retval; // 2.60
        }

        // Damn! The contents have changed
        // record the details, update the counters
        long ifaults = InterlockedIncrement(&i->faults) * 100;
        if (ifaults > 300) ifaults = 300;
        const long _fault_rate = InterlockedExchangeAdd(&fault_rate, 30+c) + 30+c; // Bias by number of cache entries searched
        if (ifaults > _fault_rate) fault_rate = ifaults;
        else if (fault_rate > 300) fault_rate = 300;

        _RPT3(0, "Cache:%x: stale frame %d, requests %d\n", this, n, i->faults);

        // Modified by a parent
        if (i->sequence_number == i->vfb->GetSequenceNumber()-1) {
          InterlockedIncrement(&g_Cache_stats.vfb_modified);
        }
        // vfb has been stolen
        else {
          InterlockedIncrement(&g_Cache_stats.vfb_stolen);
          _RPT3(0, "Cache:%x: stolen vfb %x, frame %d\n", this, i->vfb, n);
        }

        UnProtectVFB(i);
        UnlockVFB(i);

        cvf = i; // Remember this entry!
        break;
      } // if (ifn == n)

      if (ifn < iminframe) iminframe = ifn;
      if (ifn > imaxframe) imaxframe = ifn;

      // Unprotect any frames out of CACHE_WINDOW scope
      if (abs(ifn-n) >= h_span) {
        if (UnProtectVFB(i)) {
          _RPT3(0, "Cache:%x: A: Unprotect vfb %x for frame %d\n", this, i->vfb, ifn);
        }
      }

      if (UnlockVFB(i)) {  // release the vfb if it is locked
        _RPT3(0, "Cache:%x: B. unlock vfb %x for frame %d\n", this, i->vfb, ifn);
      }
    } // for (CachedVideoFrame* i =

    if (!found) { // Cache miss
      InterlockedIncrement(&g_Cache_stats.vfb_notfound);

      // Make sure cache_limit is at least this wide
      const int range = (max(n-minframe, maxframe-n) * 2 + 1) << CACHE_SCALE_SHIFT;
      if (cache_limit < range)
        cache_limit = range;
      else {
        // Accumulate towards extra buffers
        const int span = 1 + imaxframe-iminframe - (cache_limit>>CACHE_SCALE_SHIFT);
        if (span > CACHE_SCALE_FACTOR)
          InterlockedExchangeAdd(&cache_limit, CACHE_SCALE_FACTOR); // Add upto one whole buffer at a time
        else if (span >  0)
          InterlockedExchangeAdd(&cache_limit, span); // Add span 16ths towards another buffer
      }
      maxframe = imaxframe; // update the limits from what is currently cached
      minframe = iminframe;
    }

    if (cache_limit > MAX_CACHED_VF_SCALED) cache_limit = MAX_CACHED_VF_SCALED;

    _RPT4(0, "Cache:%x: size %d, limit %d, fault %d\n", this, c, cache_limit, fault_rate);

  } // if (n>=minframe
  else { // This frame is not in the range we are currently tracking
    InterlockedIncrement(&g_Cache_stats.vfb_never);
    if (InterlockedIncrement(&miss_count) > MAX_CACHE_MISSES) {
      ResetCache(env);  // The cache isn't being accessed, reset it!
      miss_count = 0x80000000; // Hugh negative
    }
  } // if (n>=minframe ... else

  if (InterlockedDecrement(&fault_rate) < 0) fault_rate = 0;  // decay fault rate

  // update the limits for cached entries
  if (n < minframe) minframe = n;
  if (n > maxframe) maxframe = n;

  _RPT4(0, "Cache:%x: generating frame %d, cache from %d to %d\n", this, n, minframe, maxframe);

  // not cached; make the filter generate it.
  __asm mov ebx,ebx  // Hack! prevent compiler from trusting ebx contents across call
  PVideoFrame result = childGetFrame(n, env);

  if (!cvf) cvf=GetACachedVideoFrame(result, env);

  ReturnVideoFrameBuffer(cvf, env); // Return old vfb to vfb pool for early reuse

  // move the newly-registered frame to the front
  Relink(&video_frames, cvf, video_frames.next);

  // Keep any fault history
  if (cvf->frame_number != n) {
    cvf->frame_number = n;
    cvf->faults = 0;
  }
/*
  int free_event=GetFreeEvent();
  cvf->e_generated_index=free_event;
  ResetEvent(e_generated[free_event]);
  cvf->status=CACHE_ST_BEING_GENERATED;

  LeaveCriticalSection(&cs_cache_V);
  __asm mov ebx,ebx  // Hack! prevent compiler from trusting ebx contents across call
  PVideoFrame result = childGetFrame(n, env);
*/
  RegisterVideoFrame(cvf, result);

  // When we have the possibility of cacheing, promote
  // the vfb to the head of the LRU list, CACHE_WINDOW
  // frames are NOT promoted hence they are fair game
  // for reuse as soon as they are unprotected. This
  // is a fair price to pay for their protection. If
  // a 2nd filter is hiting the cache outside the radius
  // of protection then we do promote protected frames.

  if (miss_count >= 0) {
    env->ManageCache(( (cache_limit>>CACHE_SCALE_SHIFT) > h_span )
            ? MC_PromoteVideoFrameBuffer
            : MC_ManageVideoFrameBuffer, result->vfb);
  }

  // If this is a CACHE_WINDOW frame protect it
  if (h_span) {
    ProtectVFB(cvf, n, env);
  }
  // If we have asked for a frame twice, lock frames.
  else if (  (fault_rate >  100) // Generated frames are subject to locking at a lower fault rate
     && (fault_rate != 130) ) {  // Throw an unlocked probing frame at the world occasionally
                                 // Once we start locking frames we cannot tell if locking is
                                 // still required. So probing is a cheap way to validate this.
    LockVFB(cvf, env);           // Increment to be sure this frame isn't modified.
    _RPT3(0, "Cache:%x: lock vfb %x, gened frame %d\n", this, cvf->vfb, n);
  }
/*
  cvf->status   = 0;
  SetEvent(e_generated[free_event]);
  InterlockedDecrement(&e_generated_refcount[free_event]);
*/
  return result;
}


VideoFrame* Cache::BuildVideoFrame(CachedVideoFrame *i, int n)
{
  Relink(&video_frames, i, video_frames.next);   // move the matching cache entry to the front of the list
  VideoFrame* result = new VideoFrame(i->vfb, i->offset, i->pitch, i->row_size, i->height, i->offsetU, // 2.60
                                      i->offsetV, i->pitchUV, i->row_sizeUV, i->heightUV); // 2.60

  // If we have asked for a stale frame more than twice, leave frames locked.
  if (  (fault_rate <= 160)     // Reissued frames are subject to locking at a slightly higher fault rate
     || (fault_rate == 190) ) { // Throw an unlocked probing frame at the world occasionally
    UnlockVFB(i);
    _RPT2(0, "Cache:%x: using cached copy of frame %d\n", this, n);
  }
  else {
    _RPT3(0, "Cache:%x: lock vfb %x, cached frame %d\n", this, i->vfb, n);
  }
  return result;
}



Cache::CachedVideoFrame* Cache::GetACachedVideoFrame(const PVideoFrame& frame, IScriptEnvironment* env)
{
  CachedVideoFrame *i, *j;
  int count=0;

  // scan forwards for the last protected CachedVideoFrame
  for (i = video_frames.next; i != &video_frames; i = i->next) {
//    if (i->vfb == frame->vfb) return i; // Don't fill cache with 100's copies of a Blankclip vfb
    count += 1;
    if ((count > h_span) && (count >= (cache_limit>>CACHE_SCALE_SHIFT))) break;
  }

  if (i != &video_frames) {
    // scan backwards for a used CachedVideoFrame
    for (j = video_frames.prev; j != i; j = j->prev) {
      count += 1;
  //    if (j->status == CACHE_ST_BEING_GENERATED) continue;
      if (j->vfb == frame->vfb) return j; // Don't fill cache with 100's copies of a Blankclip vfb
      if (j->sequence_number != j->vfb->GetSequenceNumber()) return j;
      ReturnVideoFrameBuffer(j, env); // Return out of range vfb to vfb pool for early reuse
    }
  }

  if (count >= MAX_CACHED_VIDEO_FRAMES) return video_frames.prev; // to many entries just steal the oldest CachedVideoFrame

  return new CachedVideoFrame; // need a new one
}


void Cache::RegisterVideoFrame(CachedVideoFrame *i, const PVideoFrame& frame)
{
  // copy all the info
  i->vfb = frame->vfb;
  i->sequence_number = frame->vfb->GetSequenceNumber();
  i->offset = frame->offset;
  i->offsetU = frame->offsetU;
  i->offsetV = frame->offsetV;
  i->pitch = frame->pitch;
  i->pitchUV = frame->pitchUV;
  i->row_size = frame->row_size;
  i->height = frame->height;
  i->row_sizeUV = frame->row_sizeUV; // 2.60
  i->heightUV = frame->heightUV; // 2.60
}


bool Cache::LockVFB(CachedVideoFrame *i, IScriptEnvironment* env)
{
  /* This is problematic because GetFrameBuffer is looking for VFB's
   * with refcount==0 It can find one but the cache can also recover
   * it in the meantime leading to a conflict.
   */

  if ( !i->vfb )
    return false;

  if ( i->vfb->sequence_number != i->sequence_number )
    return false; // Cached copy has been blown

  if ( !InterlockedCompareExchange(&i->vfb_locked, 1, 0) ) {
    // Inc while under cs_relink_video_frame_buffer lock
    env->ManageCache(MC_IncVFBRefcount, i->vfb);
    InterlockedIncrement(&g_Cache_stats.vfb_locks);
    // Recheck now we own it
    if (i->vfb->sequence_number != i->sequence_number) {
      UnlockVFB(i);
      return false; // Cached copy has been blown
    }
  }

  return true;
}


bool Cache::UnlockVFB(CachedVideoFrame *i)
{
  if (i->vfb && InterlockedCompareExchange(&i->vfb_locked, 0, 1)) {
    InterlockedDecrement(&i->vfb->refcount);
    InterlockedDecrement(&g_Cache_stats.vfb_locks);
    return true; // was locked
  }
  return false; // nop
}

void Cache::ProtectVFB(CachedVideoFrame *i, int n, IScriptEnvironment* env)
{
  CachedVideoFrame* j = video_frames.prev;

  if (i->vfb && !InterlockedCompareExchange(&i->vfb_protected, 1, 0)) {
    InterlockedIncrement(&protectcount);
    env->ManageCache(MC_IncVFBRefcount, i->vfb);
    InterlockedIncrement(&g_Cache_stats.vfb_protects);
  }

  // Unprotect any frames out of CACHE_WINDOW scope
  while ((protectcount > h_span) && (j != &video_frames)){
    if ( (j != i) && (abs(n - j->frame_number) >= h_span) ) {
      if (UnProtectVFB(j)) {
        _RPT3(0, "Cache:%x: B: Unprotect vfb %x for frame %d\n", this, j->vfb, j->frame_number);
        break;
      }
    }
    j = j->prev;
  }
}


bool Cache::UnProtectVFB(CachedVideoFrame *i)
{
  if (i->vfb && InterlockedCompareExchange(&i->vfb_protected, 0, 1)) {
    InterlockedDecrement(&i->vfb->refcount);
    InterlockedDecrement(&protectcount);
    InterlockedDecrement(&g_Cache_stats.vfb_protects);
    return true; // was protected
  }
  return false; // nop
}

/*********** A U D I O   C A C H E ************/

void Cache::FillZeros(void* buf, int start_offset, int count) {

    const int bps = vi.BytesPerAudioSample();
    unsigned char* byte_buf = (unsigned char*)buf;
    memset(byte_buf + start_offset * bps, 0, count * bps);
}

void __stdcall Cache::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) {
  if (count <= 0)
    return;

  if ( (!vi.HasAudio()) || (start+count <= 0) || (start >= vi.num_audio_samples)) {
    // Completely skip.
    FillZeros(buf, 0, (int)count);
    return;
  }

  if (start < 0) {  // Partial initial skip
    FillZeros(buf, 0, (int)-start);  // Fill all samples before 0 with silence.
    count += start;  // Subtract start bytes from count.
    buf = ((BYTE*)buf) - (int)(start*vi.BytesPerAudioSample());
    start = 0;
  }

  if (start+count > vi.num_audio_samples) {  // Partial ending skip
    FillZeros(buf, (int)(vi.num_audio_samples - start), (int)(count - (vi.num_audio_samples - start)));  // Fill end samples
    count = (vi.num_audio_samples - start);
  }

  long _cs = ac_currentscore;
  if (start < ac_expected_next)
    _cs = InterlockedExchangeAdd(&ac_currentscore, -5) - 5;  // revisiting old ground - a cache could help
  else if (start > ac_expected_next)
    _cs = InterlockedDecrement(&ac_currentscore);            // skiping chunks - a cache might not help
  else // (start == ac_expected_next)
    _cs = InterlockedIncrement(&ac_currentscore);            // strict linear reading - why bother with a cache

  if (_cs < -2000000)
    ac_currentscore = -2000000;
  else if (_cs > 90)
    ac_currentscore = 90;

  if (h_audiopolicy == CACHE_AUDIO_NONE && ac_currentscore <= 0) {
    int new_size = (int)(vi.BytesFromAudioSamples(count)+8191) & -8192;
    new_size = min(4096*1024, new_size);
    _RPT2(0, "CA:%x: Automatically adding %d byte audiocache!\n", this, new_size);
    SetCacheHints(CACHE_AUDIO_AUTO, new_size);
  }

  if (h_audiopolicy == CACHE_AUDIO_AUTO  && (ac_currentscore > 80) ) {
    _RPT1(0, "CA:%x: Automatically deleting cache!\n", this);
    SetCacheHints(CACHE_AUDIO_NONE, 0);
  }

  ac_expected_next = start + count;

  if (h_audiopolicy == CACHE_AUDIO_NONE || h_audiopolicy == CACHE_AUDIO_NOTHING) {
    child->GetAudio(buf, start, count, env);
    return;  // We are ok to return now!
  }

#ifdef _DEBUG
  char dbgbuf[255];
  sprintf(dbgbuf, "CA:%x:Get st:%.6d co:%.6d .cst:%.6d cco:%.6d, cs:%d\n", this,
                    int(start), int(count), int(cache_start), int(cache_count), int(ac_currentscore));
  _RPT0(0, dbgbuf);
#endif

//  EnterCriticalSection(&cs_cache_A);

  while (count>maxsamplecount) {    //is cache big enough?
    _RPT1(0, "CA:%x:Cache too small->caching last audio\n", this);
    ac_too_small_count++;

// But the current cache might have 99% of what was requested??

    if (ac_too_small_count > 2 && (maxsamplecount < vi.AudioSamplesFromBytes(8192*1024))) {  // Max size = 8MB!
      //automatically upsize cache!
      int new_size = (int)(vi.BytesFromAudioSamples(max(count, cache_start+cache_count-start))+8192) & -8192; // Yes +1 to +8192 bytes
      new_size = min(8192*1024, new_size);
      _RPT2(0, "CA:%x: Autoupsizing buffer to %d bytes!\n", this, new_size);
      SetCacheHints(h_audiopolicy, new_size); // updates maxsamplecount!!
      ac_too_small_count = 0;
    }
    else {
//      LeaveCriticalSection(&cs_cache_A);
      child->GetAudio(buf, start, count, env);
//      EnterCriticalSection(&cs_cache_A);

      cache_count = min(count, maxsamplecount); // Remember maxsamplecount gets updated
      cache_start = start+count-cache_count;
      BYTE *buff=(BYTE *)buf;
      buff += vi.BytesFromAudioSamples(cache_start - start);
      memcpy(cache, buff, (size_t)vi.BytesFromAudioSamples(cache_count));

//      LeaveCriticalSection(&cs_cache_A);

      return;
    }
  }

  if ((start < cache_start) || (start > cache_start+maxsamplecount)) { //first sample is before cache or beyond linear reach -> restart cache
    _RPT1(0, "CA:%x: Restart\n", this);

    cache_count = min(count, maxsamplecount);
    cache_start = start;
    child->GetAudio(cache, cache_start, cache_count, env);
  }
  else {
    if (start+count > cache_start+cache_count) { // Does the cache fail to cover the request?
      if (start+count > cache_start+maxsamplecount) {  // Is cache shifting necessary?
        int shiftsamples = (int)((start+count) - (cache_start+maxsamplecount)); // Align end of cache with end of request

        if ( (start - cache_start)/2 > shiftsamples ) {  //shift half cache if possible
          shiftsamples = (int)((start - cache_start)/2);
        }
        if (shiftsamples >= cache_count) { // Can we save any existing data
          cache_start = start+count - maxsamplecount; // Maximise linear access
          cache_count = 0;
        }
        else {
          memmove(cache, cache+shiftsamples*samplesize, (size_t)((cache_count-shiftsamples)*samplesize));
          cache_start = cache_start + shiftsamples;
          cache_count = cache_count - shiftsamples;
        }
      }
      // Read just enough to complete the current request, append it to the cache
      child->GetAudio(cache + cache_count*samplesize, cache_start + cache_count, start+count-(cache_start+cache_count), env);
      cache_count += start+count-(cache_start+cache_count);
    }
  }

  //copy cache to buf
  memcpy(buf, cache + (start - cache_start)*samplesize, (size_t)(count*samplesize));

//  LeaveCriticalSection(&cs_cache_A);

  return;
}

/*********** C A C H E   H I N T S ************/

int __stdcall Cache::SetCacheHints(int cachehints, int frame_range) {

  _RPT3(0, "Cache:%x: Setting cache hints (hints:%d, range:%d )\n", this, cachehints, frame_range);

  switch (cachehints) {
    // Detect if we are a cache, respond with our this pointer
    case GetMyThis:
      return (int)(void *)this;


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


    case CACHE_AUDIO:
    case CACHE_AUDIO_AUTO:
      if (!vi.HasAudio())
        break;

      // Range means for audio.
      // 0 == Create a default buffer (64kb).
      // Positive. Allocate X bytes for cache.
      if (frame_range == 0) {
        if (h_audiopolicy != CACHE_AUDIO_NONE)   // We already have a policy - no need for a default one.
          break;

        frame_range=256*1024;
      }

  //    EnterCriticalSection(&cs_cache_A);
      if (frame_range/samplesize > maxsamplecount) { // Only make bigger
        char *newcache = new char[frame_range];
        if (newcache) {
          maxsamplecount = frame_range/samplesize;
          if (cache) {
            // Keep old cache contents
            cache_count = min(cache_count, maxsamplecount);
            memcpy(newcache, cache, (size_t)(vi.BytesFromAudioSamples(cache_count)));
            delete[] cache;
          }
          else {
            cache_start=0;
            cache_count=0;
          }
          cache = newcache;
        }
      }

      if (cache) h_audiopolicy = cachehints; // CACHE_AUDIO or CACHE_AUDIO_AUTO:
  //    LeaveCriticalSection(&cs_cache_A);
      break;

    case CACHE_AUDIO_NONE:
    case CACHE_AUDIO_NOTHING:
  //    EnterCriticalSection(&cs_cache_A);
      if (cache)
        delete[] cache;

      cache = 0;
      cache_start = 0;
      cache_count = 0;
      maxsamplecount = 0;

      h_audiopolicy = cachehints; // CACHE_AUDIO_NONE or CACHE_AUDIO_NOTHING
  //    LeaveCriticalSection(&cs_cache_A);
      break;


    case CACHE_GET_AUDIO_POLICY: // Get the current audio policy.
      return h_audiopolicy;

    case CACHE_GET_AUDIO_SIZE: // Get the current audio cache size.
      return maxsamplecount * samplesize;


    case CACHE_GENERIC:
    case CACHE_FORCE_GENERIC:
    {
      if (!vi.HasVideo())
        break;

      int _cache_init = min(MAX_CACHED_VIDEO_FRAMES, frame_range);

      if (_cache_init > cache_init) // The max of all requests
        cache_init  = _cache_init;

      _cache_init <<= CACHE_SCALE_SHIFT; // Scale it

      if (_cache_init > cache_limit)
        cache_limit = _cache_init;

      if (h_policy != CACHE_WINDOW || cachehints == CACHE_FORCE_GENERIC) {
        h_policy = CACHE_GENERIC;  // This is default operation, a simple LRU cache
        h_span = 0;
      }

      break;
    }

    case CACHE_NOTHING:
      h_policy = CACHE_NOTHING;  // filter requested no caching.
      break;

    case CACHE_WINDOW:
      if (!vi.HasVideo())
        break;

      if (frame_range > h_span)  // Use the largest size when we have multiple clients
        h_span = min(MAX_CACHE_WINDOW, frame_range);

      h_policy = CACHE_WINDOW;  // An explicit cache of radius "frame_range" around the current frame, n.
      break;


    case CACHE_GET_POLICY: // Get the current policy.
      return h_policy;

    case CACHE_GET_WINDOW: // Get the current window h_span.
      return h_span;

    case CACHE_GET_RANGE: // Get the current generic frame range.
      return cache_init;


    case CACHE_PREFETCH_FRAME:  // Queue request to prefetch frame N.
//    QueueVideo(frame_range);
      break;

    case CACHE_PREFETCH_GO:  // Action video prefetches.
//    PrefetchVideo((IScriptEnvironment*)frame_range);
      break;


    case CACHE_PREFETCH_AUDIO_BEGIN:  // Begin queue request to prefetch audio (take critical section).
//    EnterCriticalSection(&cs_cache_A);
//    prefetch_audio_startlo = 0;
//    prefetch_audio_starthi = 0;
//    prefetch_audio_count  = 0;
      break;

    case CACHE_PREFETCH_AUDIO_STARTLO:  // Set low 32 bits of start.
//    prefetch_audio_startlo = (unsigned int)frame_range;
      break;

    case CACHE_PREFETCH_AUDIO_STARTHI:  // Set high 32 bits of start.
//    prefetch_audio_starthi = (unsigned int)frame_range;
      break;

    case CACHE_PREFETCH_AUDIO_COUNT:  // Set low 32 bits of length.
//    prefetch_audio_count = (unsigned int)frame_range;
      break;

    case CACHE_PREFETCH_AUDIO_COMMIT:  // Enqueue request transaction to prefetch audio (release critical section).
//    __int64 start = (__int64(prefetch_audio_starthi) << 32) || prefetch_audio_startlo;
//    __int64 count = prefetch_audio_count;
//    LeaveCriticalSection(&cs_cache_A);
//    QueueAudio(start, count);
      break;

    case CACHE_PREFETCH_AUDIO_GO:  // Action audio prefetch
//    PrefetchAudio((IScriptEnvironment*)frame_range);
      break;


    default:
      break;
  }
  return 0;
}

/*********** C L E A N U P ************/

Cache::~Cache() {
  // Excise me from the linked list
  _ASSERTE(*priorCache == this);
  if (nextCache) nextCache->priorCache = priorCache;
  *priorCache = nextCache;

  if (cache) {
    delete[] cache;
    cache = 0;
  }

  CachedVideoFrame *k, *j;
  for (k = video_frames.next; k!=&video_frames;)
  {
      j = k->next;
      UnProtectVFB(k);
      UnlockVFB(k);
      delete k;
      k = j;
  }
//  DeleteCriticalSection(&cs_cache_V);
//  DeleteCriticalSection(&cs_cache_A);
}

/*********** C R E A T E ************/

AVSValue __cdecl Cache::Create_Cache(AVSValue args, void*, IScriptEnvironment* env)
{
  PClip p=0;

  if (args.IsClip())
      p = args.AsClip();
  else
      p = args[0].AsClip();

  if (p) {
    int q = 0;

    if (p->GetVersion() > 5) // AVISYNTH_INTERFACE_VERSION which supports this
      q = p->SetCacheHints(GetMyThis, 0); // Check if "p" is a cache instance

    // Do not cache another cache!
    if (q != (int)(void *)p) return new Cache(p, env);
  }
  return p;
}

