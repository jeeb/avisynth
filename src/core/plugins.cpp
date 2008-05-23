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

#include "Error.h"    // which includes "internal.h"

const char* loadplugin_prefix = NULL;

/********************************************************************
* Native plugin support
********************************************************************/

enum { max_plugins=50 };

void FreeLibraries(void* loaded_plugins, IScriptEnvironment* env) {
  for (int i=0; i<max_plugins; ++i) {
    HMODULE plugin = ((HMODULE*)loaded_plugins)[i];
    if (plugin)
      FreeLibrary(plugin);
    else
      break;
  }
  memset(loaded_plugins, 0, max_plugins*sizeof(HMODULE));
}

static bool MyLoadLibrary(const char* filename, HMODULE* hmod, bool quiet, IScriptEnvironment* env) {
  HMODULE* loaded_plugins;
  try {
    loaded_plugins = (HMODULE*)env->GetVar("$Plugins$").AsString();
  }  // Tritical May 2005
  catch (IScriptEnvironment::NotFound) {
    HMODULE plugins[max_plugins]; // buffer to clone on stack

    memset(plugins, 0, max_plugins*sizeof(HMODULE));
    // Cheat and copy into SaveString buffer
    env->SetGlobalVar("$Plugins$", env->SaveString((const char*)plugins, max_plugins*sizeof(HMODULE)));
    try {
        loaded_plugins = (HMODULE*)env->GetVar("$Plugins$").AsString();
    }
    catch(IScriptEnvironment::NotFound) {
      if (!quiet)
        env->ThrowError("LoadPlugin: unable to get plugin list $Plugins$, loading \"%s\"", filename);
      return false;
    }
    // Register FreeLibraries(loaded_plugins) to be run at script close
    env->AtExit(FreeLibraries, loaded_plugins);
  }
  *hmod = LoadLibrary(filename);
  if (!*hmod)
    if (quiet)
      return false;
    else
      env->ThrowError("LoadPlugin: unable to load \"%s\"", filename);
  // see if we've loaded this already, and add it to the list if not
  for (int j=0; j<max_plugins; ++j) {
    if (loaded_plugins[j] == *hmod) {
      FreeLibrary(*hmod);
      return false;
    }
    if (loaded_plugins[j] == 0) {
      char result[512] = "\0";
      char* t_string = _strrev(_strdup(filename));
      int len = strlen(filename);
      int pos = len-strcspn(t_string, ".");
      int pos2 = len-strcspn(t_string, "\\");
      free(t_string);  // Tritical May 2005
      strncat(result, filename+pos2, pos-pos2-1);
      if (loadplugin_prefix) free((void*)loadplugin_prefix);  // Tritical May 2005
      loadplugin_prefix = _strdup(result);
      loaded_plugins[j] = *hmod;
      return true;
    }
  }
  FreeLibrary(*hmod);  // Tritical Jan 2006
  if (!quiet)
    env->ThrowError("LoadPlugin: too many plugins loaded already (max. %d)", max_plugins);
  return false;
}


AVSValue LoadPlugin(AVSValue args, void* user_data, IScriptEnvironment* env) {
  bool quiet = (user_data != 0);
  args = args[0];
  const char* result=0;
  for (int i=0; i<args.ArraySize(); ++i) {
    HMODULE plugin;
    const char* plugin_name = args[i].AsString();
    if (MyLoadLibrary(plugin_name, &plugin, quiet, env)) {
      typedef const char* (__stdcall *AvisynthPluginInitFunc)(IScriptEnvironment* env);
      AvisynthPluginInitFunc AvisynthPluginInit = (AvisynthPluginInitFunc)GetProcAddress(plugin, "AvisynthPluginInit2");
      if (!AvisynthPluginInit) {
        AvisynthPluginInit = (AvisynthPluginInitFunc)GetProcAddress(plugin, "_AvisynthPluginInit2@4");
/*
        if (!AvisynthPluginInit) {  // Attempt C-plugin
          AvisynthPluginInit = (AvisynthPluginInitFunc)GetProcAddress(plugin, "avisynth_c_plugin_init");
          if (AvisynthPluginInit) {
            FreeLibrary(plugin);
            return env->Invoke("LoadCPlugin", args);
          }
        }
*/
        if (!AvisynthPluginInit) {  // Older version
          FreeLibrary(plugin);
          if (quiet) {
            // remove the last handle from the list
            HMODULE* loaded_plugins = (HMODULE*)env->GetVar("$Plugins$").AsString();
            int j=0;
            while (loaded_plugins[j+1]) j++;
            loaded_plugins[j] = 0;
          } else {
            env->ThrowError("Plugin %s is not an AviSynth 2.5 plugin.",plugin_name);
          }
        } else {
          result = AvisynthPluginInit(env);
        }
      } else {
        result = AvisynthPluginInit(env);
      }
    }
  }
  if (loadplugin_prefix) free((void*)loadplugin_prefix);  // Tritical May 2005
  loadplugin_prefix = 0;
  return result ? AVSValue(result) : AVSValue();
}



/********************************************************************
* VFAPI plugin support
********************************************************************/

#define VF_STREAM_VIDEO   0x00000001
#define VF_STREAM_AUDIO   0x00000002
#define VF_OK       0x00000000
#define VF_ERROR      0x80004005

struct VF_PluginInfo {
  DWORD dwSize;
  DWORD dwAPIVersion;
  DWORD dwVersion;
  DWORD dwSupportStreamType;
  char  cPluginInfo[256];
  char  cFileType[256];
};

typedef DWORD VF_FileHandle;

