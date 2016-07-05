#ifndef _AVS_FILT_PREFETCHER_H
#define _AVS_FILT_PREFETCHER_H

#include <avisynth.h>

struct PrefetcherPimpl;
class IScriptEnvironmentInternal;

class Prefetcher : public IClip
{
private:

  PrefetcherPimpl * _pimpl;

  static AVSValue ThreadWorker(IScriptEnvironment2* env, void* data);
  int __stdcall SchedulePrefetch(int current_n, int prefetch_start, IScriptEnvironmentInternal* env);
  Prefetcher(const PClip& _child, int _nThreads);

public:
  ~Prefetcher();
  size_t NumPrefetchThreads() const;
  virtual PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
  virtual bool __stdcall GetParity(int n);
  virtual void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env);
  virtual int __stdcall SetCacheHints(int cachehints, int frame_range);
  virtual const VideoInfo& __stdcall GetVideoInfo();

  static AVSValue Create(AVSValue args, void*, IScriptEnvironment* env);

};

#endif // _AVS_FILT_PREFETCHER_H
