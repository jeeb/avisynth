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


#include "script.h"



/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

AVSFunction Script_functions[] = {
  { "floor", "f", Floor },
  { "ceil", "f", Ceil },
  { "round", "f", Round },

  { "width", "c", Width },
  { "height", "c", Height },
  { "framecount", "c", FrameCount },
  { "framerate", "c", FrameRate },
  { "audiorate", "c", AudioRate },
  { "audiolength", "c", AudioLength },
  { "audiochannels", "c", AudioChannels },
  { "audiobits", "c", AudioBits },
  { "IsRGB", "c", IsRGB },
  { "IsYUY2", "c", IsYUY2 },
  { "IsFieldBased", "c", IsFieldBased },
  { "IsFrameBased", "c", IsFrameBased },
  { "GetParity", "c[n]i", GetParity },

  { "IsBool", ".", IsBool },
  { "IsInt", ".", IsInt },
  { "IsFloat", ".", IsFloat },
  { "IsString", ".", IsString },
  { "IsClip", ".", IsClip },
  { "Defined", ".", Defined },

  { "Default", "..", Default },

  { "Eval", "s[name]s", Eval },
  { "Apply", "s.*", Apply },
  { "Import", "s+", Import },

  { "Assert", "b[message]s", Assert },
  { "Assert", "s", AssertEval },

  { "Cache", "c", Cache::Create_Cache },

  { 0 }
};










/**********************************
 *******   Script Function   ******
 *********************************/

ScriptFunction::ScriptFunction( const PExpression& _body, const char** _param_names, 
                                int param_count ) 
  : body(_body) 
{
  param_names = new const char*[param_count];
  memcpy(param_names, _param_names, param_count*sizeof(const char*));
}
  

AVSValue ScriptFunction::Execute(AVSValue args, void* user_data, IScriptEnvironment* env) 
{
  ScriptFunction* self = (ScriptFunction*)user_data;
  env->PushContext();
  for (int i=0; i<args.ArraySize(); ++i)
    env->SetVar(self->param_names[i], args[i]);
  AVSValue result = self->body->Evaluate(env);
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

CWDChanger::CWDChanger(const char* new_cwd)
{
  DWORD save_cwd_success = GetCurrentDirectory(MAX_PATH, old_working_directory);
  BOOL set_cwd_success = SetCurrentDirectory(new_cwd);
  restore = (save_cwd_success && set_cwd_success);
}


CWDChanger::~CWDChanger(void)
{
  if (restore)
    SetCurrentDirectory(old_working_directory);
}




AVSValue Assert(AVSValue args, void*, IScriptEnvironment* env) 
{
  if (!args[0].AsBool())
    env->ThrowError(args[1].Defined() ? args[1].AsString() : "Assert: assertion failed");
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
  ScriptParser parser(env, args[0].AsString(), args[1].AsString(0));
  PExpression exp = parser.Parse();
  return exp->Evaluate(env);
}

AVSValue Apply(AVSValue args, void*, IScriptEnvironment* env) 
{
  return env->Invoke(args[0].AsString(), args[1]);
}



AVSValue Import(AVSValue args, void*, IScriptEnvironment* env) 
{
  args = args[0];
  AVSValue result;
  for (int i=0; i<args.ArraySize(); ++i) {
    const char* script_name = args[i].AsString();

    TCHAR full_path[MAX_PATH];
    TCHAR* file_part;
    if (strchr(script_name, '\\') || strchr(script_name, '/')) {
      DWORD len = GetFullPathName(script_name, MAX_PATH, full_path, &file_part);
      if (len == 0 || len > MAX_PATH)
        env->ThrowError("Import: unable to open \"%s\" (path invalid?)", script_name);
    } else {
      DWORD len = SearchPath(NULL, script_name, NULL, MAX_PATH, full_path, &file_part);
      if (len == 0 || len > MAX_PATH)
        env->ThrowError("Import: unable to locate \"%s\" (try specifying a path)", script_name);
    }

    HANDLE h = ::CreateFile(full_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE)
      env->ThrowError("Import: couldn't open \"%s\"", full_path);

    *file_part = 0;
    CWDChanger change_cwd(full_path);

    DWORD size = GetFileSize(h, NULL);
    DynamicCharBuffer buf(size+1);
    if (!ReadFile(h, buf, size, &size, NULL))
      env->ThrowError("Import: unable to read \"%s\"", script_name);
    CloseHandle(h);

    ((char*)buf)[size] = 0;
    AVSValue eval_args[] = { (char*)buf, script_name };
    result = env->Invoke("Eval", AVSValue(eval_args, 2));
  }

  return result;
}



AVSValue Floor(AVSValue args, void*, IScriptEnvironment* env) { return int(args[0].AsFloat()); }
AVSValue Ceil(AVSValue args, void*, IScriptEnvironment* env) { return -int(-args[0].AsFloat()); }
AVSValue Round(AVSValue args, void*, IScriptEnvironment* env) { return int(args[0].AsFloat()+0.5); }

static inline const VideoInfo& VI(const AVSValue& arg) { return arg.AsClip()->GetVideoInfo(); }

AVSValue Width(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).width; }
AVSValue Height(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).height; }
AVSValue FrameCount(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).num_frames; }
AVSValue FrameRate(AVSValue args, void*, IScriptEnvironment* env) { const VideoInfo& vi = VI(args[0]); return float(vi.fps_numerator) / float(vi.fps_denominator); }
AVSValue AudioRate(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).audio_samples_per_second; }
AVSValue AudioLength(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).num_audio_samples; }
AVSValue AudioChannels(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).stereo ? 2 : 1; }
AVSValue AudioBits(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).sixteen_bit ? 16 : 8; }
AVSValue IsRGB(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).IsRGB(); }
AVSValue IsYUY2(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).IsYUY2(); }
AVSValue IsFieldBased(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).field_based; }
AVSValue IsFrameBased(AVSValue args, void*, IScriptEnvironment* env) { return !VI(args[0]).field_based; }
AVSValue GetParity(AVSValue args, void*, IScriptEnvironment* env) { return args[0].AsClip()->GetParity(args[1].AsInt(0)); }

AVSValue IsBool(AVSValue args, void*, IScriptEnvironment* env) { return args[0].IsBool(); }
AVSValue IsInt(AVSValue args, void*, IScriptEnvironment* env) { return args[0].IsInt(); }
AVSValue IsFloat(AVSValue args, void*, IScriptEnvironment* env) { return args[0].IsFloat(); }
AVSValue IsString(AVSValue args, void*, IScriptEnvironment* env) { return args[0].IsString(); }
AVSValue IsClip(AVSValue args, void*, IScriptEnvironment* env) { return args[0].IsClip(); }
AVSValue Defined(AVSValue args, void*, IScriptEnvironment* env) { return args[0].Defined(); }

AVSValue Default(AVSValue args, void*, IScriptEnvironment* env) { return args[0].Defined() ? args[0] : args[1]; }
