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

#include <avisynth.h>
#include "../core/internal.h"
#include "./parser/script.h"
#include "cache.h"
#include <avs/minmax.h>
#include "strings.h"
#include <avs/cpuid.h>
#include "bitblt.h"
#include "PluginManager.h"

#include <avs/win.h>
#include <objbase.h>
#include "critical_guard.h"

#include <string>
#include <cstdarg>
#include <cassert>

#ifdef _MSC_VER
  #define strnicmp(a,b,c) _strnicmp(a,b,c)
#else
  #define _RPT1(x,y,z) ((void)0)
#endif

#ifndef YieldProcessor // low power spin idle
  #define YieldProcessor() __asm { rep nop }
#endif

extern const AVSFunction Audio_filters[], Combine_filters[], Convert_filters[],
                   Convolution_filters[], Edit_filters[], Field_filters[],
                   Focus_filters[], Fps_filters[], Histogram_filters[],
                   Layer_filters[], Levels_filters[], Misc_filters[],
                   Plugin_functions[], Resample_filters[], Resize_filters[],
                   Script_functions[], Source_filters[], Text_filters[],
                   Transform_filters[], Merge_filters[], Color_filters[],
                   Debug_filters[], Turn_filters[],
                   Conditional_filters[], Conditional_funtions_filters[],
                   Cache_filters[], Greyscale_filters[],
                   Swap_filters[], Overlay_filters[];


const AVSFunction* builtin_functions[] = {
                   Audio_filters, Combine_filters, Convert_filters,
                   Convolution_filters, Edit_filters, Field_filters,
                   Focus_filters, Fps_filters, Histogram_filters,
                   Layer_filters, Levels_filters, Misc_filters,
                   Resample_filters, Resize_filters,
                   Script_functions, Source_filters, Text_filters,
                   Transform_filters, Merge_filters, Color_filters,
                   Debug_filters, Turn_filters,
                   Conditional_filters, Conditional_funtions_filters,
                   Plugin_functions, Cache_filters,
                   Overlay_filters, Greyscale_filters, Swap_filters};

// Global statistics counters
struct {
  unsigned int CleanUps;
  unsigned int Losses;
  unsigned int PlanA1;
  unsigned int PlanA2;
  unsigned int PlanB;
  unsigned int PlanC;
  unsigned int PlanD;
  char tag[36];
} g_Mem_stats = {0, 0, 0, 0, 0, 0, 0, "CleanUps, Losses, Plan[A1,A2,B,C,D]"};

const _PixelClip PixelClip;

template<typename T>
static T AlignNumber(T n, T align)
{
  assert(align && !(align & (align - 1)));  // check that 'align' is a power of two
  return (n + align-1) & (~(align-1));
}
template<typename T>
static T AlignPointer(T n, size_t align)
{
  assert(align && !(align & (align - 1)));  // check that 'align' is a power of two
  return (T)(((uintptr_t)n + align-1) & (~(uintptr_t)(align-1)));
}


class LinkedVideoFrame {
public:
  LinkedVideoFrame* next;
  VideoFrame vf;
};

class RecycleBin {  // Tritical May 2005
public:
    LinkedVideoFrame* volatile g_VideoFrame_recycle_bin;
    RecycleBin() : g_VideoFrame_recycle_bin(NULL) { };
    ~RecycleBin()
    {
        for (LinkedVideoFrame* i=g_VideoFrame_recycle_bin; i;)
        {
            LinkedVideoFrame* j = i->next;
            operator delete(i);
            i = j;
        }
    }
};


// Tsp June 2005 the heap is cleared when ScriptEnviroment is destroyed

static RecycleBin *g_Bin=0;

void* VideoFrame::operator new(unsigned) {
  // CriticalSection
  for (LinkedVideoFrame* i = g_Bin->g_VideoFrame_recycle_bin; i; i = i->next)
    if (InterlockedCompareExchange(&i->vf.refcount, 1, 0) == 0)
      return &i->vf;

  LinkedVideoFrame* result = (LinkedVideoFrame*)::operator new(sizeof(LinkedVideoFrame));
  result->vf.refcount=1;

  // SEt 13 Aug 2009
  for (;;) {
    result->next = g_Bin->g_VideoFrame_recycle_bin;
    if (InterlockedCompareExchangePointer((void *volatile *)&g_Bin->g_VideoFrame_recycle_bin, result, result->next) == result->next) break;
    YieldProcessor(); // low power spin idle
  }

  return &result->vf;
}


VideoFrame::VideoFrame(VideoFrameBuffer* _vfb, int _offset, int _pitch, int _row_size, int _height)
  : refcount(1), vfb(_vfb), offset(_offset), pitch(_pitch), row_size(_row_size), height(_height),
    offsetU(_offset),offsetV(_offset),pitchUV(0), row_sizeUV(0), heightUV(0)  // PitchUV=0 so this doesn't take up additional space
{
  InterlockedIncrement(&vfb->refcount);
}

VideoFrame::VideoFrame(VideoFrameBuffer* _vfb, int _offset, int _pitch, int _row_size, int _height,
                       int _offsetU, int _offsetV, int _pitchUV, int _row_sizeUV, int _heightUV)
  : refcount(1), vfb(_vfb), offset(_offset), pitch(_pitch), row_size(_row_size), height(_height),
    offsetU(_offsetU),offsetV(_offsetV),pitchUV(_pitchUV), row_sizeUV(_row_sizeUV), heightUV(_heightUV)
{
  InterlockedIncrement(&vfb->refcount);
}

// Hack note :- Use of SubFrame will require an "InterlockedDecrement(&retval->refcount);" after
// assignement to a PVideoFrame, the same as for a "New VideoFrame" to keep the refcount consistant.

VideoFrame* VideoFrame::Subframe(int rel_offset, int new_pitch, int new_row_size, int new_height) const {
  return new VideoFrame(vfb, offset+rel_offset, new_pitch, new_row_size, new_height);
}


VideoFrame* VideoFrame::Subframe(int rel_offset, int new_pitch, int new_row_size, int new_height,
                                 int rel_offsetU, int rel_offsetV, int new_pitchUV) const {
  // Maintain plane size relationship
  const int new_row_sizeUV = !row_size ? 0 : MulDiv(new_row_size, row_sizeUV, row_size);
  const int new_heightUV   = !height   ? 0 : MulDiv(new_height,   heightUV,   height);

  return new VideoFrame(vfb, offset+rel_offset, new_pitch, new_row_size, new_height,
                        rel_offsetU+offsetU, rel_offsetV+offsetV, new_pitchUV, new_row_sizeUV, new_heightUV);
}


VideoFrameBuffer::VideoFrameBuffer() : refcount(1), data(0), data_size(0), sequence_number(0) {}


#ifdef _DEBUG  // Add 16 guard bytes front and back -- cache can check them after every GetFrame() call
VideoFrameBuffer::VideoFrameBuffer(int size) :
  refcount(1),
  data((new(std::nothrow) BYTE[size+32])+16),
  data_size(data ? size : 0),
  sequence_number(0) {
  InterlockedIncrement(&sequence_number);
  int *p=(int *)data;
  p[-4] = 0xDEADBEAF;
  p[-3] = 0xDEADBEAF;
  p[-2] = 0xDEADBEAF;
  p[-1] = 0xDEADBEAF;
  p=(int *)(data+size);
  p[0] = 0xDEADBEAF;
  p[1] = 0xDEADBEAF;
  p[2] = 0xDEADBEAF;
  p[3] = 0xDEADBEAF;
}

