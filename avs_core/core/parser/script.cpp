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


#include "script.h"
#include <time.h>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <io.h>
#include <avs/win.h>
#include <avs/minmax.h>
#include <new>
#include <clocale>
#include "../internal.h"
#include "../Prefetcher.h"


/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/


extern const AVSFunction Script_functions[] = {
  { "muldiv",   BUILTIN_FUNC_PREFIX, "iii", Muldiv },
                
  { "floor",    BUILTIN_FUNC_PREFIX, "f", Floor },
  { "ceil",     BUILTIN_FUNC_PREFIX, "f", Ceil },
  { "round",    BUILTIN_FUNC_PREFIX, "f", Round },

  { "acos",     BUILTIN_FUNC_PREFIX, "f", Acos },
  { "asin",     BUILTIN_FUNC_PREFIX, "f", Asin },
  { "atan",     BUILTIN_FUNC_PREFIX, "f", Atan },
  { "atan2",    BUILTIN_FUNC_PREFIX, "ff", Atan2 },
  { "cos",      BUILTIN_FUNC_PREFIX, "f", Cos },
  { "cosh",     BUILTIN_FUNC_PREFIX, "f", Cosh },
  { "exp",      BUILTIN_FUNC_PREFIX, "f", Exp },
  { "fmod",     BUILTIN_FUNC_PREFIX, "ff", Fmod },
  { "log",      BUILTIN_FUNC_PREFIX, "f", Log },
  { "log10",    BUILTIN_FUNC_PREFIX, "f", Log10 },
  { "pow",      BUILTIN_FUNC_PREFIX, "ff", Pow },
  { "sin",      BUILTIN_FUNC_PREFIX, "f", Sin },
  { "sinh",     BUILTIN_FUNC_PREFIX, "f", Sinh },
  { "tan",      BUILTIN_FUNC_PREFIX, "f", Tan },
  { "tanh",     BUILTIN_FUNC_PREFIX, "f", Tanh },
  { "sqrt",     BUILTIN_FUNC_PREFIX, "f", Sqrt },


  { "abs",      BUILTIN_FUNC_PREFIX, "i", Abs },
  { "abs",      BUILTIN_FUNC_PREFIX, "f", FAbs },
  { "pi",       BUILTIN_FUNC_PREFIX, "", Pi },
#ifdef OPT_ScriptFunctionTau
  { "tau",      BUILTIN_FUNC_PREFIX, "", Tau },
#endif
  { "sign",     BUILTIN_FUNC_PREFIX, "f",Sign},

  { "bitand",   BUILTIN_FUNC_PREFIX, "ii",BitAnd},
  { "bitnot",   BUILTIN_FUNC_PREFIX, "i",BitNot},
  { "bitor",    BUILTIN_FUNC_PREFIX, "ii",BitOr},
  { "bitxor",   BUILTIN_FUNC_PREFIX, "ii",BitXor},

  { "bitlshift",  BUILTIN_FUNC_PREFIX, "ii",BitLShift},
  { "bitlshiftl", BUILTIN_FUNC_PREFIX, "ii",BitLShift},
  { "bitlshifta", BUILTIN_FUNC_PREFIX, "ii",BitLShift},
  { "bitlshiftu", BUILTIN_FUNC_PREFIX, "ii",BitLShift},
  { "bitlshifts", BUILTIN_FUNC_PREFIX, "ii",BitLShift},
  { "bitshl",     BUILTIN_FUNC_PREFIX, "ii",BitLShift},
  { "bitsal",     BUILTIN_FUNC_PREFIX, "ii",BitLShift},

  { "bitrshiftl", BUILTIN_FUNC_PREFIX, "ii",BitRShiftL},
  { "bitrshifta", BUILTIN_FUNC_PREFIX, "ii",BitRShiftA},
  { "bitrshiftu", BUILTIN_FUNC_PREFIX, "ii",BitRShiftL},
  { "bitrshifts", BUILTIN_FUNC_PREFIX, "ii",BitRShiftA},
  { "bitshr",     BUILTIN_FUNC_PREFIX, "ii",BitRShiftL},
  { "bitsar",     BUILTIN_FUNC_PREFIX, "ii",BitRShiftA},

  { "bitlrotate", BUILTIN_FUNC_PREFIX, "ii",BitRotateL},
  { "bitrrotate", BUILTIN_FUNC_PREFIX, "ii",BitRotateR},
  { "bitrol",     BUILTIN_FUNC_PREFIX, "ii",BitRotateL},
  { "bitror",     BUILTIN_FUNC_PREFIX, "ii",BitRotateR},

  { "bitchg",    BUILTIN_FUNC_PREFIX, "ii",BitChg},
  { "bitchange", BUILTIN_FUNC_PREFIX, "ii",BitChg},
  { "bitclr",    BUILTIN_FUNC_PREFIX, "ii",BitClr},
  { "bitclear",  BUILTIN_FUNC_PREFIX, "ii",BitClr},
  { "bitset",    BUILTIN_FUNC_PREFIX, "ii",BitSet},
  { "bittst",    BUILTIN_FUNC_PREFIX, "ii",BitTst},
  { "bittest",   BUILTIN_FUNC_PREFIX, "ii",BitTst},

  { "lcase",    BUILTIN_FUNC_PREFIX, "s",LCase},
  { "ucase",    BUILTIN_FUNC_PREFIX, "s",UCase},
  { "strlen",   BUILTIN_FUNC_PREFIX, "s",StrLen},
  { "revstr",   BUILTIN_FUNC_PREFIX, "s",RevStr},
  { "leftstr",  BUILTIN_FUNC_PREFIX, "si",LeftStr},
  { "midstr",   BUILTIN_FUNC_PREFIX, "si[length]i",MidStr},
  { "rightstr", BUILTIN_FUNC_PREFIX, "si",RightStr},
  { "findstr",  BUILTIN_FUNC_PREFIX, "ss",FindStr},
  { "fillstr",  BUILTIN_FUNC_PREFIX, "i[]s",FillStr},

  { "strcmp",   BUILTIN_FUNC_PREFIX, "ss",StrCmp},
  { "strcmpi",  BUILTIN_FUNC_PREFIX, "ss",StrCmpi},

  { "rand",     BUILTIN_FUNC_PREFIX, "[max]i[scale]b[seed]b", Rand },

  { "Select",   BUILTIN_FUNC_PREFIX, "i.+", Select },

  { "nop",      BUILTIN_FUNC_PREFIX, "", NOP },
  { "undefined",BUILTIN_FUNC_PREFIX, "", Undefined },

  { "width",      BUILTIN_FUNC_PREFIX, "c", Width },
  { "height",     BUILTIN_FUNC_PREFIX, "c", Height },
  { "framecount", BUILTIN_FUNC_PREFIX, "c", FrameCount },
  { "framerate",  BUILTIN_FUNC_PREFIX, "c", FrameRate },
  { "frameratenumerator",   BUILTIN_FUNC_PREFIX, "c", FrameRateNumerator },
  { "frameratedenominator", BUILTIN_FUNC_PREFIX, "c", FrameRateDenominator },
  { "audiorate",     BUILTIN_FUNC_PREFIX, "c", AudioRate },
  { "audiolength",   BUILTIN_FUNC_PREFIX, "c", AudioLength },  // Fixme: Add int64 to script
  { "audiolengthlo", BUILTIN_FUNC_PREFIX, "c[]i", AudioLengthLo }, // audiolength%i
  { "audiolengthhi", BUILTIN_FUNC_PREFIX, "c[]i", AudioLengthHi }, // audiolength/i
  { "audiolengths",  BUILTIN_FUNC_PREFIX, "c", AudioLengthS }, // as a string
  { "audiolengthf",  BUILTIN_FUNC_PREFIX, "c", AudioLengthF }, // at least this will give an order of the size
  { "audioduration", BUILTIN_FUNC_PREFIX, "c", AudioDuration }, // In seconds
  { "audiochannels", BUILTIN_FUNC_PREFIX, "c", AudioChannels },
  { "audiobits",     BUILTIN_FUNC_PREFIX, "c", AudioBits },
  { "IsAudioFloat",  BUILTIN_FUNC_PREFIX, "c", IsAudioFloat },
  { "IsAudioInt",    BUILTIN_FUNC_PREFIX, "c", IsAudioInt },
  { "IsRGB",    BUILTIN_FUNC_PREFIX, "c", IsRGB },
  { "IsYUY2",   BUILTIN_FUNC_PREFIX, "c", IsYUY2 },
  { "IsYUV",    BUILTIN_FUNC_PREFIX, "c", IsYUV },
  { "IsY8",     BUILTIN_FUNC_PREFIX, "c", IsY8 },
  { "IsYV12",   BUILTIN_FUNC_PREFIX, "c", IsYV12 },
  { "IsYV16",   BUILTIN_FUNC_PREFIX, "c", IsYV16 },
  { "IsYV24",   BUILTIN_FUNC_PREFIX, "c", IsYV24 },
  { "IsYV411",  BUILTIN_FUNC_PREFIX, "c", IsYV411 },
  { "IsPlanar", BUILTIN_FUNC_PREFIX, "c", IsPlanar },
  { "IsInterleaved", BUILTIN_FUNC_PREFIX, "c", IsInterleaved },
  { "IsRGB24",       BUILTIN_FUNC_PREFIX, "c", IsRGB24 },
  { "IsRGB32",       BUILTIN_FUNC_PREFIX, "c", IsRGB32 },
  { "IsFieldBased",  BUILTIN_FUNC_PREFIX, "c", IsFieldBased },
  { "IsFrameBased",  BUILTIN_FUNC_PREFIX, "c", IsFrameBased },
  { "GetParity", BUILTIN_FUNC_PREFIX, "c[n]i", GetParity },
  { "String",    BUILTIN_FUNC_PREFIX, ".[]s", String },
  { "Hex",       BUILTIN_FUNC_PREFIX, "i", Hex },

  { "IsBool",   BUILTIN_FUNC_PREFIX, ".", IsBool },
  { "IsInt",    BUILTIN_FUNC_PREFIX, ".", IsInt },
  { "IsFloat",  BUILTIN_FUNC_PREFIX, ".", IsFloat },
  { "IsString", BUILTIN_FUNC_PREFIX, ".", IsString },
  { "IsClip",   BUILTIN_FUNC_PREFIX, ".", IsClip },
  { "Defined",  BUILTIN_FUNC_PREFIX, ".", Defined },

  { "Default",  BUILTIN_FUNC_PREFIX, "..", Default },

  { "Eval",   BUILTIN_FUNC_PREFIX, "s[name]s", Eval },
  { "Eval",   BUILTIN_FUNC_PREFIX, "cs[name]s", EvalOop },
  { "Apply",  BUILTIN_FUNC_PREFIX, "s.*", Apply },
  { "Import", BUILTIN_FUNC_PREFIX, "s+", Import },

  { "Assert", BUILTIN_FUNC_PREFIX, "b[message]s", Assert },
  { "Assert", BUILTIN_FUNC_PREFIX, "s", AssertEval },

  { "SetMemoryMax", BUILTIN_FUNC_PREFIX, "[]i", SetMemoryMax },

  { "SetWorkingDir", BUILTIN_FUNC_PREFIX, "s", SetWorkingDir },
  { "Exist",         BUILTIN_FUNC_PREFIX, "s", Exist },

  { "Chr",    BUILTIN_FUNC_PREFIX, "i", AVSChr },
  { "Ord",    BUILTIN_FUNC_PREFIX, "s", AVSOrd },
  { "Time",   BUILTIN_FUNC_PREFIX, "s", AVSTime },
  { "Spline", BUILTIN_FUNC_PREFIX, "[x]ff+[cubic]b", Spline },

  { "int",   BUILTIN_FUNC_PREFIX, "f", Int },
  { "frac",  BUILTIN_FUNC_PREFIX, "f", Frac},
  { "float", BUILTIN_FUNC_PREFIX, "f",Float},

  { "value",    BUILTIN_FUNC_PREFIX, "s",Value},
  { "hexvalue", BUILTIN_FUNC_PREFIX, "s",HexValue},

  { "VersionNumber", BUILTIN_FUNC_PREFIX, "", VersionNumber },
  { "VersionString", BUILTIN_FUNC_PREFIX, "", VersionString },
  
  { "HasVideo", BUILTIN_FUNC_PREFIX, "c", HasVideo },
  { "HasAudio", BUILTIN_FUNC_PREFIX, "c", HasAudio },

  { "Min", BUILTIN_FUNC_PREFIX, "f+", AvsMin },
  { "Max", BUILTIN_FUNC_PREFIX, "f+", AvsMax },
 
  { "ScriptName", BUILTIN_FUNC_PREFIX, "", ScriptName },
  { "ScriptFile", BUILTIN_FUNC_PREFIX, "", ScriptFile },
  { "ScriptDir",  BUILTIN_FUNC_PREFIX, "", ScriptDir  },
 
  { "PixelType",  BUILTIN_FUNC_PREFIX, "c", PixelType  },

  { "AddAutoloadDir",     BUILTIN_FUNC_PREFIX, "s[toFront]b", AddAutoloadDir  },
  { "ClearAutoloadDirs",  BUILTIN_FUNC_PREFIX, "", ClearAutoloadDirs  },
  { "AutoloadPlugins",    BUILTIN_FUNC_PREFIX, "", AutoloadPlugins  },
  { "FunctionExists",     BUILTIN_FUNC_PREFIX, "s", FunctionExists  },
  { "InternalFunctionExists", BUILTIN_FUNC_PREFIX, "s", InternalFunctionExists  },

  { "SetFilterMTMode",  BUILTIN_FUNC_PREFIX, "si[force]b", SetFilterMTMode  },
  { "Prefetch",         BUILTIN_FUNC_PREFIX, "c[threads]i", Prefetcher::Create  },
 
  { 0 }
};



