// Avisynth v2.5.  Copyright 2002 Ben Rudiak-Gould et al.
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
  unsigned long resets;
  unsigned long vfb_found;
  unsigned long vfb_modified;
  unsigned long vfb_stolen;
  unsigned long vfb_notfound;
  unsigned long vfb_never;
  char tag[64];
} g_Cache_stats = {0, 0, 0, 0, 0, 0, "resets, vfb_[found,modified,stolen,notfound,never]"};


AVSFunction Cache_filters[] = {
  { "InternalCache", "c", Cache::Create_Cache },                    
  { 0 }
};
 

#define DEF_CACHED_VIDEO_FRAMES   4
#define MAX_CACHE_MISSES 100
#define MAX_CACHED_VIDEO_FRAMES 200


/*******************************
 *******   Cache filter   ******
 ******************************/


Cache::Cache(PClip _child) 
: GenericVideoFilter(_child) { 

  h_policy = CACHE_ALL;  // Since hints are not used per default, this is to describe the lowest default cache mode.
  h_audiopolicy = CACHE_NOTHING;  // Don't cache audio per default.

  cache_limit = 16*DEF_CACHED_VIDEO_FRAMES;
  cache_init  = cache_limit/16;
  fault_rate = 0;
  miss_count = 0x80000000; // Hugh negative

  h_total_frames = 0; // Hint cache not init'ed
  h_radius = 0;

  ac_expected_next = 0;
  ac_currentscore = 100;

  maxframe = -1;
  minframe = vi.num_frames;

 }


/* Problem :- When Plan B: steals a vfb some poor cache instance looses a frame
 * and when it scans it's list it will dutifully return that vfb for early reuse
 * however that vfb is now owned by another cache instance that currently loves
 * the vfb, i.e. it should remain at the head of the LRU not move to the tail.
 *
 * Also the heuristic MakeWriteable() detector will be deceived when Plan B: steals
 * a VFB. It currently assumes any modified VFB are modified by the calling filter.
 *
 * Perhaps as Plan B: double increments the sequence number, assume a difference
 * of one implies calling filter modified, else more implies theft.
 */
void Cache::ReturnVideoFrameBuffer(CachedVideoFrame *i, IScriptEnvironment* env)
{
	if (!i->vfb) return;

	// if vfb was probably stolen leave it alone
	if (i->vfb->GetSequenceNumber() > i->sequence_number+1) return;

	// return vfb to vfb pool for immediate reuse
    if (i->vfb) env->ManageCache(MC_ReturnVideoFrameBuffer, i->vfb);
}


void Cache::ResetCache(IScriptEnvironment* env)
{
  ++g_Cache_stats.resets;
  CachedVideoFrame *i, *j;

  _RPT3(0, "Cache:%x: Cache Reset, cache_limit %d, cache_init %d\n", this, cache_limit, 16*cache_init);

  int count=0;
  for (i = video_frames.next; i != &video_frames; i = i->next) {
	if (++count >= cache_init) goto purge_old_frame;
  }
  return;

purge_old_frame:

  // Truncate the tail of the chain
  j = i->next;
  video_frames.prev = i;
  i->next = &video_frames;
  
  // Delete the excess CachedVideoFrames
  while (j != &video_frames) {
	i = j->next;
	ReturnVideoFrameBuffer(j, env); // Maybe return old vfb to vfb pool for early reuse
	delete j;
	j = i;
  }
  cache_limit = 16*cache_init;
}


// This seems to be an excelent place for catching out of bounds violations
// on VideoFrameBuffer's. Well at least some of them, VFB's have a small
// variable margin due to alignment constraints. Here we catch those accesses
// that violate that, for those are the ones that are going to cause problems.


#ifdef _DEBUG
PVideoFrame __stdcall Cache::childGetFrame(int n, IScriptEnvironment* env) 
{ 
  PVideoFrame result = child->GetFrame(n, env);

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
  return result;
}
#else
#define childGetFrame(n, env) child->GetFrame(n,env)
#endif