VideoFrameBuffer::~VideoFrameBuffer() {
//  _ASSERTE(refcount == 0);
  InterlockedIncrement(&sequence_number); // HACK : Notify any children with a pointer, this buffer has changed!!!
  if (data) delete[] (BYTE*)(data-16);
  (BYTE*)data = 0; // and mark it invalid!!
  (int)data_size = 0;   // and don't forget to set the size to 0 as well!
}

#else

VideoFrameBuffer::VideoFrameBuffer(int size)
 : refcount(1), data(new(std::nothrow) BYTE[size]), data_size(data ? size : 0), sequence_number(0) { InterlockedIncrement(&sequence_number); }

VideoFrameBuffer::~VideoFrameBuffer() {
//  _ASSERTE(refcount == 0);
  InterlockedIncrement(&sequence_number); // HACK : Notify any children with a pointer, this buffer has changed!!!
  if (data) delete[] data;
  (BYTE*)data = 0; // and mark it invalid!!
  (int)data_size = 0;   // and don't forget to set the size to 0 as well!
}
#endif


class LinkedVideoFrameBuffer : public VideoFrameBuffer {
public:
  enum {ident = 0x00AA5500};
  LinkedVideoFrameBuffer *prev, *next;
  bool returned;
  const int signature; // Used by ManageCache to ensure that the VideoFrameBuffer
                       // it casts is really a LinkedVideoFrameBuffer
  LinkedVideoFrameBuffer(int size) : VideoFrameBuffer(size), returned(true), signature(ident) { next=prev=this; }
  LinkedVideoFrameBuffer() : returned(true), signature(ident) { next=prev=this; }
};

#include "hashtable.h"
class VarTable {

  VarTable* const dynamic_parent;
  VarTable* const lexical_parent;
  hashtable<AVSValue> variables;

public:
  VarTable(VarTable* _dynamic_parent, VarTable* _lexical_parent) :
    dynamic_parent(_dynamic_parent), lexical_parent(_lexical_parent),
    variables(1873)    // a prime number
  {}

  VarTable* Pop() {
    VarTable* _dynamic_parent = this->dynamic_parent;
    delete this;
    return _dynamic_parent;
  }

  // This method will not modify the *val argument if it returns false.
  bool Get(const char* name, AVSValue *val) const {
    AVSValue *v = variables.get(name);
    if (v != NULL)
    {
      *val = *v;
      return true;
    }

    if (lexical_parent)
      return lexical_parent->Get(name, val);
    else
      return false;
  }

  bool Set(const char* name, const AVSValue& val) {
    return variables.add(name, val);
  }
};

// This doles out storage space for strings.  No space is ever freed
// until the class instance is destroyed (which happens when a script
// file is closed).
class StringDump {
  enum { BLOCK_SIZE = 32768 };
  char* current_block;
  unsigned int block_pos, block_size;

public:
  StringDump() : current_block(0), block_pos(BLOCK_SIZE), block_size(BLOCK_SIZE) {}
  ~StringDump();
  char* SaveString(const char* s, int len = -1);
};

StringDump::~StringDump() {
  _RPT0(0,"StringDump: DeAllocating all stringblocks.\r\n");
  char* p = current_block;
  while (p) {
    char* next = *(char**)p;
    delete[] p;
    p = next;
  }
}

char* StringDump::SaveString(const char* s, int len) {
  if (len == -1)
    len = lstrlen(s);

  if (block_pos+len+1 > block_size) {
    char* new_block = new char[block_size = max(block_size, len+1+sizeof(char*))];
    _RPT0(0,"StringDump: Allocating new stringblock.\r\n");
    *(char**)new_block = current_block;   // beginning of block holds pointer to previous block
    current_block = new_block;
    block_pos = sizeof(char*);
  }
  char* result = current_block+block_pos;
  memcpy(result, s, len);
  result[len] = 0;
  block_pos += AlignNumber(len+1, (int)sizeof(char*)); // Keep word-aligned
  return result;
}


class AtExiter {
  struct AtExitRec {
    const IScriptEnvironment::ShutdownFunc func;
    void* const user_data;
    AtExitRec* const next;
    AtExitRec(IScriptEnvironment::ShutdownFunc _func, void* _user_data, AtExitRec* _next)
      : func(_func), user_data(_user_data), next(_next) {}
  };
  AtExitRec* atexit_list;

public:
  AtExiter() {
    atexit_list = 0;
  }

  void Add(IScriptEnvironment::ShutdownFunc f, void* d) {
    atexit_list = new AtExitRec(f, d, atexit_list);
  }

  void Execute(IScriptEnvironment* env) {
    while (atexit_list) {
      AtExitRec* next = atexit_list->next;
      atexit_list->func(atexit_list->user_data, env);
      delete atexit_list;
      atexit_list = next;
    }
  }
};

class ScriptEnvironment : public IScriptEnvironment2 {
public:
  ScriptEnvironment();
  void __stdcall CheckVersion(int version);
  int __stdcall GetCPUFlags();
  char* __stdcall SaveString(const char* s, int length = -1);
  char* __stdcall Sprintf(const char* fmt, ...);
  char* __stdcall VSprintf(const char* fmt, void* val);
  void __stdcall ThrowError(const char* fmt, ...);
  void __stdcall AddFunction(const char* name, const char* params, ApplyFunc apply, void* user_data=0);
  bool __stdcall FunctionExists(const char* name);
  AVSValue __stdcall Invoke(const char* name, const AVSValue args, const char* const* arg_names=0);
  AVSValue __stdcall GetVar(const char* name);
  bool __stdcall SetVar(const char* name, const AVSValue& val);
  bool __stdcall SetGlobalVar(const char* name, const AVSValue& val);
  void __stdcall PushContext(int level=0);
  void __stdcall PopContext();
  void __stdcall PopContextGlobal();
  PVideoFrame __stdcall NewVideoFrame(const VideoInfo& vi, int align);
  PVideoFrame NewVideoFrame(int row_size, int height, int align);
  PVideoFrame NewPlanarVideoFrame(int row_size, int height, int row_sizeUV, int heightUV, int align, bool U_first);
  bool __stdcall MakeWritable(PVideoFrame* pvf);
  void __stdcall BitBlt(BYTE* dstp, int dst_pitch, const BYTE* srcp, int src_pitch, int row_size, int height);
  void __stdcall AtExit(IScriptEnvironment::ShutdownFunc function, void* user_data);
  PVideoFrame __stdcall Subframe(PVideoFrame src, int rel_offset, int new_pitch, int new_row_size, int new_height);
  int __stdcall SetMemoryMax(int mem);
  int __stdcall SetWorkingDir(const char * newdir);
  __stdcall ~ScriptEnvironment();
  void* __stdcall ManageCache(int key, void* data);
  bool __stdcall PlanarChromaAlignment(IScriptEnvironment::PlanarChromaAlignmentMode key);
  PVideoFrame __stdcall SubframePlanar(PVideoFrame src, int rel_offset, int new_pitch, int new_row_size, int new_height, int rel_offsetU, int rel_offsetV, int new_pitchUV);
  void __stdcall DeleteScriptEnvironment();
  void _stdcall ApplyMessage(PVideoFrame* frame, const VideoInfo& vi, const char* message, int size, int textcolor, int halocolor, int bgcolor);
  const AVS_Linkage* const __stdcall GetAVSLinkage();

