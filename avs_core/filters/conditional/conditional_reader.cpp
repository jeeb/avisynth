/*
  ConditionalReader  (c) 2004 by Klaus Post

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

  The author can be contacted at:
  sh0dan[at]stofanet.dk
*/

#include "conditional_reader.h"
#include "../core/internal.h"
#include "../core/parser/scriptparser.h"
#include <cstdlib>

#ifdef AVS_WINDOWS
    #include <avs/win.h>
#else
    #include <avs/posix.h>
#endif

#include <avs/minmax.h>
#include "../core/parser/scriptparser.h"
#include "../core/AVSMap.h"


/*****************************************************************************
 *  Helper code from XviD (http://www.xvid.org)
 *
 *  Copyright (C)      2002 Foxer <email?>
 *                     2002 Dirk Knop <dknop@gwdg.de>
 *                2002-2003 Edouard Gomez <ed.gomez@free.fr>
 *                     2003 Pete Ross <pross@xvid.org>
 *****************************************************************************/

/* Default buffer size for reading lines */
#define BUF_SZ   1024


/* This function returns an allocated string containing a complete line read
 * from the file starting at the current position */
static char *
readline(FILE *f)
{
	char *buffer = NULL;
	int buffer_size = 0;
	int pos = 0;

	for (;;) {
		int c;

		/* Read a character from the stream */
		c = fgetc(f);

		/* Is that EOF or new line ? */
		if(c == EOF || c == '\n')
			break;

		/* Do we have to update buffer ? */
		if(pos >= buffer_size - 1) {
			buffer_size += BUF_SZ;
			char *tmpbuffer = (char*)realloc(buffer, buffer_size);
			if (tmpbuffer == NULL) {
				free(buffer);
				return(NULL);
		    }
			buffer = tmpbuffer;
		}

		buffer[pos] = (char)c;
		pos++;
	}

	/* Read \n or EOF */
	if (buffer == NULL) {
		/* EOF, so we reached the end of the file, return NULL */
		if(feof(f))
			return(NULL);

		/* Just an empty line with just a newline, allocate a 1 byte buffer to
		 * store a zero length string */
		buffer = (char*)malloc(1);
		if(buffer == NULL)
			return(NULL);
	}

	/* Zero terminated string */
	if (pos && buffer[pos-1] == '\r')
		buffer[pos-1] = '\0';
	else
		buffer[pos] = '\0';

	return(buffer);
}

/* This function returns a pointer to the first non space char in the given
 * string or the end of the string */

static char *
skipspaces(char *string)
{
	if (string == NULL) return(NULL);

	while (*string != '\0') {
		/* Test against space chars */
		if (!isspace(*string)) return(string);
		string++;
	}
	return(string);
}

/* This function returns a pointer to the first space char in the given
 * string or the end of the string */

static char *
findspace(char *string)
{
	if (string == NULL) return(NULL);

	while (*string != '\0') {
		/* Test against space chars */
		if (isspace(*string)) return(string);
		string++;
	}
	return(string);
}

/* This function returns a boolean that tells if the string is only a
 * comment */
static int
iscomment(char *string)
{
	const char comments[] =
		{
			'#',';', '%', '\0'
		};
	const char *cmtchar = comments;
	int iscomment = 0;

	if (string == NULL) return(1);

	string = skipspaces(string);

	while(*cmtchar != '\0') {
		if(*string == *cmtchar) {
			iscomment = 1;
			break;
		}
		cmtchar++;
	}

	return(iscomment);
}


// Reader ------------------------------------------------


