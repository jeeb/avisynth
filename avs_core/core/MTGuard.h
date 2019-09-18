#ifndef _AVS_MTGUARD_H
#define _AVS_MTGUARD_H

#include "internal.h"
#include <vector>
#include <memory>

class InternalEnvironment;

namespace std
{
  class mutex;
}

class FilterConstructor;
class MTGuard : public IClip
{
private:
  IScriptEnvironment2* Env;

  std::vector<PClip> ChildFilters;
  std::mutex *FilterMutex;
  size_t nThreads;
  VideoInfo vi;

  std::unique_ptr<const FilterConstructor> FilterCtor;
  const MtMode MTMode;

public:
  ~MTGuard();
  MTGuard(PClip firstChild, MtMode mtmode, std::unique_ptr<const FilterConstructor> &&funcCtor, InternalEnvironment* env);
  void EnableMT(size_t nThreads);
  std::mutex* GetMutex() const;

  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
  void __stdcall GetAudio(void* buf, int64_t start, int64_t count, IScriptEnvironment* env);
  const VideoInfo& __stdcall GetVideoInfo();
  bool __stdcall GetParity(int n);
  int __stdcall SetCacheHints(int cachehints,int frame_range);


  static bool __stdcall IsMTGuard(const PClip& p);
  static PClip Create(MtMode mode, PClip filterInstance, std::unique_ptr<const FilterConstructor> funcCtor, InternalEnvironment* env);
};

#ifdef USE_MT_GUARDEXIT
class MTGuardExit : public NonCachedGenericVideoFilter
{
private:
    MTGuard *guard = nullptr;
    const char *name;

public:
    MTGuardExit(const PClip &clip, const char *_name);
    void Activate(PClip &with_guard);

    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
    void __stdcall GetAudio(void* buf, int64_t start, int64_t count, IScriptEnvironment* env);
};
#endif

#endif // _AVS_MTGUARD_H