/**********************************
 *******   Script Function   ******
 *********************************/

ScriptFunction::ScriptFunction( const PExpression& _body, const bool* _param_floats,
                                const char** _param_names, int param_count ) 
  : body(_body) 
{
  param_floats = new bool[param_count];
  memcpy(param_floats, _param_floats, param_count*sizeof(const bool));

  param_names = new const char*[param_count];
  memcpy(param_names, _param_names, param_count*sizeof(const char*));
}
  

AVSValue ScriptFunction::Execute(AVSValue args, void* user_data, IScriptEnvironment* env) 
{
  ScriptFunction* self = (ScriptFunction*)user_data;
  env->PushContext();
  for (int i=0; i<args.ArraySize(); ++i)
    env->SetVar( self->param_names[i], // Force float args that are actually int to be float
	            (self->param_floats[i] && args[i].IsInt()) ? float(args[i].AsInt()) : args[i]);

  AVSValue result;
  try {
    result = self->body->Evaluate(env);
  }
  catch(...) {
    env->PopContext();
    throw;
  }

  env->PopContext();
  return result;
}

void ScriptFunction::Delete(void* self, IScriptEnvironment*) 
{
    delete (ScriptFunction*)self;
}



/***********************************
 *******   Helper Functions   ******
 **********************************/