ConditionalReader::ConditionalReader(PClip _child, const char* filename, const char _varname[],
  bool _show, const char *_condVarSuffix, bool _local, IScriptEnvironment* env)
 : GenericVideoFilter(_child), show(_show), mode(MODE_UNKNOWN), offset(0), local(_local), stringcache(0)
{
  FILE * f;
  char *line = 0;
  int lines;

  variableName = _varname; // std::string
  if (_condVarSuffix[0])
    variableName += _condVarSuffix; // append if parameter exists
  variableNameFixed = env->SaveString(variableName.c_str());

  if ((f = fopen(filename, "rb")) == NULL)
    env->ThrowError("ConditionalReader: Could not open file '%s'.", filename);

  lines = 0;

  try {
    while ((line = readline(f)) != NULL) {
      char *ptr;
      int fields;

      lines++;

      /* We skip spaces */
      ptr = skipspaces(line);

      /* Skip coment lines or empty lines */
      if(iscomment(ptr) || *ptr == '\0') {
        free(line);
        line = 0;
        continue;
      }

      if (mode == MODE_UNKNOWN) {
        // We have not recieved a mode - We expect type.
        char* keyword = ptr;

        ptr = findspace(ptr);
        if (*ptr) {
          *ptr++ = '\0';
          if (!lstrcmpi(keyword, "type")) {
            /* We skip spaces */
            char* type = skipspaces(ptr);

            ptr = findspace(type);
            *ptr = '\0';

            if (!lstrcmpi(type, "int")) {
              mode = MODE_INT;
              intVal = new int[vi.num_frames];
            } else if (!lstrcmpi(type, "float")) {
              mode = MODE_FLOAT;
              floatVal = new float[vi.num_frames];
            } else if (!lstrcmpi(type, "bool")) {
              mode = MODE_BOOL;
              boolVal = new bool[vi.num_frames];
            } else if (!lstrcmpi(type, "string")) {
              mode = MODE_STRING;
              stringVal = new const char*[vi.num_frames];
            } else {
              ThrowLine("ConditionalReader: Unknown 'Type' specified in line %d", lines, env);
            }// end if compare type
            SetRange(0, vi.num_frames-1, AVSValue());
          }// end if compare keyword
        }// end if fields

      } else { // We have a defined mode and allocated the values.

        char* keyword = ptr;
        char* type = findspace(keyword);

        if (*type) *type++ = '\0';

        if (!lstrcmpi(keyword, "default")) {
          AVSValue def = ConvertType(type, lines, env);
          SetRange(0, vi.num_frames-1, def);

        } else if (!lstrcmpi(keyword, "offset")) {
          fields = sscanf(type, "%d", &offset);
          if (fields != 1)
            ThrowLine("ConditionalReader: Could not read Offset in line %d", lines, env);

        } else if (keyword[0] == 'R' || keyword[0] == 'r') {  // Range
          int start;
          int stop;

          type = skipspaces(type);
          fields = sscanf(type, "%d", &start);

          type = findspace(type);
          type = skipspaces(type);
          fields += sscanf(type, "%d", &stop);

          type = findspace(type);
          if (!*type || fields != 2)
            ThrowLine("ConditionalReader: Could not read Range in line %d", lines, env);

          if (start > stop)
            ThrowLine("ConditionalReader: The Range start frame is after the end frame in line %d", lines, env);

          AVSValue set = ConvertType(type+1, lines, env);
          SetRange(start, stop, set);

        } else if (keyword[0] == 'I' || keyword[0] == 'i') {  // Interpolate
          if (mode == MODE_BOOL)
            ThrowLine("ConditionalReader: Cannot Interpolate booleans in line %d", lines, env);

          if (mode == MODE_STRING)
            ThrowLine("ConditionalReader: Cannot Interpolate strings in line %d", lines, env);

          type = skipspaces(type);
          int start;
          int stop;
          char start_value[64];
          char stop_value[64];
          fields = sscanf(type, "%d %d %63s %63s", &start, &stop, start_value, stop_value);

          if (fields != 4)
            ThrowLine("ConditionalReader: Could not read Interpolation range in line %d", lines, env);
          if (start > stop)
            ThrowLine("ConditionalReader: The Interpolation start frame is after the end frame in line %d", lines, env);

          start_value[63] = '\0';
          AVSValue set_start = ConvertType(start_value, lines, env);

          stop_value[63] = '\0';
          AVSValue set_stop = ConvertType(stop_value, lines, env);

          const int range = stop-start;
          const double diff = (set_stop.AsFloat() - set_start.AsFloat()) / range;
          for (int i = 0; i<=range; i++) {
            const double n = i * diff + set_start.AsFloat();
            SetFrame(i+start, (mode == MODE_FLOAT)
                    ? AVSValue(n)
                    : AVSValue((int)(n+0.5)));
          }
        } else {
          int cframe;
          fields = sscanf(keyword, "%d", &cframe);
          if ((*type || mode == MODE_STRING) && fields == 1) { // allow empty string
            AVSValue set = ConvertType(type, lines, env);
            SetFrame(cframe, set);
          } else {
            ThrowLine("ConditionalReader: Do not understand line %d", lines, env);
          }
        }

      } // End we have defined type
      free(line);
      line = 0;
    }// end while still some file left to read.
  }
  catch (...) {
    free(line);
    fclose(f);
    CleanUp();
    throw;
  }

  /* We are done with the file */
  fclose(f);

  if (mode == MODE_UNKNOWN)
    env->ThrowError("ConditionalReader: Type was not defined!");

}



// Converts from the char array given to the type specified.

