#ifndef _SCRIPTENVIRONMENTTLS_H
#define _SCRIPTENVIRONMENTTLS_H

#include <avisynth.h>
#include <cstdarg>
#include "vartable.h"
#include "internal.h"
#include "ThreadPool.h"
#include "BufferPool.h"
#include "InternalEnvironment.h"

#define CHECK_THREAD if(g_thread_id != thread_id) \
  core->ThrowError("Invalid ScriptEnvironment. You are using different thread's environment.")

class ScriptEnvironmentTLS : public InternalEnvironment
{
private:
  InternalEnvironment* env_; // for leak detection
  InternalEnvironment* core;
  const size_t thread_id;
  // PF 161223 why do we need thread-local global variables?
  // comment remains here until it gets cleared, anyway, I make it of no use
  VarTable var_table;
  BufferPool buffer_pool;
  volatile long refcount;

	~ScriptEnvironmentTLS()
	{
      var_table.Clear();
		env_->DecEnvCount(); // for leak detection
	}

public:
  ScriptEnvironmentTLS(size_t _thread_id, InternalEnvironment* env) :
    env_(env),
    core(NULL),
    thread_id(_thread_id),
    var_table(env->GetTopFrame()),
    buffer_pool(this),
    refcount(1)
  {
    env_->IncEnvCount(); // for leak detection
  }

  void Specialize(InternalEnvironment* _core)
  {
    core = _core->GetCoreEnvironment();
  }

  /* ---------------------------------------------------------------------------------
   *             T  L  S
   * ---------------------------------------------------------------------------------
   */

  AVSValue __stdcall GetVar(const char* name)
  {
    AVSValue val;
    if (var_table.Get(name, &val))
      return val;
    else
       throw IScriptEnvironment::NotFound();
  }

  bool __stdcall SetVar(const char* name, const AVSValue& val)
  {
    return var_table.Set(name, val);
  }

  bool __stdcall SetGlobalVar(const char* name, const AVSValue& val)
  {
    return var_table.SetGlobal(name, val);
    // or return core->SetGlobalVar(name, val); ???
    // fixme: check srestore I don't know what dm means
    // PF 161223 use real global table, runtime scripts can write globals from different threads
    // so we don't use the TLS global_var_table
  }

  void __stdcall PushContext(int level = 0)
  {
     var_table.Push();
  }

  void __stdcall PopContext()
  {
     var_table.Pop();
  }

  void __stdcall PushContextGlobal()
  {
     var_table.PushGlobal();
  }

  void __stdcall PopContextGlobal()
  {
     var_table.PopGlobal();
  }

  bool __stdcall GetVar(const char* name, AVSValue* val) const
  {
     return var_table.Get(name, val);
  }

  AVSValue __stdcall GetVarDef(const char* name, const AVSValue& def)
  {
    AVSValue val;
    if (this->GetVar(name, &val))
      return val;
    else
      return def;
  }

  bool __stdcall GetVar(const char* name, bool def) const
  {
    AVSValue val;
    if (this->GetVar(name, &val))
      return val.AsBool(def);
    else
      return def;
  }

  int __stdcall GetVar(const char* name, int def) const
  {
    AVSValue val;
    if (this->GetVar(name, &val))
      return val.AsInt(def);
    else
      return def;
  }

  double __stdcall GetVar(const char* name, double def) const
  {
    AVSValue val;
    if (this->GetVar(name, &val))
      return val.AsDblDef(def);
    else
      return def;
  }

  const char* __stdcall GetVar(const char* name, const char* def) const
  {
    AVSValue val;
    if (this->GetVar(name, &val))
      return val.AsString(def);
    else
      return def;
  }

  void* __stdcall Allocate(size_t nBytes, size_t alignment, AvsAllocType type)
  {
    if ((type != AVS_NORMAL_ALLOC) && (type != AVS_POOLED_ALLOC))
      return NULL;
    return buffer_pool.Allocate(nBytes, alignment, type == AVS_POOLED_ALLOC);
  }

  void __stdcall Free(void* ptr)
  {
    buffer_pool.Free(ptr);
  }


  /* ---------------------------------------------------------------------------------
   *             S T U B S
   * ---------------------------------------------------------------------------------
   */

  bool __stdcall InternalFunctionExists(const char* name)
  {
    return core->InternalFunctionExists(name);
  }

  void __stdcall AdjustMemoryConsumption(size_t amount, bool minus)
  {
    core->AdjustMemoryConsumption(amount, minus);
  }

  void __stdcall CheckVersion(int version)
  {
    core->CheckVersion(version);
  }

  int __stdcall GetCPUFlags()
  {
    return core->GetCPUFlags();
  }