// Unfortunatly the code for "return child->GetFrame(n,env);" seems highly likely 
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
  n = min(vi.num_frames-1, max(0,n));  // Inserted to avoid requests beyond framerange.

  __asm {emms} // Protection from rogue filter authors

  if (h_policy == CACHE_NOTHING) { // don't want a cache. Typically filters that only ever seek forward.
	__asm mov ebx,ebx  // Hack! prevent compiler from trusting ebx contents across call
    PVideoFrame result = childGetFrame(n, env);
    if (result->vfb) env->ManageCache(MC_ReturnVideoFrameBuffer, result->vfb); // return vfb to vfb pool for immediate reuse
    return result;
  }

  if (h_policy == CACHE_RANGE) {  // for filters that bash a span of frames. Typically temporal filters.
    PVideoFrame result;
    bool foundframe = false;

    for (int i = 0; i<h_total_frames; i++) {
      if (h_video_frames[i]->status & CACHE_ST_USED) {
		// Check if we already have the frame
	    if (h_video_frames[i]->frame_number == n) {
		  result = new VideoFrame(h_video_frames[i]->vfb, h_video_frames[i]->offset, h_video_frames[i]->pitch,
								  h_video_frames[i]->row_size, h_video_frames[i]->height,
								  h_video_frames[i]->offsetU, h_video_frames[i]->offsetV,
								  h_video_frames[i]->pitchUV);
		  foundframe = true;
		}
		// Check if it is out of scope
		else if (abs(h_video_frames[i]->frame_number-n)>h_radius) {
		  h_video_frames[i]->status |= CACHE_ST_DELETEME;
		  if (!(h_video_frames[i]->status & CACHE_ST_HAS_BEEN_RELEASED)) {  // Has this framebuffer been released?
			UnlockVFB(h_video_frames[i]);  // We can now release this vfb.
			h_video_frames[i]->status |= CACHE_ST_HAS_BEEN_RELEASED;
		  }
		}
      }
    } // for (int i

    if (foundframe) {   // Frame was found - build a copy and return a (dumb) pointer to it.

      VideoFrame* copy = new VideoFrame(result->vfb, result->offset, result->pitch, result->row_size,
                                        result->height, result->offsetU, result->offsetV, result->pitchUV);
      _RPT2(0, "Cache2:%x: using cached copy of frame %d\n", this, n);
      
      return copy;
    }
    else {
	  __asm mov ebx,ebx  // Hack! prevent compiler from trusting ebx contents across call
      result = childGetFrame(n, env); // Should be checking the rest of cache

      // Find a place to store it.
      for (i=0 ;; i++) {
        if (i == h_total_frames)
#ifdef _DEBUG
          env->ThrowError("Cache2:%x: Internal cache error! Report this!", this);
#else
          return result; // Should never happen
#endif
        if (h_video_frames[i]->status & CACHE_ST_DELETEME)   // Frame can be deleted
          break;

        if (!(h_video_frames[i]->status & CACHE_ST_USED))    // Frame has not yet been used.
          break;
      }  
      _RPT2(0, "Cache2:%x: Miss! Now locking frame frame %d in memory\n", this, n);
    }

    if (h_video_frames[i]->status & CACHE_ST_USED) ReturnVideoFrameBuffer(h_video_frames[i], env); // return old vfb to vfb pool for early reuse

    // Store it
    h_video_frames[i]->vfb      = result->vfb;
    h_video_frames[i]->sequence_number = result->vfb->GetSequenceNumber();
    h_video_frames[i]->offset   = result->offset;
    h_video_frames[i]->offsetU  = result->offsetU;
    h_video_frames[i]->offsetV  = result->offsetV;
    h_video_frames[i]->pitch    = result->pitch;
    h_video_frames[i]->pitchUV  = result->pitchUV;
    h_video_frames[i]->row_size = result->row_size;
    h_video_frames[i]->height   = result->height;
    h_video_frames[i]->frame_number = n;
    h_video_frames[i]->faults = 0;
  // Keep this vfb to ourselves!
  // 1. This prevents theft by Plan B:
  // 2. This make the vfb readonly i.e. MakeWriteable() returns a copy
    LockVFB(h_video_frames[i]);
    h_video_frames[i]->status   = CACHE_ST_USED;

    return result;
  }

