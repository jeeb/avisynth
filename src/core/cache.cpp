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

AVSFunction Cache_filters[] = {
  { "InternalCache", "c", Cache::Create_Cache },                    
  { 0 }
};
 

/*******************************
 *******   Cache filter   ******
 ******************************/


Cache::Cache(PClip _child) 
: GenericVideoFilter(_child) { 

  use_hints=false;   // Default policy is to cache all frames
  h_policy = CACHE_NOTHING;  // Since hints are not used per default, this is solely to describe the lowest default cache mode.
  h_audiopolicy = CACHE_NOTHING;  // Don't cache audio per default.

  ac_expected_next = 0;
  ac_currentscore = 100;

 }


PVideoFrame __stdcall Cache::GetFrame(int n, IScriptEnvironment* env) 
{ 
  n = min(vi.num_frames-1, max(0,n));  // Inserted to avoid requests beyond framerange.

  __asm {emms} 

  if (use_hints) {

    if (h_policy == CACHE_NOTHING) {
      return child->GetFrame(n,env);
    }

    PVideoFrame result;
    bool foundframe = false;
    for (int i = 0; i<h_total_frames; i++) {
      // Check if we already have the frame
      if ((h_status[i] & CACHE_ST_USED) && (h_frame_nums[i] ==n)) {
        result = new VideoFrame(h_vfb[i], h_video_frames[i]->offset, h_video_frames[i]->pitch, h_video_frames[i]->row_size, h_video_frames[i]->height, h_video_frames[i]->offsetU, h_video_frames[i]->offsetV, h_video_frames[i]->pitchUV);
        foundframe = true;
      }
      // Check if if can be used for deletion
      if ((h_status[i] & CACHE_ST_USED) && (abs(h_frame_nums[i]-n)>h_radius) ) {
        h_status[i] |= CACHE_ST_DELETEME;
        if (!(h_status[i] & CACHE_ST_HAS_BEEN_RELEASED)) {  // Has this framebuffer been released?
          InterlockedDecrement(&h_vfb[i]->refcount);  // We can now release this vfb.
          h_status[i] |= CACHE_ST_HAS_BEEN_RELEASED;
        }
      }
    }
    if (!foundframe) {
      result = child->GetFrame(n, env);
      // Find a place to store it.
      bool notfound = true;
      i = -1;
      while (notfound) {
        i++;
        if (i == h_total_frames)
          env->ThrowError("Internal cache error! Report this!");
        if (h_status[i] & CACHE_ST_DELETEME) {  // Frame can be deleted
          notfound = false;
        }
        if (!(h_status[i] & CACHE_ST_USED)) { // Frame has not yet been used.
          notfound = false;
        }
      }  
      _RPT1(0, "Cache2: Miss! Now locking frame frame %d in memory\n", n);
    } else {   // Frame was found - copy
      VideoFrame* copy = new VideoFrame(result->vfb, result->offset, result->pitch, result->row_size, result->height, result->offsetU, result->offsetV, result->pitchUV);
      _RPT1(0, "Cache2: using cached copy of frame %d\n", n);
      return copy;
    }// Store it

    h_vfb[i] = result->vfb;
    h_frame_nums[i] = n;
    h_video_frames[i]->offset = result->offset;
    h_video_frames[i]->offsetU = result->offsetU;
    h_video_frames[i]->offsetV = result->offsetV;
    h_video_frames[i]->pitch = result->pitch;
    h_video_frames[i]->pitchUV = result->pitchUV;
    h_video_frames[i]->row_size = result->row_size;
    h_video_frames[i]->height = result->height;
    h_status[i] = CACHE_ST_USED;
    InterlockedIncrement(&result->vfb->refcount);  // Keep this vfb to ourselves!

    return result;
  }

  // look for a cached copy of the frame
  int c=0;
  for (CachedVideoFrame* i = video_frames.next; i != &video_frames; i = i->next) {
    ++c;
    if (i->frame_number == n) {
      InterlockedIncrement(&i->vfb->refcount);  // Increment to be sure this frame isn't used.
      if (i->sequence_number == i->vfb->GetSequenceNumber()) {
        _RPT1(0, "Cache: using cached copy of frame %d\n", n);
        // move the matching cache entry to the front of the list
        Relink(&video_frames, i, video_frames.next);
        VideoFrame* result = new VideoFrame(i->vfb, i->offset, i->pitch, i->row_size, i->height, i->offsetU, i->offsetV, i->pitchUV);
        InterlockedDecrement(&i->vfb->refcount);
        return result;
      }
      InterlockedDecrement(&i->vfb->refcount);
    }
  }
  _RPT2(0, "Cache: generating copy of frame %d (%d cached)\n", n, c);
  // not cached; make the filter generate it.
  PVideoFrame result = child->GetFrame(n, env);
  RegisterVideoFrame(result, n, env);
  return result;
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

  if (start != ac_expected_next)
    ac_currentscore-=25;
  else
    ac_currentscore +=5;

  ac_currentscore = max(min(ac_currentscore, 450), -10000000);

  if (h_audiopolicy == CACHE_NOTHING && ac_currentscore <=0) {
    SetCacheHints(CACHE_AUDIO, 0);
    _RPT0(0, "CacheAudio: Automatically adding audiocache!\n");
  }

  if (h_audiopolicy == CACHE_AUDIO  && (ac_currentscore > 400) ) {
    SetCacheHints(CACHE_AUDIO_NONE, 0);
    _RPT0(0, "CacheAudio: Automatically deleting cache!\n");
  }

  ac_expected_next = start + count;

  if (h_audiopolicy == CACHE_NOTHING) { 
    child->GetAudio(buf, start, count, env);
    return;  // We are ok to return now!
  }
	int shiftsamples;

#ifdef _DEBUG
	sprintf(dbgbuf, "CA:Get st:%.6d co:%.6d .cst:%.6d cco:%.6d, sc:%d\n", int(start), int(count), int(cache_start), int(cache_count), int(ac_currentscore));	
	_RPT0(0,dbgbuf);
#endif

	if (count>maxsamplecount) {		//is cache big enough?
		_RPT0(0, "CA:Cache too small->caching last audio\n");
    ac_too_small_count++;

    if (ac_too_small_count > 2 && (maxsamplecount < vi.AudioSamplesFromBytes(4095*1024))) {  // Max size = 4MB!
      //automatically upsize cache!
      int new_size = vi.BytesFromAudioSamples(count)+1024;
      new_size = min(4096*1024, new_size);
      _RPT1(0, "CacheAudio: Autoupsizing buffer to %d bytes!", new_size);
      SetCacheHints(CACHE_AUDIO, new_size);
    }

		child->GetAudio(buf, start, count, env);

		cache_start = start+count-maxsamplecount;
		cache_count = maxsamplecount;
    memcpy(cache, buf, vi.BytesFromAudioSamples(maxsamplecount));
		return;
	}
	
	if ( (start<cache_start) || (start>=(cache_start+cache_count)) ){ //first sample is before or behind cache -> restart cache
	  _RPT0(0,"CA: Restart\n");

		child->GetAudio(cache, start, maxsamplecount, env);
		cache_start = start;
		cache_count = maxsamplecount;
	} else {	//at least start sample is in cache
		if ( start + count > cache_start + cache_count ) {//cache is too short. Else all is already in the cache
			if ((start - cache_start + count)>maxsamplecount) {	//cache shifting is necessary
				shiftsamples = start - cache_start + count - maxsamplecount;

				if ( (start - cache_start)/2 > shiftsamples ) {	//shift half cache if possible
					shiftsamples = (start - cache_start)/2;
				}

				memmove(cache, cache+shiftsamples*samplesize,(cache_count-shiftsamples)*samplesize);

				cache_start = cache_start + shiftsamples;
				cache_count = cache_count - shiftsamples;
			}

			//append to cache
			child->GetAudio(cache + cache_count*samplesize, cache_start + cache_count, start+count-(cache_start+cache_count), env);
			cache_count = cache_count + start+count-(cache_start+cache_count);
		}
	}

	//copy cache to buf
	memcpy(buf,cache + (start - cache_start)*samplesize, count*samplesize);

}