CWDChanger::CWDChanger(const char* new_cwd) :
  old_working_directory(NULL)
{
  DWORD cwdLen = GetCurrentDirectory(0, NULL);
  old_working_directory = new char[cwdLen];
  DWORD save_cwd_success = GetCurrentDirectory(cwdLen, old_working_directory);
  BOOL set_cwd_success = SetCurrentDirectory(new_cwd);
  restore = (save_cwd_success && set_cwd_success);
}

CWDChanger::~CWDChanger(void)
{
  if (restore)
    SetCurrentDirectory(old_working_directory);

  delete [] old_working_directory;
}

DllDirChanger::DllDirChanger(const char* new_dir) :
  old_directory(NULL)
{
  DWORD len = GetDllDirectory (0, NULL);
  old_directory = new char[len];
  DWORD save_success = GetDllDirectory (len, old_directory);
  BOOL set_success = SetDllDirectory(new_dir);
  restore = (save_success && set_success);
}

DllDirChanger::~DllDirChanger(void)
{
  if (restore)
    SetDllDirectory(old_directory);

  delete [] old_directory;
}

AVSValue Assert(AVSValue args, void*, IScriptEnvironment* env) 
{
  if (!args[0].AsBool())
    env->ThrowError("%s", args[1].Defined() ? args[1].AsString() : "Assert: assertion failed");
  return AVSValue();
}

AVSValue AssertEval(AVSValue args, void*, IScriptEnvironment* env) 
{
  const char* pred = args[0].AsString();
  AVSValue eval_args[] = { args[0].AsString(), "asserted expression" };
  AVSValue val = env->Invoke("Eval", AVSValue(eval_args, 2));
  if (!val.IsBool())
    env->ThrowError("Assert: expression did not evaluate to true or false: \"%s\"", pred);
  if (!val.AsBool())
    env->ThrowError("Assert: assertion failed: \"%s\"", pred);
  return AVSValue();
}

AVSValue Eval(AVSValue args, void*, IScriptEnvironment* env) 
{
  const char *filename = args[1].AsString(0);
  if (filename) filename = env->SaveString(filename);
  ScriptParser parser(env, args[0].AsString(), filename);
  PExpression exp = parser.Parse();
  return exp->Evaluate(env);
}

AVSValue Apply(AVSValue args, void*, IScriptEnvironment* env) 
{
  return env->Invoke(args[0].AsString(), args[1]);
}

AVSValue EvalOop(AVSValue args, void*, IScriptEnvironment* env) 
{
  AVSValue prev_last = env->GetVarDef("last");  // Store previous last
  env->SetVar("last", args[0]);              // Set implicit last

  AVSValue result;
  try {
    result = Eval(AVSValue(&args[1], 2), 0, env);
  }
  catch(...) {
    env->SetVar("last", prev_last);          // Restore implicit last
	throw;
  }
  env->SetVar("last", prev_last);            // Restore implicit last
  return result;
}

