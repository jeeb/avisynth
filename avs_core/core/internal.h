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
#include <avs/minmax.h>
#include <stdint.h>
#include "version.h"
#include <memory>

#ifdef AVS_LINUX
#include <limits.h>
#endif

#define AVS_CLASSIC_VERSION 2.60  // Note: Used by VersionNumber() script function
#define AVS_COPYRIGHT "\n\xA9 2000-2015 Ben Rudiak-Gould, et al.\nhttp://avisynth.nl\n\xA9 2013-2020 AviSynth+ Project"
#define BUILTIN_FUNC_PREFIX "AviSynth"

enum MANAGE_CACHE_KEYS
{
  MC_RegisterCache     = (int)0xFFFF0004,
  MC_UnRegisterCache   = (int)0xFFFF0006,
  MC_NodCache          = (int)0xFFFF0007,
  MC_NodAndExpandCache = (int)0xFFFF0008,
  MC_RegisterMTGuard,
  MC_UnRegisterMTGuard
};

#include <avisynth.h>
#include "parser/script.h" // TODO we only need ScriptFunction from here
#include <emmintrin.h>

class AVSFunction {

public:

  typedef AVSValue (__cdecl *apply_func_t)(AVSValue args, void* user_data, IScriptEnvironment* env);

  const apply_func_t apply;
  char* name;
  char* canon_name;
  char* param_types;
  void* user_data;
  char* dll_path;

  AVSFunction(void*);
  AVSFunction(const char* _name, const char* _plugin_basename, const char* _param_types, apply_func_t _apply);
  AVSFunction(const char* _name, const char* _plugin_basename, const char* _param_types, apply_func_t _apply, void *_user_data);
  AVSFunction(const char* _name, const char* _plugin_basename, const char* _param_types, apply_func_t _apply, void *_user_data, const char* _dll_path);
  ~AVSFunction();

  AVSFunction() = delete;
  AVSFunction(const AVSFunction&) = delete;
  AVSFunction& operator=(const AVSFunction&) = delete;
  AVSFunction(AVSFunction&&) = delete;
  AVSFunction& operator=(AVSFunction&&) = delete;

  bool empty() const;
  bool IsScriptFunction() const;
#ifdef DEBUG_GSCRIPTCLIP_MT
  bool IsRuntimeScriptFunction() const;
#endif

  static bool ArgNameMatch(const char* param_types, size_t args_names_count, const char* const* arg_names);
  static bool TypeMatch(const char* param_types, const AVSValue* args, size_t num_args, bool strict, IScriptEnvironment* env);
  static bool SingleTypeMatch(char type, const AVSValue& arg, bool strict);
};


int RGB2YUV(int rgb);
const char *GetPixelTypeName(const int pixel_type); // in script.c
const int GetPixelTypeFromName(const char *pixeltypename); // in script.c

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
  CWDChanger(const wchar_t* new_cwd);
  ~CWDChanger(void);

private:
  void Init(const wchar_t* new_cwd);
#ifdef AVS_WINDOWS
  std::unique_ptr<wchar_t[]> old_working_directory;
#else
  char old_working_directory[PATH_MAX];
#endif
  bool restore;
};

class DllDirChanger 
{  
public:
  DllDirChanger(const char* new_cwd);  
  ~DllDirChanger(void);  

private:
  std::unique_ptr<char[]> old_directory;
  bool restore;
};

std::unique_ptr<char[]> WideCharToUtf8(const wchar_t *w_string);
std::unique_ptr<char[]> WideCharToAnsi(const wchar_t *w_string);
std::unique_ptr<char[]> WideCharToAnsiACP(const wchar_t *w_string);
std::unique_ptr<char[]> WideCharToUtf8_maxn(const wchar_t *w_string, size_t maxn);
std::unique_ptr<char[]> WideCharToAnsi_maxn(const wchar_t *w_string, size_t maxn);
std::unique_ptr<wchar_t[]> AnsiToWideChar(const char *s_ansi);
std::unique_ptr<wchar_t[]> AnsiToWideCharACP(const char *s_ansi);
std::unique_ptr<wchar_t[]> Utf8ToWideChar(const char *s_ansi);

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

// 8 bit uv to float
// 16-128-240 -> -112-0-112 -> -112/255..112/255
static __inline float uv8tof(int color) {
#ifdef FLOAT_CHROMA_IS_HALF_CENTERED
  const float shift = 0.5f;
#else
  const float shift = 0.0f;
#endif
  return (color - 128) / 255.0f + shift;
}