  /* IScriptEnvironment2 */
  virtual bool  __stdcall GetVar(const char* name, AVSValue *val);
  virtual bool __stdcall GetVar(const char* name, bool def);
  virtual int  __stdcall GetVar(const char* name, int def);
  virtual double  __stdcall GetVar(const char* name, double def);
  virtual const char*  __stdcall GetVar(const char* name, const char* def);
  virtual bool __stdcall LoadPlugin(const char* filePath, bool throwOnError, AVSValue *result);
  virtual void __stdcall AddAutoloadDir(const char* dirPath, bool toFront);
  virtual void __stdcall ClearAutoloadDirs();
  virtual int __stdcall IncrImportDepth();
  virtual int __stdcall DecrImportDepth();
  virtual bool __stdcall Invoke(AVSValue *result, const char* name, const AVSValue args, const char* const* arg_names=0);

private:
  // Tritical May 2005
  // Note order here!!
  // AtExiter has functions which
  // rely on StringDump elements.
  StringDump string_dump;

  AtExiter at_exit;

  PluginManager *plugin_manager;

  VarTable* global_var_table;
  VarTable* var_table;

  int ImportDepth;

  LinkedVideoFrameBuffer video_frame_buffers, lost_video_frame_buffers, *unpromotedvfbs;
  const AVSFunction* Lookup(const char* search_name, const AVSValue* args, int num_args,
                      bool &pstrict, int args_names_count, const char* const* arg_names);
  unsigned __int64 memory_max, memory_used;

  LinkedVideoFrameBuffer* NewFrameBuffer(int size);

  LinkedVideoFrameBuffer* GetFrameBuffer2(int size);
  VideoFrameBuffer* GetFrameBuffer(int size);

  IScriptEnvironment2* This() { return this; }
  bool PlanarChromaAlignmentState;

  Cache* CacheHead;

  HRESULT hrfromcoinit;
  DWORD coinitThreadId;

  volatile static long refcount; // Global to all ScriptEnvironment objects

  static CRITICAL_SECTION cs_relink_video_frame_buffer;//tsp July 2005.

  CRITICAL_SECTION cs_var_table;

  bool closing;                 // Used to avoid deadlock, if vartable is being accessed while shutting down (Popcontext)
};

volatile long ScriptEnvironment::refcount=0;
CRITICAL_SECTION ScriptEnvironment::cs_relink_video_frame_buffer;

static unsigned __int64 ConstrainMemoryRequest(unsigned __int64 requested)
{
  // Get system memory information
  MEMORYSTATUSEX memstatus;
  memstatus.dwLength = sizeof(memstatus);
  GlobalMemoryStatusEx(&memstatus);

  // mem_limit is the largest amount of memory that makes sense to use.
  // We don't want to use more than the virtual address space,
  // and we also don't want to start paging to disk.
  unsigned __int64 mem_limit = min(memstatus.ullTotalVirtual, memstatus.ullTotalPhys);

  unsigned __int64 mem_sysreserve = 0;
  if (memstatus.ullTotalPhys > memstatus.ullTotalVirtual)
  {
    // We are probably running on a 32bit OS system where the virtual space is capped to 
    // much less than what the system can use, so it is enough to reserve only a small amount.
    mem_sysreserve = 128*1024*1024ull;  
  }
  else
  {
    // We could probably use up all the RAM in our single application, 
    // so reserve more to leave some RAM for other apps and the OS too.
    mem_sysreserve = 1024*1024*1024ull;
  }

  // Cap memory_max to at most mem_sysreserve less than total, but at least to 64MB.
  return clamp(requested, 64*1024*1024ull, mem_limit - mem_sysreserve);
}

ScriptEnvironment::ScriptEnvironment()
  : at_exit(),
    plugin_manager(NULL),
    CacheHead(0), hrfromcoinit(E_FAIL), coinitThreadId(0),
    unpromotedvfbs(&video_frame_buffers),
    closing(false),
    PlanarChromaAlignmentState(true),   // Change to "true" for 2.5.7
    ImportDepth(0)
{

  try {
    // Make sure COM is initialised
    hrfromcoinit = CoInitialize(NULL);

    // If it was already init'd then decrement
    // the use count and leave it alone!
    if(hrfromcoinit == S_FALSE) {
      hrfromcoinit=E_FAIL;
      CoUninitialize();
    }
    // Remember our threadId.
    coinitThreadId=GetCurrentThreadId();

    if(InterlockedCompareExchange(&refcount, 1, 0) == 0)//tsp June 2005 Initialize Recycle bin
    {
      g_Bin=new RecycleBin();

      // tsp June 2005 might have to change the spincount or use InitializeCriticalSection
      // if it should run on WinNT 4 without SP3 or better.
      InitializeCriticalSectionAndSpinCount(&cs_relink_video_frame_buffer, 8000);
    }
    else
      InterlockedIncrement(&refcount);

    InitializeCriticalSectionAndSpinCount(&cs_var_table, 4000);

    MEMORYSTATUSEX memstatus;
    memstatus.dwLength = sizeof(memstatus);
    GlobalMemoryStatusEx(&memstatus);
    memory_max = ConstrainMemoryRequest(memstatus.ullTotalPhys / 4);
    memory_max = min(memory_max, 1024*1024*1024ull);  // at start, cap memory usage to 1GB
    memory_used = 0ull;

    global_var_table = new VarTable(0, 0);
    var_table = new VarTable(0, global_var_table);
    global_var_table->Set("true", true);
    global_var_table->Set("false", false);
    global_var_table->Set("yes", true);
    global_var_table->Set("no", false);

    global_var_table->Set("$ScriptName$", AVSValue());
    global_var_table->Set("$ScriptFile$", AVSValue());
    global_var_table->Set("$ScriptDir$",  AVSValue());

    plugin_manager = new PluginManager(this);
    plugin_manager->AddAutoloadDir("USER_PLUS_PLUGINS", false);
    plugin_manager->AddAutoloadDir("MACHINE_PLUS_PLUGINS", false);
    plugin_manager->AddAutoloadDir("USER_CLASSIC_PLUGINS", false);
    plugin_manager->AddAutoloadDir("MACHINE_CLASSIC_PLUGINS", false);
  }
  catch (const AvisynthError &err) {
    if(SUCCEEDED(hrfromcoinit)) {
      hrfromcoinit=E_FAIL;
      CoUninitialize();
    }
    // Needs must, to not loose the text we
    // must leak a little memory.
    throw AvisynthError(_strdup(err.msg));
  }
}