AVSValue Import(AVSValue args, void*, IScriptEnvironment* env) 
{
  args = args[0];
  AVSValue result;

  IScriptEnvironment2 *env2 = static_cast<IScriptEnvironment2*>(env);
  const bool MainScript = (env2->IncrImportDepth() == 1);

  AVSValue lastScriptName = env->GetVarDef("$ScriptName$");
  AVSValue lastScriptFile = env->GetVarDef("$ScriptFile$");
  AVSValue lastScriptDir  = env->GetVarDef("$ScriptDir$");
  for (int i=0; i<args.ArraySize(); ++i) {
    const char* script_name = args[i].AsString();

    TCHAR full_path[AVS_MAX_PATH];
    TCHAR* file_part;
    if (strchr(script_name, '\\') || strchr(script_name, '/')) {
      DWORD len = GetFullPathName(script_name, AVS_MAX_PATH, full_path, &file_part);
      if (len == 0 || len > AVS_MAX_PATH)
        env->ThrowError("Import: unable to open \"%s\" (path invalid?), error=0x%x", script_name, GetLastError());
    } else {
      DWORD len = SearchPath(NULL, script_name, NULL, AVS_MAX_PATH, full_path, &file_part);
      if (len == 0 || len > AVS_MAX_PATH)
        env->ThrowError("Import: unable to locate \"%s\" (try specifying a path), error=0x%x", script_name, GetLastError());
    }

    HANDLE h = ::CreateFile(full_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE)
      env->ThrowError("Import: couldn't open \"%s\"", full_path);

    size_t dir_part_len = file_part - full_path;

    env->SetGlobalVar("$ScriptName$", env->SaveString(full_path));
    env->SetGlobalVar("$ScriptFile$", env->SaveString(file_part));
    env->SetGlobalVar("$ScriptDir$", env->SaveString(full_path, (int)dir_part_len));
    if (MainScript)
    {
      env->SetGlobalVar("$MainScriptName$", env->SaveString(full_path));
      env->SetGlobalVar("$MainScriptFile$", env->SaveString(file_part));
      env->SetGlobalVar("$MainScriptDir$", env->SaveString(full_path, (int)dir_part_len));
    }

    *file_part = 0;
    CWDChanger change_cwd(full_path);

    DWORD size = GetFileSize(h, NULL);
    std::vector<char> buf(size+1, 0);
    BOOL status = ReadFile(h, buf.data(), size, &size, NULL);
    CloseHandle(h);
    if (!status)
      env->ThrowError("Import: unable to read \"%s\"", script_name);

    // Give Unicode smartarses a hint they need to use ANSI encoding
    if (size >= 2) {
      unsigned char* q = reinterpret_cast<unsigned char*>(buf.data());

      if ((q[0]==0xFF && q[1]==0xFE) || (q[0]==0xFE && q[1]==0xFF))
          env->ThrowError("Import: Unicode source files are not supported, "
                          "re-save script with ANSI encoding! : \"%s\"", script_name);

      if (q[0]==0xEF && q[1]==0xBB && q[2]==0xBF)
          env->ThrowError("Import: UTF-8 source files are not supported, "
                          "re-save script with ANSI encoding! : \"%s\"", script_name);
    }

    buf[size] = 0;
    AVSValue eval_args[] = { buf.data(), script_name };
    result = env->Invoke("Eval", AVSValue(eval_args, 2));
  }

  env->SetGlobalVar("$ScriptName$", lastScriptName);
  env->SetGlobalVar("$ScriptFile$", lastScriptFile);
  env->SetGlobalVar("$ScriptDir$",  lastScriptDir);
  env2->DecrImportDepth();

  return result;
}


AVSValue ScriptName(AVSValue args, void*, IScriptEnvironment* env) { return env->GetVarDef("$ScriptName$"); }
AVSValue ScriptFile(AVSValue args, void*, IScriptEnvironment* env) { return env->GetVarDef("$ScriptFile$"); }
AVSValue ScriptDir (AVSValue args, void*, IScriptEnvironment* env) { return env->GetVarDef("$ScriptDir$" ); }

AVSValue SetMemoryMax(AVSValue args, void*, IScriptEnvironment* env) { return env->SetMemoryMax(args[0].AsInt(0)); }
AVSValue SetWorkingDir(AVSValue args, void*, IScriptEnvironment* env) { return env->SetWorkingDir(args[0].AsString()); }

AVSValue Muldiv(AVSValue args, void*, IScriptEnvironment* env) { return int(MulDiv(args[0].AsInt(), args[1].AsInt(), args[2].AsInt())); }

AVSValue Floor(AVSValue args, void*, IScriptEnvironment* env) { return int(floor(args[0].AsFloat())); }
AVSValue Ceil(AVSValue args, void*, IScriptEnvironment* env) { return int(ceil(args[0].AsFloat())); }
AVSValue Round(AVSValue args, void*, IScriptEnvironment* env) { return args[0].AsFloat()<0 ? -int(-args[0].AsFloat()+.5) : int(args[0].AsFloat()+.5); }

AVSValue Acos(AVSValue args, void* user_data, IScriptEnvironment* env) { return acos(args[0].AsFloat()); } 
AVSValue Asin(AVSValue args, void* user_data, IScriptEnvironment* env) { return asin(args[0].AsFloat()); } 
AVSValue Atan(AVSValue args, void* user_data, IScriptEnvironment* env) { return atan(args[0].AsFloat()); } 
AVSValue Atan2(AVSValue args, void* user_data, IScriptEnvironment* env) { return atan2(args[0].AsFloat(), args[1].AsFloat()); } 
AVSValue Cos(AVSValue args, void* user_data, IScriptEnvironment* env) { return cos(args[0].AsFloat()); }
AVSValue Cosh(AVSValue args, void* user_data, IScriptEnvironment* env) { return cosh(args[0].AsFloat()); }
AVSValue Exp(AVSValue args, void* user_data, IScriptEnvironment* env) { return exp(args[0].AsFloat()); }
AVSValue Fmod(AVSValue args, void* user_data, IScriptEnvironment* env) { return fmod(args[0].AsFloat(), args[1].AsFloat()); } 
AVSValue Log(AVSValue args, void* user_data, IScriptEnvironment* env) { return log(args[0].AsFloat()); }
AVSValue Log10(AVSValue args, void* user_data, IScriptEnvironment* env) { return log10(args[0].AsFloat()); }
AVSValue Pow(AVSValue args, void* user_data, IScriptEnvironment* env) { return pow(args[0].AsFloat(),args[1].AsFloat()); }
AVSValue Sin(AVSValue args, void* user_data, IScriptEnvironment* env) { return sin(args[0].AsFloat()); }
AVSValue Sinh(AVSValue args, void* user_data, IScriptEnvironment* env) { return sinh(args[0].AsFloat()); }
AVSValue Tan(AVSValue args, void* user_data, IScriptEnvironment* env) { return tan(args[0].AsFloat()); } 
AVSValue Tanh(AVSValue args, void* user_data, IScriptEnvironment* env) { return tanh(args[0].AsFloat()); } 
AVSValue Sqrt(AVSValue args, void* user_data, IScriptEnvironment* env) { return sqrt(args[0].AsFloat()); }

