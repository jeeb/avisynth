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


#include "stdafx.h"

#include "script.h"
#include <time.h>
 
#ifdef _MSC_VER
  #define itoa(a,b,c) _itoa(a,b,c)
#endif

/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

AVSFunction Script_functions[] = {
  { "floor", "f", Floor },
  { "ceil", "f", Ceil },
  { "round", "f", Round },

  { "sin", "f", Sin },
  { "cos", "f", Cos },
  { "pi", "", Pi },
  { "log", "f", Log },
  { "exp", "f", Exp },
  { "pow", "ff", Pow },
  { "sqrt", "f", Sqrt },

  { "abs", "i", Abs },
  { "abs", "f", FAbs },
  { "sign","f",Sign},

  { "lcase","s",LCase},
  { "ucase","s",UCase},
  { "strlen","s",StrLen},
  { "revstr","s",RevStr},
  { "leftstr","si",LeftStr},
  { "midstr","si[length]i",MidStr},
  { "rightstr","si",RightStr},
  { "findstr","ss",FindStr},

  { "rand", "[max]i[scale]b[seed]b", Rand },

  { "Select", "i.+", Select },

  { "nop","",NOP },

  { "width", "c", Width },
  { "height", "c", Height },
  { "framecount", "c", FrameCount },
  { "framerate", "c", FrameRate },
  { "audiorate", "c", AudioRate },
  { "audiolength", "c", AudioLength },  // Fixme: Add int64 to script
  { "audiochannels", "c", AudioChannels },
  { "audiobits", "c", AudioBits },
  { "IsAudioFloat", "c", IsFloat },
  { "IsAudioInt", "c", IsInt },
  { "IsRGB", "c", IsRGB },
  { "IsYUY2", "c", IsYUY2 },
  { "IsYUV", "c", IsYUV },
  { "IsYV12", "c", IsYV12 },
  { "IsPlanar", "c", IsPlanar },
  { "IsInterleaved", "c", IsInterleaved },
  { "IsRGB24", "c", IsRGB24 },
  { "IsRGB32", "c", IsRGB32 },
  { "IsFieldBased", "c", IsFieldBased },
  { "IsFrameBased", "c", IsFrameBased },
  { "GetParity", "c[n]i", GetParity },
  { "String", ".[]s", String },

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

  { "SetMemoryMax", "i", SetMemoryMax },

  { "SetWorkingDir", "s", SetWorkingDir },
  { "Exist", "s", Exist },

  { "Chr","i", AVSChr },
  { "Time", "s", AVSTime },
  { "Spline","[x]ff+[cubic]b", Spline },

  { "int", "f", Int },
  { "frac","f", Frac},
  { "float","f",Float},

  { "value","s",Value},
  { "hexvalue","s",HexValue},

  { "VersionNumber", "", VersionNumber },
  { "VersionString", "", VersionString }, 
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



AVSValue SetMemoryMax(AVSValue args, void*, IScriptEnvironment* env) { return env->SetMemoryMax(args[0].AsInt()); }
AVSValue SetWorkingDir(AVSValue args, void*, IScriptEnvironment* env) { return env->SetWorkingDir(args[0].AsString()); }

AVSValue Floor(AVSValue args, void*,IScriptEnvironment* env) { return int(floor(args[0].AsFloat())); }
AVSValue Ceil(AVSValue args, void*, IScriptEnvironment* env) { return int(ceil(args[0].AsFloat())); }
AVSValue Round(AVSValue args, void*, IScriptEnvironment* env) { return args[0].AsFloat()<0 ? -int(-args[0].AsFloat()+.5) : int(args[0].AsFloat()+.5); }

AVSValue Sin(AVSValue args, void* user_data, IScriptEnvironment* env) { return sin(args[0].AsFloat()); }
AVSValue Cos(AVSValue args, void* user_data, IScriptEnvironment* env) { return cos(args[0].AsFloat()); }
AVSValue Pi(AVSValue args, void* user_data, IScriptEnvironment* env) { return 3.14159265358979323; }
AVSValue Log(AVSValue args, void* user_data, IScriptEnvironment* env) { return log(args[0].AsFloat()); }
AVSValue Exp(AVSValue args, void* user_data, IScriptEnvironment* env) { return exp(args[0].AsFloat()); }
AVSValue Pow(AVSValue args, void* user_data, IScriptEnvironment* env) {	return pow(args[0].AsFloat(),args[1].AsFloat()); }
AVSValue Sqrt(AVSValue args, void* user_data, IScriptEnvironment* env) { return sqrt(args[0].AsFloat()); }
AVSValue FAbs(AVSValue args, void* user_data, IScriptEnvironment* env) { return fabs(args[0].AsFloat()); }
AVSValue Abs(AVSValue args, void* user_data, IScriptEnvironment* env) { return abs(args[0].AsInt()); }
AVSValue Sign(AVSValue args, void*, IScriptEnvironment* env) { return args[0].AsFloat()==0 ? 0 : args[0].AsFloat() > 0 ? 1 : -1; }

AVSValue UCase(AVSValue args, void*, IScriptEnvironment* env) { return _strupr(_strdup(args[0].AsString())); }
AVSValue LCase(AVSValue args, void*, IScriptEnvironment* env) { return _strlwr(_strdup(args[0].AsString())); }

AVSValue StrLen(AVSValue args, void*, IScriptEnvironment* env) { return int(strlen(args[0].AsString())); }
AVSValue RevStr(AVSValue args, void*, IScriptEnvironment* env) { return _strrev(_strdup(args[0].AsString())); }

AVSValue LeftStr(AVSValue args, void*, IScriptEnvironment* env)
 { char result[512] = "\0";

   if (args[1].AsInt() < 0)
      env->ThrowError("LeftStr: Negative character count not allowed");
   strncat(result, args[0].AsString(), args[1].AsInt());
   return _strdup(result); }

AVSValue MidStr(AVSValue args, void*, IScriptEnvironment* env)
{ char result[512] = "\0";
  int maxlen, len, offset;

  maxlen = strlen(args[0].AsString());
  if (args[1].AsInt() < 1)
      env->ThrowError("MidStr: Illegal character location");
  len = args[2].AsInt(maxlen);
  if (len < 0)
      env->ThrowError("MidStr: Illegal character count");
  offset = args[1].AsInt() - 1;
  if (maxlen <= offset) { offset = 0; len = 0;}
  strncat(result, args[0].AsString()+offset, len);
  return _strdup(result); }

AVSValue RightStr(AVSValue args, void*, IScriptEnvironment* env)
 { char result[512] = "\0";
   int offset;

   if (args[1].AsInt() < 0)
      env->ThrowError("RightStr: Negative character count not allowed");
   offset = strlen(args[0].AsString()) - args[1].AsInt();
   if (offset < 0) offset = 0;
   strncat(result, args[0].AsString()+offset, args[1].AsInt());
   return _strdup(result); }

AVSValue FindStr(AVSValue args, void*, IScriptEnvironment* env)
{ char *pdest;
  int result;

  pdest = strstr( args[0].AsString(),args[1].AsString() );
  result = pdest - args[0].AsString() +1;
  if (pdest == NULL) result = 0;
  return result; }

AVSValue Rand(AVSValue args, void* user_data, IScriptEnvironment* env)
 { int limit = args[0].AsInt(RAND_MAX);
   bool scale_mode = args[1].AsBool((abs(limit) > RAND_MAX));

   if (args[2].AsBool(false)) srand( (unsigned) time(NULL) ); //seed

   if (scale_mode) {
      double f = 1.0 / (RAND_MAX + 1.0);
      return int(f * rand() * limit); }
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

AVSValue Exist(AVSValue args, void*, IScriptEnvironment* env)
 { struct _finddata_t c_file;
   const char *filename = args[0].AsString();
   bool wildcard;
   wildcard = ((strchr(filename,'*')!=NULL) || (strchr(filename,'?')!=NULL));
   return _findfirst(filename,&c_file)==-1L ? false : wildcard ? false : true; 
}


//WE ->

// Spline functions to generate and evaluate a natural bicubic spline
void spline(float x[], float y[], int n, float y2[])
{
	int i,k;
	float p, qn, sig, un, u[256];

	y2[1]=u[1]=0.0;

	for (i=2; i<=n-1; i++) {
		sig = (x[i] - x[i-1])/(x[i+1] - x[i-1]);
		p = sig * y2[i-1] + 2.0;
		y2[i] = (sig - 1.0) / p;
		u[i] = (y[i+1] - y[i])/(x[i+1] - x[i]) - (y[i] - y[i-1])/(x[i] - x[i-1]);
		u[i] = (6.0*u[i]/(x[i+1] - x[i-1]) - sig*u[i-1])/p;
	}
	qn=un=0.0;
	y2[n]=(un - qn*u[n-1])/(qn * y2[n-1] + 1.0);
	for (k=n-1; k>=1; k--) {
		y2[k] = y2[k] * y2[k+1] + u[k];
	}
}

int splint(float xa[], float ya[], float y2a[], int n, float x, float * y, bool cubic)
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
	if (h==0.0) {
		y=0;
		return -1;	// all x's have to be different
	}
	a = (xa[khi] - x)/h;
	b = (x - xa[klo])/h;

	if (cubic) {
		*y = a * ya[klo] + b*ya[khi] + ((a*a*a - a)*y2a[klo] + (b*b*b - b)*y2a[khi]) * (h*h) / 6.0;
	} else {
		*y = a * ya[klo] + b*ya[khi];
	}
	return 0;
}

// the script functions 
AVSValue AVSChr(AVSValue args, void*,IScriptEnvironment* env )
{
    char *s = new char[2];

	s[0]=(char)(args[0].AsInt());
	s[1]=0;
    return s;
}

AVSValue AVSTime(AVSValue args, void*,IScriptEnvironment* env )
{
	time_t lt_t;
	struct tm * lt;
	time(&lt_t);
	lt = localtime (&lt_t);
    char *s = new char[50];
	strftime(s,50,args[0].AsString(""),lt);
    return s;
}

AVSValue Spline(AVSValue args, void*, IScriptEnvironment* env )
{
    float xa[256];
	float ya[256];
	float y2a[256];
	int n;
	float x,y;
	int i;
	bool cubic;

	AVSValue coordinates;

	x = args[0].AsFloat(0);
	coordinates = args[1];
	cubic = args[2].AsBool(true);

	n = coordinates.ArraySize() ;

	if (n<4 || n&1) env->ThrowError("Two few arguments for Spline");

	n=n/2;
	for (i=1; i<=n; i++) {
		xa[i] = coordinates[(i-1)*2].AsFloat(0);
		ya[i] = coordinates[(i-1)*2+1].AsFloat(0);
	}

	for (i=1; i<n; i++) {
		if (xa[i] >= xa[i+1]) env->ThrowError("Spline: all x values have to be different and in ascending order!");
	}
	
	spline(xa, ya, n, y2a);
	splint(xa, ya, y2a, n, x, &y, cubic);
	return y;
}

// WE <-

static inline const VideoInfo& VI(const AVSValue& arg) { return arg.AsClip()->GetVideoInfo(); }

AVSValue Width(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).width; }
AVSValue Height(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).height; }
AVSValue FrameCount(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).num_frames; }
AVSValue FrameRate(AVSValue args, void*, IScriptEnvironment* env) { const VideoInfo& vi = VI(args[0]); return float(vi.fps_numerator) / float(vi.fps_denominator); }
AVSValue AudioRate(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).audio_samples_per_second; }
AVSValue AudioLength(AVSValue args, void*, IScriptEnvironment* env) { return (int)VI(args[0]).num_audio_samples; }  // Truncated to int
AVSValue AudioChannels(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).nchannels; }
AVSValue AudioBits(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).BytesPerChannelSample()*8; }
AVSValue IsAudioFloat(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).IsSampleType(SAMPLE_FLOAT); }
AVSValue IsAudioInt(AVSValue args, void*, IScriptEnvironment* env) { return !VI(args[0]).IsSampleType(SAMPLE_FLOAT); }

