#ifndef _AVS_MTGUARD_H
#define _AVS_MTGUARD_H

#include "internal.h"
#include <vector>

namespace std
{
  class mutex;
}

class MTGuard : public IClip
{
private:
  IScriptEnvironment2* Env;

  std::vector<PClip> ChildFilters; 
  std::mutex *FilterMutex;
  size_t nThreads;
  VideoInfo vi;

  const AVSFunction* FilterFunction;
  const std::vector<AVSValue> FilterArgs;
  const MtMode MTMode;


public:
  ~MTGuard();
  MTGuard(PClip firstChild, MtMode mtmode, const AVSFunction* func, const std::vector<AVSValue>& args, IScriptEnvironment2* env);
  void EnableMT(size_t nThreads);

  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
  void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env);
  const VideoInfo& __stdcall GetVideoInfo();
  bool __stdcall GetParity(int n);
  int __stdcall SetCacheHints(int cachehints,int frame_range);
};


#endif _AVS_MTGUARD_H