  char* __stdcall SaveString(const char* s, int length = -1)
  {
    return var_table.SaveString(s, length);
  }

  char* __stdcall Sprintf(const char* fmt, ...)
  {
    va_list val;
    va_start(val, fmt);
    // do not call core->Sprintf, because cannot pass ... further
    char* result = core->VSprintf(fmt, val);
    va_end(val);
    return result;
  }

  char* __stdcall VSprintf(const char* fmt, va_list val)
  {
    return core->VSprintf(fmt, val);
  }

  void __stdcall ThrowError(const char* fmt, ...)
  {
    va_list val;
    va_start(val, fmt);
    core->VThrowError(fmt, val);
    va_end(val);
  }

  virtual void __stdcall VThrowError(const char* fmt, va_list va)
  {
    core->VThrowError(fmt, va);
  }

  virtual PVideoFrame __stdcall SubframePlanarA(PVideoFrame src, int rel_offset, int new_pitch, int new_row_size, int new_height, int rel_offsetU, int rel_offsetV, int new_pitchUV, int rel_offsetA)
  {
    return core->SubframePlanarA(src, rel_offset, new_pitch, new_row_size, new_height, rel_offsetU, rel_offsetV, new_pitchUV, rel_offsetA);
  }

  void __stdcall AddFunction(const char* name, const char* params, ApplyFunc apply, void* user_data = 0)
  {
    core->AddFunction(name, params, apply, user_data);
  }

  bool __stdcall FunctionExists(const char* name)
  {
    return core->FunctionExists(name);
  }

  AVSValue __stdcall Invoke(const char* name, const AVSValue args, const char* const* arg_names = 0)
  {
    AVSValue result;
    if (!core->InvokeThread(&result, name, args, arg_names, this)) {
      throw NotFound();
    }
    return result;
  }

  PVideoFrame __stdcall NewVideoFrame(const VideoInfo& vi, int align)
  {
    return core->NewVideoFrame(vi, align);
  }

  bool __stdcall MakeWritable(PVideoFrame* pvf)
  {
    return core->MakeWritable(pvf);
  }

  void __stdcall BitBlt(BYTE* dstp, int dst_pitch, const BYTE* srcp, int src_pitch, int row_size, int height)
  {
    core->BitBlt(dstp, dst_pitch, srcp, src_pitch, row_size, height);
  }

  void __stdcall AtExit(IScriptEnvironment::ShutdownFunc function, void* user_data)
  {
    core->AtExit(function, user_data);
  }

  PVideoFrame __stdcall Subframe(PVideoFrame src, int rel_offset, int new_pitch, int new_row_size, int new_height)
  {
    return core->Subframe(src, rel_offset, new_pitch, new_row_size, new_height);
  }

  int __stdcall SetMemoryMax(int mem)
  {
    return core->SetMemoryMax(mem);
  }

  int __stdcall SetWorkingDir(const char* newdir)
  {
    return core->SetWorkingDir(newdir);
  }

  void* __stdcall ManageCache(int key, void* data)
  {
    return core->ManageCache(key, data);
  }

  bool __stdcall PlanarChromaAlignment(IScriptEnvironment::PlanarChromaAlignmentMode key)
  {
    return core->PlanarChromaAlignment(key);
  }

  PVideoFrame __stdcall SubframePlanar(PVideoFrame src, int rel_offset, int new_pitch, int new_row_size, int new_height, int rel_offsetU, int rel_offsetV, int new_pitchUV)
  {
    return core->SubframePlanar(src, rel_offset, new_pitch, new_row_size, new_height, rel_offsetU, rel_offsetV, new_pitchUV);
  }

  void __stdcall DeleteScriptEnvironment()
  {
    core->ThrowError("Cannot delete environment from a TLS proxy.");
  }

  void __stdcall ApplyMessage(PVideoFrame* frame, const VideoInfo& vi, const char* message, int size, int textcolor, int halocolor, int bgcolor)
  {
    core->ApplyMessage(frame, vi, message, size, textcolor, halocolor, bgcolor);
  }

  const AVS_Linkage* __stdcall GetAVSLinkage()
  {
    return core->GetAVSLinkage();
  }

  /* IScriptEnvironment2 */
  virtual bool __stdcall LoadPlugin(const char* filePath, bool throwOnError, AVSValue* result)
  {
    return core->LoadPlugin(filePath, throwOnError, result);
  }

  virtual void __stdcall AddAutoloadDir(const char* dirPath, bool toFront)
  {
    core->AddAutoloadDir(dirPath, toFront);
  }

  virtual void __stdcall ClearAutoloadDirs()
  {
    core->ClearAutoloadDirs();
  }