AVSValue IsRGB(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).IsRGB(); }
AVSValue IsRGB24(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).IsRGB24(); }
AVSValue IsRGB32(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).IsRGB32(); }
AVSValue IsYUV(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).IsYUV(); }
AVSValue IsYUY2(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).IsYUY2(); }
AVSValue IsYV12(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).IsYV12(); }
AVSValue IsPlanar(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).IsPlanar(); }
AVSValue IsInterleaved(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).IsColorSpace(VideoInfo::CS_INTERLEAVED); }
AVSValue IsFieldBased(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).IsFieldBased(); }
AVSValue IsFrameBased(AVSValue args, void*, IScriptEnvironment* env) { return !VI(args[0]).IsFieldBased(); }
AVSValue GetParity(AVSValue args, void*, IScriptEnvironment* env) { return args[0].AsClip()->GetParity(args[1].AsInt(0)); }

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
		char *s = new char[12];
		return itoa(args[0].AsInt(), s, 10);
	  }
	  if (args[0].IsFloat()) {
		char *s = new char[30];
		sprintf(s,"%lf",args[0].AsFloat());
		return s;
	  }
  }
  return "";
} 

AVSValue IsBool(AVSValue args, void*, IScriptEnvironment* env) { return args[0].IsBool(); }
AVSValue IsInt(AVSValue args, void*, IScriptEnvironment* env) { return args[0].IsInt(); }
AVSValue IsFloat(AVSValue args, void*, IScriptEnvironment* env) { return args[0].IsFloat(); }
AVSValue IsString(AVSValue args, void*, IScriptEnvironment* env) { return args[0].IsString(); }
AVSValue IsClip(AVSValue args, void*, IScriptEnvironment* env) { return args[0].IsClip(); }
AVSValue Defined(AVSValue args, void*, IScriptEnvironment* env) { return args[0].Defined(); }

AVSValue Default(AVSValue args, void*, IScriptEnvironment* env) { return args[0].Defined() ? args[0] : args[1]; }
AVSValue VersionNumber(AVSValue args, void*, IScriptEnvironment* env) { return AVS_VERSION; }
AVSValue VersionString(AVSValue args, void*, IScriptEnvironment* env) { return AVS_VERSTR; }

AVSValue Int(AVSValue args, void*, IScriptEnvironment* env) { return int(args[0].AsFloat()); }
AVSValue Frac(AVSValue args, void*, IScriptEnvironment* env) { return args[0].AsFloat() - int(args[0].AsFloat()); }
AVSValue Float(AVSValue args, void*,IScriptEnvironment* env) { return args[0].AsFloat(); }

AVSValue Value(AVSValue args, void*, IScriptEnvironment* env) { char *stopstring; return strtod(args[0].AsString(),&stopstring); }
AVSValue HexValue(AVSValue args, void*, IScriptEnvironment* env) { char *stopstring; return strtol(args[0].AsString(),&stopstring,16); }