ScriptEnvironment::~ScriptEnvironment() {

  closing = true;

  // Before we start to pull the world apart
  // give every one their last wish.
  at_exit.Execute(this);

  while (var_table)
    PopContext();

  while (global_var_table)
    PopContextGlobal();

  unpromotedvfbs = &video_frame_buffers;
  LinkedVideoFrameBuffer* i = video_frame_buffers.prev;
  while (i != &video_frame_buffers) {
    LinkedVideoFrameBuffer* prev = i->prev;
    delete i;
    i = prev;
  }
  i = lost_video_frame_buffers.prev;
  while (i != &lost_video_frame_buffers) {
    LinkedVideoFrameBuffer* prev = i->prev;
    delete i;
    i = prev;
  }

  delete plugin_manager;

  if(!InterlockedDecrement(&refcount)){
    delete g_Bin;//tsp June 2005 Cleans up the heap
    g_Bin=NULL;
    DeleteCriticalSection(&cs_relink_video_frame_buffer);//tsp July 2005
  }

  DeleteCriticalSection(&cs_var_table);

  // If we init'd COM and this is the right thread then release it
  // If it's the wrong threadId then tuff, nothing we can do.
  if(SUCCEEDED(hrfromcoinit) && (coinitThreadId == GetCurrentThreadId())) {
    hrfromcoinit=E_FAIL;
    CoUninitialize();
  }
}

int __stdcall ScriptEnvironment::IncrImportDepth()
{
  ImportDepth++;
  return ImportDepth;
}
int __stdcall ScriptEnvironment::DecrImportDepth()
{
  ImportDepth--;
  return ImportDepth;
}

bool __stdcall ScriptEnvironment::LoadPlugin(const char* filePath, bool throwOnError, AVSValue *result)
{
  return plugin_manager->LoadPlugin(PluginFile(filePath), throwOnError, result);
}

void __stdcall ScriptEnvironment::AddAutoloadDir(const char* dirPath, bool toFront)
{
  plugin_manager->AddAutoloadDir(dirPath, toFront);
}


void __stdcall ScriptEnvironment::ClearAutoloadDirs()
{
  plugin_manager->ClearAutoloadDirs();
}

int ScriptEnvironment::SetMemoryMax(int mem) {

  if (mem > 0)
    memory_max = ConstrainMemoryRequest(mem * 1048576ull);

  return (int)(memory_max/1048576ull);
}

int ScriptEnvironment::SetWorkingDir(const char * newdir) {
  return SetCurrentDirectory(newdir) ? 0 : 1;
}

void ScriptEnvironment::CheckVersion(int version) {
  if (version > AVISYNTH_INTERFACE_VERSION)
    ThrowError("Plugin was designed for a later version of Avisynth (%d)", version);
}

int ScriptEnvironment::GetCPUFlags() { return ::GetCPUFlags(); }

void ScriptEnvironment::AddFunction(const char* name, const char* params, ApplyFunc apply, void* user_data) {
  plugin_manager->AddFunction(name, params, apply, user_data);
}

// Throws if unsuccessfull
AVSValue ScriptEnvironment::GetVar(const char* name) {
  if (closing) return AVSValue();  // We easily risk  being inside the critical section below, while deleting variables.
  
  {
    CriticalGuard lock(cs_var_table);
    AVSValue val;
    if (var_table->Get(name, &val))
      return val;
    else
      throw IScriptEnvironment::NotFound();
  }
}

bool ScriptEnvironment::GetVar(const char* name, AVSValue *ret) {
  if (closing) return false;  // We easily risk  being inside the critical section below, while deleting variables.
  
  {
    CriticalGuard lock(cs_var_table);
    return var_table->Get(name, ret);
  }
}

bool ScriptEnvironment::GetVar(const char* name, bool def) {
  if (closing) return def;  // We easily risk  being inside the critical section below, while deleting variables.
  
  {
    CriticalGuard lock(cs_var_table);
    AVSValue val;
    if (var_table->Get(name, &val))
      return val.AsBool(def);
    else
      return def;
  }
}

int ScriptEnvironment::GetVar(const char* name, int def) {
  if (closing) return def;  // We easily risk  being inside the critical section below, while deleting variables.
  
  {
    CriticalGuard lock(cs_var_table);
    AVSValue val;
    if (var_table->Get(name, &val))
      return val.AsInt(def);
    else
      return def;
  }
}

double ScriptEnvironment::GetVar(const char* name, double def) {
  if (closing) return def;  // We easily risk  being inside the critical section below, while deleting variables.
  
  {
    CriticalGuard lock(cs_var_table);
    AVSValue val;
    if (var_table->Get(name, &val))
      return val.AsDblDef(def);
    else
      return def;
  }
}

const char* ScriptEnvironment::GetVar(const char* name, const char* def) {
  if (closing) return def;  // We easily risk  being inside the critical section below, while deleting variables.
  
  {
    CriticalGuard lock(cs_var_table);
    AVSValue val;
    if (var_table->Get(name, &val))
      return val.AsString(def);
    else
      return def;
  }
}

bool ScriptEnvironment::SetVar(const char* name, const AVSValue& val) {
  if (closing) return true;  // We easily risk  being inside the critical section below, while deleting variables.

  CriticalGuard lock(cs_var_table);
  return var_table->Set(name, val);
}

bool ScriptEnvironment::SetGlobalVar(const char* name, const AVSValue& val) {
  if (closing) return true;  // We easily risk  being inside the critical section below, while deleting variables.

  CriticalGuard lock(cs_var_table);
  return global_var_table->Set(name, val);
}

PVideoFrame ScriptEnvironment::NewPlanarVideoFrame(int row_size, int height, int row_sizeUV, int heightUV, int align, bool U_first)
{
#ifdef _DEBUG
  if (align < 0){
    _RPT0(_CRT_WARN, "Warning: A negative value for the 'align' parameter is deprecated and will be treated as positive.");
  }
#endif

  align = max(align, FRAME_ALIGN);

  int pitchUV;
  const int pitchY = AlignNumber(row_size, align);
  if (!PlanarChromaAlignmentState && (row_size == row_sizeUV*2) && (height == heightUV*2)) { // Meet old 2.5 series API expectations for YV12
    // Legacy alignment - pack Y as specified, pack UV half that
    pitchUV = (pitchY+1)>>1;  // UV plane, width = 1/2 byte per pixel - don't align UV planes seperately.
  }
  else {
    // Align planes seperately
    pitchUV = AlignNumber(row_sizeUV, align);
  }

  const int size = pitchY * height + 2 * pitchUV * heightUV;
  VideoFrameBuffer* vfb = GetFrameBuffer(size + align-1);
  if (!vfb)
    ThrowError("NewPlanarVideoFrame: Returned 0 image pointer!");
#ifdef _DEBUG
  {
    static const BYTE filler[] = { 0x0A, 0x11, 0x0C, 0xA7, 0xED };
    BYTE* p = vfb->GetWritePtr();
    BYTE* q = p + vfb->GetDataSize()/5*5;
    for (; p<q; p+=5) {
      p[0]=filler[0]; p[1]=filler[1]; p[2]=filler[2]; p[3]=filler[3]; p[4]=filler[4];
    }
  }
#endif

  int  offsetU, offsetV;
  const int offsetY = AlignPointer(vfb->GetWritePtr(), align) - vfb->GetWritePtr(); // first line offset for proper alignment
  if (U_first) {
    offsetU = offsetY + pitchY * height;
    offsetV = offsetY + pitchY * height + pitchUV * heightUV;
  } else {
    offsetV = offsetY + pitchY * height;
    offsetU = offsetY + pitchY * height + pitchUV * heightUV;
  }
  return new VideoFrame(vfb, offsetY, pitchY, row_size, height, offsetU, offsetV, pitchUV, row_sizeUV, heightUV);
}