AVSValue Abs(AVSValue args, void* user_data, IScriptEnvironment* env) { return abs(args[0].AsInt()); }
AVSValue FAbs(AVSValue args, void* user_data, IScriptEnvironment* env) { return fabs(args[0].AsFloat()); }
AVSValue Pi(AVSValue args, void* user_data, IScriptEnvironment* env)  { return 3.14159265358979324; }
#ifdef OPT_ScriptFunctionTau
AVSValue Tau(AVSValue args, void* user_data, IScriptEnvironment* env) { return 6.28318530717958648; }
#endif
AVSValue Sign(AVSValue args, void*, IScriptEnvironment* env) { return args[0].AsFloat()==0 ? 0 : args[0].AsFloat() > 0 ? 1 : -1; }

AVSValue BitAnd(AVSValue args, void*, IScriptEnvironment* env) { return args[0].AsInt() & args[1].AsInt(); }
AVSValue BitNot(AVSValue args, void*, IScriptEnvironment* env) { return ~args[0].AsInt(); }
AVSValue BitOr(AVSValue args, void*, IScriptEnvironment* env)  { return args[0].AsInt() | args[1].AsInt(); }
AVSValue BitXor(AVSValue args, void*, IScriptEnvironment* env) { return args[0].AsInt() ^ args[1].AsInt(); }

AVSValue BitLShift(AVSValue args, void*, IScriptEnvironment* env) { return args[0].AsInt() << args[1].AsInt(); }
AVSValue BitRShiftL(AVSValue args, void*, IScriptEnvironment* env) { return int(unsigned(args[0].AsInt()) >> unsigned(args[1].AsInt())); }
AVSValue BitRShiftA(AVSValue args, void*, IScriptEnvironment* env) { return args[0].AsInt() >> args[1].AsInt(); }

static int a_rol(int value, int shift) {
  if ((shift &= sizeof(value)*8 - 1) == 0)
      return value;
  return (value << shift) | (value >> (sizeof(value)*8 - shift));
}

static int a_ror(int value, int shift) {
  if ((shift &= sizeof(value)*8 - 1) == 0)
      return value;
  return (value >> shift) | (value << (sizeof(value)*8 - shift));
}

static int a_btc(int value, int bit) {
  value ^= 1 << bit;
  return value;
}

static int a_btr(int value, int bit) {
  value &= ~(1 << bit);
  return value;
}

static int a_bts(int value, int bit) {
  value |= (1 << bit);
  return value;
}

static bool a_bt (int value, int bit) {
  return (value & (1 << bit)) ? true : false;
}

AVSValue BitRotateL(AVSValue args, void*, IScriptEnvironment* env) { return a_rol(args[0].AsInt(), args[1].AsInt()); }
AVSValue BitRotateR(AVSValue args, void*, IScriptEnvironment* env) { return a_ror(args[0].AsInt(), args[1].AsInt()); }

AVSValue BitChg(AVSValue args, void*, IScriptEnvironment* env) { return a_btc(args[0].AsInt(), args[1].AsInt()); }
AVSValue BitClr(AVSValue args, void*, IScriptEnvironment* env) { return a_btr(args[0].AsInt(), args[1].AsInt()); }
AVSValue BitSet(AVSValue args, void*, IScriptEnvironment* env) { return a_bts(args[0].AsInt(), args[1].AsInt()); }
AVSValue BitTst(AVSValue args, void*, IScriptEnvironment* env) { return a_bt (args[0].AsInt(), args[1].AsInt()); }

AVSValue UCase(AVSValue args, void*, IScriptEnvironment* env) { return _strupr(env->SaveString(args[0].AsString())); }
AVSValue LCase(AVSValue args, void*, IScriptEnvironment* env) { return _strlwr(env->SaveString(args[0].AsString())); }

AVSValue StrLen(AVSValue args, void*, IScriptEnvironment* env) { return int(strlen(args[0].AsString())); }
AVSValue RevStr(AVSValue args, void*, IScriptEnvironment* env) { return _strrev(env->SaveString(args[0].AsString())); }

AVSValue LeftStr(AVSValue args, void*, IScriptEnvironment* env)
 {
   const int count = args[1].AsInt();
   if (count < 0)
      env->ThrowError("LeftStr: Negative character count not allowed");
   char *result = new(std::nothrow) char[count+1];
   if (!result) env->ThrowError("LeftStr: malloc failure!");
   *result = 0;
   strncat(result, args[0].AsString(), count);
   AVSValue ret = env->SaveString(result);
   delete[] result;
   return ret; 
 }

AVSValue MidStr(AVSValue args, void*, IScriptEnvironment* env)
{
  const int maxlen = (int)strlen(args[0].AsString());
  if (args[1].AsInt() < 1)
      env->ThrowError("MidStr: Illegal character location");
  int len = args[2].AsInt(maxlen);
  if (len < 0)
      env->ThrowError("MidStr: Illegal character count");
  int offset = args[1].AsInt() - 1;
  if (maxlen <= offset) { offset = 0; len = 0;}
  char *result = new(std::nothrow) char[len+1];
  if (!result) env->ThrowError("MidStr: malloc failure!");
  *result = 0;
  strncat(result, args[0].AsString()+offset, len);
  AVSValue ret = env->SaveString(result);
  delete[] result;
  return ret;
}