AVSValue ConditionalReader::ConvertType(const char* content, int line, IScriptEnvironment* env)
{
  if (mode == MODE_UNKNOWN)
    ThrowLine("ConditionalReader: Type has not been defined. Line %d", line, env);

  int fields;
  switch (mode) {
    case MODE_INT:
      int ival;
      fields = sscanf(content, "%d", &ival);
      if (fields != 1)
        ThrowLine("ConditionalReader: Could not find an expected integer at line %d!", line, env);

      return AVSValue(ival);

    case MODE_FLOAT:
      float fval;
      fields = sscanf(content, "%e", &fval);
      if (fields != 1)
        ThrowLine("ConditionalReader: Could not find an expected float at line %d!", line, env);

      return AVSValue(fval);

    case MODE_BOOL:
      char bval[8];
      bval[0] = '\0';
      fields = sscanf(content, "%7s", bval);
      bval[7] = '\0';
      if (!lstrcmpi(bval, "true")) {
        return AVSValue(true);
      }
      else if (!lstrcmpi(bval, "t")) {
        return AVSValue(true);
      }
      else if (!lstrcmpi(bval, "yes")) {
        return AVSValue(true);
      }
      else if (!lstrcmp(bval, "1")) {
        return AVSValue(true);
      }
      else if (!lstrcmpi(bval, "false")) {
        return AVSValue(false);
      }
      else if (!lstrcmpi(bval, "f")) {
        return AVSValue(false);
      }
      else if (!lstrcmpi(bval, "no")) {
        return AVSValue(false);
      }
      else if (!lstrcmp(bval, "0")) {
        return AVSValue(false);
      }
      ThrowLine("ConditionalReader: Boolean value was not true or false in line %d", line, env);

    case MODE_STRING:
      StringCache *str;

      // Look for an existing duplicate
      for (str = stringcache; str; str = str->next ) {
        if (!lstrcmp(str->string, content)) break;
      }
      // Could not find one, add it
      if (!str) {
        str = new StringCache;
        str->string = _strdup(content);
        str->next   = stringcache;
        stringcache = str;
      }
      return AVSValue(str->string);
  }
  return AVSValue();
}


// Sets range with both start and stopframe inclusive.

void ConditionalReader::SetRange(int start_frame, int stop_frame, AVSValue v) {
  int i;
  start_frame = max(start_frame+offset, 0);
  stop_frame = min(stop_frame+offset, vi.num_frames-1);
  int p;
  float q;
  bool r;
  const char* s;

  switch (mode) {
    case MODE_INT:
      p = v.Defined() ? v.AsInt() : 0;
      for (i = start_frame; i <= stop_frame; i++) {
        intVal[i] = p;
      }
      break;
    case MODE_FLOAT:
      q = v.Defined() ? v.AsFloatf() : 0.0f;
      for (i = start_frame; i <= stop_frame; i++) {
        floatVal[i] = q;
      }
      break;
    case MODE_BOOL:
      r = v.Defined() ? v.AsBool() : false;
      for (i = start_frame; i <= stop_frame; i++) {
        boolVal[i] = r;
      }
      break;
    case MODE_STRING:
      s = v.AsString("");
      for (i = start_frame; i <= stop_frame; i++) {
        stringVal[i] = s;
      }
      break;
  }
}

// Sets the value of one frame.

void ConditionalReader::SetFrame(int framenumber, AVSValue v) {

  if ((framenumber+offset) < 0 || (framenumber+offset) > vi.num_frames-1 )
    return;

  switch (mode) {
    case MODE_INT:
      intVal[framenumber+offset] = v.AsInt();
      break;
    case MODE_FLOAT:
      floatVal[framenumber+offset] = v.AsFloatf();
      break;
    case MODE_BOOL:
      boolVal[framenumber+offset] = v.AsBool();
      break;
    case MODE_STRING:
      stringVal[framenumber+offset] = v.AsString("");
      break;
  }
}

// Get the value of a frame.
AVSValue ConditionalReader::GetFrameValue(int framenumber) {
  framenumber = clamp(framenumber, 0, vi.num_frames-1);

  switch (mode) {
    case MODE_INT:
      return AVSValue(intVal[framenumber]);

    case MODE_FLOAT:
      return AVSValue(floatVal[framenumber]);

    case MODE_BOOL:
      return AVSValue(boolVal[framenumber]);

    case MODE_STRING:
      return AVSValue(stringVal[framenumber]);

  }
  return AVSValue(0);
}

// Destructor
ConditionalReader::~ConditionalReader(void)
{
  CleanUp();
}