struct VF_FileInfo {
  DWORD dwSize;
  DWORD dwHasStreams;
};

struct VF_StreamInfo_Video {
  DWORD dwSize;
  DWORD dwLengthL;
  DWORD dwLengthH;
  DWORD dwRate;
  DWORD dwScale;
  DWORD dwWidth;
  DWORD dwHeight;
  DWORD dwBitCount;
};

struct VF_StreamInfo_Audio {
  DWORD dwSize;
  DWORD dwLengthL;
  DWORD dwLengthH;
  DWORD dwRate;
  DWORD dwScale;
  DWORD dwChannels;
  DWORD dwBitsPerSample;
  DWORD dwBlockAlign;
};

struct VF_ReadData_Video {
  DWORD dwSize;
  DWORD dwFrameNumberL;
  DWORD dwFrameNumberH;
  void  *lpData;
  long  lPitch;
};

struct VF_ReadData_Audio {
  DWORD dwSize;
  DWORD dwSamplePosL;
  DWORD dwSamplePosH;
  DWORD dwSampleCount;
  DWORD dwReadedSampleCount;
  DWORD dwBufSize;
  void  *lpBuf;
};

struct VF_PluginFunc {
  DWORD dwSize;
  HRESULT (_stdcall *OpenFile)( const char *lpFileName, VF_FileHandle* lpFileHandle );
  HRESULT (_stdcall *CloseFile)( VF_FileHandle hFileHandle );
  HRESULT (_stdcall *GetFileInfo)( VF_FileHandle hFileHandle, VF_FileInfo* lpFileInfo );
  HRESULT (_stdcall *GetStreamInfo)( VF_FileHandle hFileHandle,DWORD dwStream,void *lpStreamInfo );
  HRESULT (_stdcall *ReadData)( VF_FileHandle hFileHandle,DWORD dwStream,void *lpData ); 
};

typedef HRESULT (__stdcall *VF_GetPluginInfo)(VF_PluginInfo* lpPluginInfo);
typedef HRESULT (__stdcall *VF_GetPluginFunc)(VF_PluginFunc* lpPluginFunc);

void CheckHresult(IScriptEnvironment* env, HRESULT hr) {
  if (FAILED(hr)) {
    env->ThrowError("VFAPI plugin returned an error (0x%X)", hr);
  }
}

void DeleteVFPluginFunc(void* vfpf, IScriptEnvironment*) {
  delete (VF_PluginFunc*)vfpf;
}

class VFAPIPluginProxy : public IClip {
  VideoInfo vi;
  IScriptEnvironment* const env;
  const VF_PluginFunc* const plugin_func;
  VF_FileHandle h;
public:
  VFAPIPluginProxy(const char* filename, const VF_PluginFunc* _plugin_func, IScriptEnvironment* _env)
    : env(_env), plugin_func(_plugin_func)
  {
    CheckHresult(env, plugin_func->OpenFile(filename, &h));
    VF_FileInfo file_info = { sizeof(VF_FileInfo) };
    CheckHresult(env, plugin_func->GetFileInfo(h, &file_info));

    memset(&vi, 0, sizeof(VideoInfo));
    if (file_info.dwHasStreams & VF_STREAM_VIDEO) {
      VF_StreamInfo_Video stream_info = { sizeof(VF_StreamInfo_Video) };
      CheckHresult(env, plugin_func->GetStreamInfo(h, VF_STREAM_VIDEO, &stream_info));
      if (stream_info.dwBitCount == 24) {
        vi.pixel_type = VideoInfo::CS_BGR24;
      } else if (stream_info.dwBitCount == 32) {
        vi.pixel_type = VideoInfo::CS_BGR32;
      } else {
        env->ThrowError("VFAPIPluginProxy: plugin returned invalid bit depth (%d)", stream_info.dwBitCount);
      }
      vi.width = stream_info.dwWidth;
      vi.height = stream_info.dwHeight;
      vi.num_frames = stream_info.dwLengthL;
      vi.SetFPS(stream_info.dwRate, stream_info.dwScale);
    }
    if (file_info.dwHasStreams & VF_STREAM_AUDIO) {
      VF_StreamInfo_Audio stream_info = { sizeof(VF_StreamInfo_Audio) };
      CheckHresult(env, plugin_func->GetStreamInfo(h, VF_STREAM_AUDIO, &stream_info));
      vi.audio_samples_per_second = stream_info.dwRate / stream_info.dwScale;
      vi.num_audio_samples = stream_info.dwLengthL;
      vi.nchannels = stream_info.dwChannels;

      if (stream_info.dwBitsPerSample == 8)
        vi.sample_type = SAMPLE_INT8;
      else if (stream_info.dwBitsPerSample == 16)
        vi.sample_type = SAMPLE_INT16;
      else if (stream_info.dwBitsPerSample == 24)
        vi.sample_type = SAMPLE_INT24;
      else if (stream_info.dwBitsPerSample == 32)
        vi.sample_type = SAMPLE_INT32;
      else
        env->ThrowError("VFAPIPluginProxy: plugin returned invalid audio sample depth (%d)", stream_info.dwBitsPerSample);
    }

    if (!vi.HasVideo() && !vi.HasAudio())
      env->ThrowError("VFAPIPluginProxy: no video or audio stream");
  }

