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

#include "stdafx.h"

#include <stdarg.h>

#include "../internal.h"
#include "./parser/script.h"
#include "memcpy_amd.h"
#include "cache.h"

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
                   Debug_filters[], Image_filters[], Turn_filters[],
                   Conditional_filters[], Conditional_funtions_filters[],
                   CPlugin_filters[], Cache_filters[], SSRC_filters[],
                   Greyscale_filters[], Swap_filters[],
                   SuperEq_filters[], Overlay_filters[], Soundtouch_filters[];


const AVSFunction* const builtin_functions[] = {
                   Audio_filters, Combine_filters, Convert_filters,
                   Convolution_filters, Edit_filters, Field_filters,
                   Focus_filters, Fps_filters, Histogram_filters,
                   Layer_filters, Levels_filters, Misc_filters,
                   Resample_filters, Resize_filters,
                   Script_functions, Source_filters, Text_filters,
                   Transform_filters, Merge_filters, Color_filters,
                   Debug_filters, Image_filters, Turn_filters,
                   Conditional_filters, Conditional_funtions_filters,
                   Plugin_functions, CPlugin_filters, Cache_filters,
                   SSRC_filters, SuperEq_filters, Overlay_filters,
                   Greyscale_filters, Swap_filters,
                   Soundtouch_filters };

// Global statistics counters
struct {
  unsigned long CleanUps;
  unsigned long Losses;
  unsigned long PlanA1;
  unsigned long PlanA2;
  unsigned long PlanB;
  unsigned long PlanC;
  unsigned long PlanD;
  char tag[36];
} g_Mem_stats = {0, 0, 0, 0, 0, 0, 0, "CleanUps, Losses, Plan[A1,A2,B,C,D]"};

const HKEY RegUserKey = HKEY_CURRENT_USER;
const HKEY RegRootKey = HKEY_LOCAL_MACHINE;
const char RegAvisynthKey[] = "Software\\Avisynth";
const char RegPluginDir[] = "PluginDir2_5";


_PixelClip PixelClip;


extern const char* loadplugin_prefix;  // in plugin.cpp

// in plugins.cpp
AVSValue __cdecl LoadPlugin(AVSValue args, void* user_data, IScriptEnvironment* env);
void __cdecl FreeLibraries(void* loaded_plugins, IScriptEnvironment* env);



class LinkedVideoFrame {
private:
  LinkedVideoFrame() : next(0), vf(0, 0, 0, 0, 0) { }; // Compiler winge. This should never be actually used.

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


VideoFrame::VideoFrame(VideoFrameBuffer* _vfb, size_t _offset, int _pitch, int _row_size, int _height)
  : refcount(1), vfb(_vfb), offset(_offset), pitch(_pitch), row_size(_row_size), height(_height),
    offsetU(_offset), offsetV(_offset), pitchUV(0), row_sizeUV(0), heightUV(0)  // PitchUV=0 so this doesn't take up additional space
{
  InterlockedIncrement(&vfb->refcount);
}

VideoFrame::VideoFrame(VideoFrameBuffer* _vfb, size_t _offset, int _pitch, int _row_size, int _height,
                       size_t _offsetU, size_t _offsetV, int _pitchUV, int _row_sizeUV, int _heightUV)
  : refcount(1), vfb(_vfb), offset(_offset), pitch(_pitch), row_size(_row_size), height(_height),
    offsetU(_offsetU), offsetV(_offsetV), pitchUV(_pitchUV), row_sizeUV(_row_sizeUV), heightUV(_heightUV)
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
                        offsetU+rel_offsetU, offsetV+rel_offsetV, new_pitchUV, new_row_sizeUV, new_heightUV);
}


VideoFrameBuffer::VideoFrameBuffer() : refcount(1), data(0), data_size(0), sequence_number(0) {}


#ifdef _DEBUG  // Add 16 guard bytes front and back -- cache can check them after every GetFrame() call
VideoFrameBuffer::VideoFrameBuffer(size_t size) :
  refcount(1),
  data((new BYTE[size+32])+16),
  data_size(data ? size : 0),
  sequence_number(0) {
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
  *(BYTE **)&data = 0; // and mark it invalid!!
  *(size_t *)&data_size = 0;   // and don't forget to set the size to 0 as well!
}

#else

VideoFrameBuffer::VideoFrameBuffer(size_t size)
 : refcount(1), data(new BYTE[size]), data_size(data ? size : 0), sequence_number(0) {}

VideoFrameBuffer::~VideoFrameBuffer() {
//  _ASSERTE(refcount == 0);
  InterlockedIncrement(&sequence_number); // HACK : Notify any children with a pointer, this buffer has changed!!!
  if (data) delete[] data;
  *(BYTE **)&data = 0; // and mark it invalid!!
  *(size_t *)&data_size = 0;   // and don't forget to set the size to 0 as well!
}
#endif


class LinkedVideoFrameBuffer : public VideoFrameBuffer {
public:
  enum {ident = 0x00AA5500};
  LinkedVideoFrameBuffer *prev, *next;
  bool returned;
  const int signature; // Used by ManageCache to ensure that the VideoFrameBuffer
                       // it casts is really a LinkedVideoFrameBuffer
  LinkedVideoFrameBuffer(size_t size) : VideoFrameBuffer(size), returned(true), signature(ident) { next=prev=this; }
  LinkedVideoFrameBuffer() : returned(true), signature(ident) { next=prev=this; }
};


class VarTable {
  VarTable* const dynamic_parent;
  VarTable* const lexical_parent;

  struct Variable {
    Variable* next;
    const char* const name;
    AVSValue val;
    Variable(const char* _name, Variable* _next) : name(_name), next(_next) {}
  };

  Variable variables;   // first entry is "last"

public:
  VarTable(VarTable* _dynamic_parent, VarTable* _lexical_parent)
    : dynamic_parent(_dynamic_parent), lexical_parent(_lexical_parent), variables("last", 0) {}

  ~VarTable() {
    Variable* v = variables.next;
    while (v) {
      Variable* next = v->next;
      delete v;
      v = next;
    }
  }

  VarTable* Pop() {
    VarTable* _dynamic_parent = this->dynamic_parent;
    delete this;
    return _dynamic_parent;
  }

  const AVSValue& Get(const char* name) {
    for (Variable* v = &variables; v; v = v->next)
      if (!lstrcmpi(name, v->name))
        return v->val;
    if (lexical_parent)
      return lexical_parent->Get(name);
    else
      throw IScriptEnvironment::NotFound();
  }

  const AVSValue& GetDef(const char* name, const AVSValue& def) {
    for (Variable* v = &variables; v; v = v->next)
      if (!lstrcmpi(name, v->name))
        return v->val;
    if (lexical_parent)
      return lexical_parent->GetDef(name, def);
    else
      return def;
  }

  bool Set(const char* name, const AVSValue& val) {
    for (Variable* v = &variables; v; v = v->next)
      if (!lstrcmpi(name, v->name)) {
        v->val = val;
        return false;
      }
    variables.next = new Variable(name, variables.next);
    variables.next->val = val;
    return true;
  }
};


class FunctionTable {
  struct LocalFunction : AVSFunction {
    LocalFunction* prev;
  };

  struct Plugin {
    const char* name;
    LocalFunction* plugin_functions;
    Plugin* prev;
  };

  LocalFunction* local_functions;
  Plugin* plugins;
  bool prescanning, reloading;

  IScriptEnvironment* const env;

  char *PluginFunctions;
  size_t PluginFunctionsLen;
  size_t PluginFunctionsSize;

public:

  FunctionTable(IScriptEnvironment* _env) : env(_env), prescanning(false), reloading(false) {
    local_functions = 0;
    plugins = 0;
    PluginFunctions = 0;
    PluginFunctionsLen = 0;
    PluginFunctionsSize = 0;
  }

