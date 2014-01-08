#ifndef _AVS_MTGUARD_H
#define _AVS_MTGUARD_H

#include "internal.h"

namespace std
{
  class mutex;
}

class MTGuard : public IClip
{
private:
  PClip* ChildFilters; 
  std::mutex *FilterMutex;
  MTMODES MTMode;
  const size_t nThreads;
  VideoInfo vi;

  MTGuard(size_t nThreads, PClip* threadFilters, MTMODES mtmode);

public:
  ~MTGuard();
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
  void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env);
  const VideoInfo& __stdcall GetVideoInfo();
  bool __stdcall GetParity(int n);
  int __stdcall SetCacheHints(int cachehints,int frame_range);

  static AVSValue __stdcall Create(const AVSFunction* func, const AVSValue& args, IScriptEnvironment2* env);
};


#endif _AVS_MTGUARD_H