  const VideoInfo& __stdcall GetVideoInfo() { return vi; }

  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env) {
    n = max(min(n, vi.num_frames-1), 0);
    PVideoFrame result = env->NewVideoFrame(vi);
    VF_ReadData_Video vfrdv = { sizeof(VF_ReadData_Video), n, 0, result->GetWritePtr(), result->GetPitch() };
    CheckHresult(env, plugin_func->ReadData(h, VF_STREAM_VIDEO, &vfrdv));
    return result;
  }

  void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) {
    if (start < 0) {
      int bytes = vi.BytesFromAudioSamples(-start);
      memset(buf, 0, bytes);
      buf = (char*)buf + bytes;
      count += start;
      start = 0;
    }
    VF_ReadData_Audio vfrda = { sizeof(VF_ReadData_Audio), start, 0, count, 0, vi.BytesFromAudioSamples(count), buf };
    CheckHresult(env, plugin_func->ReadData(h, VF_STREAM_AUDIO, &vfrda));
    if (int(vfrda.dwReadedSampleCount) < count) {
      memset((char*)buf + vi.BytesFromAudioSamples(vfrda.dwReadedSampleCount),
        0, vi.BytesFromAudioSamples(count - vfrda.dwReadedSampleCount));
    }
  }

  bool __stdcall GetParity(int n) { return false; }
  void __stdcall SetCacheHints(int cachehints,int frame_range) { };

  static AVSValue __cdecl Create(AVSValue args, void* user_data, IScriptEnvironment* env) {
    args = args[0];
    PClip result = new VFAPIPluginProxy(args[0].AsString(), (VF_PluginFunc*)user_data, env);
    for (int i=1; i<args.ArraySize(); ++i)
      result = new_Splice(result, new VFAPIPluginProxy(args[i].AsString(), (VF_PluginFunc*)user_data, env), false, env);
    return result;
  }
};


AVSValue LoadVFAPIPlugin(AVSValue args, void*, IScriptEnvironment* env) {
  HMODULE plugin;
  const char* plugin_name = args[0].AsString();
  if (MyLoadLibrary(plugin_name, &plugin, false, env)) {
    VF_GetPluginInfo vfGetPluginInfo = (VF_GetPluginInfo)GetProcAddress(plugin, "vfGetPluginInfo");
    VF_GetPluginFunc vfGetPluginFunc = (VF_GetPluginFunc)GetProcAddress(plugin, "vfGetPluginFunc");
    if (!vfGetPluginInfo || !vfGetPluginFunc)
      env->ThrowError("LoadPlugin: \"%s\" is not a VFAPI plugin", plugin_name);
    VF_PluginInfo plugin_info = { sizeof(VF_PluginInfo) };
    CheckHresult(env, vfGetPluginInfo(&plugin_info));

    VF_PluginFunc* plugin_func = new VF_PluginFunc;
    env->AtExit(DeleteVFPluginFunc, plugin_func);
    plugin_func->dwSize = sizeof(VF_PluginFunc);
    CheckHresult(env, vfGetPluginFunc(plugin_func));

    env->AddFunction(args[1].AsString(), "s+", VFAPIPluginProxy::Create, plugin_func);

    return plugin_info.cPluginInfo ? AVSValue(plugin_info.cPluginInfo) : AVSValue();
  } else {
    return AVSValue();
  }
}

/********************************************************************
* VirtualDub plugin support
********************************************************************/

//  VirtualDub - Video processing and capture application
//  Copyright (C) 1998-2000 Avery Lee
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.



class CScriptValue;
struct CScriptObject;


//////////////////// from sylia/ScriptInterpreter.h ////////////////////


class IScriptInterpreter {
public:
  virtual void Destroy()                    =0;

  virtual void SetRootHandler(void*, void*) =0;

  virtual void ExecuteLine(char *s)             =0;

  virtual void ScriptError(int e)               =0;
  virtual char* TranslateScriptError(void* cse)   =0;
  virtual char** AllocTempString(long l)            =0;

  virtual CScriptValue LookupObjectMember(CScriptObject *obj, void *, char *szIdent) = 0;
};


//////////////////// from sylia/ScriptValue.h ////////////////////

class FilterActivation;

typedef void (*ScriptVoidFunctionPtr)(IScriptInterpreter *, FilterActivation *, CScriptValue *, int);

struct ScriptFunctionDef {
  ScriptVoidFunctionPtr func_ptr;
  char *name;
  char *arg_list;
};

struct CScriptObject {
  void* Lookup;
  ScriptFunctionDef* func_list;
  void* obj_list;
};

class CScriptValue {
public:
  enum { T_VOID, T_INT, T_PINT, T_STR, T_ARRAY, T_OBJECT, T_FNAME, T_FUNCTION, T_VARLV } type;
  CScriptObject *thisPtr;
  union {
    int i;
    char **s;
  } u;
  void *lpVoid;
  CScriptValue()            { type = T_VOID; }
  void operator=(int i)         { type = T_INT;     u.i = i; }
  void operator=(char **s)        { type = T_STR;     u.s = s; }
};


//////////////////// from VBitmap.h ////////////////////


class VBitmap {
public:
  Pixel *     data;
  Pixel *     palette;
  int       depth;
  PixCoord    w, h;
  PixOffset   pitch;
  PixOffset   modulo;
  PixOffset   size;
  PixOffset   offset;

  PixOffset PitchAlign4() {
    return ((w * depth + 31)/32)*4;
  }

  PixOffset PitchAlign8() {
    return ((w * depth + 63)/64)*8;
  }

  PixOffset Modulo() {
    return pitch - (w*depth+7)/8;
  }

  PixOffset Size() {
    return pitch*h;
  }

  //////

  virtual VBitmap& init(void *data, PixDim w, PixDim h, int depth) throw();
  virtual VBitmap& init(void *data, BITMAPINFOHEADER *) throw();