void ConditionalReader::CleanUp(void)
{
  switch (mode) {
    case MODE_INT:
      delete[] intVal;
      break;
    case MODE_FLOAT:
      delete[] floatVal;
      break;
    case MODE_BOOL:
      delete[] boolVal;
      break;
    case MODE_STRING:
      delete[] stringVal;

      //free the cached strings
      for (StringCache* str = stringcache; str; ) {
        StringCache* curr = str;
        free(str->string);
        str = str->next;
        delete curr;
      }
      stringcache = 0;

      break;
  }
  mode = MODE_UNKNOWN;
}


void ConditionalReader::ThrowLine(const char* err, int line, IScriptEnvironment* env) {
  env->ThrowError(err, line);
}


PVideoFrame __stdcall ConditionalReader::GetFrame(int n, IScriptEnvironment* env)
{
  AVSValue v = GetFrameValue(n);

  InternalEnvironment* envI = static_cast<InternalEnvironment*>(env);

  std::unique_ptr<GlobalVarFrame> var_frame;

  AVSValue child_val = child;

  if (!local) {
    env->SetGlobalVar(variableNameFixed, v);
  }
  else {
    // Neo's default, correct but incompatible with previous Avisynth versions
    var_frame = std::unique_ptr<GlobalVarFrame>(new GlobalVarFrame(envI)); // allocate new frame
    env->SetGlobalVar(variableNameFixed, v);
  }


  PVideoFrame src = child->GetFrame(n,env);

  if (show) {
    AVSValue v2 = env->Invoke("String", v);
    env->MakeWritable(&src);
    env->ApplyMessage(&src, vi, v2.AsString(""), vi.width/2, 0xa0a0a0, 0, 0);
  }
  return src;
}

int __stdcall ConditionalReader::SetCacheHints(int cachehints, int frame_range)
{
  switch (cachehints)
  {
  case CACHE_GET_MTMODE:
    return MT_NICE_FILTER;
  }
  return 0;  // We do not pass cache requests upwards.
}



AVSValue __cdecl ConditionalReader::Create(AVSValue args, void*, IScriptEnvironment* env)
{
  const bool runtime_local_default = false; // Avisynth compatibility: false, Neo: true.

  return new ConditionalReader(args[0].AsClip(), args[1].AsString(""), args[2].AsString("Conditional") , args[3].AsBool(false), args[4].AsString(""), args[5].AsBool(runtime_local_default), env);
}


// Write ------------------------------------------------


static const char EMPTY[]  = "";
static const char AplusT[] = "a+t";
static const char WplusT[] = "w+t";


Write::Write(PClip _child, const char* _filename, AVSValue args, int _linecheck, bool _append, bool _flush, bool _local, IScriptEnvironment* env) :
  GenericVideoFilter(_child), linecheck(_linecheck), flush(_flush), append(_append), local(_local), arglist(0)
{
#ifdef AVS_WINDOWS
  _fullpath(filename, _filename, _MAX_PATH);
#else
  realpath(_filename, filename);
#endif

  fout = fopen(filename, append ? AplusT : WplusT);	//append or purge file
  if (!fout) env->ThrowError("Write: File '%s' cannot be opened.", filename);

  if (flush) fclose(fout);	//will be reopened in FileOut

  arrsize = args.ArraySize();

  arglist = new exp_res[arrsize];

  for (int i = 0; i < arrsize; i++) {
    arglist[i].expression = args[i];
    arglist[i].string = EMPTY;
  }

  if (linecheck != -1 && linecheck != -2)
    return;

  InternalEnvironment* envI = static_cast<InternalEnvironment*>(env);

  AVSValue prev_last;
  AVSValue prev_current_frame;
  std::unique_ptr<GlobalVarFrame> var_frame;

  AVSValue child_val = child;

  if (!local) {
    prev_last = env->GetVarDef("last");  // Store previous last
    prev_current_frame = env->GetVarDef("current_frame");  // Store previous current_frame
    env->SetVar("last", child_val);       // Set implicit last
    env->SetVar("current_frame", (AVSValue)linecheck);  // special -1 or -2
  }
  else {
    // Neo's default, correct but incompatible with previous Avisynth versions
    var_frame = std::unique_ptr<GlobalVarFrame>(new GlobalVarFrame(envI)); // allocate new frame
    env->SetGlobalVar("last", child_val);       // Set explicit last
    env->SetGlobalVar("current_frame", (AVSValue)linecheck);  // special -1 or -2
  }

  Write::DoEval(env); // at both write at start and write at end

  if (linecheck == -1) { //write at start
    Write::FileOut(env, AplusT);
  }

  if (!local) {
    env->SetVar("last", prev_last);       // Restore implicit last
    env->SetVar("current_frame", prev_current_frame);       // Restore current_frame
  }
}