  ~FunctionTable() {
    while (local_functions) {
      LocalFunction* prev = local_functions->prev;
      delete local_functions;
      local_functions = prev;
    }
    while (plugins) {
      RemovePlugin(plugins);
    }
    PluginFunctionsSize = 0;
    PluginFunctionsLen = 0;
    if (PluginFunctions) {
      env->SetGlobalVar("$PluginFunctions$", AVSValue());
      free(PluginFunctions);
    }
  }

  void StartPrescanning() { prescanning = true; }
  void StopPrescanning() { prescanning = false; }

  void PrescanPluginStart(const char* name)
  {
    if (!prescanning)
      env->ThrowError("FunctionTable: Not in prescanning state");
    _RPT1(0, "Prescanning plugin: %s\n", name);
    Plugin* p = new Plugin;
    p->name = name;
    p->plugin_functions = 0;
    p->prev = plugins;
    plugins = p;
  }

  void RemovePlugin(Plugin* p)
  {
    LocalFunction* cur = p->plugin_functions;
    while (cur) {
      LocalFunction* prev = cur->prev;
      delete cur;
      cur = prev;
    }
    if (p == plugins) {
      plugins = plugins->prev;
    } else {
      Plugin* pp = plugins;
      while (pp->prev != p) pp = pp->prev;
      pp->prev = p->prev;
    }
    delete p;
  }

  static bool IsParameterTypeSpecifier(char c) {
    switch (c) {
      case 'b': case 'i': case 'f': case 's': case 'c': case '.':
        return true;
      default:
        return false;
    }
  }

  static bool IsParameterTypeModifier(char c) {
    switch (c) {
      case '+': case '*':
        return true;
      default:
        return false;
    }
  }

  static bool IsValidParameterString(const char* p) {
    int state = 0;
    char c;
    while ((c = *p++) != '\0' && state != -1) {
      switch (state) {
        case 0:
          if (IsParameterTypeSpecifier(c)) {
            state = 1;
          }
          else if (c == '[') {
            state = 2;
          }
          else {
            state = -1;
          }
          break;

        case 1:
          if (IsParameterTypeSpecifier(c)) {
            // do nothing; stay in the current state
          }
          else if (c == '[') {
            state = 2;
          }
          else if (IsParameterTypeModifier(c)) {
            state = 0;
          }
          else {
            state = -1;
          }
          break;

        case 2:
          if (c == ']') {
            state = 3;
          }
          else {
            // do nothing; stay in the current state
          }
          break;

        case 3:
          if (IsParameterTypeSpecifier(c)) {
            state = 1;
          }
          else {
            state = -1;
          }
          break;
      }
    }

    // states 0, 1 are the only ending states we accept
    return state == 0 || state == 1;
  }

  // Update $Plugin! -- Tritical Jan 2006
  void AddFunction(const char* name, const char* params, IScriptEnvironment::ApplyFunc apply, void* user_data) {
    if (prescanning && !plugins)
      env->ThrowError("FunctionTable in prescanning state but no plugin has been set");

    if (!IsValidParameterString(params))
      env->ThrowError("%s has an invalid parameter string (bug in filter)", name);

    bool duse = false;

// Option for Tritcal - Nonstandard, manifestly changes behaviour
#ifdef OPT_TRITICAL_NOOVERLOAD

// Do not allow LoadPlugin or LoadCPlugin to be overloaded
// to access the new function the alternate name must be used
    if      (lstrcmpi(name, "LoadPlugin")  == 0) duse = true;
    else if (lstrcmpi(name, "LoadCPlugin") == 0) duse = true;
#endif
    LocalFunction *f = NULL;
    if (!duse) {
      f = new LocalFunction;
      f->name = name;
      f->param_types = params;
      if (!prescanning) {
        f->apply = apply;
        f->user_data = user_data;
        f->prev = local_functions;
        local_functions = f;
      } else {
        _RPT1(0, "  Function %s (prescan)\n", name);
        f->prev = plugins->plugin_functions;
        plugins->plugin_functions = f;
      }
    }

    const char* alt_name = 0;
    LocalFunction *f2 = NULL;
    if (loadplugin_prefix) {
      _RPT1(0, "  Plugin name %s\n", loadplugin_prefix);
      f2 = new LocalFunction;
      f2->name = env->Sprintf("%s_%s", loadplugin_prefix, name);
      f2->param_types = params;
      alt_name = f2->name;
      if (prescanning) {
        f2->prev = plugins->plugin_functions;
        plugins->plugin_functions = f2;
      } else {
        f2->apply = apply;
        f2->user_data = user_data;
        f2->prev = local_functions;
        local_functions = f2;
      }
    }
// *******************************************************************
// *** Make Plugin Functions readable for external apps            ***
// *** Tobias Minich, Mar 2003                                     ***
// BEGIN *************************************************************
    if (prescanning) {
      size_t nameLen = 0;
      size_t alt_nameLen = 0;

      if (!duse)
        nameLen = strlen(name);

      if (alt_name)
        alt_nameLen = strlen(alt_name);

      if (PluginFunctionsLen+nameLen+alt_nameLen+3 > PluginFunctionsSize) {
        PluginFunctions = (char*)realloc(PluginFunctions, PluginFunctionsSize += 2048);

        env->SetGlobalVar("$PluginFunctions$", AVSValue(PluginFunctions));
      }

      if (nameLen) {
        if (PluginFunctionsLen) PluginFunctions[PluginFunctionsLen++] = ' ';
        memcpy(PluginFunctions+PluginFunctionsLen, name, nameLen);
        PluginFunctionsLen += nameLen;
      }

      if (alt_nameLen) {
        if (PluginFunctionsLen) PluginFunctions[PluginFunctionsLen++] = ' ';
        memcpy(PluginFunctions+PluginFunctionsLen, alt_name, alt_nameLen);
        PluginFunctionsLen += alt_nameLen;
      }
      PluginFunctions[PluginFunctionsLen] = '\0';

      if (f)
        env->SetGlobalVar(env->Sprintf("$Plugin!%s!Param$", name), AVSValue(f->param_types)); // Fizick

      if (f2 && alt_name)
        env->SetGlobalVar(env->Sprintf("$Plugin!%s!Param$", alt_name), AVSValue(f2->param_types));

    }
// END ***************************************************************

  }

  const AVSFunction* Lookup(const char* search_name, const AVSValue* args, int num_args,
                      bool &pstrict, int args_names_count, const char* const* arg_names) {
    int oanc;
    do {
      for (int strict = 1; strict >= 0; --strict) {
        pstrict = strict&1;
        // first, look in loaded plugins
        for (LocalFunction* p = local_functions; p; p = p->prev)
          if (!lstrcmpi(p->name, search_name) &&
              TypeMatch(p->param_types, args, num_args, strict&1, env) &&
              ArgNameMatch(p->param_types, args_names_count, arg_names))
            return p;
        // now looks in prescanned plugins
        for (Plugin* pp = plugins; pp; pp = pp->prev)
          for (LocalFunction* p = pp->plugin_functions; p; p = p->prev)
            if (!lstrcmpi(p->name, search_name) &&
                TypeMatch(p->param_types, args, num_args, strict&1, env) &&
                ArgNameMatch(p->param_types, args_names_count, arg_names)) {
              _RPT2(0, "Loading plugin %s (lookup for function %s)\n", pp->name, p->name);
              // sets reloading in case the plugin is performing env->FunctionExists() calls
              reloading = true;
              LoadPlugin(AVSValue(AVSValue(AVSValue(pp->name), 1), 1), (void*)false, env);
              reloading = false;
              // just in case the function disappeared from the plugin, avoid infinte recursion
              RemovePlugin(pp);
              // restart the search
              return Lookup(search_name, args, num_args, pstrict, args_names_count, arg_names);
            }
        // finally, look for a built-in function
        for (int i = 0; i < sizeof(builtin_functions)/sizeof(builtin_functions[0]); ++i)
          for (const AVSFunction* j = builtin_functions[i]; j->name; ++j)
            if (!lstrcmpi(j->name, search_name) &&
                TypeMatch(j->param_types, args, num_args, strict&1, env) &&
                ArgNameMatch(j->param_types, args_names_count, arg_names))
              return j;
      }
      // Try again without arg name matching
      oanc = args_names_count;
      args_names_count = 0;
    } while (oanc);
    return 0;
  }