AVSValue RightStr(AVSValue args, void*, IScriptEnvironment* env)
 {
   if (args[1].AsInt() < 0)
      env->ThrowError("RightStr: Negative character count not allowed");

   int offset = (int)strlen(args[0].AsString()) - args[1].AsInt();
   if (offset < 0) offset = 0;
   char *result = new(std::nothrow) char[args[1].AsInt()+1];
   if (!result) env->ThrowError("RightStr: malloc failure!");
   *result = 0;
   strncat(result, args[0].AsString()+offset, args[1].AsInt());
   AVSValue ret = env->SaveString(result);
   delete[] result;
   return ret; 
 }

AVSValue StrCmp(AVSValue args, void*, IScriptEnvironment* env)
{
  return lstrcmp( args[0].AsString(), args[1].AsString() );
}

AVSValue StrCmpi(AVSValue args, void*, IScriptEnvironment* env)
{
  return lstrcmpi( args[0].AsString(), args[1].AsString() );
}

AVSValue FindStr(AVSValue args, void*, IScriptEnvironment* env)
{
  const char *pdest = strstr( args[0].AsString(),args[1].AsString() );
  int result = (int)(pdest - args[0].AsString() + 1);
  if (pdest == NULL) result = 0;
  return result; 
}

AVSValue Rand(AVSValue args, void* user_data, IScriptEnvironment* env)
 { int limit = args[0].AsInt(RAND_MAX);
   bool scale_mode = args[1].AsBool((abs(limit) > RAND_MAX));

   if (args[2].AsBool(false)) srand( (unsigned) time(NULL) ); //seed

   if (scale_mode) {
      double f = 1.0 / (RAND_MAX + 1.0);
      return int(f * rand() * limit);
   }
   else { //modulus mode
      int s = (limit < 0 ? -1 : 1);
      if (limit==0) return 0;
       else return s * rand() % limit;
   }
 }

AVSValue Select(AVSValue args, void*, IScriptEnvironment* env)
{ int i = args[0].AsInt();
  if ((args[1].ArraySize() <= i) || (i < 0))
    env->ThrowError("Select: Index value out of range");
  return args[1][i];
}

AVSValue NOP(AVSValue args, void*, IScriptEnvironment* env) { return NULL;}

AVSValue Undefined(AVSValue args, void*, IScriptEnvironment* env) { return AVSValue();}

AVSValue Exist(AVSValue args, void*, IScriptEnvironment* env) {
  const char *filename = args[0].AsString();

  if (strchr(filename, '*') || strchr(filename, '?')) // wildcard
      return false;

  struct _finddata_t c_file;

  intptr_t f = _findfirst(filename, &c_file);

  if (f == -1)
      return false;

  _findclose(f);

  return true;
}


//WE ->

// Spline functions to generate and evaluate a natural bicubic spline
void spline(float x[], float y[], int n, float y2[])
{
	int i,k;
	float p, qn, sig, un, *u;

	u = new float[n];

	y2[1]=u[1]=0.0f;

	for (i=2; i<=n-1; i++) {
		sig = (x[i] - x[i-1])/(x[i+1] - x[i-1]);
		p = sig * y2[i-1] + 2.0f;
		y2[i] = (sig - 1.0f) / p;
		u[i] = (y[i+1] - y[i])/(x[i+1] - x[i]) - (y[i] - y[i-1])/(x[i] - x[i-1]);
		u[i] = (6.0f*u[i]/(x[i+1] - x[i-1]) - sig*u[i-1])/p;
	}
	qn=un=0.0f;
	y2[n]=(un - qn*u[n-1])/(qn * y2[n-1] + 1.0f);
	for (k=n-1; k>=1; k--) {
		y2[k] = y2[k] * y2[k+1] + u[k];
	}

	delete[] u;
}

int splint(float xa[], float ya[], float y2a[], int n, float x, float &y, bool cubic)
{
	int klo, khi, k;
	float h,b,a;

	klo=1;
	khi=n;
	while (khi-klo > 1) {
		k=(khi + klo) >> 1;
		if (xa[k] > x ) khi = k;
		else klo = k;
	}
	h = xa[khi] - xa[klo];
	if (h==0.0f) {
		y=0.0f;
		return -1;	// all x's have to be different
	}
	a = (xa[khi] - x)/h;
	b = (x - xa[klo])/h;

	if (cubic) {
		y = a * ya[klo] + b*ya[khi] + ((a*a*a - a)*y2a[klo] + (b*b*b - b)*y2a[khi]) * (h*h) / 6.0f;
	} else {
		y = a * ya[klo] + b*ya[khi];
	}
	return 0;
}

// the script functions 
AVSValue AVSChr(AVSValue args, void*, IScriptEnvironment* env )
{
    char s[2];

	s[0]=(char)(args[0].AsInt());
	s[1]=0;
    return env->SaveString(s);
}

AVSValue AVSOrd(AVSValue args, void*, IScriptEnvironment* env )
{
    return (int)args[0].AsString()[0] & 0xFF;
}

AVSValue FillStr(AVSValue args, void*, IScriptEnvironment* env )
{
    const int count = args[0].AsInt();
    if (count <= 0)
      env->ThrowError("FillStr: Repeat count must greater than zero!");

    const char *str = args[1].AsString(" ");
    const int len = lstrlen(str);
    const int total = count * len;

    char *buff = new(std::nothrow) char[total];
    if (!buff)
      env->ThrowError("FillStr: malloc failure!");

    for (int i=0; i<total; i+=len)
      memcpy(buff+i, str, len);

    AVSValue ret = env->SaveString(buff, total);
    delete[] buff;
    return ret; 
}

AVSValue AVSTime(AVSValue args, void*, IScriptEnvironment* env )
{
	time_t lt_t;
	struct tm * lt;
	time(&lt_t);
	lt = localtime (&lt_t);
    char s[1024];
    strftime(s,1024,args[0].AsString(""),lt);
    s[1023] = 0;
    return env->SaveString(s);
}