  virtual void MakeBitmapHeader(BITMAPINFOHEADER *bih) const throw();

  virtual void AlignTo4() throw();
  virtual void AlignTo8() throw();

  virtual void BitBlt(PixCoord x2, PixCoord y2, const VBitmap *src, PixCoord x1, PixCoord y1, PixDim dx, PixDim dy) const;
  virtual void BitBltDither(PixCoord x2, PixCoord y2, const VBitmap *src, PixDim x1, PixDim y1, PixDim dx, PixDim dy, bool to565) const;
  virtual void BitBlt565(PixCoord x2, PixCoord y2, const VBitmap *src, PixDim x1, PixDim y1, PixDim dx, PixDim dy) const;

  virtual bool BitBltXlat1(PixCoord x2, PixCoord y2, const VBitmap *src, PixCoord x1, PixCoord y1, PixDim dx, PixDim dy, const Pixel8 *tbl) const;
  virtual bool BitBltXlat3(PixCoord x2, PixCoord y2, const VBitmap *src, PixCoord x1, PixCoord y1, PixDim dx, PixDim dy, const Pixel32 *tbl) const;

  virtual bool StretchBltNearestFast(PixCoord x1, PixCoord y1, PixDim dx, PixDim dy, const VBitmap *src, double x2, double y2, double dx1, double dy1) const;

  virtual bool StretchBltBilinearFast(PixCoord x1, PixCoord y1, PixDim dx, PixDim dy, const VBitmap *src, double x2, double y2, double dx1, double dy1) const;

  virtual bool RectFill(PixCoord x1, PixCoord y1, PixDim dx, PixDim dy, Pixel32 c) const;

  virtual bool Histogram(PixCoord x, PixCoord y, PixCoord dx, PixCoord dy, long *pHisto, int iHistoType) const;

  //// NEW AS OF VIRTUALDUB V1.2B

  virtual bool BitBltFromYUY2(PixCoord x2, PixCoord y2, const VBitmap *src, PixCoord x1, PixCoord y1, PixDim dx, PixDim dy) const;
  virtual bool BitBltFromI420(PixCoord x2, PixCoord y2, const VBitmap *src, PixCoord x1, PixCoord y1, PixDim dx, PixDim dy) const;
};


VBitmap& VBitmap::init(void *lpData, BITMAPINFOHEADER *bmih) throw() {
  data      = (Pixel *)lpData;
  palette     = (Pixel *)(bmih+1);
  depth     = bmih->biBitCount;
  w       = bmih->biWidth;
  h       = bmih->biHeight;
  offset      = 0;
  AlignTo4();

  return *this;
}

VBitmap& VBitmap::init(void *data, PixDim w, PixDim h, int depth) throw() {
  this->data    = (Pixel32 *)data;
  this->palette = NULL;
  this->depth   = depth;
  this->w     = w;
  this->h     = h;
  this->offset  = 0;
  AlignTo8();

  return *this;
}

void VBitmap::MakeBitmapHeader(BITMAPINFOHEADER *bih) const throw() {
  bih->biSize       = sizeof(BITMAPINFOHEADER);
  bih->biBitCount     = depth;
  bih->biPlanes     = 1;
  bih->biCompression    = BI_RGB;

  if (pitch == ((w*bih->biBitCount + 31)/32) * 4)
    bih->biWidth    = w;
  else
    bih->biWidth    = pitch*8 / depth;

  bih->biHeight     = h;
  bih->biSizeImage    = pitch*h;
  bih->biClrUsed      = 0;
  bih->biClrImportant   = 0;
  bih->biXPelsPerMeter  = 0;
  bih->biYPelsPerMeter  = 0;
}

void VBitmap::AlignTo4() throw() {
  pitch   = PitchAlign4();
  modulo    = Modulo();
  size    = Size();
}

void VBitmap::AlignTo8() throw() {
  pitch   = PitchAlign8();
  modulo    = Modulo();
  size    = Size();
}


//////////////////// from Filter.h ////////////////////


// This is really dumb, but necessary to support VTbls in C++.

struct FilterVTbls {
  void *pvtblVBitmap;
};

//////////////////

enum {
  FILTERPARAM_SWAP_BUFFERS  = 0x00000001L,
  FILTERPARAM_NEEDS_LAST    = 0x00000002L,
};

///////////////////

class VFBitmap;
struct FilterFunctions;

typedef int  (*FilterInitProc     )(FilterActivation *fa, const FilterFunctions *ff);
typedef void (*FilterDeinitProc   )(FilterActivation *fa, const FilterFunctions *ff);
typedef int  (*FilterRunProc      )(const FilterActivation *fa, const FilterFunctions *ff);
typedef long (*FilterParamProc    )(FilterActivation *fa, const FilterFunctions *ff);
typedef int  (*FilterConfigProc   )(FilterActivation *fa, const FilterFunctions *ff, HWND hWnd);
typedef void (*FilterStringProc   )(const FilterActivation *fa, const FilterFunctions *ff, char *buf);
typedef int  (*FilterStartProc    )(FilterActivation *fa, const FilterFunctions *ff);
typedef int  (*FilterEndProc      )(FilterActivation *fa, const FilterFunctions *ff);
typedef bool (*FilterScriptStrProc)(FilterActivation *fa, const FilterFunctions *, char *, int);

typedef int (__cdecl *FilterModuleInitProc)(struct FilterModule *fm, const FilterFunctions *ff, int& vdfd_ver, int& vdfd_compat);
typedef void (__cdecl *FilterModuleDeinitProc)(struct FilterModule *fm, const FilterFunctions *ff);