  virtual void __stdcall AutoloadPlugins()
  {
    core->AutoloadPlugins();
  }

  virtual void __stdcall AddFunction(const char* name, const char* params, ApplyFunc apply, void* user_data, const char* exportVar)
  {
    core->AddFunction(name, params, apply, user_data, exportVar);
  }

  virtual int __stdcall IncrImportDepth()
  {
    return core->IncrImportDepth();
  }

  virtual int __stdcall DecrImportDepth()
  {
    return core->DecrImportDepth();
  }

  virtual bool __stdcall Invoke(AVSValue* result, const char* name, const AVSValue& args, const char* const* arg_names = 0)
  {
    return core->InvokeThread(result, name, args, arg_names, this);
  }

  size_t  __stdcall GetProperty(AvsEnvProperty prop)
  {
    switch (prop)
    {
    case AEP_THREAD_ID:
      return thread_id;
    default:
      return core->GetProperty(prop);
    }
  }

  virtual void __stdcall SetFilterMTMode(const char* filter, MtMode mode, bool force)
  {
    core->SetFilterMTMode(filter, mode, force);
  }

  virtual MtMode __stdcall GetFilterMTMode(const AVSFunction* filter, bool* is_forced) const
  {
    return core->GetFilterMTMode(filter, is_forced);
  }

  bool __stdcall FilterHasMtMode(const AVSFunction* filter) const
  {
    return core->FilterHasMtMode(filter);
  }

  virtual IJobCompletion* __stdcall NewCompletion(size_t capacity)
  {
    return core->NewCompletion(capacity);
  }

  virtual void __stdcall ParallelJob(ThreadWorkerFuncPtr jobFunc, void* jobData, IJobCompletion* completion)
  {
    core->GetThreadPool()->QueueJob(jobFunc, jobData, this, static_cast<JobCompletion*>(completion));
  }

  virtual ClipDataStore* __stdcall ClipData(IClip* clip)
  {
    return core->ClipData(clip);
  }

  virtual MtMode __stdcall GetDefaultMtMode() const
  {
    return core->GetDefaultMtMode();
  }

  virtual void __stdcall SetLogParams(const char* target, int level)
  {
    core->SetLogParams(target, level);
  }

  virtual void __stdcall LogMsg(int level, const char* fmt, ...)
  {
    va_list val;
    va_start(val, fmt);
    core->LogMsg_valist(level, fmt, val);
    va_end(val);
  }
  virtual void __stdcall LogMsg_valist(int level, const char* fmt, va_list va)
  {
    core->LogMsg_valist(level, fmt, va);
  }

  virtual void __stdcall LogMsgOnce(const OneTimeLogTicket& ticket, int level, const char* fmt, ...)
  {
    va_list val;
    va_start(val, fmt);
    core->LogMsgOnce_valist(ticket, level, fmt, val);
    va_end(val);
  }

  virtual void __stdcall LogMsgOnce_valist(const OneTimeLogTicket& ticket, int level, const char* fmt, va_list va)
  {
    core->LogMsgOnce_valist(ticket, level, fmt, va);
  }

  virtual InternalEnvironment* __stdcall GetCoreEnvironment()
  {
    return core->GetCoreEnvironment();
  }

	virtual ThreadPool* __stdcall GetThreadPool()
	{
		return core->GetThreadPool();
	}

  virtual ThreadPool* __stdcall NewThreadPool(size_t nThreads)
  {
    return core->NewThreadPool(nThreads);
  }

  virtual bool __stdcall InvokeThread(AVSValue* result, const char* name, const AVSValue& args,
    const char* const* arg_names, IScriptEnvironment2* env)
  {
    return core->InvokeThread(result, name, args, arg_names, env);
  }

  virtual void __stdcall AddRef() {
    InterlockedIncrement(&refcount);
  }

  virtual void __stdcall Release() {
    if (InterlockedDecrement(&refcount) == 0) {
      delete this;
    }
  }

  virtual void __stdcall IncEnvCount() {
    core->IncEnvCount();
  }

  virtual void __stdcall DecEnvCount() {
    core->DecEnvCount();
  }

  virtual ConcurrentVarStringFrame* __stdcall GetTopFrame()
  {
    return core->GetTopFrame();
  }

  virtual void __stdcall CopyFrameProps(PVideoFrame src, PVideoFrame dst)
  {
    core->CopyFrameProps(src, dst);
  }

  virtual AVSMap* __stdcall GetAVSMap(PVideoFrame& frame)
  {
    return core->GetAVSMap(frame);
  }
};

#undef CHECK_THREAD
#endif  // _SCRIPTENVIRONMENTTLS_H