AVSValue Spline(AVSValue args, void*, IScriptEnvironment* env )
{
	int n;
	float x,y;
	int i;
	bool cubic;

	AVSValue coordinates;

	x = args[0].AsFloatf(0);
	coordinates = args[1];
	cubic = args[2].AsBool(true);

	n = coordinates.ArraySize() ;

	if (n<4 || n&1) env->ThrowError("To few arguments for Spline");

	n=n/2;

  float *buf = new float[(n+1)*3];
  float *xa  = &(buf[(n+1) * 0]);
  float *ya  = &(buf[(n+1) * 1]);
  float *y2a = &(buf[(n+1) * 2]);

	for (i=1; i<=n; i++) {
		xa[i] = coordinates[(i-1)*2+0].AsFloatf(0);
		ya[i] = coordinates[(i-1)*2+1].AsFloatf(0);
	}

	for (i=1; i<n; i++) {
		if (xa[i] >= xa[i+1]) env->ThrowError("Spline: all x values have to be different and in ascending order!");
	}
	
	spline(xa, ya, n, y2a);
	splint(xa, ya, y2a, n, x, y, cubic);

  delete[] buf;

	return y;
}

// WE <-

static inline const VideoInfo& VI(const AVSValue& arg) { return arg.AsClip()->GetVideoInfo(); }

AVSValue PixelType (AVSValue args, void*, IScriptEnvironment* env) {
  switch (VI(args[0]).pixel_type) {
    case VideoInfo::CS_BGR24 :
	  return "RGB24";
    case VideoInfo::CS_BGR32 :
	  return "RGB32";
    case VideoInfo::CS_YUY2  :
	  return "YUY2";
    case VideoInfo::CS_YV24  :
	  return "YV24";
    case VideoInfo::CS_YV16  :
	  return "YV16";
    case VideoInfo::CS_YV12  :
    case VideoInfo::CS_I420  :
	  return "YV12";
    case VideoInfo::CS_YUV9  :
	  return "YUV9";
    case VideoInfo::CS_YV411 :
	  return "YV411";
    case VideoInfo::CS_Y8    :
	  return "Y8";
    case VideoInfo::CS_YUV420P16 :
    return "YUV420P16";
    case VideoInfo::CS_YUV422P16 :
    return "YUV422P16";
    case VideoInfo::CS_YUV444P16 :
    return "YUV444P16";
    case VideoInfo::CS_Y16       :
    return "Y16";
    case VideoInfo::CS_YUV420PS  :
    return "YUV420PS";
    case VideoInfo::CS_YUV422PS  :
    return "YUV422PS";
    case VideoInfo::CS_YUV444PS  :
    return "YUV444PS";
    case VideoInfo::CS_Y32       :
    return "Y32";
    default:
	  break;
  }
  return "";
}

AVSValue Width(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).width; }
AVSValue Height(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).height; }
AVSValue FrameCount(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).num_frames; }
AVSValue FrameRate(AVSValue args, void*, IScriptEnvironment* env) { const VideoInfo& vi = VI(args[0]); return (double)vi.fps_numerator / vi.fps_denominator; } // maximise available precision
AVSValue FrameRateNumerator(AVSValue args, void*, IScriptEnvironment* env) { return (int)VI(args[0]).fps_numerator; } // unsigned int truncated to int
AVSValue FrameRateDenominator(AVSValue args, void*, IScriptEnvironment* env) { return (int)VI(args[0]).fps_denominator; } // unsigned int truncated to int
AVSValue AudioRate(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).audio_samples_per_second; }
AVSValue AudioLength(AVSValue args, void*, IScriptEnvironment* env) { return (int)VI(args[0]).num_audio_samples; }  // Truncated to int
AVSValue AudioLengthLo(AVSValue args, void*, IScriptEnvironment* env) { return (int)(VI(args[0]).num_audio_samples % (unsigned)args[1].AsInt(1000000000)); }
AVSValue AudioLengthHi(AVSValue args, void*, IScriptEnvironment* env) { return (int)(VI(args[0]).num_audio_samples / (unsigned)args[1].AsInt(1000000000)); }
AVSValue AudioLengthS(AVSValue args, void*, IScriptEnvironment* env) { char s[32]; return env->SaveString(_i64toa(VI(args[0]).num_audio_samples, s, 10)); } 
AVSValue AudioLengthF(AVSValue args, void*, IScriptEnvironment* env) { return (float)VI(args[0]).num_audio_samples; } // at least this will give an order of the size
AVSValue AudioDuration(AVSValue args, void*, IScriptEnvironment* env) {
  const VideoInfo& vi = VI(args[0]);
  return (double)vi.num_audio_samples / vi.audio_samples_per_second;
}

AVSValue AudioChannels(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).HasAudio() ? VI(args[0]).nchannels : 0; }
AVSValue AudioBits(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).BytesPerChannelSample()*8; }
AVSValue IsAudioFloat(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).IsSampleType(SAMPLE_FLOAT); }
AVSValue IsAudioInt(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).IsSampleType(SAMPLE_INT8 | SAMPLE_INT16 | SAMPLE_INT24 | SAMPLE_INT32 ); }

AVSValue IsRGB(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).IsRGB(); }
AVSValue IsRGB24(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).IsRGB24(); }
AVSValue IsRGB32(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).IsRGB32(); }
AVSValue IsYUV(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).IsYUV(); }
AVSValue IsYUY2(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).IsYUY2(); }
AVSValue IsY8(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).IsY8(); }
AVSValue IsYV12(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).IsYV12(); }
AVSValue IsYV16(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).IsYV16(); }
AVSValue IsYV24(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).IsYV24(); }
AVSValue IsYV411(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).IsYV411(); }
AVSValue IsPlanar(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).IsPlanar(); }
AVSValue IsInterleaved(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).IsColorSpace(VideoInfo::CS_INTERLEAVED); }
AVSValue IsFieldBased(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).IsFieldBased(); }
AVSValue IsFrameBased(AVSValue args, void*, IScriptEnvironment* env) { return !VI(args[0]).IsFieldBased(); }
AVSValue GetParity(AVSValue args, void*, IScriptEnvironment* env) { return args[0].AsClip()->GetParity(args[1].AsInt(0)); }

