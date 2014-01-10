#include "Prefetcher.h"

#include <mutex>
#include <atomic>
#include <avisynth.h>
#include "LruCache.h"
#include "ScriptEnvironmentTLS.h"

struct PrefetcherPimpl
{
  PClip child;
  VideoInfo vi;

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

  std::shared_ptr<LruCache<size_t, PVideoFrame> > VideoCache;
  std::atomic<size_t> running_workers;  
  std::mutex worker_exception_mutex;
  std::exception_ptr worker_exception;
  bool worker_exception_present;

  PrefetcherPimpl(const PClip& _child, size_t _nThreads) :
    child(_child),
    vi(_child->GetVideoInfo()),
    nThreads(_nThreads),
    LockedPattern(1),
    PatternLength(0),
    Pattern(1),
    LastRequestedFrame(0),
    VideoCache(NULL),
    running_workers(0),
    worker_exception_present(0)
  {
  }
};


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
    PVideoFrame frame = prefetcher->_pimpl->child->GetFrame(n, env);
    prefetcher->_pimpl->VideoCache->commit_value(&cache_handle, &frame);
    --(prefetcher->_pimpl->running_workers);
  }
  catch(...)
  {
    prefetcher->_pimpl->VideoCache->rollback(&cache_handle);
    --(prefetcher->_pimpl->running_workers);

    std::lock_guard<std::mutex> lock(prefetcher->_pimpl->worker_exception_mutex);
    prefetcher->_pimpl->worker_exception = std::current_exception();
    prefetcher->_pimpl->worker_exception_present = true;
  }

  return AVSValue();
}

Prefetcher::Prefetcher(const PClip& _child, size_t _nThreads, IScriptEnvironment2 *env) :
  _pimpl(NULL)
{
  _pimpl = new PrefetcherPimpl(_child, _nThreads);
  _pimpl->VideoCache = std::make_shared<LruCache<size_t, PVideoFrame> >(_nThreads*6); // TODO

  env->SetPrefetcher(this);
}

Prefetcher::~Prefetcher()
{
  while(_pimpl->running_workers > 0);
  delete _pimpl;
}

size_t Prefetcher::NumPrefetchThreads() const
{
  return _pimpl->nThreads;
}

void __stdcall Prefetcher::SchedulePrefetch(int current_n, IScriptEnvironment2* env)
{
  bool found;
  LruCache<size_t, PVideoFrame>::handle cache_handle;
  PVideoFrame* frame = NULL;

  int n = current_n;
  while (_pimpl->running_workers < _pimpl->nThreads)
  {
    n += _pimpl->LockedPattern;
    if (n < _pimpl->vi.num_frames)
    {
      PVideoFrame* prefetchedFrame = _pimpl->VideoCache->lookup(n, &found, &cache_handle);
      if (!found && (prefetchedFrame != NULL))
      {
        PrefetcherJobParams *p = new PrefetcherJobParams(); // TODO avoid heap, possibly fold into Completion object
        p->frame = n;
        p->prefetcher = this;
        p->cache_handle = cache_handle;
        env->ParallelJob(ThreadWorker, p, NULL);
        ++_pimpl->running_workers;
      }
    }
    else
    {
      break;
    }
  }
}

PVideoFrame __stdcall Prefetcher::GetFrame(int n, IScriptEnvironment* env)
{
  IScriptEnvironment2 *env2 = static_cast<IScriptEnvironment2*>(env);

  int pattern = n - _pimpl->LastRequestedFrame;
  if (pattern == 0)
    pattern = 1;

  if (pattern != _pimpl->LockedPattern)
  {
    if (pattern != _pimpl->Pattern)
    {
      _pimpl->Pattern = pattern;
      _pimpl->PatternLength = 1;
    }
    else
    {
      ++_pimpl->PatternLength;
    }

    if (_pimpl->PatternLength == PATTERN_LOCK_LENGTH)
    {
      _pimpl->LockedPattern = _pimpl->Pattern;
    }
  }
  else
  {
    _pimpl->PatternLength = 0;
  }

  {
    std::lock_guard<std::mutex> lock(_pimpl->worker_exception_mutex);
    if (_pimpl->worker_exception_present)
    {
      std::rethrow_exception(_pimpl->worker_exception);
    }
  }

  // Prefetch
  SchedulePrefetch(n, env2);

  // Get requested frame
  bool found;
  LruCache<size_t, PVideoFrame>::handle cache_handle;

  PVideoFrame* frame = _pimpl->VideoCache->lookup(n, &found, &cache_handle);
  if (frame == NULL)
  {
      return _pimpl->child->GetFrame(n, env);
  }

  if (!found)
  {
    try
    {
      *frame = _pimpl->child->GetFrame(n, env);
      _pimpl->VideoCache->commit_value(&cache_handle, frame);
    }
    catch(...)
    {
      _pimpl->VideoCache->rollback(&cache_handle);
      throw;
    }
  }

  return *frame;
}

bool __stdcall Prefetcher::GetParity(int n)
{
  return _pimpl->child->GetParity(n);
}

void __stdcall Prefetcher::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env)
{
  _pimpl->child->GetAudio(buf, start, count, env);
}

int __stdcall Prefetcher::SetCacheHints(int cachehints, int frame_range)
{
  return _pimpl->child->SetCacheHints(cachehints, frame_range);
}

const VideoInfo& __stdcall Prefetcher::GetVideoInfo()
{
  return _pimpl->vi;
}