  bool Exists(const char* search_name) {
    for (LocalFunction* p = local_functions; p; p = p->prev)
      if (!lstrcmpi(p->name, search_name))
        return true;
    if (!reloading) {
      for (Plugin* pp = plugins; pp; pp = pp->prev)
        for (LocalFunction* p = pp->plugin_functions; p; p = p->prev)
          if (!lstrcmpi(p->name, search_name))
            return true;
    }
    for (int i = 0; i < sizeof(builtin_functions)/sizeof(builtin_functions[0]); ++i)
      for (const AVSFunction* j = builtin_functions[i]; j->name; ++j)
        if (!lstrcmpi(j->name, search_name))
          return true;
    return false;
  }

  static bool SingleTypeMatch(char type, const AVSValue& arg, bool strict) {
    switch (type) {
      case '.': return true;
      case 'b': return arg.IsBool();
      case 'i': return arg.IsInt();
      case 'f': return arg.IsFloat() && (!strict || !arg.IsInt());
      case 's': return arg.IsString();
      case 'c': return arg.IsClip();
      default:  return false;
    }
  }

  bool TypeMatch(const char* param_types, const AVSValue* args, int num_args, bool strict, IScriptEnvironment* env) {

    bool optional = false;

    int i = 0;
    while (i < num_args) {

      if (*param_types == '\0') {
        // more args than params
        return false;
      }

      if (*param_types == '[') {
        // named arg: skip over the name
        param_types = strchr(param_types+1, ']');
        if (param_types == NULL) {
          env->ThrowError("TypeMatch: unterminated parameter name (bug in filter)");
        }

        ++param_types;
        optional = true;

        if (*param_types == '\0') {
          env->ThrowError("TypeMatch: no type specified for optional parameter (bug in filter)");
        }
      }

      if (param_types[1] == '*') {
        // skip over initial test of type for '*' (since zero matches is ok)
        ++param_types;
      }

      switch (*param_types) {
        case 'b': case 'i': case 'f': case 's': case 'c':
          if (   (!optional || args[i].Defined())
              && !SingleTypeMatch(*param_types, args[i], strict))
            return false;
          // fall through
        case '.':
          ++param_types;
          ++i;
          break;
        case '+': case '*':
          if (!SingleTypeMatch(param_types[-1], args[i], strict)) {
            // we're done with the + or *
            ++param_types;
          }
          else {
            ++i;
          }
          break;
        default:
          env->ThrowError("TypeMatch: invalid character in parameter list (bug in filter)");
      }
    }

    // We're out of args.  We have a match if one of the following is true:
    // (a) we're out of params.
    // (b) remaining params are named i.e. optional.
    // (c) we're at a '+' or '*' and any remaining params are optional.

    if (*param_types == '+'  || *param_types == '*')
      param_types += 1;

    if (*param_types == '\0' || *param_types == '[')
      return true;

    while (param_types[1] == '*') {
      param_types += 2;
      if (*param_types == '\0' || *param_types == '[')
        return true;
    }

    return false;
  }

  bool ArgNameMatch(const char* param_types, int args_names_count, const char* const* arg_names) {

    for (int i=0; i<args_names_count; ++i) {
      if (arg_names[i]) {
        bool found = false;
        int len = strlen(arg_names[i]);
        for (const char* p = param_types; *p; ++p) {
          if (*p == '[') {
            p += 1;
            const char* q = strchr(p, ']');
            if (!q) return false;
            if (len == q-p && !strnicmp(arg_names[i], p, q-p)) {
              found = true;
              break;
            }
            p = q+1;
          }
        }
        if (!found) return false;
      }
    }
    return true;
  }
};


// This doles out storage space for strings.  No space is ever freed
// until the class instance is destroyed (which happens when a script
// file is closed).
class StringDump {
  enum { BLOCK_SIZE = 32768 };
  char* current_block;
  int block_pos, block_size;

public:
  StringDump() : current_block(0), block_pos(BLOCK_SIZE), block_size(BLOCK_SIZE) {}
  ~StringDump();
  char* SaveString(const char* s, int len = -1);
};

StringDump::~StringDump() {
  _RPT0(0, "StringDump: DeAllocating all stringblocks.\r\n");
  char* p = current_block;
  while (p) {
    char* next = *(char**)p;
    delete[] p;
    p = next;
  }
}