void __stdcall Cache::SetCacheHints(int cachehints,int frame_range) {   
  _RPT2(0, "Cache: Setting cache hints (hints:%d, range:%d )\n", cachehints, frame_range);

  if (cachehints == CACHE_AUDIO) {

    if (!vi.HasAudio())
      return;

    // Range means for audio.
    // 0 == Create a default buffer (64kb).
    // Positive. Allocate X bytes for cache.

    if (h_audiopolicy != CACHE_NOTHING && frame_range)   // We already have a policy - no need for a default one.
      return;

    if (h_audiopolicy != CACHE_NOTHING) 
      delete[] cache;       

    h_audiopolicy = CACHE_AUDIO;

    if (!frame_range)
      frame_range=64*1024;

    if (frame_range) {
		  cache = new char[frame_range];
		  samplesize = vi.BytesPerAudioSample();
		  maxsamplecount = frame_range/samplesize - 1;
		  cache_start=0;
		  cache_count=0;  
      ac_too_small_count = 0;
    }
    return;
  }

  if (cachehints == CACHE_AUDIO_NONE) {
    if (h_audiopolicy != CACHE_NOTHING) 
      delete[] cache;
    h_audiopolicy = CACHE_NOTHING;
  }

  if (cachehints == CACHE_ALL) {
    // This is default operation, so if another filter
    h_policy = CACHE_ALL;
  }

  if ((cachehints == CACHE_NOTHING) || (!frame_range)) {
    if (h_policy == CACHE_NOTHING) {  // no other filter set hints, or other filter requested no caching.
      use_hints=true;  // Enable hints.
      return;
    }
  }

  if (cachehints == CACHE_RANGE) {
    if (h_policy == CACHE_RANGE) {  // Do we already have a filter below us, that requested a cache range?
      if (use_hints) { // Has hints data not been erased yet?
        for (int i = 0; i<h_total_frames; i++) free(h_video_frames[i]);
        free(h_vfb);
        delete[] h_frame_nums;
        delete[] h_status;
      }
      use_hints=false;
      return;
    }

    h_radius = frame_range;
    h_total_frames = frame_range*2+1;
    h_video_frames = (CachedVideoFrame **)malloc(sizeof(CachedVideoFrame*)*h_total_frames);
    h_vfb = (VideoFrameBuffer**)malloc(sizeof(VideoFrameBuffer*)*h_total_frames);
    h_frame_nums = new int[h_total_frames];
    h_status = new int[h_total_frames];

    for (int i = 0; i<h_total_frames; i++) {
      h_video_frames[i]=new CachedVideoFrame;
      h_frame_nums[i]=-1;
      h_status[i]=0;
    }
    h_policy = CACHE_RANGE;
    use_hints = true;
  }
} 