//////////

class IFilterPreview {
public:
  virtual void SetButtonCallback(void*, void*)=0;
  virtual void SetSampleCallback(void*, void*)=0;

  virtual bool isPreviewEnabled()=0;
  virtual void Toggle(HWND)=0;
  virtual void Display(HWND, bool)=0;
  virtual void RedoFrame()=0;
  virtual void RedoSystem()=0;
  virtual void UndoSystem()=0;
  virtual void InitButton(HWND)=0;
  virtual void Close()=0;
  virtual bool SampleCurrentFrame()=0;
  virtual long SampleFrames()=0;
};

//////////

#define VIRTUALDUB_FILTERDEF_VERSION    (6)
#define VIRTUALDUB_FILTERDEF_COMPATIBLE   (4)

// v3: added lCurrentSourceFrame to FrameStateInfo
// v4 (1.2): lots of additions (VirtualDub 1.2)
// v5 (1.3d): lots of bugfixes - stretchblt bilinear, and non-zero startproc
// v6 (1.4): added error handling functions

typedef struct FilterModule {
  struct FilterModule *next, *prev;
  HINSTANCE       hInstModule;
  FilterModuleInitProc  initProc;
  FilterModuleDeinitProc  deinitProc;
    IScriptEnvironment*   env;
    const char*       avisynth_function_name;
    int preroll;
} FilterModule;

typedef struct FilterDefinition {

  struct FilterDefinition *next, *prev;
  FilterModule *module;

  char *        name;
  char *        desc;
  char *        maker;
  void *        private_data;
  int         inst_data_size;

  FilterInitProc    initProc;
  FilterDeinitProc  deinitProc;
  FilterRunProc   runProc;
  FilterParamProc   paramProc;
  FilterConfigProc  configProc;
  FilterStringProc  stringProc;
  FilterStartProc   startProc;
  FilterEndProc   endProc;

  CScriptObject *script_obj;

  FilterScriptStrProc fssProc;

} FilterDefinition;

//////////

// FilterStateInfo: contains dynamic info about file being processed

class FilterStateInfo {
public:
  long  lCurrentFrame;        // current output frame
  long  lMicrosecsPerFrame;     // microseconds per output frame
  long  lCurrentSourceFrame;    // current source frame
  long  lMicrosecsPerSrcFrame;    // microseconds per source frame
  long  lSourceFrameMS;       // source frame timestamp
  long  lDestFrameMS;       // output frame timestamp
};

// VFBitmap: VBitmap extended to hold filter-specific information

class VFBitmap : public VBitmap {
public:
  enum {
    NEEDS_HDC   = 0x00000001L,
  };

  DWORD dwFlags;
  HDC   hdc;
};

// FilterActivation: This is what is actually passed to filters at runtime.

class FilterActivation {
public:
  FilterDefinition *filter;
  void *filter_data;
  VFBitmap &dst, &src;
  VFBitmap *__reserved0, *const last;
  unsigned long x1, y1, x2, y2;

  FilterStateInfo *pfsi;
  IFilterPreview *ifp;

  FilterActivation(VFBitmap& _dst, VFBitmap& _src, VFBitmap *_last) : dst(_dst), src(_src), last(_last) {}
};

struct FilterFunctions {
  FilterDefinition *(*addFilter)(FilterModule *, FilterDefinition *, int fd_len);
  void (*removeFilter)(FilterDefinition *);
  bool (*isFPUEnabled)();
  bool (*isMMXEnabled)();
  void (*InitVTables)(struct FilterVTbls *);

  // These functions permit you to throw MyError exceptions from a filter.
  // YOU MUST ONLY CALL THESE IN runProc, initProc, and startProc.

  void (*ExceptOutOfMemory)();        // ADDED: V6 (VirtualDub 1.4)
  void (*Except)(const char *format, ...);  // ADDED: V6 (VirtualDub 1.4)

  long (*getCPUFlags)();            // ADDED: V6 (VirtualDub 1.4)
};


////////////////////////////////////////////////////////////


// Avisynth doesn't support the following functions.

void VBitmap::BitBlt(PixCoord x2, PixCoord y2, const VBitmap *src, PixCoord x1, PixCoord y1, PixDim dx, PixDim dy) const
{
  throw AvisynthError("Unsupported VBitmap method: BitBlt");
}

void VBitmap::BitBltDither(PixCoord x2, PixCoord y2, const VBitmap *src, PixDim x1, PixDim y1, PixDim dx, PixDim dy, bool to565) const
{
  throw AvisynthError("Unsupported VBitmap method: BitBltDither");
}

void VBitmap::BitBlt565(PixCoord x2, PixCoord y2, const VBitmap *src, PixDim x1, PixDim y1, PixDim dx, PixDim dy) const
{
  throw AvisynthError("Unsupported VBitmap method: BitBlt565");
}

bool VBitmap::BitBltXlat1(PixCoord x2, PixCoord y2, const VBitmap *src, PixCoord x1, PixCoord y1, PixDim dx, PixDim dy, const Pixel8 *tbl) const
{
  throw AvisynthError("Unsupported VBitmap method: BitBltXlat1");
}

bool VBitmap::BitBltXlat3(PixCoord x2, PixCoord y2, const VBitmap *src, PixCoord x1, PixCoord y1, PixDim dx, PixDim dy, const Pixel32 *tbl) const
{
  throw AvisynthError("Unsupported VBitmap method: BitBltXlat3");
}

