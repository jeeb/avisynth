#ifndef _AVS_FILT_PREFETCHER_H
#define _AVS_FILT_PREFETCHER_H

#include <avisynth.h>
#include "cache.h"  // TODO we only need LruCache from here


class Prefetcher : public IClip
{
private:

  PClip child;
  VideoInfo vi;

  boost::shared_ptr<LruCache<size_t, PVideoFrame> > VideoCache;

  // The number of threads to use for prefetching
  const size_t nThreads;

  // Contains the pattern we are locked on to
  int LockedPattern;

  // The number of consecutive frames Pattern has repeated itself
  int PatternLength;

  // The current pattern that we are not locked on to
  int Pattern;

  // The frame number that GetFrame() has been called with the last time
  int LastRequestedFrame;

  boost::atomic<size_t> running_workers;  
  std::vector<IJobCompletion*> completions;
  size_t front;

  static AVSValue ThreadWorker(IScriptEnvironment2* env, void* data);

public:
  Prefetcher(const PClip& _child, size_t _nThreads, IScriptEnvironment2 *env);
  ~Prefetcher();
  size_t NumPrefetchThreads() const;
  virtual PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
  virtual bool __stdcall GetParity(int n);
  virtual void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env);
  virtual int __stdcall SetCacheHints(int cachehints, int frame_range);
  virtual const VideoInfo& __stdcall GetVideoInfo();

};

#endif // _AVS_FILT_PREFETCHER_H
