#include "Prefetcher.h"

#include <avisynth.h>
#include "ScriptEnvironmentTLS.h"
#include "cache.h"  // TODO we only need LruCache from here
#include <boost/make_shared.hpp>


// The number of intervals a pattern has to repeat itself to become locked
#define PATTERN_LOCK_LENGTH 3

struct PrefetcherJobParams
{
  int frame;
  Prefetcher* prefetcher;
  LruCache<size_t, PVideoFrame>::handle cache_handle;
};

AVSValue Prefetcher::ThreadWorker(IScriptEnvironment2* env, void* data)
{
  PrefetcherJobParams *ptr = (PrefetcherJobParams*)data;
  Prefetcher *prefetcher = ptr->prefetcher;
  int n = ptr->frame;
  LruCache<size_t, PVideoFrame>::handle cache_handle = ptr->cache_handle;
  delete ptr;

  try
  {
    PVideoFrame frame = prefetcher->child->GetFrame(n, env);
    prefetcher->VideoCache->commit_value(&cache_handle, frame);
    --(prefetcher->running_workers);
  }
  catch(...)
  {
    --(prefetcher->running_workers);
    prefetcher->VideoCache->rollback(&cache_handle);

    boost::lock_guard<boost::mutex> lock(prefetcher->worker_exception_mutex);
    prefetcher->worker_exception = boost::current_exception();
    prefetcher->worker_exception_present = true;
  }

  return AVSValue();
}

Prefetcher::Prefetcher(const PClip& _child, size_t _nThreads, IScriptEnvironment2 *env) :
  child(_child),
  vi(_child->GetVideoInfo()),
  VideoCache(NULL),
  nThreads(_nThreads),
  LockedPattern(1),
  PatternLength(0),
  Pattern(1),
  LastRequestedFrame(0),
  running_workers(0),
  worker_exception_present(false)
{
  env->SetPrefetcher(this);
  VideoCache = boost::make_shared<LruCache<size_t, PVideoFrame> >(_nThreads*6); // TODO
}

Prefetcher::~Prefetcher()
{
  while(running_workers > 0);
}

size_t Prefetcher::NumPrefetchThreads() const
{
  return nThreads;
}

PVideoFrame __stdcall Prefetcher::GetFrame(int n, IScriptEnvironment* env)
{
  IScriptEnvironment2 *env2 = static_cast<IScriptEnvironment2*>(env);

  int pattern = n - LastRequestedFrame;
  if (pattern == 0)
    pattern = 1;

  if (pattern != LockedPattern)
  {
    if (pattern != Pattern)
    {
      Pattern = pattern;
      PatternLength = 1;
    }
    else
    {
      ++PatternLength;
    }

    if (PatternLength == PATTERN_LOCK_LENGTH)
    {
      LockedPattern = Pattern;
    }
  }
  else
  {
    PatternLength = 0;
  }

  {
    boost::lock_guard<boost::mutex> lock(worker_exception_mutex);
    if (worker_exception_present)
    {
      boost::rethrow_exception(worker_exception);
    }
  }

  // Get requested frame
  PVideoFrame frame = NULL;
  LruCache<size_t, PVideoFrame>::handle cache_handle;
  if (!VideoCache->get_insert(n, &frame, &cache_handle))
  {
    try
    {
      frame = child->GetFrame(n, env);
      VideoCache->commit_value(&cache_handle, frame);
    }
    catch(...)
    {
      VideoCache->rollback(&cache_handle);
      throw;
    }
  }

  // Prefetch
  for(size_t i = 0; (i < nThreads) && (running_workers < nThreads); /* i incremented in body */ )
  {
    n += LockedPattern;
    if (n >= vi.num_frames)
      break;

    PVideoFrame prefetchedFrame = NULL;
    if (!VideoCache->get_insert(n, &prefetchedFrame, &cache_handle))
    {
      PrefetcherJobParams *p = new PrefetcherJobParams(); // TODO avoid heap, possibly fold into Completion object
      p->frame = n;
      p->prefetcher = this;
      p->cache_handle = cache_handle;
      env2->ParallelJob(ThreadWorker, p, NULL);
      ++running_workers;

      ++i;
    }
  }

  return frame;
}

bool __stdcall Prefetcher::GetParity(int n)
{
  return child->GetParity(n);
}

void __stdcall Prefetcher::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env)
{
  child->GetAudio(buf, start, count, env);
}

int __stdcall Prefetcher::SetCacheHints(int cachehints, int frame_range)
{
  return child->SetCacheHints(cachehints, frame_range);
}

const VideoInfo& __stdcall Prefetcher::GetVideoInfo()
{
  return vi;
}