//if (h_policy == CACHE_ALL) { // The default policy, pot luck!

    if (video_frames.next->vfb_locked) {  // release the head vfb if it is locked
      UnlockVFB(video_frames.next);
      _RPT3(0, "Cache:%x: unlocking vfb %x for frame %d\n", this, video_frames.next->vfb, video_frames.next->frame_number);
    }

	CachedVideoFrame* cvf=0;

    // look for a cached copy of the frame
    if (n>=minframe && n<=maxframe) { // have we ever seen this frame?
	  miss_count = 0; // Start/Reset cache reset counter

      int c=0;
      int imaxframe = -1;
      int iminframe = vi.num_frames;
      bool found = false;

      for (CachedVideoFrame* i = video_frames.next; i != &video_frames; i = i->next) {
        ++c;
        const int ifn = i->frame_number;

        if (i->vfb_locked) {  // release the vfb if it is locked
          UnlockVFB(i);
          _RPT3(0, "Cache:%x: B. unlock vfb %x for frame %d\n", this, i->vfb, i->frame_number);
        }

        if (ifn == n) {
          found = true;

          LockVFB(i);  // Lock to be sure this frame isn't updated.

          if (i->sequence_number == i->vfb->GetSequenceNumber()) {
		    ++g_Cache_stats.vfb_found;
			return BuildVideoFrame(i, n);
          }

		  ++i->faults;
		  fault_rate += 30 + c;
		  if (100*i->faults > fault_rate) fault_rate = 100*i->faults;
		  if (fault_rate > 300) fault_rate = 300;
		  _RPT3(0, "Cache:%x: stale frame %d, requests %d\n", this, n, i->faults);

          if (i->sequence_number == i->vfb->GetSequenceNumber()-1) { // Modified by a caller
		    ++g_Cache_stats.vfb_modified;
          }
		  else { // vfb has been stolen
		    ++g_Cache_stats.vfb_stolen;
            _RPT3(0, "Cache:%x: stolen vfb %x, frame %d\n", this, i->vfb, n);
		  }

          UnlockVFB(i);

		  cvf = i; // Remember this entry!
		  break;
        } // if (ifn == n) 

        if (ifn < iminframe) iminframe = ifn;
        if (ifn > imaxframe) imaxframe = ifn;

      } // for (CachedVideoFrame* i =

      if (!found) { // Cache miss - allocate extra buffers
		++g_Cache_stats.vfb_notfound;
	    int span = 1 + imaxframe-iminframe - cache_limit/16;
		if (span > 16)
	      cache_limit += 16; // Add upto one whole buffer
		else if (span >  0)
	      cache_limit += span; // Add span 16ths towards another buffer

        if (cache_limit > 16*MAX_CACHED_VIDEO_FRAMES) cache_limit = 16*MAX_CACHED_VIDEO_FRAMES;

		maxframe = imaxframe; // update the limits from what is currently cached
		minframe = iminframe;
	  }

      _RPT4(0, "Cache:%x: size %d, limit %d, fault %d\n", this, c, cache_limit, fault_rate);
    } // if (n>=minframe
	else {
      ++g_Cache_stats.vfb_never;
	  if (++miss_count > MAX_CACHE_MISSES) {
	    ResetCache(env);  // The cache isn't being accessed, reset it!
		miss_count = 0x80000000; // Hugh negative
	  }
	} // if (n>=minframe ... else
    
    if (fault_rate > 0) --fault_rate;  // decay fault rate

    _RPT4(0, "Cache:%x: generating frame %d, cache from %d to %d\n", this, n, minframe, maxframe);
    // not cached; make the filter generate it.
	__asm mov ebx,ebx  // Hack! prevent compiler from trusting ebx contents across call
    PVideoFrame result = childGetFrame(n, env);

	if (!cvf) cvf=GetACachedVideoFrame(result);

    RegisterVideoFrame(cvf, result, n, env);

	// If we have asked for a same stale frame twice, lock frames.
	if ((fault_rate > 100) && (fault_rate != 130)) { // Generated frames are subject to locking for longer
	  LockVFB(cvf);  // Increment to be sure this frame isn't modified.
	  _RPT3(0, "Cache:%x: lock vfb %x, gened frame %d\n", this, cvf->vfb, n);
	}

    return result;
