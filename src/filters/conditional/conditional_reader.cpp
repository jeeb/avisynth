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

#include "stdafx.h"

#include "conditional_reader.h"


ConditionalReader::ConditionalReader(PClip _child, const char* filename, const char* _varname, bool _show, IScriptEnvironment* _env) :
  GenericVideoFilter(_child), variableName(_varname), show(_show), env(_env)
{
	FILE * f;
	char *line;
	int lines;

  if ((f = fopen(filename, "rb")) == NULL)
		env->ThrowError("ConditionalReader: Could not open file.");

  lines = 0;
  mode = MODE_UNKNOWN;

	while ((line = readline(f)) != NULL) {
		char *ptr;
		int fields;

		lines++;

		/* We skip spaces */
		ptr = skipspaces(line);

    /* Skip coment lines or empty lines */
		if(iscomment(ptr) || *ptr == '\0') {
			free(line);
			continue;
		}

    if (mode == MODE_UNKNOWN) {
      // We have not recieved a mode - We expect type.
      char* keyword [1024];
      char* type [1024];
      fields = sscanf(ptr,"%1024s %1024s", keyword, type);
      if (fields) {
        if (!lstrcmpi((const char*)keyword, "type")) {
          if (!lstrcmpi((const char*)type, "int")) {
            mode = MODE_INT;
            intVal = new int[vi.num_frames];
          } else if (!lstrcmpi((const char*)type, "float")) {
            mode = MODE_FLOAT;
            floatVal = new float[vi.num_frames];
          } else if (!lstrcmpi((const char*)type, "bool")) {
            mode = MODE_BOOL;
            boolVal = new bool[vi.num_frames];
          } else {
            ThrowLine("ConditionalReader: Unknown 'type' specified in line %d", lines);
          }// end if compare type
        }// end if compare keyword
      }// end if fields

    } else { // We have a defined mode and allocated the values.

      char* keyword [1024];
      char* type [1024];
      fields = sscanf(ptr,"%1024s %1024s", keyword, type);

      if (!lstrcmpi((const char*)keyword, "default")) {
        AVSValue def = ConvertType((const char*)type, lines);
        SetRange(0, vi.num_frames-1, def);
			  free(line);
        continue;
      } // end if "default"

      if (ptr[0] == 'R' || ptr[0] == 'r') {  // Range
        ptr++;
		    ptr = skipspaces(ptr);
        int start;
        int stop;
        char* value [64];
        fields = sscanf(ptr, "%d %d %64s", &start, &stop, value);

        if (fields != 3) 
          ThrowLine("ConditionalReader: Could not read range in line %d", lines);

        AVSValue set = ConvertType((const char*)value, lines);
        SetRange(start, stop, set);
      } else {
        char* value [64];
        int cframe;
        fields = sscanf(ptr, "%d %64s", &cframe, value);
        if (fields == 2) {
          AVSValue set = ConvertType((const char*)value, lines);
          SetFrame(cframe, set);
        } else {
          _RPT1(0,"ConditionalReader: Ignored line %d.\n", lines);
        }
      }
    
    } // End we have defined type
  	free(line);
  }// end while still some file left to read.

	/* We are done with the file */
	fclose(f);

  if (mode == MODE_UNKNOWN)
    env->ThrowError("ConditionalReader: Mode was not defined!");

}



// Converts from the char array given to the type specified.

AVSValue ConditionalReader::ConvertType(const char* content, int line)
{
  if (mode == MODE_UNKNOWN)
    ThrowLine("ConditionalReader: Type has not been defined. Line %d", line);

  int fields;
  switch (mode) {
    case MODE_INT:
      int ival;
      fields = sscanf(content, "%d", &ival);
      if (!fields)
        ThrowLine("ConditionalReader: Could not find an expected integer at line %d!", line);

      return AVSValue(ival);

    case MODE_FLOAT:
      float fval;
      fields = sscanf(content, "%e", &fval);
      if (!fields)
        ThrowLine("ConditionalReader: Could not find an expected float at line %d!", line);
      return AVSValue(fval);

    case MODE_BOOL:
      char* bval [6];
      fields = sscanf(content, "%6s", bval);
      if (!lstrcmpi((const char*)bval, "true")) {
        return AVSValue(true);
      } else if (!lstrcmpi((const char*)bval, "false")) {
        return AVSValue(false);
      } 
      ThrowLine("ConditionalReader: Boolean value was not true or false in line %d", line);
  }
  return AVSValue(0);
}


// Sets range with both start and stopframe inclusive.

void ConditionalReader::SetRange(int start_frame, int stop_frame, AVSValue v) {
  int i;
  start_frame = max(min(start_frame, vi.num_frames-1), 0);
  stop_frame = max(min(stop_frame, vi.num_frames-1), 0);
  int p;
  float q;
  bool r;

  switch (mode) {
    case MODE_INT:
      p = v.AsInt();
      for (i = start_frame; i <= stop_frame; i++) {
        intVal[i] = p;
      }
      break;
    case MODE_FLOAT:
      q = v.AsFloat();
      for (i = start_frame; i <= stop_frame; i++) {
        floatVal[i] = q;
      }
      break;
    case MODE_BOOL:
      r = v.AsBool();
      for (i = start_frame; i <= stop_frame; i++) {
        boolVal[i] = r;
      }
      break;
  }
}

// Sets the value of one frame.

void ConditionalReader::SetFrame(int framenumber, AVSValue v) {
  framenumber = max(min(framenumber, vi.num_frames-1), 0);

  switch (mode) {
    case MODE_INT:
      intVal[framenumber] = v.AsInt();
      break;
    case MODE_FLOAT:
      floatVal[framenumber] = v.AsFloat();
      break;
    case MODE_BOOL:
      boolVal[framenumber] = v.AsBool();
      break;
  }
}

// Get the value of a frame.
AVSValue ConditionalReader::GetFrameValue(int framenumber) {
  framenumber = max(min(framenumber, vi.num_frames-1), 0);

  switch (mode) {
    case MODE_INT:
      return AVSValue(intVal[framenumber]);
      break;
    case MODE_FLOAT:
      return AVSValue(floatVal[framenumber]);
      break;
    case MODE_BOOL:
      return AVSValue(boolVal[framenumber]);
      break;
  }
  return AVSValue(0);
}

// Destructor
ConditionalReader::~ConditionalReader(void) 
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
  }
}


void ConditionalReader::ThrowLine(const char* err, int line) {
  char* error = (char*)malloc(strlen(err)+16);
  sprintf(error, err, line);
  env->ThrowError(error);
}


PVideoFrame __stdcall ConditionalReader::GetFrame(int n, IScriptEnvironment* env)
{
  env->SetGlobalVar(variableName, GetFrameValue(n));
  PVideoFrame src = child->GetFrame(n,env);
  return src;
}




AVSValue __cdecl ConditionalReader::Create(AVSValue args, void* user_data, IScriptEnvironment* env)
{
  return new ConditionalReader(args[0].AsClip(), args[1].AsString(""), args[2].AsString("Conditional") , args[3].AsBool(true), env);
}


