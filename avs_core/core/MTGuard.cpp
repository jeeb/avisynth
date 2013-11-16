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

#include "MTGuard.h"
#include "cache.h"
#include "internal.h"
#include <cassert>
#include <boost/thread.hpp>

#ifdef X86_32
#include <mmintrin.h>
#endif

MTGuard::MTGuard(size_t nThreads, PClip* threadFilters, MTMODES mtmode) :
  ChildFilters(threadFilters),
  FilterMutex(NULL),
  MTMode(nThreads > 1 ? mtmode : MT_NICE_PLUGIN),
  nThreads(nThreads)
{
  assert(nThreads > 0);
  assert((nThreads > 1) || (mtmode != MT_MULTI_INSTANCE));

  vi = ChildFilters[0]->GetVideoInfo();

  if (MTMode == MT_SERIALIZED)
  {
    FilterMutex = new boost::mutex();
  }
}

MTGuard::~MTGuard()
{
  delete FilterMutex;
  delete [] ChildFilters;
}

PVideoFrame __stdcall MTGuard::GetFrame(int n, IScriptEnvironment* env)
{
  assert(nThreads > 0);
  IScriptEnvironment2 *env2 = static_cast<IScriptEnvironment2*>(env);

  PVideoFrame frame = NULL;

  switch (MTMode)
  {
  case MT_NICE_PLUGIN:
    {
      frame = ChildFilters[0]->GetFrame(n, env);
      break;
    }
  case MT_MULTI_INSTANCE:
    {
      frame = ChildFilters[env2->GetProperty(AEP_THREAD_ID)]->GetFrame(n, env);
      break;
    }
  case MT_SERIALIZED:
    {
      boost::lock_guard<boost::mutex> lock(*FilterMutex);
      frame = ChildFilters[0]->GetFrame(n, env);
      break;
    }
  default:
    {
      assert(0);
      env2->ThrowError("Invalid Avisynth logic.");
      break;
    }
  } // switch

#ifdef X86_32
  _mm_empty();
#endif

  return frame;
}

void __stdcall MTGuard::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env)
{
  assert(nThreads > 0);
  IScriptEnvironment2 *env2 = static_cast<IScriptEnvironment2*>(env);

  switch (MTMode)
  {
  case MT_NICE_PLUGIN:
    {
      ChildFilters[0]->GetAudio(buf, start, count, env);
      break;
    }
  case MT_MULTI_INSTANCE:
    {
      ChildFilters[env2->GetProperty(AEP_THREAD_ID)]->GetAudio(buf, start, count, env);
      break;
    }
  case MT_SERIALIZED:
    {
      boost::lock_guard<boost::mutex> lock(*FilterMutex);
      ChildFilters[0]->GetAudio(buf, start, count, env);
      break;
    }
  default:
    {
      assert(0);
      env2->ThrowError("Invalid Avisynth logic.");
      break;
    }
  } // switch

#ifdef X86_32
  _mm_empty();
#endif
}

const VideoInfo& __stdcall MTGuard::GetVideoInfo()
{
  return vi;
}

bool __stdcall MTGuard::GetParity(int n)
{
  return ChildFilters[0]->GetParity(n);
}

int __stdcall MTGuard::SetCacheHints(int cachehints, int frame_range)
{
  // TODO
  return 0;
}

AVSValue __stdcall MTGuard::Create(const AVSFunction* func, const AVSValue& args, IScriptEnvironment2* env)
{
  size_t nThreads = env->GetProperty(AEP_THREADPOOL_THREADS);
  AVSValue func_result = func->apply(args, func->user_data, env);

  if (func_result.IsClip() && (nThreads > 1) && !Cache::IsCache(func_result.AsClip()))
  {
    switch (env->GetFilterMTMode(func->name))
    {
    case MT_NICE_PLUGIN:
      {
        return func_result;
      }
    case MT_SERIALIZED:
      {
        PClip* children = new PClip[1];
        children[0] = func_result.AsClip();
        return new MTGuard(nThreads, children, MT_SERIALIZED);
      }
    case MT_MULTI_INSTANCE:
      {
        PClip* children = new PClip[nThreads];
        children[0] = func_result.AsClip();
        for (size_t i = 1; i < nThreads; ++i)
        {
          children[i] = func->apply(args, func->user_data, env).AsClip();
        }
        return new MTGuard(nThreads, children, MT_MULTI_INSTANCE);
      }
    default:
      assert(0);
      return NULL;
    }
  }
  else
  {
    return func_result;
  }
}