//}
}


VideoFrame* Cache::BuildVideoFrame(CachedVideoFrame *i, int n)
{
  Relink(&video_frames, i, video_frames.next);   // move the matching cache entry to the front of the list
  VideoFrame* result = new VideoFrame(i->vfb, i->offset, i->pitch, i->row_size, i->height, i->offsetU, i->offsetV, i->pitchUV);

  // If we have asked for a same stale frame twice, leave frames locked.
  if ((fault_rate <= 160) || (fault_rate == 190)) { // Reissued frames are not subject to locking as readily
	UnlockVFB(i);
	_RPT2(0, "Cache:%x: using cached copy of frame %d\n", this, n);
  }
  else {
	_RPT3(0, "Cache:%x: lock vfb %x, cached frame %d\n", this, i->vfb, n);
  }
  return result;
}



Cache::CachedVideoFrame* Cache::GetACachedVideoFrame(const PVideoFrame& frame)
{
  CachedVideoFrame *i, *j;
  int count=0;

  // scan forwards for the last protected CachedVideoFrame
  for (i = video_frames.next; i != &video_frames; i = i->next) {
    if (i->vfb == frame->vfb) return i; // Don't fill cache with 100's copies of a Blankclip vfb
	if (++count >= cache_limit/16) break;
  }

  // scan backwards for a used CachedVideoFrame
  for (j = video_frames.prev; j != i; j = j->prev) {
    if (j->vfb == frame->vfb) return j; // Don't fill cache with 100's copies of a Blankclip vfb
    if (j->sequence_number != j->vfb->GetSequenceNumber()) return j;
	++count;
  }

  if (count >= MAX_CACHED_VIDEO_FRAMES) return video_frames.prev; // to many entries just steal the oldest CachedVideoFrame

  return new CachedVideoFrame; // need a new one
}


void Cache::RegisterVideoFrame(CachedVideoFrame *i, const PVideoFrame& frame, int n, IScriptEnvironment* env) 
{
  ReturnVideoFrameBuffer(i, env); // Maybe return old vfb to vfb pool for early reuse

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
  // Keep any fault history
  if (i->frame_number != n) {
    i->frame_number = n;
    i->faults = 0;
  }

  // move the newly-registered frame to the front
  Relink(&video_frames, i, video_frames.next);

  // update the limits for cached entries
  if (n < minframe) minframe = n;
  if (n > maxframe) maxframe = n;
}


void Cache::LockVFB(CachedVideoFrame *i)
{
  if (i->vfb) {
	i->vfb_locked = true;
	InterlockedIncrement(&i->vfb->refcount);
  }
}


void Cache::UnlockVFB(CachedVideoFrame *i)
{
  if (i->vfb) {
	InterlockedDecrement(&i->vfb->refcount);
	i->vfb_locked = false;
  }
}


void Cache::FillZeros(void* buf, int start_offset, int count) {

    int bps = vi.BytesPerAudioSample();
    unsigned char* byte_buf = (unsigned char*)buf;
    memset(byte_buf + start_offset * bps, 0, count * bps);
}