PVideoFrame ScriptEnvironment::NewVideoFrame(int row_size, int height, int align)
{
#ifdef _DEBUG
  if (align < 0){
    _RPT0(_CRT_WARN, "Warning: A negative value for the 'align' parameter is deprecated and will be treated as positive.");
  }
#endif

  align = max(align, FRAME_ALIGN);

  const int pitch = AlignNumber(row_size, align);
  const int size = pitch * height;
  VideoFrameBuffer* vfb = GetFrameBuffer(size + align-1);
  if (!vfb)
    ThrowError("NewVideoFrame: Returned 0 frame buffer pointer!");
#ifdef _DEBUG
  {
    static const BYTE filler[] = { 0x0A, 0x11, 0x0C, 0xA7, 0xED };
    BYTE* p = vfb->GetWritePtr();
    BYTE* q = p + vfb->GetDataSize()/5*5;
    for (; p<q; p+=5) {
      p[0]=filler[0]; p[1]=filler[1]; p[2]=filler[2]; p[3]=filler[3]; p[4]=filler[4];
    }
  }
#endif
  const int offset = AlignPointer(vfb->GetWritePtr(), align) - vfb->GetWritePtr(); // first line offset for proper alignment
  return new VideoFrame(vfb, offset, pitch, row_size, height);
}


PVideoFrame __stdcall ScriptEnvironment::NewVideoFrame(const VideoInfo& vi, int align) {
  // Check requested pixel_type:
  switch (vi.pixel_type) {
    case VideoInfo::CS_BGR24:
    case VideoInfo::CS_BGR32:
    case VideoInfo::CS_YUY2:
    case VideoInfo::CS_Y8:
    case VideoInfo::CS_YV12:
    case VideoInfo::CS_YV16:
    case VideoInfo::CS_YV24:
    case VideoInfo::CS_YV411:
    case VideoInfo::CS_I420:
      break;
    default:
      ThrowError("Filter Error: Filter attempted to create VideoFrame with invalid pixel_type.");
  }

  PVideoFrame retval;

  if (vi.IsPlanar() && !vi.IsY8()) { // Planar requires different math ;)
    const int xmod  = 1 << vi.GetPlaneWidthSubsampling(PLANAR_U);
    const int xmask = xmod - 1;
    if (vi.width & xmask)
      ThrowError("Filter Error: Attempted to request a planar frame that wasn't mod%d in width!", xmod);

    const int ymod  = 1 << vi.GetPlaneHeightSubsampling(PLANAR_U);
    const int ymask = ymod - 1;
    if (vi.height & ymask)
      ThrowError("Filter Error: Attempted to request a planar frame that wasn't mod%d in height!", ymod);

    const int heightUV = vi.height >> vi.GetPlaneHeightSubsampling(PLANAR_U);
    retval=NewPlanarVideoFrame(vi.RowSize(PLANAR_Y), vi.height, vi.RowSize(PLANAR_U), heightUV, align, !vi.IsVPlaneFirst());
  }
  else {
    if ((vi.width&1)&&(vi.IsYUY2()))
      ThrowError("Filter Error: Attempted to request an YUY2 frame that wasn't mod2 in width.");

    retval=NewVideoFrame(vi.RowSize(), vi.height, align);
  }
  // After the VideoFrame has been assigned to a PVideoFrame it is safe to decrement the refcount (from 2 to 1).
  InterlockedDecrement(&retval->vfb->refcount);
  InterlockedDecrement(&retval->refcount);
  return retval;
}

bool ScriptEnvironment::MakeWritable(PVideoFrame* pvf) {
  const PVideoFrame& vf = *pvf;

  {
    //we don't want cacheMT::LockVFB to mess up the refcount
    CriticalGuard lock(cs_relink_video_frame_buffer);

    // If the frame is already writable, do nothing.
    if (vf->IsWritable())
      return false;
  }

  // Otherwise, allocate a new frame (using NewVideoFrame) and
  // copy the data into it.  Then modify the passed PVideoFrame
  // to point to the new buffer.
  const int row_size = vf->GetRowSize();
  const int height   = vf->GetHeight();
  PVideoFrame dst;

  if (vf->GetPitch(PLANAR_U)) {  // we have no videoinfo, so we assume that it is Planar if it has a U plane.
    const int row_sizeUV = vf->GetRowSize(PLANAR_U);
    const int heightUV   = vf->GetHeight(PLANAR_U);

    dst = NewPlanarVideoFrame(row_size, height, row_sizeUV, heightUV, FRAME_ALIGN, false);  // Always V first on internal images
  } else {
    dst = NewVideoFrame(row_size, height, FRAME_ALIGN);
  }

  //After the VideoFrame has been assigned to a PVideoFrame it is safe to decrement the refcount (from 2 to 1).
  InterlockedDecrement(&dst->vfb->refcount);
  InterlockedDecrement(&dst->refcount);

  BitBlt(dst->GetWritePtr(), dst->GetPitch(), vf->GetReadPtr(), vf->GetPitch(), row_size, height);
  // Blit More planes (pitch, rowsize and height should be 0, if none is present)
  BitBlt(dst->GetWritePtr(PLANAR_V), dst->GetPitch(PLANAR_V), vf->GetReadPtr(PLANAR_V),
         vf->GetPitch(PLANAR_V), vf->GetRowSize(PLANAR_V), vf->GetHeight(PLANAR_V));
  BitBlt(dst->GetWritePtr(PLANAR_U), dst->GetPitch(PLANAR_U), vf->GetReadPtr(PLANAR_U),
         vf->GetPitch(PLANAR_U), vf->GetRowSize(PLANAR_U), vf->GetHeight(PLANAR_U));

  *pvf = dst;
  return true;
}


void ScriptEnvironment::AtExit(IScriptEnvironment::ShutdownFunc function, void* user_data) {
  at_exit.Add(function, user_data);
}

void ScriptEnvironment::PushContext(int level) {
  CriticalGuard lock(cs_var_table);
  var_table = new VarTable(var_table, global_var_table);
}

void ScriptEnvironment::PopContext() {
  CriticalGuard lock(cs_var_table);
  var_table = var_table->Pop();
}

void ScriptEnvironment::PopContextGlobal() {
  CriticalGuard lock(cs_var_table);
  global_var_table = global_var_table->Pop();
}


PVideoFrame __stdcall ScriptEnvironment::Subframe(PVideoFrame src, int rel_offset, int new_pitch, int new_row_size, int new_height) {
  PVideoFrame retval = src->Subframe(rel_offset, new_pitch, new_row_size, new_height);
  InterlockedDecrement(&retval->refcount);
  return retval;
}

//tsp June 2005 new function compliments the above function
PVideoFrame __stdcall ScriptEnvironment::SubframePlanar(PVideoFrame src, int rel_offset, int new_pitch, int new_row_size,
                                                        int new_height, int rel_offsetU, int rel_offsetV, int new_pitchUV) {
  PVideoFrame retval = src->Subframe(rel_offset, new_pitch, new_row_size, new_height, rel_offsetU, rel_offsetV, new_pitchUV);
  InterlockedDecrement(&retval->refcount);
  return retval;
}

