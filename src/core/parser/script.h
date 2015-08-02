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

#ifndef __Script_H__
#define __Script_H__

#include "../../internal.h"
#include "expression.h"
#include "scriptparser.h"


/********************************************************************
********************************************************************/

// Provision for UTF-8 max 4 bytes per code point
#define AVS_MAX_PATH MAX_PATH*4


class ScriptFunction 
/**
  * Executes a script
 **/
{
public:
  ScriptFunction(const PExpression& _body, const bool* _param_floats, const char** _param_names, int param_count);
  virtual ~ScriptFunction() 
    {
      delete[] param_floats;
      delete[] param_names;
    }

  static AVSValue __cdecl Execute(AVSValue args, void* user_data, IScriptEnvironment* env);
  static void __cdecl Delete(void* self, IScriptEnvironment*);

private:
  const PExpression body;
  bool *param_floats;
  const char** param_names;
};







/****  Helper Classes  ****/

class CWDChanger 
/**
  * Class to change the current working directory
 **/
{  
public:
  CWDChanger(const char* new_cwd);  
  virtual ~CWDChanger(void);  

private:
  TCHAR old_working_directory[AVS_MAX_PATH];
  bool restore;
};


class DynamicCharBuffer 
/**
  * Simple dynamic character buffer
 **/
{
public:
  DynamicCharBuffer(int size) : p(new char[size]) {}
  operator char*() const { return p; }
  ~DynamicCharBuffer() { delete[] p; }

private:
  char* const p;
};




/****    Helper functions   ****/

