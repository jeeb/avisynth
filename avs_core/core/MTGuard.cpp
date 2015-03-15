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
#include <mutex>

#ifdef X86_32
#include <mmintrin.h>
#endif

MTGuard::MTGuard(PClip firstChild, MtMode mtmode, const AVSFunction* func, std::vector<AVSValue>* args2, std::vector<AVSValue>* args3, IScriptEnvironment2* env) :
  FilterMutex(NULL),
  MTMode(mtmode),
  nThreads(1),
  FilterFunction(func),
  FilterArgsArrStore(std::move(*args2)),
  FilterArgs(std::move(*args3)),
  Env(env)
{
  assert( ((int)mtmode > (int)MT_INVALID) && ((int)mtmode < (int)MT_MODE_COUNT) );

  ChildFilters.emplace_back(firstChild);
  vi = ChildFilters[0]->GetVideoInfo();

  Env->ManageCache(MC_RegisterMTGuard, reinterpret_cast<void*>(this));
}

MTGuard::~MTGuard()
{
  Env->ManageCache(MC_UnRegisterMTGuard, reinterpret_cast<void*>(this));
  delete FilterMutex;
}

void MTGuard::EnableMT(size_t nThreads)
{
  assert(nThreads >= 1);

  this->nThreads = nThreads;

  if (nThreads > 1)
  {
    switch (MTMode)
    {
    case MT_NICE_FILTER:
      {
        // Nothing to do
        break;
      }
    case MT_MULTI_INSTANCE:
      {
        ChildFilters.reserve(nThreads);
        AVSValue args(FilterArgs.data(), FilterArgs.size());
        while (ChildFilters.size() < nThreads)
        {
          ChildFilters.emplace_back(FilterFunction->apply(args, FilterFunction->user_data, Env).AsClip());
        }
        break;
      }
    case MT_SERIALIZED:
      {
        this->FilterMutex = new std::mutex();
        break;
      }
    default:
      {
        assert(0);
        break;
      }
    }
  }

  // We don't need the stored parameters any more,
  // free their memory.
  std::vector<AVSValue>().swap(FilterArgs);
  std::vector<AVSValue>().swap(FilterArgsArrStore);
}

PVideoFrame __stdcall MTGuard::GetFrame(int n, IScriptEnvironment* env)
{
  assert(nThreads > 0);

  if (nThreads == 1)
    return ChildFilters[0]->GetFrame(n, env);

  IScriptEnvironment2 *env2 = static_cast<IScriptEnvironment2*>(env);
  PVideoFrame frame = NULL;

  switch (MTMode)
  {
  case MT_NICE_FILTER:
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
      std::lock_guard<std::mutex> lock(*FilterMutex);
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

  if (nThreads == 1)
  {
    ChildFilters[0]->GetAudio(buf, start, count, env);
    return;
  }

  IScriptEnvironment2 *env2 = static_cast<IScriptEnvironment2*>(env);

  switch (MTMode)
  {
  case MT_NICE_FILTER:
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
      std::lock_guard<std::mutex> lock(*FilterMutex);
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
  return 0;
}

bool __stdcall MTGuard::IsMTGuard(const PClip& p)
{
  return ((p->GetVersion() >= 5) && (p->SetCacheHints(CACHE_IS_MTGUARD_REQ, 0) == CACHE_IS_MTGUARD_ANS));
}

AVSValue MTGuard::Create(const AVSFunction* func, std::vector<AVSValue>* args2, std::vector<AVSValue>* args3, IScriptEnvironment2* env)
{
  AVSValue avsargs(args3->data(), (int)args3->size());
  AVSValue func_result = func->apply(avsargs, func->user_data, env);

  if (func_result.IsClip() && !Cache::IsCache(func_result.AsClip()) && !MTGuard::IsMTGuard(func_result.AsClip()))
  {
    PClip filter_instance = func_result.AsClip();

    bool mode_forced;
    MtMode mode = env->GetFilterMTMode(func, &mode_forced);
    /*if ( !mode_forced
      && (filter_instance->GetVersion() >= 5)
      && (filter_instance->SetCacheHints(CACHE_GET_MTMODE, 0) != 0) )
    {
      mode = (MtMode)filter_instance->SetCacheHints(CACHE_GET_MTMODE, 0);
    }*/

    switch (mode)
    {
    case MT_NICE_FILTER:
      {
        return func_result;
      }
    case MT_MULTI_INSTANCE: // Fall-through intentional
    case MT_SERIALIZED:
      {
        return new MTGuard(filter_instance, mode, func, args2, args3, env);
        // args2 and args3 are not valid after this point anymore
      }
    default:
      // There are broken plugins out there in the wild that have (GetVersion() >= 5), but still 
      // return garbage for SetCacheHints(). This default label should also catch those.
      assert(0);
      // TODO: Log warning about probably broken plugin
      return new MTGuard(filter_instance, MT_SERIALIZED, func, args2, args3, env);
    }
  }
  else
  {
    return func_result;
  }
}