void* ScriptEnvironment::ManageCache(int key, void* data) {
// An extensible interface for providing system or user access to the
// ScriptEnvironment class without extending the IScriptEnvironment
// definition.

  switch (key)
  {
  // Allow the cache to designate a VideoFrameBuffer as expired thus
  // allowing it to be reused in favour of any of it's older siblings.
  case MC_ReturnVideoFrameBuffer:
  {
    if (!data) break;

    LinkedVideoFrameBuffer *lvfb = (LinkedVideoFrameBuffer*)data;

    // The Cache volunteers VFB's it no longer tracks for reuse. This closes the loop
    // for Memory Management. MC_PromoteVideoFrameBuffer moves VideoFrameBuffer's to
    // the head of the list and here we move unloved VideoFrameBuffer's to the end.

    // Check to make sure the vfb wasn't discarded and is really a LinkedVideoFrameBuffer.
    if ((lvfb->data == 0) || (lvfb->signature != LinkedVideoFrameBuffer::ident)) break;

    CriticalGuard lock(cs_relink_video_frame_buffer); //Don't want to mess up with GetFrameBuffer(2)

    // Adjust unpromoted sublist if required
    if (unpromotedvfbs == lvfb) unpromotedvfbs = lvfb->next;

    // Move unloved VideoFrameBuffer's to the end of the video_frame_buffers LRU list.
    Relink(video_frame_buffers.prev, lvfb, &video_frame_buffers);

    // Flag it as returned, i.e. for immediate reuse.
    lvfb->returned = true;

    return (void*)1;
  }
  // Allow the cache to designate a VideoFrameBuffer as being managed thus
  // preventing it being reused as soon as it becomes free.
  case MC_ManageVideoFrameBuffer:
  {
    if (!data) break;

    LinkedVideoFrameBuffer *lvfb = (LinkedVideoFrameBuffer*)data;

    // Check to make sure the vfb wasn't discarded and is really a LinkedVideoFrameBuffer.
    if ((lvfb->data == 0) || (lvfb->signature != LinkedVideoFrameBuffer::ident)) break;

    // Flag it as not returned, i.e. currently managed
    lvfb->returned = false;

    return (void*)1;
  }
  // Allow the cache to designate a VideoFrameBuffer as cacheable thus
  // requesting it be moved to the head of the video_frame_buffers LRU list.
  case MC_PromoteVideoFrameBuffer:
  {
    if (!data) break;

    LinkedVideoFrameBuffer *lvfb = (LinkedVideoFrameBuffer*)data;

    // When a cache instance detects attempts to refetch previously generated frames
    // it starts to promote VFB's to the head of the video_frame_buffers LRU list.
    // Previously all VFB's cycled to the head now only cacheable ones do.

    // Check to make sure the vfb wasn't discarded and is really a LinkedVideoFrameBuffer.
    if ((lvfb->data == 0) || (lvfb->signature != LinkedVideoFrameBuffer::ident)) break;

    CriticalGuard lock(cs_relink_video_frame_buffer); //Don't want to mess up with GetFrameBuffer(2)

    // Adjust unpromoted sublist if required
    if (unpromotedvfbs == lvfb) unpromotedvfbs = lvfb->next;

    // Move loved VideoFrameBuffer's to the head of the video_frame_buffers LRU list.
    Relink(&video_frame_buffers, lvfb, video_frame_buffers.next);

    // Flag it as not returned, i.e. currently managed
    lvfb->returned = false;

    return (void*)1;
  }
  // Register Cache instances onto a linked list, so all Cache instances
  // can be poked as a single unit thru the PokeCache interface
  case MC_RegisterCache:
  {
    if (!data) break;

    Cache *cache = (Cache*)data;

    CriticalGuard lock(cs_relink_video_frame_buffer); // Borrow this lock in case of post compile graph mutation

    if (CacheHead) CacheHead->priorCache = &(cache->nextCache);
    cache->priorCache = &CacheHead;

    cache->nextCache = CacheHead;
    CacheHead = cache;

    return (void*)1;
  }
  // Provide the Caches with a safe method to reclaim a
  // VideoFrameBuffer without conflict with GetFrameBuffer(2)
  case MC_IncVFBRefcount:
  {
    if (!data) break;

    VideoFrameBuffer *vfb = (VideoFrameBuffer*)data;

    CriticalGuard lock(cs_relink_video_frame_buffer); //Don't want to mess up with GetFrameBuffer(2)

    // Bump the refcount while under lock
    InterlockedIncrement(&vfb->refcount);

    return (void*)1;
  }

  default:
    break;
  }
  return 0;
}


bool ScriptEnvironment::PlanarChromaAlignment(IScriptEnvironment::PlanarChromaAlignmentMode key){
  bool oldPlanarChromaAlignmentState = PlanarChromaAlignmentState;

  switch (key)
  {
  case IScriptEnvironment::PlanarChromaAlignmentOff:
  {
    PlanarChromaAlignmentState = false;
    break;
  }
  case IScriptEnvironment::PlanarChromaAlignmentOn:
  {
    PlanarChromaAlignmentState = true;
    break;
  }
  default:
    break;
  }
  return oldPlanarChromaAlignmentState;
}


LinkedVideoFrameBuffer* ScriptEnvironment::NewFrameBuffer(int size) {
  memory_used += size;
  _RPT1(0, "Frame buffer memory used: %I64d\n", memory_used);
  return new LinkedVideoFrameBuffer(size);
}