bool VBitmap::StretchBltNearestFast(PixCoord x1, PixCoord y1, PixDim dx, PixDim dy, const VBitmap *src, double x2, double y2, double dx1, double dy1) const
{
  throw AvisynthError("Unsupported VBitmap method: StretchBltNearestFast");
}

bool VBitmap::StretchBltBilinearFast(PixCoord x1, PixCoord y1, PixDim dx, PixDim dy, const VBitmap *src, double x2, double y2, double dx1, double dy1) const
{
  throw AvisynthError("Unsupported VBitmap method: StretchBltBilinearFast");
}

bool VBitmap::RectFill(PixCoord x1, PixCoord y1, PixDim dx, PixDim dy, Pixel32 c) const
{
  throw AvisynthError("Unsupported VBitmap method: RectFill");
}

bool VBitmap::Histogram(PixCoord x, PixCoord y, PixCoord dx, PixCoord dy, long *pHisto, int iHistoType) const
{
  throw AvisynthError("Unsupported VBitmap method: Histogram");
}

bool VBitmap::BitBltFromYUY2(PixCoord x2, PixCoord y2, const VBitmap *src, PixCoord x1, PixCoord y1, PixDim dx, PixDim dy) const
{
  throw AvisynthError("Unsupported VBitmap method: BitBltFromYUY2");
}

bool VBitmap::BitBltFromI420(PixCoord x2, PixCoord y2, const VBitmap *src, PixCoord x1, PixCoord y1, PixDim dx, PixDim dy) const
{
  throw AvisynthError("Unsupported VBitmap method: BitBltFromI420");
}


class DummyFilterPreview : public IFilterPreview {
  void Die() { throw AvisynthError("IFilterPreview not supported"); }
public:
  virtual void SetButtonCallback(void*, void*) { Die(); }
  virtual void SetSampleCallback(void*, void*) { Die(); }

    virtual bool isPreviewEnabled() { return false; }
    virtual void Toggle(HWND) {}
    virtual void Display(HWND, bool) {}
    virtual void RedoFrame() {}
    virtual void RedoSystem() {}
    virtual void UndoSystem() {}
    virtual void InitButton(HWND) {}
    virtual void Close() {}
    virtual bool SampleCurrentFrame() { return false; }
    virtual long SampleFrames() { return 0; }
};


//////////////////// from Filters.cpp ////////////////////

extern char exception_conversion_buffer[2048];    // in AVIReadHandler.cpp

static void FilterThrowExcept(const char *format, ...) {
  va_list val;
  va_start(val, format);
  wvsprintf(exception_conversion_buffer, format, val);
  va_end(val);
  throw AvisynthError(exception_conversion_buffer);
}

static void FilterThrowExceptMemory() {
  throw MyMemoryError();
}

// This is really disgusting...

static void InitVTables(struct FilterVTbls *pvtbls) {
  pvtbls->pvtblVBitmap = *(void **)&VBitmap();
}


FilterDefinition *FilterAdd(FilterModule *fm, FilterDefinition *pfd, int fd_len);

void FilterRemove(FilterDefinition*) {}

bool isFPUEnabled() { return !!(GetCPUFlags() & CPUF_FPU); }
bool isMMXEnabled() { return !!(GetCPUFlags() & CPUF_MMX); }

FilterFunctions g_filterFuncs={
  FilterAdd, FilterRemove, isFPUEnabled, isMMXEnabled, InitVTables, FilterThrowExceptMemory, FilterThrowExcept, GetCPUFlags
};


////////////////////////////////////////////////////////////


class MyScriptInterpreter : public IScriptInterpreter {
  IScriptEnvironment* const env;
public:
  MyScriptInterpreter(IScriptEnvironment* _env) : env(_env) {}
  void Destroy() {}
  void SetRootHandler(void*, void*) {}
  void ExecuteLine(char *s) {}
  void ScriptError(int e) {
    switch (e) {
      case 21: env->ThrowError("VirtualdubFilterProxy: OUT_OF_MEMORY");
      case 24: env->ThrowError("VirtualdubFilterProxy: FCALL_OUT_OF_RANGE");
      case 26: env->ThrowError("VirtualdubFilterProxy: FCALL_UNKNOWN_STR");
      default: env->ThrowError("VirtualdubFilterProxy: unknown error code");
    }
  }
  char* TranslateScriptError(void* cse) { return ""; }
  char** AllocTempString(long l) { return (char**)0; }
  CScriptValue LookupObjectMember(CScriptObject *obj, void *, char *szIdent) { return CScriptValue(); }
};


class VirtualdubFilterProxy : public GenericVideoFilter {
  PVideoFrame src, dst, last;
  VFBitmap vbSrc, vbDst, vbLast;
  FilterDefinition* const fd;
  FilterStateInfo fsi;
  FilterActivation fa;
  DummyFilterPreview fp;
  int expected_frame_number;

  void CallStartProc() {
    if (fd->startProc) {
      int result = fd->startProc(&fa, &g_filterFuncs);
      if (result != 0) {
        if (fd->endProc)
          fd->endProc(&fa, &g_filterFuncs);
        throw AvisynthError("VirtualdubFilterProxy: error calling startProc");
      }
    }
  }