char* StringDump::SaveString(const char* s, int len) {
  if (len == -1)
    len = int(strlen(s));

  if (block_pos+len+1 > block_size) {
    block_size = max(BLOCK_SIZE, len+1+int(sizeof(char*)));
	block_size +=  2047;
	block_size &= ~2047;
    char* new_block = new char[block_size];
    _RPT0(0, "StringDump: Allocating new stringblock.\r\n");
    *(char**)new_block = current_block;   // beginning of block holds pointer to previous block
    current_block = new_block;
    block_pos = sizeof(char*);
  }
  char* result = current_block+block_pos;
  memcpy(result, s, len);
  result[len] = 0;
  block_pos += (len+sizeof(char*)) & -int(sizeof(char*)); // Keep 32bit aligned
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


class ScriptEnvironment : public IScriptEnvironment {
public:
  ScriptEnvironment();
  void __stdcall CheckVersion(int version);
  long __stdcall GetCPUFlags();
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
  AVSValue __stdcall GetVarDef(const char* name, const AVSValue& def = AVSValue());

private:
  // Tritical May 2005
  // Note order here!!
  // AtExiter has functions which
  // rely on StringDump elements.
  StringDump string_dump;

  AtExiter at_exit;

  FunctionTable function_table;

  VarTable* global_var_table;
  VarTable* var_table;

  LinkedVideoFrameBuffer video_frame_buffers, lost_video_frame_buffers, *unpromotedvfbs;
  __int64 memory_max, memory_used;

  LinkedVideoFrameBuffer* NewFrameBuffer(size_t size);

  LinkedVideoFrameBuffer* GetFrameBuffer2(size_t size);
  VideoFrameBuffer* GetFrameBuffer(size_t size);
  long CPU_id;

  // helper for Invoke
  int Flatten(const AVSValue& src, AVSValue* dst, int index, int max, const char* const* arg_names=0);

  IScriptEnvironment* This() { return this; }
  const char* GetPluginDirectory();
  bool LoadPluginsMatching(const char* pattern);
  void PrescanPlugins();
  void ExportFilters();
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
extern long CPUCheckForExtensions();  // in cpuaccel.cpp

ScriptEnvironment::ScriptEnvironment()
  : at_exit(),
    function_table(This()),
    CacheHead(0), hrfromcoinit(E_FAIL), coinitThreadId(0),
    unpromotedvfbs(&video_frame_buffers),
    closing(false),
    PlanarChromaAlignmentState(true){ // Change to "true" for 2.5.7

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

    CPU_id = CPUCheckForExtensions();

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

    MEMORYSTATUS memstatus;
    GlobalMemoryStatus(&memstatus);
    // Minimum 16MB
    // else physical memory/4
    // Maximum 0.5GB
    if (memstatus.dwAvailPhys    > 64*1024*1024)
      memory_max = memstatus.dwAvailPhys >> 2;
    else
      memory_max = 16*1024*1024;

    if (memory_max <= 0 || memory_max > 512*1024*1024) // More than 0.5GB
      memory_max = 512*1024*1024;

    memory_used = 0;
    global_var_table = new VarTable(0, 0);
    var_table = new VarTable(0, global_var_table);
    global_var_table->Set("true", true);
    global_var_table->Set("false", false);
    global_var_table->Set("yes", true);
    global_var_table->Set("no", false);

    global_var_table->Set("$ScriptName$", AVSValue());
    global_var_table->Set("$ScriptFile$", AVSValue());
    global_var_table->Set("$ScriptDir$",  AVSValue());

    PrescanPlugins();
    ExportFilters();
  }
  catch (AvisynthError err) {
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

  if(!InterlockedDecrement(&refcount)){
    delete g_Bin;//tsp June 2005 Cleans up the heap
    g_Bin=NULL;
    DeleteCriticalSection(&cs_relink_video_frame_buffer);//tsp July 2005
  }

  DeleteCriticalSection(&cs_var_table);

  // Before we start to pull the world apart
  // give every one their last wish.
  at_exit.Execute(this);

  // If we init'd COM and this is the right thread then release it
  // If it's the wrong threadId then tuff, nothing we can do.
  if(SUCCEEDED(hrfromcoinit) && (coinitThreadId == GetCurrentThreadId())) {
    hrfromcoinit=E_FAIL;
    CoUninitialize();
  }
}

int ScriptEnvironment::SetMemoryMax(int mem) {
  if (mem > 0) {
    MEMORYSTATUS memstatus;
    __int64 mem_limit;

    GlobalMemoryStatus(&memstatus); // Correct call for a 32Bit process. -Ex gives numbers we cannot use!

    memory_max = mem * 1048576i64;                          // mem as megabytes
    if (memory_max < memory_used) memory_max = memory_used; // can't be less than we already have

    if (memstatus.dwAvailVirtual < memstatus.dwAvailPhys) // Check for big memory in Vista64
      mem_limit = memstatus.dwAvailVirtual;
    else
      mem_limit = memstatus.dwAvailPhys;

    mem_limit += memory_used - 5242880i64;
    if (memory_max > mem_limit) memory_max = mem_limit;     // can't be more than 5Mb less than total
    if (memory_max < 4194304i64) memory_max = 4194304i64;   // can't be less than 4Mb -- Tritical Jan 2006
  }
  return (int)(memory_max/1048576i64);
}

int ScriptEnvironment::SetWorkingDir(const char * newdir) {
  return SetCurrentDirectory(newdir) ? 0 : 1;
}

void ScriptEnvironment::CheckVersion(int version) {
  if (version > AVISYNTH_INTERFACE_VERSION)
    ThrowError("Plugin was designed for a later version of Avisynth (%d)", version);
}


long ScriptEnvironment::GetCPUFlags() { return CPU_id; }

void ScriptEnvironment::AddFunction(const char* name, const char* params, ApplyFunc apply, void* user_data) {
  function_table.AddFunction(ScriptEnvironment::SaveString(name), ScriptEnvironment::SaveString(params), apply, user_data);
}

AVSValue ScriptEnvironment::GetVar(const char* name) {
  if (closing) return AVSValue();  // We easily risk  being inside the critical section below, while deleting variables.

  EnterCriticalSection(&cs_var_table);
  AVSValue retval;
  try  {
    retval = var_table->Get(name);
  }
  catch(...)  {
    LeaveCriticalSection(&cs_var_table);
    throw;
  }
  LeaveCriticalSection(&cs_var_table);

  return retval;
}

AVSValue ScriptEnvironment::GetVarDef(const char* name, const AVSValue& def) {
  if (closing) return AVSValue();  // We easily risk  being inside the critical section below, while deleting variables.

  EnterCriticalSection(&cs_var_table);
  AVSValue retval = var_table->GetDef(name, def);
  LeaveCriticalSection(&cs_var_table);

  return retval;
}

bool ScriptEnvironment::SetVar(const char* name, const AVSValue& val) {
  if (closing) return true;  // We easily risk  being inside the critical section below, while deleting variables.

  EnterCriticalSection(&cs_var_table);
  bool retval = var_table->Set(name, val);
  LeaveCriticalSection(&cs_var_table);

  return retval;
}

bool ScriptEnvironment::SetGlobalVar(const char* name, const AVSValue& val) {
  if (closing) return true;  // We easily risk  being inside the critical section below, while deleting variables.

  EnterCriticalSection(&cs_var_table);
  bool retval = global_var_table->Set(name, val);
  LeaveCriticalSection(&cs_var_table);

  return retval;
}

char* GetRegString(HKEY rootKey, const char path[], const char entry[]) {
    HKEY AvisynthKey;

    if (RegOpenKeyEx(rootKey, path, 0, KEY_READ, &AvisynthKey))
      return 0;

    DWORD size;
    if (RegQueryValueEx(AvisynthKey, entry, 0, 0, 0, &size)) {
      RegCloseKey(AvisynthKey); // Dave Brueck - Dec 2005
      return 0;
    }

    char* retStr = new char[size];
    if (!retStr || RegQueryValueEx(AvisynthKey, entry, 0, 0, (LPBYTE)retStr, &size)) {
      delete[] retStr;
      RegCloseKey(AvisynthKey); // Dave Brueck - Dec 2005
      return 0;
    }
    RegCloseKey(AvisynthKey); // Dave Brueck - Dec 2005

    return retStr;
}

const char* ScriptEnvironment::GetPluginDirectory()
{
  const char* plugin_dir = GetVarDef("$PluginDir$").AsString(0);

  if (plugin_dir)
	return plugin_dir;

  // Allow per user override of plugin directory - henktiggelaar, Jan 2011
  // Try HKEY_CURRENT_USER
  plugin_dir = GetRegString(RegUserKey, RegAvisynthKey, RegPluginDir); // Returns new'd char[]!

  if (!plugin_dir) // Try HKEY_LOCAL_MACHINE
	plugin_dir = GetRegString(RegRootKey, RegAvisynthKey, RegPluginDir);

  if (!plugin_dir)
	return 0;

  // remove trailing backslashes
  int l = strlen(plugin_dir);
  while (plugin_dir[l-1] == '\\')
	l--;
  SetGlobalVar("$PluginDir$", AVSValue(SaveString(plugin_dir, l)));  // Tritical May 2005

  delete[] plugin_dir;

  return GetVarDef("$PluginDir$").AsString(0);
}

bool ScriptEnvironment::LoadPluginsMatching(const char* pattern)
{
  WIN32_FIND_DATA FileData;
  char file[MAX_PATH];
  char* dummy;
  int count = 0;
  HANDLE hFind = FindFirstFile(pattern, &FileData);

  if (hFind == INVALID_HANDLE_VALUE)
    return false;

  do {
    // we have to use full pathnames here
    ++count;
    if (count > 20) {
      HMODULE* loaded_plugins = (HMODULE*)GetVar("$Plugins$").AsString();  // throws IScriptEnvironment::NotFound
      FreeLibraries(loaded_plugins, this);
      count = 0;
    }
    GetFullPathName(FileData.cFileName, MAX_PATH, file, &dummy);
    const char *_file = ScriptEnvironment::SaveString(file);
    function_table.PrescanPluginStart(_file);
    LoadPlugin(AVSValue(AVSValue(AVSValue(_file), 1), 1), (void*)true, this);
  }
  while (FindNextFile(hFind, &FileData));

  FindClose(hFind);
  return true;
}

void ScriptEnvironment::PrescanPlugins()
{
  const char* plugin_dir = GetPluginDirectory();
  if (plugin_dir)
  {
    WIN32_FIND_DATA FileData;
    HANDLE hFind = FindFirstFile(plugin_dir, &FileData);

    if (hFind != INVALID_HANDLE_VALUE) {
      FindClose(hFind);
      CWDChanger cwdchange(plugin_dir);

      function_table.StartPrescanning();
//      Doh! Cannot autoload VDub plugins, *.vdf, need to manually assign an Avisynth filter name.
//      if (LoadPluginsMatching("*.dll") | LoadPluginsMatching("*.vdf")) {  // not || because of shortcut boolean eval.
      if (LoadPluginsMatching("*.dll")) {
        // Unloads all plugins
        HMODULE* loaded_plugins = (HMODULE*)GetVar("$Plugins$").AsString();  // throws IScriptEnvironment::NotFound
        FreeLibraries(loaded_plugins, this);
      }
      function_table.StopPrescanning();

      char file[MAX_PATH];
      strcpy(file, plugin_dir);
      strcat(file, "\\*.avsi");

      hFind = FindFirstFile(file, &FileData);
      if (hFind != INVALID_HANDLE_VALUE) {
        do {
          Import(AVSValue(AVSValue(AVSValue(FileData.cFileName), 1), 1), 0, this);
        }
        while (FindNextFile(hFind, &FileData));

        FindClose(hFind);
      }
    }
  }
}

void ScriptEnvironment::ExportFilters()
{
  enum { chunk = 4096 };

  char *builtin_names = (char*)malloc(2*chunk);
  int length = 0;
  int size = 2*chunk;

  for (int i = 0; i < sizeof(builtin_functions)/sizeof(builtin_functions[0]); ++i) {
    for (const AVSFunction* j = builtin_functions[i]; j->name; ++j) {
	  if (length > size-128) {
	    size += chunk;
		builtin_names = (char*)realloc(builtin_names, size); 
	  }

      const char *p = j->name;
      while (*p) builtin_names[length++] = *p++;
      builtin_names[length++] = ' ';

      SetGlobalVar( Sprintf("$Plugin!%s!Param$", j->name), AVSValue(j->param_types) );
    }
  }

  builtin_names[length-1] = 0;
  SetGlobalVar("$InternalFunctions$", AVSValue( SaveString(builtin_names, length) ));

  free(builtin_names);
}


PVideoFrame ScriptEnvironment::NewPlanarVideoFrame(int row_size, int height, int row_sizeUV, int heightUV, int _align, bool U_first) {
  int align, pitchUV;
  size_t Uoffset, Voffset;

  // If align is negative, it will be forced, if not it may be made bigger
  if (_align < 0)
    align = -_align;
  else
    align = max(_align, FRAME_ALIGN);

  const int pitch = (row_size+align-1) / align * align;

  if (_align < 0) {
    // Forced alignment - pack Y as specified, pack UV subsample of that
    pitchUV = MulDiv(pitch, row_sizeUV, row_size);  // Don't align UV planes seperately.
  }
  else if (!PlanarChromaAlignmentState && (row_size == row_sizeUV*2) && (height == heightUV*2)) { // Meet old 2.5 series API expectations for YV12
    // Legacy alignment - pack Y as specified, pack UV half that
    pitchUV = (pitch+1)>>1;  // UV plane, width = 1/2 byte per pixel - don't align UV planes seperately.
  }
  else {
    // Align planes seperately
    pitchUV = (row_sizeUV+align-1) / align * align;
  }

  const size_t size = pitch * height + 2 * pitchUV * heightUV;
  VideoFrameBuffer* vfb = GetFrameBuffer(size + (align < FRAME_ALIGN ? FRAME_ALIGN*4 : align*4));
  if (!vfb)
    ThrowError("NewPlanarVideoFrame: Returned 0 image pointer!");
#ifdef _DEBUG
  {
    static const BYTE filler[] = { 0x0A, 0x11, 0x0C, 0xA7, 0xED };
    BYTE* p = (BYTE*)vfb->GetReadPtr(); // Cheat! Don't update sequence number
    BYTE* q = p + vfb->GetDataSize()/5*5;
    for (; p<q; p+=5) {
      p[0]=filler[0]; p[1]=filler[1]; p[2]=filler[2]; p[3]=filler[3]; p[4]=filler[4];
    }
  }
#endif
  const size_t offset = (-int(vfb->GetWritePtr())) & (FRAME_ALIGN-1);  // align first line offset

  if (U_first) {
    Uoffset = offset + pitch * height;
    Voffset = offset + pitch * height + pitchUV * heightUV;
  } else {
    Voffset = offset + pitch * height;
    Uoffset = offset + pitch * height + pitchUV * heightUV;
  }
  return new VideoFrame(vfb, offset, pitch, row_size, height, Uoffset, Voffset, pitchUV, row_sizeUV, heightUV);
}


PVideoFrame ScriptEnvironment::NewVideoFrame(int row_size, int height, int align) {
  // If align is negative, it will be forced, if not it may be made bigger
  if (align < 0)
    align = -align;
  else
    align = max(align, FRAME_ALIGN);

  const int pitch = (row_size+align-1) / align * align;
  const size_t size = pitch * height;
  const int _align = (align < FRAME_ALIGN) ? FRAME_ALIGN : align;
  VideoFrameBuffer* vfb = GetFrameBuffer(size+(_align*4));
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
  const size_t offset = (-int(vfb->GetWritePtr())) & (FRAME_ALIGN-1);  // align first line offset  (alignment is free here!)
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

  EnterCriticalSection(&cs_relink_video_frame_buffer);//we don't want cacheMT::LockVFB to mess up the refcount
  // If the frame is already writable, do nothing.
  if (vf->IsWritable()) {
    LeaveCriticalSection(&cs_relink_video_frame_buffer);
    return false;
  }

  LeaveCriticalSection(&cs_relink_video_frame_buffer);

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
  EnterCriticalSection(&cs_var_table);
  var_table = new VarTable(var_table, global_var_table);
  LeaveCriticalSection(&cs_var_table);
}

void ScriptEnvironment::PopContext() {
  EnterCriticalSection(&cs_var_table);
  var_table = var_table->Pop();
  LeaveCriticalSection(&cs_var_table);
}

void ScriptEnvironment::PopContextGlobal() {
  EnterCriticalSection(&cs_var_table);
  global_var_table = global_var_table->Pop();
  LeaveCriticalSection(&cs_var_table);
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

    EnterCriticalSection(&cs_relink_video_frame_buffer); //Don't want to mess up with GetFrameBuffer(2)

    // Adjust unpromoted sublist if required
    if (unpromotedvfbs == lvfb) unpromotedvfbs = lvfb->next;

    // Move unloved VideoFrameBuffer's to the end of the video_frame_buffers LRU list.
    Relink(video_frame_buffers.prev, lvfb, &video_frame_buffers);

    // Flag it as returned, i.e. for immediate reuse.
    lvfb->returned = true;

    LeaveCriticalSection(&cs_relink_video_frame_buffer);

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

    EnterCriticalSection(&cs_relink_video_frame_buffer); //Don't want to mess up with GetFrameBuffer(2)

    // Adjust unpromoted sublist if required
    if (unpromotedvfbs == lvfb) unpromotedvfbs = lvfb->next;

    // Move loved VideoFrameBuffer's to the head of the video_frame_buffers LRU list.
    Relink(&video_frame_buffers, lvfb, video_frame_buffers.next);

    // Flag it as not returned, i.e. currently managed
    lvfb->returned = false;

    LeaveCriticalSection(&cs_relink_video_frame_buffer);

    return (void*)1;
  }
  // Register Cache instances onto a linked list, so all Cache instances
  // can be poked as a single unit thru the PokeCache interface
  case MC_RegisterCache:
  {
    if (!data) break;

    Cache *cache = (Cache*)data;

    EnterCriticalSection(&cs_relink_video_frame_buffer); // Borrow this lock in case of post compile graph mutation

    if (CacheHead) CacheHead->priorCache = &(cache->nextCache);
    cache->priorCache = &CacheHead;

    cache->nextCache = CacheHead;
    CacheHead = cache;

    LeaveCriticalSection(&cs_relink_video_frame_buffer);

    return (void*)1;
  }
  // Provide the Caches with a safe method to reclaim a
  // VideoFrameBuffer without conflict with GetFrameBuffer(2)
  case MC_IncVFBRefcount:
  {
    if (!data) break;

    VideoFrameBuffer *vfb = (VideoFrameBuffer*)data;

    EnterCriticalSection(&cs_relink_video_frame_buffer); //Don't want to mess up with GetFrameBuffer(2)

    // Bump the refcount while under lock
    InterlockedIncrement(&vfb->refcount);

    LeaveCriticalSection(&cs_relink_video_frame_buffer);

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


LinkedVideoFrameBuffer* ScriptEnvironment::NewFrameBuffer(size_t size) {
  memory_used += size;
  _RPT1(0, "Frame buffer memory used: %I64d\n", memory_used);
  return new LinkedVideoFrameBuffer(size);
}


LinkedVideoFrameBuffer* ScriptEnvironment::GetFrameBuffer2(size_t size) {
  LinkedVideoFrameBuffer *i, *j;

  // Before we allocate a new framebuffer, check our memory usage, and if we
  // are 12.5% or more above allowed usage discard some unreferenced frames.
  if (memory_used >=  memory_max + max(size, (memory_max >> 3)) ) {
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
    _RPT2(0, "Freed %d frames, consisting of %d bytes.\n", freed_count, freed);
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

VideoFrameBuffer* ScriptEnvironment::GetFrameBuffer(size_t size) {
  EnterCriticalSection(&cs_relink_video_frame_buffer);

  LinkedVideoFrameBuffer* result = GetFrameBuffer2(size);
  if (!result || !result->data) {
    // Damn! we got a NULL from malloc
    _RPT3(0, "GetFrameBuffer failure, size=%d, memory_max=%I64d, memory_used=%I64d", size, memory_max, memory_used);

    // Put that VFB on the lost souls chain
    if (result) Relink(lost_video_frame_buffers.prev, result, &lost_video_frame_buffers);

    const __int64 save_max = memory_max;

    // Set memory_max to 12.5% below memory_used
    memory_max = max(4*1024*1024, memory_used - max(size, (memory_used/9)));

    // Retry the request
    result = GetFrameBuffer2(size);

    memory_max = save_max;

    if (!result || !result->data) {
      // Damn!! Damn!! we are really screwed, winge!
      if (result) Relink(lost_video_frame_buffers.prev, result, &lost_video_frame_buffers);

      LeaveCriticalSection(&cs_relink_video_frame_buffer);

      MEMORYSTATUS memstatus;
      GlobalMemoryStatus(&memstatus); // Correct call for a 32Bit process. -Ex gives numbers we cannot use!

      ThrowError("GetFrameBuffer: Returned a VFB with a 0 data pointer!\n"
                 "size=%d, max=%I64d, used=%I64d, free=%u, phys=%u\n"
                 "I think we have run out of memory folks!",
                 size, memory_max, memory_used, memstatus.dwAvailVirtual, memstatus.dwAvailPhys);
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

  LeaveCriticalSection(&cs_relink_video_frame_buffer);
  return result;
}


int ScriptEnvironment::Flatten(const AVSValue& src, AVSValue* dst, int index, int max, const char* const* arg_names) {
  if (src.IsArray()) {
    const int array_size = src.ArraySize();
    for (int i=0; i<array_size; ++i) {
      if (!arg_names || arg_names[i] == 0)
        index = Flatten(src[i], dst, index, max);
    }
  } else {
    if (index < max) {
      dst[index++] = src;
    } else {
      ThrowError("Too many arguments passed to function (max. is %d)", max);
    }
  }
  return index;
}


void InvokeHelper2(AVSValue &retval, IScriptEnvironment::ApplyFunc f, const AVSValue &args, void* user_data, IScriptEnvironment* env) {

  retval = f(args, user_data, env);
}


int CopyExceptionRecord(PEXCEPTION_POINTERS ep, PEXCEPTION_RECORD er);
void ReportException(const char *title, PEXCEPTION_RECORD er, IScriptEnvironment* env);


void InvokeHelper(AVSValue &retval, IScriptEnvironment::ApplyFunc f, const AVSValue &args, void* user_data, IScriptEnvironment* env) {

  EXCEPTION_RECORD er = {0};

  __try {
    InvokeHelper2(retval, f, args, user_data, env);
  }
  __except (CopyExceptionRecord(GetExceptionInformation(), &er)) {
    ReportException("Invoke", &er, env);
  }
}


AVSValue ScriptEnvironment::Invoke(const char* name, const AVSValue args, const char* const* arg_names) {

  int args2_count;
  bool strict = false;
  const AVSFunction *f;
  AVSValue retval;

  const int args_names_count = (arg_names && args.IsArray()) ? args.ArraySize() : 0;

  AVSValue *args1 = new AVSValue[ScriptParser::max_args]; // Save stack space - put on heap!!!

  try {
    // flatten unnamed args
    args2_count = Flatten(args, args1, 0, ScriptParser::max_args, arg_names);

    // find matching function
    f = function_table.Lookup(name, args1, args2_count, strict, args_names_count, arg_names);
    if (!f)
      throw NotFound();
  }
  catch (...) {
    delete[] args1;
    throw;
  }

  // collapse the 1024 element array
  AVSValue *args2 = new AVSValue[args2_count];
  for (int i=0; i< args2_count; i++)
    args2[i] = args1[i];
  delete[] args1;

  // combine unnamed args into arrays
  int src_index=0, dst_index=0;
  const char* p = f->param_types;
  const int maxarg3 = max(args2_count, int(strlen(p))); // well it can't be any longer than this.

  AVSValue *args3 = new AVSValue[maxarg3];

  try {
    while (*p) {
      if (*p == '[') {
        p = strchr(p+1, ']');
        if (!p) break;
        p++;
      } else if (p[1] == '*' || p[1] == '+') {
        int start = src_index;
        while (src_index < args2_count && FunctionTable::SingleTypeMatch(*p, args2[src_index], strict))
          src_index++;
        args3[dst_index++] = AVSValue(&args2[start], src_index - start); // can't delete args2 early because of this
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
              } else if (args[i].Defined() && !FunctionTable::SingleTypeMatch(q[1], args[i], false)) {
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
    InvokeHelper(retval, f->apply, AVSValue(args3, args3_count), f->user_data, this);
  }
  catch (...) {
    delete[] args3;
    delete[] args2;
    throw;
  }
  delete[] args3;
  delete[] args2;

  return retval;
}


bool ScriptEnvironment::FunctionExists(const char* name) {
  return function_table.Exists(name);
}

  /*****************************
  * Assembler bitblit by Steady
   *****************************/


void asm_BitBlt_ISSE(BYTE* dstp, int dst_pitch, const BYTE* srcp, int src_pitch, int row_size, int height) {

  // Warning! : If you modify this routine, check the generated assembler to make sure
  //            the stupid compiler is saving the ebx register in the entry prologue.
  //            And don't just add an extra push/pop ebx pair around the code, try to
  //            convince the compiler to do the right thing, it's not hard, usually a
  //            slight shuffle or a well placed "__asm mov ebx,ebx" does the trick.

  if(row_size==0 || height==0) return; //abort on goofs
  //move backwards for easier looping and to disable hardware prefetch
  const BYTE* srcStart=srcp+src_pitch*(height-1);
  BYTE* dstStart=dstp+dst_pitch*(height-1);

  if(row_size < 64) {
    _asm {
      mov   esi,srcStart  //move rows from bottom up
      mov   edi,dstStart
      mov   edx,row_size
      dec   edx
      mov   ebx,height
      align 16
memoptS_rowloop:
      mov   ecx,edx
//      rep movsb
memoptS_byteloop:
      mov   AL,[esi+ecx]
      mov   [edi+ecx],AL
      sub   ecx,1
      jnc   memoptS_byteloop
      sub   esi,src_pitch
      sub   edi,dst_pitch
      dec   ebx
      jne   memoptS_rowloop
    };
    return;
  }//end small version

  else if( (int(dstp) | row_size | src_pitch | dst_pitch) & 7) {//not QW aligned
    //unaligned version makes no assumptions on alignment

    _asm {
//****** initialize
      mov   esi,srcStart  //bottom row
      mov   AL,[esi]
      mov   edi,dstStart
      mov   edx,row_size
      mov   ebx,height

//********** loop starts here ***********

      align 16
memoptU_rowloop:
      mov   ecx,edx     //row_size
      dec   ecx         //offset to last byte in row
      add   ecx,esi     //ecx= ptr last byte in row
      and   ecx,~63     //align to first byte in cache line
memoptU_prefetchloop:
      mov   AX,[ecx]    //tried AL,AX,EAX, AX a tiny bit faster
      sub   ecx,64
      cmp   ecx,esi
      jae   memoptU_prefetchloop

//************ write *************

      movq    mm6,[esi]     //move the first unaligned bytes
      movntq  [edi],mm6
//************************
      mov   eax,edi
      neg   eax
      mov   ecx,eax
      and   eax,63      //eax=bytes from [edi] to start of next 64 byte cache line
      and   ecx,7       //ecx=bytes from [edi] to next QW
      align 16
memoptU_prewrite8loop:        //write out odd QW's so 64 bit write is cache line aligned
      cmp   ecx,eax           //start of cache line ?
      jz    memoptU_pre8done  //if not, write single QW
      movq    mm7,[esi+ecx]
      movntq  [edi+ecx],mm7
      add   ecx,8
      jmp   memoptU_prewrite8loop

      align 16
memoptU_write64loop:
      movntq  [edi+ecx-64],mm0
      movntq  [edi+ecx-56],mm1
      movntq  [edi+ecx-48],mm2
      movntq  [edi+ecx-40],mm3
      movntq  [edi+ecx-32],mm4
      movntq  [edi+ecx-24],mm5
      movntq  [edi+ecx-16],mm6
      movntq  [edi+ecx- 8],mm7
memoptU_pre8done:
      add   ecx,64
      cmp   ecx,edx         //while(offset <= row_size) do {...
      ja    memoptU_done64
      movq    mm0,[esi+ecx-64]
      movq    mm1,[esi+ecx-56]
      movq    mm2,[esi+ecx-48]
      movq    mm3,[esi+ecx-40]
      movq    mm4,[esi+ecx-32]
      movq    mm5,[esi+ecx-24]
      movq    mm6,[esi+ecx-16]
      movq    mm7,[esi+ecx- 8]
      jmp   memoptU_write64loop
memoptU_done64:

      sub     ecx,64    //went to far
      align 16
memoptU_write8loop:
      add     ecx,8           //next QW
      cmp     ecx,edx         //any QW's left in row ?
      ja      memoptU_done8
      movq    mm0,[esi+ecx-8]
      movntq  [edi+ecx-8],mm0
      jmp   memoptU_write8loop
memoptU_done8:

      movq    mm1,[esi+edx-8] //write the last unaligned bytes
      movntq  [edi+edx-8],mm1
      sub   esi,src_pitch
      sub   edi,dst_pitch
      dec   ebx               //row counter (=height at start)
      jne   memoptU_rowloop

      sfence
      emms
    };
    return;
  }//end unaligned version

  else {//QW aligned version (fastest)
  //else dstp and row_size QW aligned - hope for the best from srcp
  //QW aligned version should generally be true when copying full rows
    _asm {
      mov   esi,srcStart  //start of bottom row
      mov   edi,dstStart
      mov   ebx,height
      mov   edx,row_size
      align 16
memoptA_rowloop:
      mov   ecx,edx //row_size
      dec   ecx     //offset to last byte in row

//********forward routine
      add   ecx,esi
      and   ecx,~63   //align prefetch to first byte in cache line(~3-4% faster)
      align 16
memoptA_prefetchloop:
      mov   AX,[ecx]
      sub   ecx,64
      cmp   ecx,esi
      jae   memoptA_prefetchloop

      mov   eax,edi
      xor   ecx,ecx
      neg   eax
      and   eax,63            //eax=bytes from edi to start of cache line
      align 16
memoptA_prewrite8loop:        //write out odd QW's so 64bit write is cache line aligned
      cmp   ecx,eax           //start of cache line ?
      jz    memoptA_pre8done  //if not, write single QW
      movq    mm7,[esi+ecx]
      movntq  [edi+ecx],mm7
      add   ecx,8
      jmp   memoptA_prewrite8loop

      align 16
memoptA_write64loop:
      movntq  [edi+ecx-64],mm0
      movntq  [edi+ecx-56],mm1
      movntq  [edi+ecx-48],mm2
      movntq  [edi+ecx-40],mm3
      movntq  [edi+ecx-32],mm4
      movntq  [edi+ecx-24],mm5
      movntq  [edi+ecx-16],mm6
      movntq  [edi+ecx- 8],mm7
memoptA_pre8done:
      add   ecx,64
      cmp   ecx,edx
      ja    memoptA_done64    //less than 64 bytes left
      movq    mm0,[esi+ecx-64]
      movq    mm1,[esi+ecx-56]
      movq    mm2,[esi+ecx-48]
      movq    mm3,[esi+ecx-40]
      movq    mm4,[esi+ecx-32]
      movq    mm5,[esi+ecx-24]
      movq    mm6,[esi+ecx-16]
      movq    mm7,[esi+ecx- 8]
      jmp   memoptA_write64loop

memoptA_done64:
      sub   ecx,64

      align 16
memoptA_write8loop:           //less than 8 QW's left
      add   ecx,8
      cmp   ecx,edx
      ja    memoptA_done8     //no QW's left
      movq    mm7,[esi+ecx-8]
      movntq  [edi+ecx-8],mm7
      jmp   memoptA_write8loop

memoptA_done8:
      sub   esi,src_pitch
      sub   edi,dst_pitch
      dec   ebx               //row counter (height)
      jne   memoptA_rowloop

      sfence
      emms
    };
    return;
  }//end aligned version
}//end BitBlt_memopt()



void ScriptEnvironment::BitBlt(BYTE* dstp, int dst_pitch, const BYTE* srcp, int src_pitch, int row_size, int height) {
  if (height<0)
    ThrowError("Filter Error: Attempting to blit an image with negative height.");
  if (row_size<0)
    ThrowError("Filter Error: Attempting to blit an image with negative row size.");

  if ( (!height) || (!row_size)) return;
  if (GetCPUFlags() & CPUF_INTEGER_SSE) {
    if (height == 1 || (src_pitch == dst_pitch && dst_pitch == row_size)) {
      memcpy_amd(dstp, srcp, row_size*height);
    } else {
      asm_BitBlt_ISSE(dstp, dst_pitch, srcp, src_pitch, row_size, height);
    }
    return;
  }
  if (height == 1 || (dst_pitch == src_pitch && src_pitch == row_size)) {
    memcpy(dstp, srcp, row_size*height);
  } else {
    for (int y=height; y>0; --y) {
      memcpy(dstp, srcp, row_size);
      dstp += dst_pitch;
      srcp += src_pitch;
    }
  }
}


char* ScriptEnvironment::SaveString(const char* s, int len) {
  // This function is mostly used to save strings for variables
  // so it is fairly acceptable that it shares the same critical
  // section as the vartable
  EnterCriticalSection(&cs_var_table);
  char* retval = string_dump.SaveString(s, len);
  LeaveCriticalSection(&cs_var_table);
  return retval;
}


char* ScriptEnvironment::VSprintf(const char* fmt, void* val) {
  char *buf = NULL;
  int size = 0, count = -1;
  while (count == -1)
  {
    if (buf) delete[] buf;
    size += 4096;
    buf = new char[size];
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
    strcpy(buf, "Exception while processing ScriptEnvironment::ThrowError().");
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


void _stdcall ScriptEnvironment::ApplyMessage(PVideoFrame* frame, const VideoInfo& vi,
              const char* message, int size, int textcolor, int halocolor, int bgcolor) {
  ::ApplyMessage(frame, vi, message, size, textcolor, halocolor, bgcolor, this);
}


void __stdcall ScriptEnvironment::DeleteScriptEnvironment() {
  // Provide a method to delete this ScriptEnvironment in
  // the same malloc context in which it was created below.
  delete this;
}


IScriptEnvironment* __stdcall CreateScriptEnvironment(int version) {
  if (loadplugin_prefix) free((void*)loadplugin_prefix);
  loadplugin_prefix = 0;
  if (version <= AVISYNTH_INTERFACE_VERSION)
    return new ScriptEnvironment;
  else
    return 0;
}


const char* const ExceptionCodeToText(const DWORD code) {

  switch (code) {
  case STATUS_GUARD_PAGE_VIOLATION:      // 0x80000001
    return "Guard Page Violation";
  case STATUS_DATATYPE_MISALIGNMENT:     // 0x80000002
    return "Datatype Misalignment";
  case STATUS_BREAKPOINT:                // 0x80000003
    return "Breakpoint";
  case STATUS_SINGLE_STEP:               // 0x80000004
    return "Single Step";

  case STATUS_ACCESS_VIOLATION:          // 0xC0000005
    return "Access Violation";
  case STATUS_IN_PAGE_ERROR:             // 0xC0000006
    return "In Page Error";
  case STATUS_INVALID_HANDLE:            // 0xC0000008
    return "Invalid Handle";
  case STATUS_NO_MEMORY:                 // 0xC0000017
    return "No Memory";
  case STATUS_ILLEGAL_INSTRUCTION:       // 0xC000001D
    return "Illegal Instruction";
  case STATUS_NONCONTINUABLE_EXCEPTION:  // 0xC0000025
    return "Noncontinuable Exception";
  case STATUS_INVALID_DISPOSITION:       // 0xC0000026
    return "Invalid Disposition";
  case STATUS_ARRAY_BOUNDS_EXCEEDED:     // 0xC000008C
    return "Array Bounds Exceeded";
  case STATUS_FLOAT_DENORMAL_OPERAND:    // 0xC000008D
    return "Float Denormal Operand";
  case STATUS_FLOAT_DIVIDE_BY_ZERO:      // 0xC000008E
    return "Float Divide by Zero";
  case STATUS_FLOAT_INEXACT_RESULT:      // 0xC000008F
    return "Float Inexact Result";
  case STATUS_FLOAT_INVALID_OPERATION:   // 0xC0000090
    return "Float Invalid Operation";
  case STATUS_FLOAT_OVERFLOW:            // 0xC0000091
    return "Float Overflow";
  case STATUS_FLOAT_STACK_CHECK:         // 0xC0000092
    return "Float Stack Check";
  case STATUS_FLOAT_UNDERFLOW:           // 0xC0000093
    return "Float Underflow";
  case STATUS_INTEGER_DIVIDE_BY_ZERO:    // 0xC0000094
    return "Integer Divide by Zero";
  case STATUS_INTEGER_OVERFLOW:          // 0xC0000095
    return "Integer Overflow";
  case STATUS_PRIVILEGED_INSTRUCTION:    // 0xC0000096
    return "Privileged Instruction";
  case STATUS_STACK_OVERFLOW:            // 0xC00000FD
    return "Stack Overflow";
  case 0xC0000135:                       // 0xC0000135
    return "DLL Not Found";
  case 0xC0000142:                       // 0xC0000142
    return "DLL Initialization Failed";

  case 0xC06d007E:                       // 0xC06D007E
    return "Delay-load Module Not Found";
  case 0xC06d007F:                       // 0xC06D007E
    return "Delay-load Proceedure Not Found";

  default:
    break;
  }
  
  return 0;
}

/* Use as :-

  EXCEPTION_RECORD er = {0};

  __try {
    ...
  }
  __except (CopyExceptionRecord(GetExceptionInformation(), &er)) {
    ReportException("title", &er, env);
  }

*/
int CopyExceptionRecord(PEXCEPTION_POINTERS ep, PEXCEPTION_RECORD er) {

  if (ep->ExceptionRecord->ExceptionCode == 0xE06D7363) // C++
    return EXCEPTION_CONTINUE_SEARCH;

  const DWORD np = ep->ExceptionRecord->NumberParameters;

  size_t nbytes = sizeof(EXCEPTION_RECORD) -
                  sizeof(ULONG_PTR) * (EXCEPTION_MAXIMUM_PARAMETERS - np);
  
  if (nbytes > sizeof(EXCEPTION_RECORD)) nbytes = sizeof(EXCEPTION_RECORD);

  memcpy(er, ep->ExceptionRecord, nbytes);

  return EXCEPTION_EXECUTE_HANDLER;
}



void ReportException(const char *title, PEXCEPTION_RECORD er, IScriptEnvironment* env) {

  const DWORD ec = er->ExceptionCode;

  if (ec == 0)
    env->ThrowError("%s: unidentified exception.", title);

  char name[MAX_PATH], text[64];
  MEMORY_BASIC_INFORMATION mbi;
  const DWORD np = er->NumberParameters;
  const char* extext = ExceptionCodeToText(ec);

  if (!extext) {
    strcpy(text, "System exception 0x");
    _ultoa(ec, text+strlen(text), 16);
    extext = text;
  }

  // Lookup AllocationBase for ExceptionAddress
  if (VirtualQuery(er->ExceptionAddress, &mbi, sizeof(mbi)) == sizeof(mbi) &&
      // Use AllocationBase to find module name
      GetModuleFileName((HMODULE)mbi.AllocationBase, name, sizeof(name)/sizeof(*name))) {

    // Calculate offset into module of ExceptionAddress
    const size_t offset = (size_t)er->ExceptionAddress - (size_t)mbi.AllocationBase;

    if (np >= 2) // We have fault address information
      env->ThrowError("%s: %s at %s+0x%X(0x%08X)\nattempting to %s 0x%08X",
                      title, extext, name, offset, er->ExceptionAddress,
                      er->ExceptionInformation[0] == 8 ? "execute" :
                      er->ExceptionInformation[0] == 1 ? "write to" : "read from",
                      er->ExceptionInformation[1]);
    // Vanilla report
    env->ThrowError("%s: %s at %s+0x%X(0x%08X)", title, extext, name, offset, er->ExceptionAddress);
  }

  // Cannot find module name
  if (np >= 2) // We have fault address information
    env->ThrowError("%s: %s at 0x%08X\nattempting to %s 0x%08X",
                    title, extext, er->ExceptionAddress,
                    er->ExceptionInformation[0] == 8 ? "execute" :
                    er->ExceptionInformation[0] == 1 ? "write to" : "read from",
                    er->ExceptionInformation[1]);
  // Vanilla report
  env->ThrowError("%s: %s at 0x%08X", title, extext, er->ExceptionAddress);
}