LinkedVideoFrameBuffer* ScriptEnvironment::GetFrameBuffer2(int size) {
  LinkedVideoFrameBuffer *i, *j;

  // Before we allocate a new framebuffer, check our memory usage, and if we
  // are 12.5% or more above allowed usage discard some unreferenced frames.
  if (memory_used >=  memory_max + max((unsigned __int64)size, (memory_max >> 3)) ) {
    ++g_Mem_stats.CleanUps;
    int freed = 0;
    int freed_count = 0;
    // Deallocate enough unused frames.
    for (i = video_frame_buffers.prev; i != &video_frame_buffers; i = i->prev) {
      if (InterlockedCompareExchange(&i->refcount, 1, 0) == 0) {
        if (i->next != i->prev) {
          // Adjust unpromoted sublist if required
          if (unpromotedvfbs == i) unpromotedvfbs = i->next;
          // Store size.
          freed += i->data_size;
          freed_count++;
          // Delete data;
          i->~LinkedVideoFrameBuffer();  // Can't delete me because caches have pointers to me
          // Link onto tail of lost_video_frame_buffers chain.
          j = i;
          i = i -> next; // step back one
          Relink(lost_video_frame_buffers.prev, j, &lost_video_frame_buffers);
          if ((memory_used+size - freed) < memory_max)
            break; // Stop, we are below 100% utilization
        }
        else break;
      }
    }
    _RPT2(0,"Freed %d frames, consisting of %d bytes.\n",freed_count, freed);
    memory_used -= freed;
    g_Mem_stats.Losses += freed_count;
  }

  // Plan A: When we are below our memory usage :-
  if (memory_used + size < memory_max) {
    //   Part 1: look for a returned free buffer of the same size and reuse it
    for (i = video_frame_buffers.prev; i != &video_frame_buffers; i = i->prev) {
      if (i->returned && (i->GetDataSize() == size)) {
        if (InterlockedCompareExchange(&i->refcount, 1, 0) == 0) {
          ++g_Mem_stats.PlanA1;
          return i;
        }
      }
    }
    //   Part 2: else just allocate a new buffer
    ++g_Mem_stats.PlanA2;
    return NewFrameBuffer(size);
  }

  // To avoid Plan D we prod the caches to surrender any VFB's
  // they maybe guarding. We start gently and ask for just the
  // most recently locked VFB from previous cycles, then we ask
  // for the most recently locked VFB, then we ask for all the
  // locked VFB's. Next we start on the CACHE_RANGE protected
  // VFB's, as an offset we promote these.
  // Plan C is not invoked until the Cache's have been poked once.
  j = 0;
  for (int c=Cache::PC_Nil; c <= Cache::PC_UnProtectAll; c++) {
    if (CacheHead) CacheHead->PokeCache(c, size, this);

    // Plan B: Steal the oldest existing free buffer of the same size
    for (i = video_frame_buffers.prev; i != &video_frame_buffers; i = i->prev) {
      if (InterlockedCompareExchange(&i->refcount, 1, 0) == 0) {
        if (i->GetDataSize() == size) {
          ++g_Mem_stats.PlanB;
          if (j) InterlockedDecrement(&j->refcount);  // Release any alternate candidate
          InterlockedIncrement(&i->sequence_number);  // Signal to the cache that the vfb has been stolen
          return i;
        }
        if ( // Remember the smallest VFB that is bigger than our size
             (c > Cache::PC_Nil || !CacheHead) &&              // Pass 2 OR no PokeCache
             i->GetDataSize() > size           &&              // Bigger
             (j == 0 || i->GetDataSize() < j->GetDataSize())   // Not got one OR better candidate
        ) {
          if (j) InterlockedDecrement(&j->refcount);  // Release any alternate candidate
          j = i;
        }
        else { // not usefull so free again
          InterlockedDecrement(&i->refcount);
        }
      }
    }

    // Plan C: Steal the oldest, smallest free buffer that is greater in size
    if (j) {
      ++g_Mem_stats.PlanC;
      InterlockedIncrement(&j->sequence_number);  // Signal to the cache that the vfb has been stolen
      return j;
    }

    if (!CacheHead) break; // No PokeCache so cache state will not change in next loop iterations
  }

  // Plan D: Allocate a new buffer, regardless of current memory usage
  ++g_Mem_stats.PlanD;
  return NewFrameBuffer(size);
}

VideoFrameBuffer* ScriptEnvironment::GetFrameBuffer(int size) {
  CriticalGuard lock(cs_relink_video_frame_buffer);

  LinkedVideoFrameBuffer* result = GetFrameBuffer2(size);
  if (!result || !result->data) {
    // Damn! we got a NULL from malloc
    _RPT3(0, "GetFrameBuffer failure, size=%d, memory_max=%I64d, memory_used=%I64d", size, memory_max, memory_used);

    // Put that VFB on the lost souls chain
    if (result) Relink(lost_video_frame_buffers.prev, result, &lost_video_frame_buffers);

    const unsigned __int64 save_max = memory_max;

    // Set memory_max to 12.5% below memory_used
    memory_max = max(4*1024*1024ull, memory_used - max((unsigned __int64)size, (memory_used/9)));

    // Retry the request
    result = GetFrameBuffer2(size);

    memory_max = save_max;

    if (!result || !result->data) {
      // Damn!! Damn!! we are really screwed, winge!
      if (result) Relink(lost_video_frame_buffers.prev, result, &lost_video_frame_buffers);

      ThrowError("GetFrameBuffer: Returned a VFB with a 0 data pointer!\n"
                 "size=%d, max=%I64d, used=%I64d\n"
                 "I think we have run out of memory folks!", size, memory_max, memory_used);
    }
  }

#if 0
# if 0
  // Link onto head of video_frame_buffers chain.
  Relink(&video_frame_buffers, result, video_frame_buffers.next);
# else
  // Link onto tail of video_frame_buffers chain.
  Relink(video_frame_buffers.prev, result, &video_frame_buffers);
# endif
#else
  // Link onto head of unpromoted video_frame_buffers chain.
  Relink(unpromotedvfbs->prev, result, unpromotedvfbs);
  // Adjust unpromoted sublist
  unpromotedvfbs = result;
#endif
  // Flag it as returned, i.e. currently not managed
  result->returned = true;

  return result;
}

/* A helper for Invoke.
   Copy a nested array of 'src' into a flat array 'dst'.
   Returns the number of elements that have been written to 'dst'.
   If 'dst' is NULL, will still return the number of elements 
   that would have been written to 'dst', but will not actually write to 'dst'.
*/
static int Flatten(const AVSValue& src, AVSValue* dst, int index, const char* const* arg_names = NULL) {
  if (src.IsArray()) {
    const int array_size = src.ArraySize();
    for (int i=0; i<array_size; ++i) {
      if (!arg_names || arg_names[i] == 0)
        index = Flatten(src[i], dst, index);
    }
  } else {
    if (dst != NULL)
      dst[index] = src;
    ++index;
  }
  return index;
}

const AVSFunction* ScriptEnvironment::Lookup(const char* search_name, const AVSValue* args, int num_args,
                    bool &pstrict, int args_names_count, const char* const* arg_names)
{
  const AVSFunction *result = NULL;

  int oanc;
  do {
    for (int strict = 1; strict >= 0; --strict) {
      pstrict = strict&1;

      // first, look in loaded plugins
      result = plugin_manager->Lookup(search_name, args, num_args, pstrict, args_names_count, arg_names);
      if (result)
        return result;

      // then, look for a built-in function
      for (int i = 0; i < sizeof(builtin_functions)/sizeof(builtin_functions[0]); ++i)
        for (const AVSFunction* j = builtin_functions[i]; j->name; ++j)
          if (streqi(j->name, search_name) &&
              AVSFunction::TypeMatch(j->param_types, args, num_args, pstrict, this) &&
              AVSFunction::ArgNameMatch(j->param_types, args_names_count, arg_names))
            return j;
    }
    // Try again without arg name matching
    oanc = args_names_count;
    args_names_count = 0;
  } while (oanc);

  // If we got here it means the function has not been found.
  // If we haven't done so yet, load the plugins in the autoload folders
  // and try again.
  if (!plugin_manager->HasAutoloadExecuted())
  {
    plugin_manager->AutoloadPlugins();
    return Lookup(search_name, args, num_args, pstrict, args_names_count, arg_names);
  }

  return NULL;
}

AVSValue ScriptEnvironment::Invoke(const char* name, const AVSValue args, const char* const* arg_names)
{
  AVSValue result;
  if (!Invoke(&result, name, args, arg_names))
  {
    throw NotFound();
  }

  return result;
}