AVSValue HasVideo(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).HasVideo(); }
AVSValue HasAudio(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).HasAudio(); }

AVSValue String(AVSValue args, void*, IScriptEnvironment* env)
{
  if (args[0].IsString()) return args[0];
  if (args[0].IsBool()) return (args[0].AsBool()?"true":"false");
  if (args[1].Defined()) {	// WE --> a format parameter is present 
		if (args[0].IsFloat()) {	//if it is an Int: IsFloat gives True, also !
			return  env->Sprintf(args[1].AsString("%f"),args[0].AsFloat());
		}
		return "";	// <--WE
  } else {	// standard behaviour
	  if (args[0].IsInt()) {
		char s[12];
		return env->SaveString(_itoa(args[0].AsInt(), s, 10));
	  }
	  if (args[0].IsFloat()) {
		char s[30];
    _locale_t locale = _create_locale(LC_NUMERIC, "C"); // decimal point: dot
    _sprintf_l(s,"%lf", locale, args[0].AsFloat());
    _free_locale(locale);
		return env->SaveString(s);
	  }
  }
  return "";
} 

AVSValue Hex(AVSValue args, void*, IScriptEnvironment* env) { char s[9]; return env->SaveString(_itoa(args[0].AsInt(), s, 16)); } 

AVSValue IsBool(AVSValue args, void*, IScriptEnvironment* env) { return args[0].IsBool(); }
AVSValue IsInt(AVSValue args, void*, IScriptEnvironment* env) { return args[0].IsInt(); }
AVSValue IsFloat(AVSValue args, void*, IScriptEnvironment* env) { return args[0].IsFloat(); }
AVSValue IsString(AVSValue args, void*, IScriptEnvironment* env) { return args[0].IsString(); }
AVSValue IsClip(AVSValue args, void*, IScriptEnvironment* env) { return args[0].IsClip(); }
AVSValue Defined(AVSValue args, void*, IScriptEnvironment* env) { return args[0].Defined(); }

AVSValue Default(AVSValue args, void*, IScriptEnvironment* env) { return args[0].Defined() ? args[0] : args[1]; }
AVSValue VersionNumber(AVSValue args, void*, IScriptEnvironment* env) { return AVS_CLASSIC_VERSION; }
AVSValue VersionString(AVSValue args, void*, IScriptEnvironment* env) { return AVS_FULLVERSION; }

AVSValue Int(AVSValue args, void*, IScriptEnvironment* env) { return int(args[0].AsFloat()); }
AVSValue Frac(AVSValue args, void*, IScriptEnvironment* env) { return args[0].AsFloat() - __int64(args[0].AsFloat()); }
AVSValue Float(AVSValue args, void*, IScriptEnvironment* env) { return args[0].AsFloat(); }

AVSValue Value(AVSValue args, void*, IScriptEnvironment* env) { char *stopstring; return strtod(args[0].AsString(),&stopstring); }
AVSValue HexValue(AVSValue args, void*, IScriptEnvironment* env) { char *stopstring; return (int)strtoul(args[0].AsString(),&stopstring,16); }

AVSValue AvsMin(AVSValue args, void*, IScriptEnvironment* env )
{
  int i;
  bool isInt = true;

  const int n = args[0].ArraySize();
  if (n < 2) env->ThrowError("To few arguments for Min");

  // If all numbers are Ints return an Int
  for (i=0; i < n; i++)
    if (!args[0][i].IsInt()) {
      isInt = false;
      break;
  }

  if (isInt) {
    int V = args[0][0].AsInt();
    for (i=1; i < n; i++)
      V = min(V, args[0][i].AsInt());
    return V;
  }
  else {
    float V = args[0][0].AsFloatf();
    for (i=1; i < n; i++)
      V = min(V, args[0][i].AsFloatf());
    return V;
  }
}

AVSValue AvsMax(AVSValue args, void*, IScriptEnvironment* env )
{
  int i;
  bool isInt = true;

  const int n = args[0].ArraySize();
  if (n < 2) env->ThrowError("To few arguments for Max");

  // If all numbers are Ints return an Int
  for (i=0; i < n; i++)
    if (!args[0][i].IsInt()) {
      isInt = false;
      break;
  }

  if (isInt) {
    int V = args[0][0].AsInt();
    for (i=1; i < n; i++)
      V = max(V, args[0][i].AsInt());
    return V;
  }
  else {
    float V = args[0][0].AsFloatf();
    for (i=1; i < n; i++)
      V = max(V, args[0][i].AsFloatf());
    return V;
  }
}

AVSValue AddAutoloadDir (AVSValue args, void*, IScriptEnvironment* env)
{
  IScriptEnvironment2 *env2 = static_cast<IScriptEnvironment2*>(env);
  env2->AddAutoloadDir(args[0].AsString(), args[1].AsBool(true));
  return AVSValue();
}

AVSValue ClearAutoloadDirs (AVSValue args, void*, IScriptEnvironment* env)
{
  IScriptEnvironment2 *env2 = static_cast<IScriptEnvironment2*>(env);
  env2->ClearAutoloadDirs();
  return AVSValue();
}

AVSValue AutoloadPlugins (AVSValue args, void*, IScriptEnvironment* env)
{
  IScriptEnvironment2 *env2 = static_cast<IScriptEnvironment2*>(env);
  env2->AutoloadPlugins();
  return AVSValue();
}

AVSValue FunctionExists (AVSValue args, void*, IScriptEnvironment* env)
{
  return env->FunctionExists(args[0].AsString());
}

AVSValue InternalFunctionExists (AVSValue args, void*, IScriptEnvironment* env)
{
  IScriptEnvironment2 *env2 = static_cast<IScriptEnvironment2*>(env);
  return env2->InternalFunctionExists(args[0].AsString());
}

AVSValue SetFilterMTMode (AVSValue args, void*, IScriptEnvironment* env)
{
  IScriptEnvironment2 *env2 = static_cast<IScriptEnvironment2*>(env);
  env2->SetFilterMTMode(args[0].AsString(), (MtMode)args[1].AsInt(), args[2].AsBool(false));
  return AVSValue();
}