void __stdcall Cache::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) {
  if (count <= 0) 
    return;

  if ( (!vi.HasAudio()) || (start+count <= 0) || (start >= vi.num_audio_samples)) {
    // Complely skip.
    FillZeros(buf, 0, count);
    return;
  }

  if (start < 0) {  // Partial initial skip
    FillZeros(buf, 0, -start);  // Fill all samples before 0 with silence.
    count += start;  // Subtract start bytes from count.
    buf = ((BYTE*)buf) - (int)(start*vi.BytesPerAudioSample());   
    start = 0;
  }

  if (start+count >= vi.num_audio_samples) {  // Partial ending skip
    FillZeros(buf, 0 , count);  // Fill all samples
    count = (vi.num_audio_samples - start);
  }

  if (start < ac_expected_next)
    ac_currentscore-=25;    // revisiting old ground - a cache could help
  else if (start > ac_expected_next)
    ac_currentscore-=5;     // skiping chunks - a cache may not help
  else // (start == ac_expected_next)
    ac_currentscore +=5;    // strict linear reading - why bother with a cache

  ac_currentscore = max(min(ac_currentscore, 450), -10000000);

  if (h_audiopolicy == CACHE_NOTHING && ac_currentscore <=0) {
    SetCacheHints(CACHE_AUDIO, 0);
    _RPT1(0, "CacheAudio:%x: Automatically adding audiocache!\n", this);
  }

  if (h_audiopolicy == CACHE_AUDIO  && (ac_currentscore > 400) ) {
    SetCacheHints(CACHE_AUDIO_NONE, 0);
    _RPT1(0, "CacheAudio:%x: Automatically deleting cache!\n", this);
  }

  ac_expected_next = start + count;

  if (h_audiopolicy == CACHE_NOTHING) {
    child->GetAudio(buf, start, count, env);
    return;  // We are ok to return now!
  }

#ifdef _DEBUG
  sprintf(dbgbuf, "CA:Get st:%.6d co:%.6d .cst:%.6d cco:%.6d, sc:%d\n",
                    int(start), int(count), int(cache_start), int(cache_count), int(ac_currentscore));
  _RPT0(0,dbgbuf);
#endif

  int shiftsamples;

  if (count>maxsamplecount) {    //is cache big enough?
    _RPT1(0, "CA:%x:Cache too small->caching last audio\n", this);
    ac_too_small_count++;

// But the current cache might have 99% of what was requested??

// Populate "buf" with whats available BEFORE any resizing!
// Only GetAudio() the missing bits, worst case the whole lot,
// best case a small chunk at the begining and/or the end. Then
// repopulate the cache aligned with the end of "buf".

    if (ac_too_small_count > 2 && (maxsamplecount < vi.AudioSamplesFromBytes(4095*1024))) {  // Max size = 4MB!
      //automatically upsize cache!
      int new_size = vi.BytesFromAudioSamples(count)+1024;
      new_size = min(4096*1024, new_size);
      _RPT2(0, "CacheAudio:%x: Autoupsizing buffer to %d bytes!", this, new_size);
      SetCacheHints(CACHE_AUDIO, new_size); // updates maxsamplecount!!
    }

    child->GetAudio(buf, start, count, env);

	cache_start = start+count-maxsamplecount;
	cache_count = maxsamplecount;
    memcpy(cache, buf, vi.BytesFromAudioSamples(maxsamplecount));
    return;
  }

// Maybe :- if ( (start+count<cache_start) || ... i.e. there is no overlap at all

  if ( (start<cache_start) || (start>=(cache_start+cache_count)) ){ //first sample is before or behind cache -> restart cache
    _RPT1(0,"CA:%x: Restart\n", this);

// This is excessive, should only be asking for count!

    child->GetAudio(cache, start, maxsamplecount, env);
    cache_start = start;
    cache_count = maxsamplecount;
  } else {  //at least start sample is in cache
    if ( start + count > cache_start + cache_count ) {//cache is too short. Else all is already in the cache
      if ((start - cache_start + count)>maxsamplecount) {  //cache shifting is necessary
        shiftsamples = start - cache_start + count - maxsamplecount;

// Why this test? - Make the most of having to do a memmove
        if ( (start - cache_start)/2 > shiftsamples ) {  //shift half cache if possible
          shiftsamples = (start - cache_start)/2;
        }

        memmove(cache, cache+shiftsamples*samplesize,(cache_count-shiftsamples)*samplesize);

        cache_start = cache_start + shiftsamples;
        cache_count = cache_count - shiftsamples;
      }

// Good read just enough to complete the current request
      //append to cache
      child->GetAudio(cache + cache_count*samplesize, cache_start + cache_count, start+count-(cache_start+cache_count), env);
      cache_count += start+count-(cache_start+cache_count);
    }
  }

  //copy cache to buf
  memcpy(buf,cache + (start - cache_start)*samplesize, count*samplesize);

}