PVideoFrame __stdcall Write::GetFrame(int n, IScriptEnvironment* env) {

  //changed to call write AFTER the child->GetFrame


  PVideoFrame tmpframe = child->GetFrame(n, env);

  if (linecheck < 0) return tmpframe;	//do nothing here when writing only start or end

  InternalEnvironment* envI = static_cast<InternalEnvironment*>(env);

  AVSValue prev_last;
  AVSValue prev_current_frame;
  std::unique_ptr<GlobalVarFrame> var_frame;

  AVSValue child_val = child;

  if (!local) {
    prev_last = env->GetVarDef("last");  // Store previous last
    prev_current_frame = env->GetVarDef("current_frame");  // Store previous current_frame
    env->SetVar("last", (AVSValue)child_val);       // Set implicit last
    env->SetVar("current_frame", (AVSValue)n);  // Set frame to be tested by the conditional filters.
  }
  else {
    // Neo's default, correct but incompatible with previous Avisynth versions
    var_frame = std::unique_ptr<GlobalVarFrame>(new GlobalVarFrame(envI)); // allocate new frame
    env->SetGlobalVar("last", child_val);       // Set implicit last (to avoid recursive stack calls?)
    env->SetGlobalVar("current_frame", (AVSValue)n);  // Set frame to be tested by the conditional filters.
  }

  if (Write::DoEval(env)) {
    Write::FileOut(env, AplusT);
  }

  if (!local) {
    env->SetVar("last", prev_last);       // Restore implicit last
    env->SetVar("current_frame", prev_current_frame);       // Restore current_frame
  }

  return tmpframe;

};

Write::~Write(void) {
	if (linecheck == -2) {	//write at end
		Write::FileOut(0, append ? AplusT : WplusT); // Allow for retruncating at actual end
	}
	if (!flush) fclose(fout);

	delete[] arglist;
};

void Write::FileOut(IScriptEnvironment* env, const char* mode) {
	int i;
	if (flush) {
		fout = fopen(filename, mode);
		if (!fout) {
			if (env) env->ThrowError("Write: File '%s' cannot be opened.", filename);
			return;
		}
	}
	for (i= ( (linecheck==1) ? 1 : 0) ; i<arrsize; i++ ) {
		fputs(arglist[i].string, fout);
	}
	fputs("\n", fout);
	if (flush) {
		fclose(fout);
	}
}

bool Write::DoEval( IScriptEnvironment* env_) {
	bool keep_this_line = true;
	int i;
	AVSValue expr;
	AVSValue result;
  InternalEnvironment* env = static_cast<InternalEnvironment*>(env_);

	for (i=0; i<arrsize; i++) {
		expr = arglist[i].expression;

		if ( (linecheck==1) && (i==0)) {
			try {
        if (expr.IsFunction()) {
          result = env->Invoke(child, expr.AsFunction(), AVSValue(nullptr, 0));
        }
        else {
          expr = expr.AsString(EMPTY);
          result = env->Invoke("Eval", expr);
        }
				if (!result.AsBool(true)) {
					keep_this_line = false;
					break;
				}
			} catch (const AvisynthError&) {
//				env->ThrowError("Write: Can't eval linecheck expression!"); // results in KEEPING the line
			}
		} else {
			try {
        if (expr.IsFunction()) {
          result = env->Invoke(child, expr.AsFunction(), AVSValue(nullptr, 0));
        }
        else {
          expr = expr.AsString(EMPTY);
          result = env->Invoke("Eval", expr);
        }
				result = env->Invoke("string",result);	//convert all results to a string
				arglist[i].string = result.AsString(EMPTY);
			} catch (const AvisynthError &error) {
				arglist[i].string = env->SaveString(error.msg);
			}
		}
	}
	return keep_this_line;
}

int __stdcall Write::SetCacheHints(int cachehints, int frame_range)
{
  switch (cachehints)
  {
  case CACHE_GET_MTMODE:
    return MT_SERIALIZED;
  }
  return 0;  // We do not pass cache requests upwards.
}

AVSValue __cdecl Write::Create(AVSValue args, void*, IScriptEnvironment* env)
{
  bool runtime_local_default = false;
  // Param 2: string/function or array of strings/functions
  if (args[2].IsFunction() || (args[2].IsArray() && args[2].ArraySize() > 0 && args[2][0].IsFunction()))
    runtime_local_default = true;
  // Avisynth compatibility: false, Neo: true. functions are legacy Neo

	return new Write(args[0].AsClip(), args[1].AsString(EMPTY), args[2], 0, args[3].AsBool(true),args[4].AsBool(true), args[5].AsBool(runtime_local_default), env);
}

