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

#ifndef __Cache_H__
#define __Cache_H__

#include "internal.h"
#include <vector>
using namespace std;

/********************************************************************
********************************************************************/


//polymorphic frame cache for use by the Cache Filter
class FrameCache {

public:
  virtual PVideoFrame __stdcall  fetch(int n) { return PVideoFrame(); }
  virtual void __stdcall  store(int n, const PVideoFrame& frame) { }
  virtual ~FrameCache() { }

protected:
	struct CachedVideoFrame {
	  int n;
	  long seq_number;
	  PVideoFrame frame;
    CachedVideoFrame() : n(-1) { }
    CachedVideoFrame(int _n, const PVideoFrame& _frame);
    void set(int _n, const PVideoFrame& _frame);
    bool isValid() const ;
  };
  typedef vector<CachedVideoFrame> CacheVector;

};  

//frame cache who keeps frames within a fixed range
//frames out of range are likely to be uncached
class RangeCache : public FrameCache {

  CacheVector cache;

public:
  RangeCache(int scale) : cache(scale) { }
  
  virtual PVideoFrame __stdcall fetch(int n);
  virtual void __stdcall store(int n, const PVideoFrame& frame);
};

//frame cache who keeps the last used frames
//(fetched frames restart at the beginning of the queue)
class QueueCache : public FrameCache {

  CacheVector cache;

public:
  QueueCache(int scale) : cache(scale) { }

  virtual PVideoFrame __stdcall fetch(int n);
  virtual void __stdcall store(int n, const PVideoFrame& frame);
};



class Cache : public GenericVideoFilter 
/**
  * Manages a video frame cache
 **/
{
public:
  Cache(PClip _child);
  ~Cache();
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
  void __stdcall SetCacheHints(int cachehints,int frame_range);
  static AVSValue __cdecl Create_Cache(AVSValue args, void*, IScriptEnvironment* env);

private:
  void RegisterVideoFrame(const PVideoFrame& frame, int n, IScriptEnvironment*);

  struct CachedVideoFrame 
  {
    CachedVideoFrame *prev, *next;
    VideoFrameBuffer* vfb;
    int sequence_number;
    int offset, pitch, row_size, height, offsetU, offsetV, pitchUV;
    int frame_number;
    CachedVideoFrame() { next=prev=this; }
  };
  CachedVideoFrame video_frames;
// hint vars:
  CachedVideoFrame** h_video_frames;
  VideoFrameBuffer** h_vfb;
  int* h_frame_nums;
  int* h_status;
  int h_total_frames;
  bool use_hints;
  int h_radius;
  enum {CACHE_ST_USED = 1<<0,
        CACHE_ST_DELETEME = 1<<1,
        CACHE_ST_BEING_GENERATED = 1<<2
  };


/* Replacement code of the private section when using FrameCache
  
  FrameCache* cache;

 */

};


#endif  // __Cache_H__