AVSValue __cdecl Assert(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl AssertEval(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl Eval(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl EvalOop(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl Apply(AVSValue args, void*, IScriptEnvironment* env) ;

AVSValue __cdecl Import(AVSValue args, void*, IScriptEnvironment* env);

AVSValue __cdecl SetMemoryMax(AVSValue args, void*, IScriptEnvironment* env);

AVSValue __cdecl SetWorkingDir(AVSValue args, void*, IScriptEnvironment* env);

/*****   Entry/Factory Methods   ******/

AVSValue __cdecl Muldiv(AVSValue args, void*, IScriptEnvironment* env);

AVSValue __cdecl Floor(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl Ceil(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl Round(AVSValue args, void*, IScriptEnvironment* env);

AVSValue __cdecl Acos(AVSValue args, void* user_data, IScriptEnvironment* env);
AVSValue __cdecl Asin(AVSValue args, void* user_data, IScriptEnvironment* env);
AVSValue __cdecl Atan(AVSValue args, void* user_data, IScriptEnvironment* env);
AVSValue __cdecl Atan2(AVSValue args, void* user_data, IScriptEnvironment* env);
AVSValue __cdecl Cos(AVSValue args, void* user_data, IScriptEnvironment* env);
AVSValue __cdecl Cosh(AVSValue args, void* user_data, IScriptEnvironment* env);
AVSValue __cdecl Exp(AVSValue args, void* user_data, IScriptEnvironment* env);
AVSValue __cdecl Fmod(AVSValue args, void* user_data, IScriptEnvironment* env);
AVSValue __cdecl Log(AVSValue args, void* user_data, IScriptEnvironment* env);
AVSValue __cdecl Log10(AVSValue args, void* user_data, IScriptEnvironment* env);
AVSValue __cdecl Pow(AVSValue args, void* user_data, IScriptEnvironment* env);
AVSValue __cdecl Sin(AVSValue args, void* user_data, IScriptEnvironment* env);
AVSValue __cdecl Sinh(AVSValue args, void* user_data, IScriptEnvironment* env);
AVSValue __cdecl Tan(AVSValue args, void* user_data, IScriptEnvironment* env);
AVSValue __cdecl Tanh(AVSValue args, void* user_data, IScriptEnvironment* env);
AVSValue __cdecl Sqrt(AVSValue args, void* user_data, IScriptEnvironment* env);

AVSValue __cdecl Abs(AVSValue args, void* user_data, IScriptEnvironment* env);
AVSValue __cdecl FAbs(AVSValue args, void* user_data, IScriptEnvironment* env);
AVSValue __cdecl Pi(AVSValue args, void* user_data, IScriptEnvironment* env);
#ifdef OPT_ScriptFunctionTau
AVSValue __cdecl Tau(AVSValue args, void* user_data, IScriptEnvironment* env);
#endif
AVSValue __cdecl Sign(AVSValue args, void*, IScriptEnvironment* env);


static inline const VideoInfo& VI(const AVSValue& arg);

AVSValue __cdecl PixelType (AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl Width(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl Height(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl FrameCount(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl FrameRate(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl FrameRateNumerator(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl FrameRateDenominator(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl AudioRate(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl AudioLength(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl AudioLengthLo(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl AudioLengthHi(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl AudioLengthS(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl AudioLengthF(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl AudioDuration(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl AudioChannels(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl AudioBits(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl IsAudioFloat(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl IsAudioInt(AVSValue args, void*, IScriptEnvironment* env);

AVSValue __cdecl IsRGB(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl IsY8(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl IsYV12(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl IsYV16(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl IsYV24(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl IsYV411(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl IsYUY2(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl IsYUV(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl IsRGB24(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl IsRGB32(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl IsPlanar(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl IsInterleaved(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl IsFieldBased(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl IsFrameBased(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl GetParity(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl String(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl Hex(AVSValue args, void*, IScriptEnvironment* env);

AVSValue __cdecl IsBool(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl IsInt(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl IsFloat(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl IsString(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl IsClip(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl Defined(AVSValue args, void*, IScriptEnvironment* env);

AVSValue __cdecl Default(AVSValue args, void*, IScriptEnvironment* env);

AVSValue __cdecl VersionNumber(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl VersionString(AVSValue args, void*, IScriptEnvironment* env); 

AVSValue __cdecl Int(AVSValue args, void*, IScriptEnvironment* env); 
AVSValue __cdecl Frac(AVSValue args, void*, IScriptEnvironment* env); 
AVSValue __cdecl Float(AVSValue args, void*, IScriptEnvironment* env); 
AVSValue __cdecl Value(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl HexValue(AVSValue args, void*, IScriptEnvironment* env);

AVSValue __cdecl BitAnd(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl BitNot(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl BitOr(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl BitXor(AVSValue args, void*, IScriptEnvironment* env);

AVSValue __cdecl BitLShift(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl BitRShiftL(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl BitRShiftA(AVSValue args, void*, IScriptEnvironment* env);

AVSValue __cdecl BitRotateL(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl BitRotateR(AVSValue args, void*, IScriptEnvironment* env);

AVSValue __cdecl BitChg(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl BitClr(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl BitSet(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl BitTst(AVSValue args, void*, IScriptEnvironment* env);

AVSValue __cdecl UCase(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl LCase(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl StrLen(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl RevStr(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl LeftStr(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl MidStr(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl RightStr(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl FindStr(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl FillStr(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl StrCmp(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl StrCmpi(AVSValue args, void*, IScriptEnvironment* env);

AVSValue __cdecl Rand(AVSValue args, void* user_data, IScriptEnvironment* env);

AVSValue __cdecl Select(AVSValue args, void*, IScriptEnvironment* env);

AVSValue __cdecl NOP(AVSValue args, void*, IScriptEnvironment* env);

AVSValue __cdecl Undefined(AVSValue args, void*, IScriptEnvironment* env);

AVSValue __cdecl Exist(AVSValue args, void*, IScriptEnvironment* env);

// WE ->
AVSValue __cdecl AVSChr(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl AVSOrd(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl AVSTime(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl Spline(AVSValue args, void*, IScriptEnvironment* env);
// WE <-

AVSValue __cdecl HasVideo(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl HasAudio(AVSValue args, void*, IScriptEnvironment* env);

AVSValue __cdecl AvsMin(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl AvsMax(AVSValue args, void*, IScriptEnvironment* env);

AVSValue __cdecl ScriptName(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl ScriptFile(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl ScriptDir (AVSValue args, void*, IScriptEnvironment* env);

#endif  // __Script_H__