AVSValue __cdecl Write::Create_If(AVSValue args, void*, IScriptEnvironment* env)
{
  bool runtime_local_default = false;
  // Param 2: string/function or array of strings/functions
  if (args[2].IsFunction() || (args[2].IsArray() && args[2].ArraySize() > 0 && args[2][0].IsFunction()))
    runtime_local_default = true;
  // Avisynth compatibility: false, Neo: true. functions are legacy Neo

	return new Write(args[0].AsClip(), args[1].AsString(EMPTY), args[2], 1, args[3].AsBool(true),args[4].AsBool(true), args[5].AsBool(runtime_local_default), env);
}

AVSValue __cdecl Write::Create_Start(AVSValue args, void*, IScriptEnvironment* env)
{
  bool runtime_local_default = false;
  // Param 2: string/function or array of strings/functions
  /*if (args[2].IsFunction() || (args[2].IsArray() && args[2].ArraySize() > 0 && args[2][0].IsFunction()))
    runtime_local_default = true;
  */
  // Avisynth compatibility: false, Neo: also false as of 2020.03.28

	return new Write(args[0].AsClip(), args[1].AsString(EMPTY), args[2], -1, args[3].AsBool(false), true, args[4].AsBool(runtime_local_default), env);
}

AVSValue __cdecl Write::Create_End(AVSValue args, void*, IScriptEnvironment* env)
{
  bool runtime_local_default = false;
  // Param 2: string/function or array of strings/functions
  /*
  if (args[2].IsFunction() || (args[2].IsArray() && args[2].ArraySize() > 0 && args[2][0].IsFunction()))
    runtime_local_default = true;
  */
  // Avisynth compatibility: false, Neo: also false as of 2020.03.28

	return new Write(args[0].AsClip(), args[1].AsString(EMPTY), args[2], -2, args[3].AsBool(true), args[4].AsBool(runtime_local_default), true, env);
}


UseVar::UseVar(PClip _child, AVSValue vars, IScriptEnvironment* env)
   : GenericVideoFilter(_child)
{
  IScriptEnvironment2* env2 = static_cast<IScriptEnvironment2*>(env);

   vars_.resize(vars.ArraySize());
   for (int i = 0; i < vars.ArraySize(); ++i) {
      auto name = vars_[i].name = vars[i].AsString();
      if (!env2->GetVar(name, &vars_[i].val)) {
        env->ThrowError("UseVar: No variable named %s", name);
      }
   }
}

UseVar::~UseVar() { }

PVideoFrame __stdcall UseVar::GetFrame(int n, IScriptEnvironment* env)
{
   GlobalVarFrame var_frame(static_cast<InternalEnvironment*>(env)); // allocate new frame

   // set variables
   for (int i = 0; i < (int)vars_.size(); ++i) {
      env->SetGlobalVar(vars_[i].name, vars_[i].val);
   }

   return child->GetFrame(n, env);
}

int __stdcall UseVar::SetCacheHints(int cachehints, int frame_range) {
  switch (cachehints)
  {
  case CACHE_GET_MTMODE:
    return MT_NICE_FILTER;
  /*case CACHE_GET_DEV_TYPE:
    return (child->GetVersion() >= 5) ? child->SetCacheHints(CACHE_GET_DEV_TYPE, 0) : 0;
  */
  }
  return 0;  // We do not pass cache requests upwards.
}

AVSValue __cdecl UseVar::Create(AVSValue args, void* user_data, IScriptEnvironment* env)
{
   return new UseVar(args[0].AsClip(), args[1], env);
}

#define W_DIVISOR 5  // Width divisor for onscreen messages


// Avisynth+ frame property support
//**************************************************
// propSet, propSetInt, propSetFloat, propSetString

SetProperty::SetProperty(PClip _child, const char* name, const PFunction& func, const int kind,
  const int mode, IScriptEnvironment* env)
  : GenericVideoFilter(_child)
  , name(name)
  , func(func)
  , kind(kind)
  , append_mode(mode)
{ }

SetProperty::~SetProperty() { }