void __stdcall Cache::SetCacheHints(int cachehints,int frame_range) {
  _RPT3(0, "Cache:%x: Setting cache hints (hints:%d, range:%d )\n", this, cachehints, frame_range);

  if (cachehints == CACHE_AUDIO) {

    if (!vi.HasAudio())
      return;

    // Range means for audio.
    // 0 == Create a default buffer (64kb).
    // Positive. Allocate X bytes for cache.

    if (h_audiopolicy != CACHE_NOTHING && (frame_range == 0))   // We already have a policy - no need for a default one.
      return;

    if (h_audiopolicy != CACHE_NOTHING) 
      delete[] cache;       

    h_audiopolicy = CACHE_AUDIO;

    if (frame_range == 0)
      frame_range=64*1024;

    cache = new char[frame_range];
    samplesize = vi.BytesPerAudioSample();
    maxsamplecount = frame_range/samplesize - 1;
    cache_start=0;
    cache_count=0;  
    ac_too_small_count = 0;
    return;
  }

  if (cachehints == CACHE_AUDIO_NONE) {
    if (h_audiopolicy != CACHE_NOTHING) 
      delete[] cache;
    h_audiopolicy = CACHE_NOTHING;
  }


  if (cachehints == CACHE_ALL) {

    ReleaseHintCache();

    h_policy = CACHE_ALL;  // This is default operation, a simple LRU cache

    if (frame_range <= 0)
      cache_limit = 16*DEF_CACHED_VIDEO_FRAMES;
    else if (frame_range >= MAX_CACHED_VIDEO_FRAMES)
      cache_limit = 16*MAX_CACHED_VIDEO_FRAMES;
    else
    cache_limit = 16*frame_range;
    cache_init  = cache_limit/16;

    return;
  }

  if (cachehints == CACHE_NOTHING) {

    ReleaseHintCache();

    h_policy = CACHE_NOTHING;  // filter requested no caching.
    return;
  }

  if (cachehints == CACHE_RANGE) {

    h_policy = CACHE_RANGE;  // An explicit cache of radius "frame_range" around the current frame, n.

    if (frame_range <= h_radius)  // Use the largest size when we have multiple clients
      return;

    ReleaseHintCache();

    cache_limit = 0;
    cache_init  = 0;

    h_radius = frame_range;
    h_total_frames = frame_range*2+1;

	// Need to manually allocate h_video_frames[] and it's members so
	// I can move the members between this and the video_frames list

    h_video_frames = new CachedVideoFrame*[h_total_frames];

    for (int i = 0; i<h_total_frames; i++) {
      h_video_frames[i]=new CachedVideoFrame;
    }
  }
} 

void Cache::ReleaseHintCache()
{
  if (h_total_frames > 0) { // if we got some -- release em!
    for (int i = 0; i<h_total_frames; i++) {
      if ( (h_video_frames[i]->status & CACHE_ST_USED) &&
		  !(h_video_frames[i]->status & CACHE_ST_HAS_BEEN_RELEASED) )
        UnlockVFB(h_video_frames[i]);  // We can now release this vfb.
      delete h_video_frames[i];
	  h_video_frames[i] = 0;
    }
    delete[] h_video_frames;
    h_total_frames = 0;
    h_radius = 0;
  }
}


Cache::~Cache() {
  ReleaseHintCache();
  if (h_audiopolicy != CACHE_NOTHING)
    delete[] cache;
}

AVSValue __cdecl Cache::Create_Cache(AVSValue args, void*, IScriptEnvironment* env) 
{
  return new Cache(args[0].AsClip());
}