Cache::~Cache() {
  if (use_hints) {
    if (h_policy == CACHE_RANGE) {
      for (int i = 0; i<h_total_frames; i++) {
        if ((h_status[i] & CACHE_ST_USED) && (!(h_status[i] & CACHE_ST_HAS_BEEN_RELEASED)))
          InterlockedDecrement(&h_vfb[i]->refcount);  // We can now release this vfb.
        free(h_video_frames[i]);
      }
      free(h_vfb);
      delete[] h_frame_nums;
      delete[] h_status;
    }
  }
  if (h_audiopolicy != CACHE_NOTHING)
    delete[] cache;
}

AVSValue __cdecl Cache::Create_Cache(AVSValue args, void*, IScriptEnvironment* env) 
{
  return new Cache(args[0].AsClip());
}


void Cache::RegisterVideoFrame(const PVideoFrame& frame, int n, IScriptEnvironment*) 
{
  // look for an available list elt (thread-safe, because once the
  // sequence_numbers disagree they will remain in disagreement)
  CachedVideoFrame* i;
  for (i = video_frames.prev; i != &video_frames; i = i->prev) {
    if (i->sequence_number != i->vfb->GetSequenceNumber())
      goto found_old_frame;
  }
  // need a new one
  i = new CachedVideoFrame;
found_old_frame:
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
  i->frame_number = n;
  // move the newly-registered frame to the front
  Relink(&video_frames, i, video_frames.next);
}


