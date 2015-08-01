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

#ifndef __Cache_H__
#define __Cache_H__

#include "../internal.h"

/********************************************************************
********************************************************************/



class Cache : public GenericVideoFilter
/**
  * Manages a video frame cache
 **/
{
  friend class ScriptEnvironment;

public:
  Cache(PClip _child, IScriptEnvironment* env);
  ~Cache();
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
  int __stdcall SetCacheHints(int cachehints,int frame_range);
  static AVSValue __cdecl Create_Cache(AVSValue args, void*, IScriptEnvironment* env);
  void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env);

protected:
  Cache *nextCache, **priorCache;

  enum PC_Keys {
    PC_Nil=0,
    PC_UnlockOld,
    PC_UnlockAll,
    PC_UnProtect,
    PC_UnProtectAll
  };

  void PokeCache(int key, size_t size, IScriptEnvironment* env);

private:
  enum {GetMyThis = 0x8686 };

  struct CachedVideoFrame;
  void RegisterVideoFrame(CachedVideoFrame *i, const PVideoFrame& frame);
  void FillZeros(void* buf, int start_offset, int count);
  void ResetCache(IScriptEnvironment* env);
  void ReturnVideoFrameBuffer(CachedVideoFrame *i, IScriptEnvironment* env);
  CachedVideoFrame* GetACachedVideoFrame(const PVideoFrame& frame, IScriptEnvironment* env);
  VideoFrame* BuildVideoFrame(CachedVideoFrame *i, int n);
  bool LockVFB(CachedVideoFrame *i, IScriptEnvironment* env);
  bool UnlockVFB(CachedVideoFrame *i);
  void ProtectVFB(CachedVideoFrame *i, int n, IScriptEnvironment* env);
  bool UnProtectVFB(CachedVideoFrame *i);
  PVideoFrame __stdcall childGetFrame(int n, IScriptEnvironment* env);

//  void QueueVideo(int frame_range);
//  void PrefetchVideo(IScriptEnvironment* env);
//  void QueueAudio(_int64 start, _int64 count);
//  void PrefetchAudio(IScriptEnvironment* env);

  struct CachedVideoFrame 
  {
    CachedVideoFrame *prev, *next;
    VideoFrameBuffer* vfb;
    int sequence_number;
    size_t offset;
    int pitch, row_size, height;
    size_t offsetU, offsetV;
    int pitchUV, row_sizeUV, heightUV;
	int frame_number;
    long faults;  // the number of times this frame was requested and found to be stale(modified)
    long vfb_locked;
    long vfb_protected;
//    int status;

    CachedVideoFrame() { 
        next=prev=this; 
        vfb=0; 
        frame_number=-1; 
        faults=0;
        vfb_locked=0;
        vfb_protected=0;
//        status=0;
    }
  };
  CachedVideoFrame video_frames;

  // Video cache
  int h_policy;
  int h_span;
  long protectcount;
//  CRITICAL_SECTION cs_cache_V;

  // Audio cache:
//  CRITICAL_SECTION cs_cache_A;
  int h_audiopolicy;
  char * cache;
  int samplesize;
  int maxsamplecount;
  __int64 cache_start;
  __int64 cache_count;

  // For audio cache prediction
  __int64 ac_expected_next;
  int ac_too_small_count;
  long ac_currentscore;

  // Cached range limits
  int minframe, maxframe;
  int cache_init;   // The Initial cache size
  long cache_limit;  // 16 time the current maximum number of CachedVideoFrame entries
  long fault_rate;   // A decaying average of 100 times the peak fault count, used to control vfb auto-locking
  long miss_count;   // Count of consecutive cache misses

  long Tick;
  // These are global to all Cache instances
  static long Clock;
  static long cacheDepth;

  int childcost;       // Child estimated processing cost.
  int childaccesscost; // Child preferred access pattern.
  int childthreadmode; // Child thread safetyness.

//  unsigned int prefetch_audio_startlo;
//  unsigned int prefetch_audio_starthi;
//  unsigned int prefetch_audio_count;

  enum {
    // Old 2.5 poorly defined cache hints.
    // Reserve values used by 2.5 API
    // Do not use in new filters
    CACHE_25_NOTHING=0, 
    CACHE_25_RANGE=1,
    CACHE_25_ALL=2,
    CACHE_25_AUDIO=3,
    CACHE_25_AUDIO_NONE=4,
    CACHE_25_AUDIO_AUTO=5,
  };
};

#endif  // __Cache_H__