  void CallEndProc() {
    if (fd->endProc) {
      int result = fd->endProc(&fa, &g_filterFuncs);
      if (result != 0) {
        throw AvisynthError("VirtualdubFilterProxy: error calling endProc");
      }
    }
  }

public:
  VirtualdubFilterProxy(PClip _child, FilterDefinition* _fd, AVSValue args, IScriptEnvironment* env)
    : GenericVideoFilter(_child), fd(_fd), fa(vbDst, vbSrc, &vbLast)
  {
    if (!vi.IsRGB32())
      throw AvisynthError("VirtualdubFilterProxy: only RGB32 supported for VirtualDub filters");

    fa.filter = fd;
    fa.pfsi = &fsi;
    fa.ifp = &fp;
    fa.filter_data = 0;
    fsi.lMicrosecsPerFrame = fsi.lMicrosecsPerSrcFrame = MulDiv(vi.fps_denominator, 1000000, vi.fps_numerator);

    if (fd->inst_data_size) {
      fa.filter_data = new char[fd->inst_data_size];
      memset(fa.filter_data, 0, fd->inst_data_size);
      if (fd->initProc) {
        if (fd->initProc(&fa, &g_filterFuncs) != 0)
          throw AvisynthError("VirtualdubFilterProxy: Error calling initProc");
      }
      if (args.ArraySize() > 1)
        InvokeSyliaConfigFunction(fd, args, env);
    }

    src = env->NewVideoFrame(vi);
    SetVFBitmap(src, &vbSrc);
    SetVFBitmap(src, &vbLast);
    SetVFBitmap(src, &vbDst);

    long flags = fd->paramProc ? fd->paramProc(&fa, &g_filterFuncs) : FILTERPARAM_SWAP_BUFFERS;
    bool two_buffers = !!(flags & FILTERPARAM_SWAP_BUFFERS);
    bool needs_last = !!(flags & FILTERPARAM_NEEDS_LAST);
    bool src_needs_hdc = (vbSrc.dwFlags & VFBitmap::NEEDS_HDC);
    bool dst_needs_hdc = (vbDst.dwFlags & VFBitmap::NEEDS_HDC);

    if (src_needs_hdc || dst_needs_hdc) {
//      throw AvisynthError("VirtualdubFilterProxy: HDC not supported");
      vbSrc.hdc = vbDst.hdc = vbLast.hdc = GetDC(NULL);
    }

    if (needs_last) {
      last = env->NewVideoFrame(vi);
      SetVFBitmap(last, &vbLast);
    }

    if (two_buffers) {
      vi.width = vbDst.pitch >> 2;
      vi.height = vbDst.h;
      dst = env->NewVideoFrame(vi);
      SetVFBitmap(dst, &vbDst);
    }

    CallStartProc();
    expected_frame_number = 0;
  }

  void SetVFBitmap(const PVideoFrame& pvf, VFBitmap* pvb) {
    pvb->data = (Pixel*)pvf->GetReadPtr();
    pvb->palette = 0;
    pvb->depth = 32;
    pvb->w = pvf->GetRowSize() >> 2;
    pvb->h = pvf->GetHeight();
    pvb->pitch = pvf->GetPitch();
    pvb->modulo = pvb->Modulo();
    pvb->size = pvb->Size();
    pvb->offset = 0;
    pvb->dwFlags = 0;
    pvb->hdc = 0;
  }

  PVideoFrame FilterFrame(int n, IScriptEnvironment* env, bool in_preroll) {
    if (last) {
      BitBlt(last->GetWritePtr(), last->GetPitch(), src->GetReadPtr(), src->GetPitch(),
        last->GetRowSize(), last->GetHeight());
    }
    {
      PVideoFrame _src = child->GetFrame(n, env);
      BitBlt(src->GetWritePtr(), src->GetPitch(), _src->GetReadPtr(), _src->GetPitch(),
        src->GetRowSize(), src->GetHeight());
    }

    fsi.lCurrentSourceFrame = fsi.lCurrentFrame = n;
    fsi.lDestFrameMS = fsi.lSourceFrameMS = MulDiv(n, fsi.lMicrosecsPerFrame, 1000);

    fd->runProc(&fa, &g_filterFuncs);

    if (in_preroll) {
      return 0;
    } else {
      PVideoFrame _dst = env->NewVideoFrame(vi);
      BitBlt(_dst->GetWritePtr(), _dst->GetPitch(), (dst?dst:src)->GetReadPtr(),
        _dst->GetPitch(), _dst->GetRowSize(), _dst->GetHeight());
      return _dst;
    }
  }

  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env) {
    if (n != expected_frame_number) {
      CallEndProc();
      CallStartProc();
      for (int i = min(n, fd->module->preroll); i > 0; i--)
        FilterFrame(n-i, env, true);
    }
    expected_frame_number = n+1;
    return FilterFrame(n, env, false);
  }

  static int ConvertArgs(const AVSValue* args, CScriptValue* sylia_args, int count) {
    for (int i=0; i<count; ++i) {
      if (args[i].IsInt()) {
        sylia_args[i] = args[i].AsInt();
      } else if (args[i].IsString()) {
        sylia_args[i].lpVoid = (void*)args[i].AsString();
        sylia_args[i] = (char**)&sylia_args[i].lpVoid;
      } else if (args[i].IsArray()) {
        return i+ConvertArgs(&args[i][0], sylia_args+i, args[i].ArraySize());
      } else {
        return -1000;
      }
    }
    return count;
  }

  void InvokeSyliaConfigFunction(FilterDefinition* fd, AVSValue args, IScriptEnvironment* env) {
    if (fd->script_obj && fd->script_obj->func_list && args.ArraySize() > 1) {
      for (ScriptFunctionDef* i = fd->script_obj->func_list; i->arg_list; i++) {
        const char* p = i->arg_list;
        int j;
        for (j=1; j<args.ArraySize(); j++) {
          if (p[j] == 'i' && args[j].IsInt()) continue;
          else if (p[j] == 's' && args[j].IsString()) continue;
          else if (p[j] == '.' && args[j].IsArray()) continue;
          else break;
        }
        if (j == args.ArraySize() && p[j] == 0) {
          // match
          MyScriptInterpreter si(env);
          CScriptValue sylia_args[30];
          int sylia_arg_count = ConvertArgs(&args[1], sylia_args, args.ArraySize()-1);
          if (sylia_arg_count < 0)
            env->ThrowError("VirtualdubFilterProxy: arguments (after first) must be integers and strings only");
          i->func_ptr(&si, &fa, sylia_args, sylia_arg_count);
          return;
        }
      }
      env->ThrowError("VirtualdubFilterProxy: no matching config function (this shouldn't happen)");
    }
  }

  ~VirtualdubFilterProxy() {
    CallEndProc();
    if (vbSrc.hdc)
      ReleaseDC(NULL, vbSrc.hdc);
  }

  static AVSValue __cdecl Create(AVSValue args, void* user_data, IScriptEnvironment* env) {
    FilterDefinition* fd = (FilterDefinition*)user_data;
    return new VirtualdubFilterProxy(args[0].AsClip(), fd, args, env);
  }
};


