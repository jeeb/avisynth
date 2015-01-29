// Avisynth v2.5.  Copyright 2002 Ben Rudiak-Gould et al.
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


#ifndef __Internal_H__
#define __Internal_H__

#include <avs/config.h>
#include "version.h"

#ifdef X86_32
#define AVS_ARCHSTR "x86"
#else
#define AVS_ARCHSTR "x64"
#endif

#define AVS_VERSION 2.60  // Note: Used by VersionNumber() script function
#define AVS_VERSTR "AviSynth+ 0.1 (r" AVS_SEQREV_STR ", " AVS_ARCHSTR ")"
#define AVS_COPYRIGHT "\n\xA9 2000-2015 Ben Rudiak-Gould, et al.\nhttp://avisynth.nl\n\xA9 2013-2015 AviSynth+ Project\nhttp://avs-plus.net"

extern const char _AVS_VERSTR[], _AVS_COPYRIGHT[];

enum MANAGE_CACHE_KEYS
{
  MC_RegisterCache     = 0xFFFF0004,
  MC_UnRegisterCache   = 0xFFFF0006,
  MC_NodCache          = 0xFFFF0007,
  MC_NodAndExpandCache = 0xFFFF0008,
  MC_RegisterMTGuard,
  MC_UnRegisterMTGuard
};

#include <avisynth.h>
#include <cstring>
#include "parser/script.h" // TODO we only need ScriptFunction from here

struct AVSFunction {
  const char* name;
  const char* param_types;
  AVSValue (__cdecl *apply)(AVSValue args, void* user_data, IScriptEnvironment* env);
  void* user_data;

  bool IsScriptFunction() const
  {
    return apply == &(ScriptFunction::Execute);
  }

  static bool ArgNameMatch(const char* param_types, size_t args_names_count, const char* const* arg_names);
  static bool TypeMatch(const char* param_types, const AVSValue* args, size_t num_args, bool strict, IScriptEnvironment* env);
  static bool SingleTypeMatch(char type, const AVSValue& arg, bool strict);
};


int RGB2YUV(int rgb);

PClip Create_MessageClip(const char* message, int width, int height,
  int pixel_type, bool shrink, int textcolor, int halocolor, int bgcolor,
  IScriptEnvironment* env);

PClip new_Splice(PClip _child1, PClip _child2, bool realign_sound, IScriptEnvironment* env);
PClip new_SeparateFields(PClip _child, IScriptEnvironment* env);
PClip new_AssumeFrameBased(PClip _child);


/* Used to clip/clamp a byte to the 0-255 range.
   Uses a look-up table internally for performance.
*/
class _PixelClip {
  enum { buffer=320 };
  BYTE lut[256+buffer*2];
public:
  _PixelClip() {  
    memset(lut, 0, buffer);
    for (int i=0; i<256; ++i) lut[i+buffer] = (BYTE)i;
    memset(lut+buffer+256, 255, buffer);
  }
  BYTE operator()(int i) const { return lut[i+buffer]; }
};

extern const _PixelClip PixelClip;


template<class ListNode>
static __inline void Relink(ListNode* newprev, ListNode* me, ListNode* newnext) {
  if (me == newprev || me == newnext) return;
  me->next->prev = me->prev;
  me->prev->next = me->next;
  me->prev = newprev;
  me->next = newnext;
  me->prev->next = me->next->prev = me;
}

class CWDChanger 
/**
  * Class to change the current working directory
 **/
{  
public:
  CWDChanger(const char* new_cwd);  
  ~CWDChanger(void);  

private:
  char *old_working_directory;
  bool restore;
};

class DllDirChanger 
{  
public:
  DllDirChanger(const char* new_cwd);  
  ~DllDirChanger(void);  

private:
  char *old_directory;
  bool restore;
};


class NonCachedGenericVideoFilter : public GenericVideoFilter 
/**
  * Class to select a range of frames from a longer clip
 **/
{
public:
  NonCachedGenericVideoFilter(PClip _child);
  int __stdcall SetCacheHints(int cachehints, int frame_range);
};



/*** Inline helper methods ***/


static __inline BYTE ScaledPixelClip(int i) {
  return PixelClip((i+32768) >> 16);
}


static __inline bool IsClose(int a, int b, unsigned threshold) 
  { return (unsigned(a-b+threshold) <= threshold*2); }




#endif  // __Internal_H__