PVideoFrame __stdcall SetProperty::GetFrame(int n, IScriptEnvironment* env)
{
  GlobalVarFrame var_frame(static_cast<InternalEnvironment*>(env)); // allocate new frame
  env->SetGlobalVar("last", (AVSValue)child);       // Set implicit last
  env->SetGlobalVar("current_frame", (AVSValue)n);  // Set frame to be tested

  AVSValue result;
  const char* error_msg = nullptr;
  try {
    const AVSValue empty_args_array = AVSValue(nullptr, 0); // invoke's parameter is const AVSValue&, don't do it inline.
    result = static_cast<InternalEnvironment*>(env)->Invoke(child, func, empty_args_array);
  }
  catch (IScriptEnvironment::NotFound) {
    error_msg = env->Sprintf("AddProperties: Invalid function parameter type '%s'(%s)\n"
      "Function should have no argument",
      func->GetDefinition()->param_types, func->ToString(env));
  }
  catch (const AvisynthError& error) {
    error_msg = env->Sprintf("%s\nAddProperties: Error in %s",
      error.msg, func->ToString(env));
  }

  PVideoFrame frame = child->GetFrame(n, env);

  if (error_msg) {
    env->MakeWritable(&frame);
    env->ApplyMessage(&frame, vi, error_msg, vi.width / W_DIVISOR, 0xa0a0a0, 0, 0);
    return frame;
  }

/*
  usage:
   ScriptClip("""propSetInt("frameluma",func(AverageLuma))""")
   ScriptClip("""SubTitle(string(propGetInt("frameluma")))""")
  or
   ps = func(propSetterFunc) # make function object from function
   ScriptClip(ps) # pass function object to scriptclip
   ScriptClip(function[](clip c) { SubTitle(string(propGetInt("frameprop_demo")), y=20) })
   ScriptClip(function[](clip c) { SubTitle(string(propGetInt("frameprop_demo2")), y=40) })
   function propSetterFunc(Clip x) {
    x
    propSetInt("frameprop_demo", func(AverageLuma))
    propSetInt("frameprop_demo2", function[]() { current_frame })
  }
*/

  env->MakeWritable(&frame);

  AVSMap* avsmap = env->getFramePropsRW(frame);

  int propType = kind;
  // vUnset, vInt, vFloat, vData/*, vNode*/, vFrame/*, vMethod*/ }
  // 0: auto
  // 1: integer
  // 2: float
  // 3: char (null terminated data)

  try {
    // check auto
    if (propType == 0) {
      // 'u'nset, 'i'nteger, 'f'loat, 's'string, 'c'lip, 'v'ideoframe, 'm'ethod };
      if (result.IsInt())
        propType = 1;
      else if (result.IsFloat())
        propType = 2;
      else if (result.IsString())
        propType = 3;
      else if (result.IsArray())
        propType = 4;
      else if (result.IsClip())
        env->ThrowError("Clip frame properties not yet supported");
      else
        env->ThrowError("Invalid return type (Was a %s)", GetAVSTypeName(result));
    }

    int res = 0;

    // special case: zero sized array -> entry deleted
    if (result.IsArray() && result.ArraySize() == 0)
      res = env->propDeleteKey(avsmap, name); // 0 is success
    else if (propType == 1 && result.IsInt())
      res = env->propSetInt(avsmap, name, result.AsInt(), append_mode);
    else if (propType == 2 && result.IsFloat())
      res = env->propSetFloat(avsmap, name, result.AsFloat(), append_mode);
    else if (propType == 3 && result.IsString())
    {
      const char* s = result.AsString(); // no need for SaveString, it has its own storage
      res = env->propSetData(avsmap, name, s, -1, append_mode); // -1: auto string length
    }
    else if (propType == 4 && result[0].IsInt())
    {
      int size = result.ArraySize();
      std::vector<int64_t> int64array(size); // avs can do int only, temporary array needed
      for (int i = 0; i < size; i++)
        int64array[i] = result[i].AsInt(); // all elements should be int
      res = env->propSetIntArray(avsmap, name, int64array.data(), size);
    }
    else if (propType == 4 && result[0].IsFloat())
    {
      int size = result.ArraySize();
      std::vector<double> d_array(size); // avs can do float only, temporary array needed
      for (int i = 0; i < size; i++)
        d_array[i] = result[i].AsFloat(); // all elements should be float or int
      res = env->propSetFloatArray(avsmap, name, d_array.data(), size);
    }
    else if (propType == 4 && result[0].IsString())
    {
      const int size = result.ArraySize();
      // no such api like propSetDataArray
      env->propDeleteKey(avsmap, name);
      for (int i = 0; i < size; i++) {
        res = env->propSetData(avsmap, name, result[i].AsString(), -1, AVSPropAppendMode::paAppend); // all elements should be string
        if (res)
          break;
      }
    }
    else
    {
      env->ThrowError("Wrong data type, property '%s' type is not %s", name, GetAVSTypeName(result)); // fixme: res reasons
    }

    if (res)
      env->ThrowError("error setting property '%s', error = %d", name, res); // fixme: res reasons
  }
  catch (const AvisynthError& error) {
    error_msg = env->Sprintf("propAdd: %s", error.msg);
  }

  if (error_msg) {
    env->MakeWritable(&frame);
    env->ApplyMessage(&frame, vi, error_msg, vi.width / W_DIVISOR, 0xa0a0a0, 0, 0);
  }

  return frame;
}

