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


/*******************************
 *******   Cache filter   ******
 ******************************/


Cache::Cache(PClip _child) 
: GenericVideoFilter(_child) { 

  use_hints=false;   // Default policy is to cache all frames
  h_policy = CACHE_NOTHING;  // Since hints are not used per default, this is solely to describe the lowest default cache mode.

 }


PVideoFrame __stdcall Cache::GetFrame(int n, IScriptEnvironment* env) 
{ 
  n = min(vi.num_frames-1, max(0,n));  // Inserted to avoid requests beyond framerange.

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

void __stdcall Cache::SetCacheHints(int cachehints,int frame_range) {   
  _RPT2(0, "Cache: Setting cache hints (hints:%d, range:%d )\n", cachehints, frame_range);

  /*
  if (cachehints == CACHE_CLIENT_ID) {
    h_lastID = frame_range;
    return;
  }
  */

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

// Currently not implemented.

/*

CacheClient::CacheClient(PClip _child) 
: GenericVideoFilter(_child) { clientID = rand();}

PVideoFrame __stdcall CacheClient::GetFrame(int n, IScriptEnvironment* env)  {
  n = min(vi.num_frames-1, max(0,n));  // Inserted to avoid requests beyond framerange.
  child->SetCacheHints(CACHE_CLIENT_ID,clientID);
  return child->GetFrame(n,env);
}

void __stdcall CacheClient::SetCacheHints(int cachehints,int frame_range) {   
  child->SetCacheHints(CACHE_CLIENT_ID,clientID);
  child->SetCacheHints(cachehints, frame_range);
}

AVSValue __cdecl CacheClient::Create_Cache(AVSValue args, void*, IScriptEnvironment* env) 
{
  return new Cache(args[0].AsClip());
}
*/