// 16-128-240 -> -112-0-112 -> -0.5..0.5
static __inline float uv8tof_limited(int color) {
  const float range = (float)(240 - 16);
#ifdef FLOAT_CHROMA_IS_HALF_CENTERED
  const float shift = 0.5f;
#else
  const float shift = 0.0f;
#endif
  return (color - 128) / range + shift;
}

// 8 bit fullscale to float
static __inline float c8tof(int color) {
  return color / 255.0f;
}

static __inline BYTE ScaledPixelClip(int i) {
  // return PixelClip((i+32768) >> 16);
  // PF: clamp is faster than lut
  return (uint8_t)clamp((i + 32768) >> 16, 0, 255);
}

static __inline uint16_t ScaledPixelClip(long long int i) {
    return (uint16_t)clamp((i + 32768) >> 16, 0LL, 65535LL);
}

static __inline uint16_t ScaledPixelClipEx(long long int i, int max_value) {
  return (uint16_t)clamp((int)((i + 32768) >> 16), 0, max_value);
}

static __inline bool IsClose(int a, int b, unsigned threshold) 
  { return (unsigned(a-b+threshold) <= threshold*2); }

static __inline bool IsCloseFloat(float a, float b, float threshold)
{ return (a-b+threshold <= threshold*2); }

// useful SIMD helpers

// sse2 replacement of _mm_mullo_epi32 in SSE4.1
// use it after speed test, may have too much overhead and C is faster
static AVS_FORCEINLINE __m128i _MM_MULLO_EPI32(const __m128i &a, const __m128i &b)
{
  // for SSE 4.1: return _mm_mullo_epi32(a, b);
  __m128i tmp1 = _mm_mul_epu32(a,b); // mul 2,0
  __m128i tmp2 = _mm_mul_epu32( _mm_srli_si128(a,4), _mm_srli_si128(b,4)); // mul 3,1
  // shuffle results to [63..0] and pack. a2->a1, a0->a0
  return _mm_unpacklo_epi32(_mm_shuffle_epi32(tmp1, _MM_SHUFFLE (0,0,2,0)), _mm_shuffle_epi32(tmp2, _MM_SHUFFLE (0,0,2,0)));
}

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4309)
#endif
// fake _mm_packus_epi32 (orig is SSE4.1 only)
static AVS_FORCEINLINE __m128i _MM_PACKUS_EPI32( __m128i a, __m128i b )
{
  const __m128i val_32 = _mm_set1_epi32(0x8000);
  const __m128i val_16 = _mm_set1_epi16(0x8000);

  a = _mm_sub_epi32(a, val_32);
  b = _mm_sub_epi32(b, val_32);
  a = _mm_packs_epi32(a, b);
  a = _mm_add_epi16(a, val_16);
  return a;
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
// fake _mm_packus_epi32 (orig is SSE4.1 only)
// only for packing 00000000..0000FFFF range integers, does not clamp properly above that, e.g. 00010001
static AVS_FORCEINLINE __m128i _MM_PACKUS_EPI32_SRC_TRUEWORD(__m128i a, __m128i b)
{
  a = _mm_slli_epi32 (a, 16);
  a = _mm_srai_epi32 (a, 16);
  b = _mm_slli_epi32 (b, 16);
  b = _mm_srai_epi32 (b, 16);
  a = _mm_packs_epi32 (a, b);
  return a;
}

static AVS_FORCEINLINE __m128i _MM_CMPLE_EPU16(__m128i x, __m128i y)
{
  // Returns 0xFFFF where x <= y:
  return _mm_cmpeq_epi16(_mm_subs_epu16(x, y), _mm_setzero_si128());
}

static AVS_FORCEINLINE __m128i _MM_BLENDV_SI128(__m128i x, __m128i y, __m128i mask)
{
  // Replace bit in x with bit in y when matching bit in mask is set:
  return _mm_or_si128(_mm_andnot_si128(mask, x), _mm_and_si128(mask, y));
}

// sse2 simulation of SSE4's _mm_min_epu16
static AVS_FORCEINLINE __m128i _MM_MIN_EPU16(__m128i x, __m128i y)
{
  // Returns x where x <= y, else y:
  return _MM_BLENDV_SI128(y, x, _MM_CMPLE_EPU16(x, y));
}

// sse2 simulation of SSE4's _mm_max_epu16
static AVS_FORCEINLINE __m128i _MM_MAX_EPU16(__m128i x, __m128i y)
{
  // Returns x where x >= y, else y:
  return _MM_BLENDV_SI128(x, y, _MM_CMPLE_EPU16(x, y));
}

#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
                ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |   \
                ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))
#endif

#endif  // __Internal_H__