int __stdcall SetProperty::SetCacheHints(int cachehints, int frame_range)
{
  AVS_UNUSED(frame_range);
  switch (cachehints)
  {
  case CACHE_GET_MTMODE:
    return MT_NICE_FILTER;
  }
  return 0;  // We do not pass cache requests upwards.
}

AVSValue __cdecl SetProperty::Create(AVSValue args, void* user_data, IScriptEnvironment* env)
{
  const int kind = (int)(intptr_t)user_data;
  const int defaultMode = (int)AVSPropAppendMode::paReplace;

  int mode = paReplace;
  if(kind != 4) // at propSetArray there is no mode parameter
    mode = args[3].AsInt(defaultMode);

  /*
    paReplace = 0,
    paAppend = 1,
    paTouch = 2
  */
  return new SetProperty(args[0].AsClip(), args[1].AsString(), args[2].AsFunction(), kind, mode, env);
}

//**************************************************
// propDelete

DeleteProperty::DeleteProperty(PClip _child, const char* name, IScriptEnvironment* env)
  : GenericVideoFilter(_child)
  , name(name)
{ }

DeleteProperty::~DeleteProperty() { }

PVideoFrame __stdcall DeleteProperty::GetFrame(int n, IScriptEnvironment* env)
{
  GlobalVarFrame var_frame(static_cast<InternalEnvironment*>(env)); // allocate new frame
  env->SetGlobalVar("last", (AVSValue)child);       // Set implicit last
  env->SetGlobalVar("current_frame", (AVSValue)n);  // Set frame to be tested

  PVideoFrame frame = child->GetFrame(n, env);

  /*
    usage:
      ScriptClip("""propDelete("frameluma")""")
  */

  env->MakeWritable(&frame);

  AVSMap* avsmap = env->getFramePropsRW(frame);
  int res = env->propDeleteKey(avsmap, name); // 0 is success

  if (!res) {
    const char *error_msg = env->Sprintf("propDelete: error deleting property '%s'", name);
    env->ApplyMessage(&frame, vi, error_msg, vi.width / W_DIVISOR, 0xa0a0a0, 0, 0);
  }

  return frame;
}

int __stdcall DeleteProperty::SetCacheHints(int cachehints, int frame_range)
{
  AVS_UNUSED(frame_range);
  switch (cachehints)
  {
  case CACHE_GET_MTMODE:
    return MT_NICE_FILTER;
  }
  return 0;  // We do not pass cache requests upwards.
}

AVSValue __cdecl DeleteProperty::Create(AVSValue args, void*, IScriptEnvironment* env)
{
  return new DeleteProperty(args[0].AsClip(), args[1].AsString(), env);
}

//**************************************************
// propDelete

ClearProperties::ClearProperties(PClip _child, IScriptEnvironment* env)
  : GenericVideoFilter(_child)
{ }

ClearProperties::~ClearProperties() { }

PVideoFrame __stdcall ClearProperties::GetFrame(int n, IScriptEnvironment* env)
{
  GlobalVarFrame var_frame(static_cast<InternalEnvironment*>(env)); // allocate new frame
  env->SetGlobalVar("last", (AVSValue)child);       // Set implicit last
  env->SetGlobalVar("current_frame", (AVSValue)n);  // Set frame to be tested

  PVideoFrame frame = child->GetFrame(n, env);

  /*
    usage:
      ScriptClip("""propClear()""")
  */

  env->MakeWritable(&frame);

  AVSMap* avsmap = env->getFramePropsRW(frame);
  env->clearMap(avsmap);

  return frame;
}

int __stdcall ClearProperties::SetCacheHints(int cachehints, int frame_range)
{
  AVS_UNUSED(frame_range);
  switch (cachehints)
  {
  case CACHE_GET_MTMODE:
    return MT_NICE_FILTER;
  }
  return 0;  // We do not pass cache requests upwards.
}

AVSValue __cdecl ClearProperties::Create(AVSValue args, void*, IScriptEnvironment* env)
{
  return new ClearProperties(args[0].AsClip(), env);
}