void __cdecl FreeFilterDefinition(void* user_data, IScriptEnvironment* env) {
  FilterDefinition* fd = (FilterDefinition*)user_data;
  delete fd;
}


FilterDefinition *FilterAdd(FilterModule *fm, FilterDefinition *pfd, int fd_len) {
  FilterDefinition *fd = new FilterDefinition;

  if (fd) {
    memcpy(fd, pfd, min(size_t(fd_len), sizeof(FilterDefinition)));
    fd->module  = fm;
    fd->prev  = NULL;
    fd->next  = NULL;
  }

  fm->env->AtExit(FreeFilterDefinition, fd);

  fm->env->AddFunction(fm->avisynth_function_name, "c", VirtualdubFilterProxy::Create, fd);
  if (fd->script_obj && fd->script_obj->func_list) {
    for (ScriptFunctionDef* i = fd->script_obj->func_list; i->arg_list; i++) {
      const char* params = fm->env->Sprintf("c%s%s", i->arg_list+1, strchr(i->arg_list+1, '.') ? "*" : "");
      fm->env->AddFunction(fm->avisynth_function_name, params, VirtualdubFilterProxy::Create, fd);
    }
  }

  return fd;
}


void __cdecl FreeFilterModule(void* user_data, IScriptEnvironment* env) {
  FilterModule* fm = (FilterModule*)user_data;
  fm->deinitProc(fm, &g_filterFuncs);
  FreeLibrary(fm->hInstModule);
  if (fm->prev)
    fm->prev->next = fm->next;
  if (fm->next)
    fm->next->prev = fm->prev;
  delete fm;
}


AVSValue LoadVirtualdubPlugin(AVSValue args, void*, IScriptEnvironment* env) {
  const char* const szModule = args[0].AsString();
  const char* const avisynth_function_name = args[1].AsString();
  const int preroll = args[2].AsInt(0);

  HMODULE hmodule = LoadLibrary(szModule);
  if (!hmodule)
    env->ThrowError("LoadVirtualdubPlugin: Error opening \"%s\"", szModule);

  FilterModuleInitProc initProc   = (FilterModuleInitProc  )GetProcAddress(hmodule, "VirtualdubFilterModuleInit2");
  FilterModuleDeinitProc deinitProc = (FilterModuleDeinitProc)GetProcAddress(hmodule, "VirtualdubFilterModuleDeinit");

  if (!initProc || !deinitProc) {
    FreeLibrary(hmodule);
    env->ThrowError("LoadVirtualdubPlugin: Module \"%s\" does not contain VirtualDub filters.", szModule);
  }

  FilterModule* loaded_modules = 0;
  try {
    loaded_modules = (FilterModule*)env->GetVar("$LoadVirtualdubPlugin$").AsString();
  }
  catch (IScriptEnvironment::NotFound) {}

  for (FilterModule* i = loaded_modules; i; i = i->next) {
    if (i->hInstModule == hmodule) {
      FreeLibrary(hmodule);
      return AVSValue();
    }
  }

  FilterModule* fm = new FilterModule;
  fm->hInstModule = hmodule;
  fm->initProc = initProc;
  fm->deinitProc = deinitProc;
  fm->env = env;
  fm->avisynth_function_name = avisynth_function_name;
  fm->preroll = preroll;
  fm->next = loaded_modules;
  fm->prev = 0;
  if (fm->next)
    fm->next->prev = fm;

  int ver_hi = VIRTUALDUB_FILTERDEF_VERSION;
  int ver_lo = VIRTUALDUB_FILTERDEF_COMPATIBLE;
  if (fm->initProc(fm, &g_filterFuncs, ver_hi, ver_lo)) {
    FreeLibrary(hmodule);
    delete fm;
    env->ThrowError("LoadVirtualdubPlugin: Error initializing module \"%s\"", szModule);
  }

  env->SetGlobalVar("$LoadVirtualdubPlugin$", (const char*)fm);
  env->AtExit(FreeFilterModule, fm);

  return AVSValue();
}


AVSFunction Plugin_functions[] = {
  { "LoadPlugin", "s+", LoadPlugin, (void*)false },
  { "LoadVirtualdubPlugin", "ss[preroll]i", LoadVirtualdubPlugin },
  { "LoadVFAPIPlugin", "ss", LoadVFAPIPlugin },
  { 0 }
};
