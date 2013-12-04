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

#include <avisynth.h>
#include <boost/circular_buffer.hpp>
#include "LruCache.h"

class CacheStats
{
private:
  size_t TotalRequests_;
  size_t TotalMisses_;
  size_t MissAvgWindowSum_;
  boost::circular_buffer<int> MissAvgWindow_;

public:
  CacheStats() : 
    TotalRequests_(0),
    TotalMisses_(0),
    MissAvgWindowSum_(0),
    MissAvgWindow_(32, 0)
  {}

  void AddMiss()
  {
    ++TotalRequests_;
    ++TotalMisses_;

    if (MissAvgWindow_.full())
    {
      MissAvgWindowSum_ -= MissAvgWindow_.back();
      MissAvgWindow_.pop_back();
    }
    MissAvgWindowSum_ += 1;
    MissAvgWindow_.push_front(1);
  }

  void AddHit()
  {
    ++TotalRequests_;

    if (MissAvgWindow_.full())
    {
      MissAvgWindowSum_ -= MissAvgWindow_.back();
      MissAvgWindow_.pop_back();
    }
    MissAvgWindowSum_ += 0;
    MissAvgWindow_.push_front(0);
  }

  float WindowedMissAvg() const
  {
    if (MissAvgWindow_.full())
    {
      return float(MissAvgWindowSum_) / MissAvgWindow_.size();
    }
    else
    {
      return 1.0f;
    }
  }
};

class Cache : public IClip
{
private:
  PClip child; 
  VideoInfo vi;

  // Video cache
  CachePolicyHint VideoPolicy;
  int VideoCacheWindowRange;
  boost::shared_ptr<LruCache<size_t, PVideoFrame> > VideoCache;
  int   StatsLastCheck;
  int   StatsLastCheckCooldown;
  float StatsLastResult;
  int  CacheCanEnlarge;
  CacheStats VideoCacheStats;

  // Audio cache
  CachePolicyHint AudioPolicy;
  char* AudioCache;
  size_t SampleSize;
  size_t MaxSampleCount;

public:
  Cache(const PClip& child);
  ~Cache();
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
  void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env);
  const VideoInfo& __stdcall GetVideoInfo();
  bool __stdcall GetParity(int n);
  int __stdcall SetCacheHints(int cachehints,int frame_range);

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);
  static bool __stdcall IsCache(const PClip& c);
};

#endif  // __Cache_H__