bool __stdcall ScriptEnvironment::Invoke(AVSValue *result, const char* name, const AVSValue args, const char* const* arg_names)
{
  bool strict = false;
  const AVSFunction *f;
  AVSValue retval;

  const int args_names_count = (arg_names && args.IsArray()) ? args.ArraySize() : 0;

  // get how many args we will need to store
  int args2_count = Flatten(args, NULL, 0, arg_names);
  if (args2_count > ScriptParser::max_args)
    ThrowError("Too many arguments passed to function (max. is %d)", ScriptParser::max_args);

  // flatten unnamed args
  AVSValue *args2 = new AVSValue[args2_count];
  Flatten(args, args2, 0, arg_names);

  // find matching function
  f = this->Lookup(name, args2, args2_count, strict, args_names_count, arg_names);
  if (!f)
  {
    delete[] args2;
    return false;
  }

  // combine unnamed args into arrays
  int src_index=0, dst_index=0;
  const char* p = f->param_types;
  const int maxarg3 = max((size_t)args2_count, strlen(p)); // well it can't be any longer than this.

  AVSValue *args3 = new AVSValue[maxarg3];

  try {
    while (*p) {
      if (*p == '[') {
        p = strchr(p+1, ']');
        if (!p) break;
        p++;
      } else if (p[1] == '*' || p[1] == '+') {
        int start = src_index;
        while (src_index < args2_count && AVSFunction::SingleTypeMatch(*p, args2[src_index], strict))
          src_index++;
        int size = src_index - start;
        assert((args2_count >= size) && (size >= 0));

        // Even if the AVSValue below is an array of zero size, we can't skip adding it to args3,
        // because filters like BlankClip might still be expecting it.
        args3[dst_index++] = AVSValue(size > 0 ? &args2[start] : NULL, size); // can't delete args2 early because of this

        p += 2;
      } else {
        if (src_index < args2_count)
          args3[dst_index] = args2[src_index];
        src_index++;
        dst_index++;
        p++;
      }
    }
    if (src_index < args2_count)
      ThrowError("Too many arguments to function %s", name);

    const int args3_count = dst_index;

    // copy named args
    for (int i=0; i<args_names_count; ++i) {
      if (arg_names[i]) {
        int named_arg_index = 0;
        for (const char* p = f->param_types; *p; ++p) {
          if (*p == '*' || *p == '+') {
            continue;   // without incrementing named_arg_index
          } else if (*p == '[') {
            p += 1;
            const char* q = strchr(p, ']');
            if (!q) break;
            if (strlen(arg_names[i]) == unsigned(q-p) && !strnicmp(arg_names[i], p, q-p)) {
              // we have a match
              if (args3[named_arg_index].Defined()) {
                ThrowError("Script error: the named argument \"%s\" was passed more than once to %s", arg_names[i], name);
              } else if (args[i].IsArray()) {
                ThrowError("Script error: can't pass an array as a named argument");
              } else if (args[i].Defined() && !AVSFunction::SingleTypeMatch(q[1], args[i], false)) {
                ThrowError("Script error: the named argument \"%s\" to %s had the wrong type", arg_names[i], name);
              } else {
                args3[named_arg_index] = args[i];
                goto success;
              }
            } else {
              p = q+1;
            }
          }
          named_arg_index++;
        }
        // failure
        ThrowError("Script error: %s does not have a named argument \"%s\"", name, arg_names[i]);
success:;
      }
    }
    // ... and we're finally ready to make the call
    retval = f->apply(AVSValue(args3, args3_count), f->user_data, this);
  }
  catch (...) {
    delete[] args3;
    delete[] args2;
    throw;
  }
  delete[] args3;
  delete[] args2;

  *result = retval;
  return true;
}


bool ScriptEnvironment::FunctionExists(const char* name)
{
  if (plugin_manager->FunctionExists(name))
    return true;

  for (int i = 0; i < sizeof(builtin_functions)/sizeof(builtin_functions[0]); ++i)
    for (const AVSFunction* j = builtin_functions[i]; j->name; ++j)
      if (!lstrcmpi(j->name, name))
        return true;

  return false;
}

void ScriptEnvironment::BitBlt(BYTE* dstp, int dst_pitch, const BYTE* srcp, int src_pitch, int row_size, int height) {
  if (height<0)
    ThrowError("Filter Error: Attempting to blit an image with negative height.");
  if (row_size<0)
    ThrowError("Filter Error: Attempting to blit an image with negative row size.");
  ::BitBlt(dstp, dst_pitch, srcp, src_pitch, row_size, height);
}


char* ScriptEnvironment::SaveString(const char* s, int len) {
  // This function is mostly used to save strings for variables
  // so it is fairly acceptable that it shares the same critical
  // section as the vartable
  CriticalGuard lock(cs_var_table);
  return string_dump.SaveString(s, len);
}


char* ScriptEnvironment::VSprintf(const char* fmt, void* val) {
  char *buf = NULL;
  int size = 0, count = -1;
  while (count == -1)
  {
    delete[] buf;
    size += 4096;
    buf = new(std::nothrow) char[size];
    if (!buf) return 0;
    count = _vsnprintf(buf, size, fmt, (va_list)val);
  }
  char *i = ScriptEnvironment::SaveString(buf, count); // SaveString will add the NULL in len mode.
  delete[] buf;
  return i;
}

char* ScriptEnvironment::Sprintf(const char* fmt, ...) {
  va_list val;
  va_start(val, fmt);
  char* result = ScriptEnvironment::VSprintf(fmt, val);
  va_end(val);
  return result;
}


void ScriptEnvironment::ThrowError(const char* fmt, ...) {
  char buf[8192];
  va_list val;
  va_start(val, fmt);
  try {
    _vsnprintf(buf, sizeof(buf)-1, fmt, val);
    if (!this) throw this; // Force inclusion of try catch code!
  } catch (...) {
    strcpy(buf,"Exception while processing ScriptEnvironment::ThrowError().");
  }
  va_end(val);
  buf[sizeof(buf)-1] = '\0';
  throw AvisynthError(ScriptEnvironment::SaveString(buf));
}


extern void ApplyMessage(PVideoFrame* frame, const VideoInfo& vi,
  const char* message, int size, int textcolor, int halocolor, int bgcolor,
  IScriptEnvironment* env);


const AVS_Linkage* const __stdcall ScriptEnvironment::GetAVSLinkage() {
  extern const AVS_Linkage* const AVS_linkage; // In interface.cpp

  return AVS_linkage;
}


void _stdcall ScriptEnvironment::ApplyMessage(PVideoFrame* frame, const VideoInfo& vi, const char* message, int size, int textcolor, int halocolor, int bgcolor) {
  ::ApplyMessage(frame, vi, message, size, textcolor, halocolor, bgcolor, this);
}


void __stdcall ScriptEnvironment::DeleteScriptEnvironment() {
  // Provide a method to delete this ScriptEnvironment in
  // the same malloc context in which it was created below.
  delete this;
}


IScriptEnvironment* __stdcall CreateScriptEnvironment(int version) {
  return CreateScriptEnvironment2(version);
}

IScriptEnvironment2* __stdcall CreateScriptEnvironment2(int version) {
  if (version <= AVISYNTH_INTERFACE_VERSION)
    return new ScriptEnvironment;
  else
    return NULL;
}
