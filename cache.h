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

#ifndef __Cache_H__
#define __Cache_H__

#include "internal.h"


/********************************************************************
********************************************************************/


class Cache : public GenericVideoFilter 
/**
  * Manages a video frame cache
 **/
{
public:
  Cache(PClip _child);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

  static AVSValue __cdecl Create_Cache(AVSValue args, void*, IScriptEnvironment* env);

private:
  void RegisterVideoFrame(const PVideoFrame& frame, int n, IScriptEnvironment*);

  struct CachedVideoFrame 
  {
    CachedVideoFrame *prev, *next;
    VideoFrameBuffer* vfb;
    int sequence_number;
    int offset, pitch, row_size, height;
    int frame_number;
    CachedVideoFrame() { next=prev=this; }
  };

  CachedVideoFrame video_frames;
};


#endif  // __Cache_H__