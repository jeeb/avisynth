// Avisynth v1.0 beta.  Copyright 2000 Ben Rudiak-Gould.
// http://www.math.berkeley.edu/~benrg/avisynth.html

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


#include "cache.h"


/*******************************
 *******   Cache filter   ******
 ******************************/

Cache::Cache(PClip _child) 
  : GenericVideoFilter(_child) {}


PVideoFrame __stdcall Cache::GetFrame(int n, IScriptEnvironment* env) 
{
  // look for a cached copy of the frame
  int c=0;
  for (CachedVideoFrame* i = video_frames.next; i != &video_frames; i = i->next) {
    ++c;
    if (i->frame_number == n) {
      InterlockedIncrement(&i->vfb->refcount);
      if (i->sequence_number == i->vfb->GetSequenceNumber()) {
        _RPT1(0, "Cache: using cached copy of frame %d\n", n);
        // move the matching cache entry to the front of the list
        Relink(&video_frames, i, video_frames.next);
        VideoFrame* result = new VideoFrame(i->vfb, i->offset, i->pitch, i->row_size, i->height);
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
  i->pitch = frame->pitch;
  i->row_size = frame->row_size;
  i->height = frame->height;
  i->frame_number = n;
  // move the newly-registered frame to the front
  Relink(&video_frames, i, video_frames.next);
}
