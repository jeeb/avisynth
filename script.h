// Avisynth v1.0 beta.  Copyright 2000 Ben Rudiak-Gould.
// http://www.math.berkeley.edu/~benrg/avisynth.html

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

#ifndef __Script_H__
#define __Script_H__

#include "internal.h"
#include "expression.h"
#include "scriptparser.h"


/********************************************************************
********************************************************************/




class ScriptFunction 
/**
  * Executes a script
 **/
{
public:
  ScriptFunction(const PExpression& _body, const char** _param_names, int param_count);
  virtual ~ScriptFunction() 
    { delete[] param_names; }

  static AVSValue Execute(AVSValue args, void* user_data, IScriptEnvironment* env);
  static void Delete(void* self, IScriptEnvironment*);

private:
  const PExpression body;
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
  TCHAR old_working_directory[MAX_PATH];
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

AVSValue Assert(AVSValue args, void*, IScriptEnvironment* env);
AVSValue AssertEval(AVSValue args, void*, IScriptEnvironment* env);
AVSValue Eval(AVSValue args, void*, IScriptEnvironment* env);
AVSValue Apply(AVSValue args, void*, IScriptEnvironment* env) ;

AVSValue Import(AVSValue args, void*, IScriptEnvironment* env);

AVSValue SetMemoryMax(AVSValue args, void*, IScriptEnvironment* env);

AVSValue SetWorkingDir(AVSValue args, void*, IScriptEnvironment* env);

/*****   Entry/Factory Methods   ******/

AVSValue Floor(AVSValue args, void*, IScriptEnvironment* env);
AVSValue Ceil(AVSValue args, void*, IScriptEnvironment* env);
AVSValue Round(AVSValue args, void*, IScriptEnvironment* env);

AVSValue Sin(AVSValue args, void* user_data, IScriptEnvironment* env);
AVSValue Cos(AVSValue args, void* user_data, IScriptEnvironment* env);
AVSValue Pi(AVSValue args, void* user_data, IScriptEnvironment* env);
AVSValue Log(AVSValue args, void* user_data, IScriptEnvironment* env);
AVSValue Exp(AVSValue args, void* user_data, IScriptEnvironment* env);
AVSValue Pow(AVSValue args, void* user_data, IScriptEnvironment* env);
AVSValue Sqrt(AVSValue args, void* user_data, IScriptEnvironment* env);


static inline const VideoInfo& VI(const AVSValue& arg);

AVSValue Width(AVSValue args, void*, IScriptEnvironment* env);
AVSValue Height(AVSValue args, void*, IScriptEnvironment* env);
AVSValue FrameCount(AVSValue args, void*, IScriptEnvironment* env);
AVSValue FrameRate(AVSValue args, void*, IScriptEnvironment* env);
AVSValue AudioRate(AVSValue args, void*, IScriptEnvironment* env);
AVSValue AudioLength(AVSValue args, void*, IScriptEnvironment* env);
AVSValue AudioChannels(AVSValue args, void*, IScriptEnvironment* env);
AVSValue AudioBits(AVSValue args, void*, IScriptEnvironment* env);
AVSValue IsRGB(AVSValue args, void*, IScriptEnvironment* env);
AVSValue IsYUY2(AVSValue args, void*, IScriptEnvironment* env);
AVSValue IsFieldBased(AVSValue args, void*, IScriptEnvironment* env);
AVSValue IsFrameBased(AVSValue args, void*, IScriptEnvironment* env);
AVSValue GetParity(AVSValue args, void*, IScriptEnvironment* env);
AVSValue String(AVSValue args, void*, IScriptEnvironment* env);

AVSValue IsBool(AVSValue args, void*, IScriptEnvironment* env);
AVSValue IsInt(AVSValue args, void*, IScriptEnvironment* env);
AVSValue IsFloat(AVSValue args, void*, IScriptEnvironment* env);
AVSValue IsString(AVSValue args, void*, IScriptEnvironment* env);
AVSValue IsClip(AVSValue args, void*, IScriptEnvironment* env);
AVSValue Defined(AVSValue args, void*, IScriptEnvironment* env);

AVSValue Default(AVSValue args, void*, IScriptEnvironment* env);


#endif  // __Script_H__