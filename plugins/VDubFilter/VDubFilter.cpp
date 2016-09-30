// Avisynth v2.5. 
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

#include <avisynth.h>
#include <avs/win.h>
#include <avs/minmax.h>
#include <cstdio>
#include <new>

typedef unsigned int    Pixel;
typedef unsigned char   Pixel8;
typedef int             PixCoord; // long in vbitmap.h
typedef int             PixDim;   // long in vbitmap.h
//typedef int             PixOffset; // P.F. 160421 this really broke on 64 bit machines
typedef ptrdiff_t       PixOffset; // much better

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

int GetCPUFlags();

#define VDcall __cdecl

class CScriptValue;
class CScriptValueStringHelper; // char ** helper
struct CScriptObject;


//////////////////// from sylia/ScriptInterpreter.h ////////////////////


class IScriptInterpreter {
public:
  virtual void VDcall Destroy()                    =0;

  virtual void VDcall SetRootHandler(void*, void*) =0;

  virtual void VDcall ExecuteLine(char *s)             =0;

  virtual void VDcall ScriptError(int e)               =0;
  virtual char* VDcall TranslateScriptError(void* cse)   =0;
  virtual char** VDcall AllocTempString(long l)            =0;

  virtual CScriptValue VDcall LookupObjectMember(CScriptObject *obj, void *, char *szIdent) = 0;
};


//////////////////// from sylia/ScriptValue.h ////////////////////

class FilterActivation;

typedef void (VDcall *ScriptVoidFunctionPtr)(IScriptInterpreter *, FilterActivation *, CScriptValue *, int);

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
  enum { T_VOID, T_INT, T_PINT, T_STR, T_ARRAY, T_OBJECT, T_FNAME, T_FUNCTION, T_VARLV, T_LONG, T_DOUBLE} type;
  CScriptObject *thisPtr;
  // see vdub: vdvideofilt.h
  /* fails in x64 + vdub + neatvideo
  union {
    int i;
    char **s;
  } u;
  void *lpVoid; // in 64 bit, the union is already 8 bytes, this extra helper pointer/padder makes it too big
  */
  union {
    int i;
    char **s;
    __int64 l; // in avs n/a
    double d;  // new type from 160420
  } u;
  CScriptValue()            { type = T_VOID; }
  void operator=(int i)     { type = T_INT;    u.i = i; }
  void operator=(char **s)  { type = T_STR;    u.s = s; }
  void operator=(__int64 l) { type = T_LONG;   u.l = l; } // not used, only integer exists in avs
  void operator=(double d)  { type = T_DOUBLE; u.d = d; }
  void operator=(float f)   { type = T_DOUBLE; u.d = (double)f; }
};

class CScriptValueStringHelper {
public:
  void *lpVoid; // char ** helper, introduced for x64, the CScriptValue union is already 8 bytes
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

  virtual VBitmap& VDcall init(void *data, PixDim w, PixDim h, int depth) throw();
  virtual VBitmap& VDcall init(void *data, BITMAPINFOHEADER *) throw();

  virtual void VDcall MakeBitmapHeader(BITMAPINFOHEADER *bih) const throw();

  virtual void VDcall AlignTo4() throw();
  virtual void VDcall AlignTo8() throw();

  virtual void VDcall BitBlt(PixCoord x2, PixCoord y2, const VBitmap *src, PixCoord x1, PixCoord y1, PixDim dx, PixDim dy) const;
  virtual void VDcall BitBltDither(PixCoord x2, PixCoord y2, const VBitmap *src, PixDim x1, PixDim y1, PixDim dx, PixDim dy, bool to565) const;
  virtual void VDcall BitBlt565(PixCoord x2, PixCoord y2, const VBitmap *src, PixDim x1, PixDim y1, PixDim dx, PixDim dy) const;

  virtual bool VDcall BitBltXlat1(PixCoord x2, PixCoord y2, const VBitmap *src, PixCoord x1, PixCoord y1, PixDim dx, PixDim dy, const Pixel8 *tbl) const;
  virtual bool VDcall BitBltXlat3(PixCoord x2, PixCoord y2, const VBitmap *src, PixCoord x1, PixCoord y1, PixDim dx, PixDim dy, const Pixel32 *tbl) const;

  virtual bool VDcall StretchBltNearestFast(PixCoord x1, PixCoord y1, PixDim dx, PixDim dy, const VBitmap *src, double x2, double y2, double dx1, double dy1) const;

  virtual bool VDcall StretchBltBilinearFast(PixCoord x1, PixCoord y1, PixDim dx, PixDim dy, const VBitmap *src, double x2, double y2, double dx1, double dy1) const;

  virtual bool VDcall RectFill(PixCoord x1, PixCoord y1, PixDim dx, PixDim dy, Pixel32 c) const;

  virtual bool VDcall Histogram(PixCoord x, PixCoord y, PixCoord dx, PixCoord dy, long *pHisto, int iHistoType) const;

  //// NEW AS OF VIRTUALDUB V1.2B

  virtual bool VDcall BitBltFromYUY2(PixCoord x2, PixCoord y2, const VBitmap *src, PixCoord x1, PixCoord y1, PixDim dx, PixDim dy) const;
  virtual bool VDcall BitBltFromI420(PixCoord x2, PixCoord y2, const VBitmap *src, PixCoord x1, PixCoord y1, PixDim dx, PixDim dy) const;
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
  bih->biBitCount   = (WORD)depth;
  bih->biPlanes     = 1;
  bih->biCompression    = BI_RGB;

  if (pitch == ((w*bih->biBitCount + 31)/32) * 4)
    bih->biWidth    = w;
  else
    bih->biWidth    = LONG(pitch*8 / depth);

  bih->biHeight     = h;
  bih->biSizeImage    = DWORD(pitch*h);
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

typedef int  (VDcall *FilterInitProc     )(FilterActivation *fa, const FilterFunctions *ff);
typedef void (VDcall *FilterDeinitProc   )(FilterActivation *fa, const FilterFunctions *ff);
typedef int  (VDcall *FilterRunProc      )(const FilterActivation *fa, const FilterFunctions *ff);
typedef long (VDcall *FilterParamProc    )(FilterActivation *fa, const FilterFunctions *ff);
typedef int  (VDcall *FilterConfigProc   )(FilterActivation *fa, const FilterFunctions *ff, HWND hWnd);
typedef void (VDcall *FilterStringProc   )(const FilterActivation *fa, const FilterFunctions *ff, char *buf);
typedef int  (VDcall *FilterStartProc    )(FilterActivation *fa, const FilterFunctions *ff);
typedef int  (VDcall *FilterEndProc      )(FilterActivation *fa, const FilterFunctions *ff);
typedef bool (VDcall *FilterScriptStrProc)(FilterActivation *fa, const FilterFunctions *, char *, int);

typedef int  (VDcall *FilterModuleInitProc)(struct FilterModule *fm, const FilterFunctions *ff, int& vdfd_ver, int& vdfd_compat);
typedef void (VDcall *FilterModuleDeinitProc)(struct FilterModule *fm, const FilterFunctions *ff);

//////////

class IFilterPreview {
public:
  virtual void VDcall SetButtonCallback(void*, void*)=0;
  virtual void VDcall SetSampleCallback(void*, void*)=0;

  virtual bool VDcall isPreviewEnabled()=0;
  virtual void VDcall Toggle(HWND)=0;
  virtual void VDcall Display(HWND, bool)=0;
  virtual void VDcall RedoFrame()=0;
  virtual void VDcall RedoSystem()=0;
  virtual void VDcall UndoSystem()=0;
  virtual void VDcall InitButton(HWND)=0;
  virtual void VDcall Close()=0;
  virtual bool VDcall SampleCurrentFrame()=0;
  virtual long VDcall SampleFrames()=0;
};

//////////

#define VIRTUALDUB_FILTERDEF_VERSION    (6)
#define VIRTUALDUB_FILTERDEF_COMPATIBLE   (4)

// v3: added lCurrentSourceFrame to FrameStateInfo
// v4 (1.2): lots of additions (VirtualDub 1.2)
// v5 (1.3d): lots of bugfixes - stretchblt bilinear, and non-zero startproc
// v6 (1.4): added error handling functions

class FilterDefinitionList;

typedef struct FilterModule {
  struct FilterModule    *next, *prev;
  HINSTANCE              hInstModule;
  FilterModuleInitProc   initProc;
  FilterModuleDeinitProc deinitProc;

  IScriptEnvironment*    env;
  const char*            avisynth_function_name;
  int                    preroll;
  FilterDefinitionList*  fdl;
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

class FilterDefinitionList {
public:
  FilterModule* fm;
  FilterDefinition* fd;
  FilterDefinitionList* fdl;

  FilterDefinitionList(FilterModule* _fm, FilterDefinition* _fd) : fm(_fm), fd(_fd), fdl(fm->fdl) { };
};

//////////

// FilterStateInfo: contains dynamic info about file being processed

class FilterStateInfo {
public:
  long  lCurrentFrame;         // current sequence frame (previously called output frame)
  long  lMicrosecsPerFrame;    // microseconds per output frame
  long  lCurrentSourceFrame;   // current source frame
  long  lMicrosecsPerSrcFrame; // microseconds per source frame
  long  lSourceFrameMS;        // source frame timestamp
  long  lDestFrameMS;          // output frame timestamp
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
  unsigned int x1, y1, x2, y2;

  FilterStateInfo *pfsi;
  IFilterPreview *ifp;

  FilterActivation(VFBitmap& _dst, VFBitmap& _src, VFBitmap *_last) : dst(_dst), src(_src), last(_last) {}
};

struct FilterFunctions {
  FilterDefinition *(VDcall *addFilter)(FilterModule *, FilterDefinition *, int fd_len);
  void (VDcall *removeFilter)(FilterDefinition *);
  bool (VDcall *isFPUEnabled)();
  bool (VDcall *isMMXEnabled)();
  void (VDcall *InitVTables)(struct FilterVTbls *);

  // These functions permit you to throw MyError exceptions from a filter.
  // YOU MUST ONLY CALL THESE IN runProc, initProc, and startProc.

  void (VDcall *ExceptOutOfMemory)();        // ADDED: V6 (VirtualDub 1.4)
  void (VDcall *Except)(const char *format, ...);  // ADDED: V6 (VirtualDub 1.4)

  long (VDcall *getCPUFlags)();            // ADDED: V6 (VirtualDub 1.4)
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
  virtual void VDcall SetButtonCallback(void*, void*) { Die(); }
  virtual void VDcall SetSampleCallback(void*, void*) { Die(); }

    virtual bool VDcall isPreviewEnabled() { return false; }
    virtual void VDcall Toggle(HWND) {}
    virtual void VDcall Display(HWND, bool) {}
    virtual void VDcall RedoFrame() {}
    virtual void VDcall RedoSystem() {}
    virtual void VDcall UndoSystem() {}
    virtual void VDcall InitButton(HWND) {}
    virtual void VDcall Close() {}
    virtual bool VDcall SampleCurrentFrame() { return false; }
    virtual long VDcall SampleFrames() { return 0; }
};


//////////////////// from Filters.cpp ////////////////////

char exception_conversion_buffer[2048];

static void VDcall FilterThrowExcept(const char *format, ...) {
  va_list val;
  va_start(val, format);
  _vsnprintf(exception_conversion_buffer, sizeof(exception_conversion_buffer)-1, format, val);
  va_end(val);
  exception_conversion_buffer[sizeof(exception_conversion_buffer)-1] = '\0';
  throw AvisynthError(exception_conversion_buffer);
}

static void VDcall FilterThrowExceptMemory() {
  throw AvisynthError("VDubFilter: Out of memory.");
}

// This is really disgusting...

#pragma warning( push )
#pragma warning (disable: 4238) // nonstandard extension used : class rvalue used as lvalue
static void VDcall InitVTables(struct FilterVTbls *pvtbls) {
  pvtbls->pvtblVBitmap = *(void **)&VBitmap();
}


FilterDefinition *VDcall FilterAdd(FilterModule *fm, FilterDefinition *pfd, int fd_len);

void VDcall FilterRemove(FilterDefinition*) {}

static long VDcall VD_GetCPUFlags() {
  return GetCPUFlags();
}

bool VDcall isFPUEnabled() { return !!(GetCPUFlags() & CPUF_FPU); }
bool VDcall isMMXEnabled() { return !!(GetCPUFlags() & CPUF_MMX); }

FilterFunctions g_filterFuncs={
  FilterAdd, FilterRemove, isFPUEnabled, isMMXEnabled, InitVTables, 
  FilterThrowExceptMemory, FilterThrowExcept, VD_GetCPUFlags
};


////////////////////////////////////////////////////////////


class MyScriptInterpreter : public IScriptInterpreter {
  IScriptEnvironment* const env;
public:
  MyScriptInterpreter(IScriptEnvironment* _env) : env(_env) {}
  void VDcall Destroy() {}
  void VDcall SetRootHandler(void*, void*) {}
  void VDcall ExecuteLine(char *s) {}
  void VDcall ScriptError(int e) {
    switch (e) {
      case 21: env->ThrowError("VirtualdubFilterProxy: OUT_OF_MEMORY");
      case 24: env->ThrowError("VirtualdubFilterProxy: FCALL_OUT_OF_RANGE");
      case 26: env->ThrowError("VirtualdubFilterProxy: FCALL_UNKNOWN_STR");
      default: env->ThrowError("VirtualdubFilterProxy: Unknown error code %d", e);
    }
  }
  char* VDcall TranslateScriptError(void* cse) { return ""; }
  char** VDcall AllocTempString(long l) { return (char**)0; }
  CScriptValue VDcall LookupObjectMember(CScriptObject *obj, void *, char *szIdent) { return CScriptValue(); }
};


class VirtualdubFilterProxy : public GenericVideoFilter {
  PVideoFrame src, dst, last;
  VFBitmap vbSrc, vbDst, vbLast;
  FilterDefinitionList* const fdl;
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
  VirtualdubFilterProxy(PClip _child, FilterDefinitionList* _fdl, AVSValue args, IScriptEnvironment* env)
    : GenericVideoFilter(_child), fdl(_fdl), fd(_fdl->fd), fa(vbDst, vbSrc, &vbLast)
  {
	if (!fd)
		env->ThrowError("VirtualdubFilterProxy: No FilterDefinition structure!");
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
      vi.width = int(vbDst.pitch >> 2);
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
      env->BitBlt(last->GetWritePtr(), last->GetPitch(), src->GetReadPtr(), src->GetPitch(),
        last->GetRowSize(), last->GetHeight());
    }
    {
      PVideoFrame _src = child->GetFrame(n, env);
      env->BitBlt(src->GetWritePtr(), src->GetPitch(), _src->GetReadPtr(), _src->GetPitch(),
        src->GetRowSize(), src->GetHeight());
    }

    fsi.lCurrentSourceFrame = fsi.lCurrentFrame = n;
    fsi.lDestFrameMS = fsi.lSourceFrameMS = MulDiv(n, fsi.lMicrosecsPerFrame, 1000);

    fd->runProc(&fa, &g_filterFuncs);

    if (in_preroll) {
      return 0;
    } else {
      PVideoFrame _dst = env->NewVideoFrame(vi);
      env->BitBlt(_dst->GetWritePtr(), _dst->GetPitch(), (dst?dst:src)->GetReadPtr(),
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

  static int ConvertArgs(const AVSValue* args, CScriptValue* sylia_args, CScriptValueStringHelper* sylia_args_string_helper, int count) {
    for (int i=0; i<count; ++i) {
      if (args[i].IsInt()) {
        sylia_args[i] = args[i].AsInt();
      } else if (args[i].IsFloat()) { // new from 160420 double support
        sylia_args[i] = args[i].AsFloat();
      } else if (args[i].IsString()) {
        // Oops, where can we put the pointer to pointer in x64? no place in CScriptValue struct
        // helper class/struct needed.
        sylia_args_string_helper[i].lpVoid = (void*)args[i].AsString();
        sylia_args[i] = (char**)&sylia_args_string_helper[i].lpVoid;
        /* original, works only for 32 bit
        sylia_args[i].lpVoid = (void*)args[i].AsString();
        sylia_args[i] = (char**)&sylia_args[i].lpVoid;
        */
      } else if (args[i].IsArray()) {
        return i+ConvertArgs(&args[i][0], sylia_args+i, sylia_args_string_helper+i, args[i].ArraySize()); 
      } else {
        return -1000;
      }
    }
    return count;
  }

  void InvokeSyliaConfigFunction(FilterDefinition* fd, AVSValue args, IScriptEnvironment* env) {
    if (fd->script_obj && fd->script_obj->func_list && args.ArraySize() > 1) {
      for (ScriptFunctionDef* i = fd->script_obj->func_list; i->arg_list; i++) {
        const char* p = i->arg_list; // p: original virtualdub param list e.g. 0ddddddddd
        int j;
        for (j=1; j<args.ArraySize(); j++) {
          if (p[j] == 'i' && args[j].IsInt()) continue;
          else if (p[j] == 'l' && args[j].IsInt()) continue;    // param long is only Int in avs
          else if (p[j] == 'd' && args[j].IsFloat()) continue;  // 160420 type double support
          else if (p[j] == 's' && args[j].IsString()) continue;
          else if (p[j] == '.' && args[j].IsArray()) continue;
          else break;
        }
        if (j == args.ArraySize() && p[j] == 0) {
          // match
          MyScriptInterpreter si(env);
          CScriptValue sylia_args[30];
          CScriptValueStringHelper sylia_args_string_helper[30]; // helper class. x64 char ** helper did not fit into CScriptValue class size 
          int sylia_arg_count = ConvertArgs(&args[1], sylia_args, sylia_args_string_helper, args.ArraySize()-1);
          if (sylia_arg_count < 0)
            env->ThrowError("VirtualdubFilterProxy: arguments (after first) must be integers, double and strings only"); // 160420 double
          i->func_ptr(&si, &fa, sylia_args, sylia_arg_count);
          return;
        }
      }
      env->ThrowError("VirtualdubFilterProxy: no matching config function (this shouldn't happen)");
    }
  }

  ~VirtualdubFilterProxy() {
    CallEndProc();
	FreeFilterModule(fdl->fm);
    if (vbSrc.hdc)
      ReleaseDC(NULL, vbSrc.hdc);
  }

  void __cdecl FreeFilterModule(FilterModule* fm) {
	  for (FilterDefinitionList* fdl = fm->fdl; fdl; fdl = fdl->fdl) {
		  delete fdl->fd;
		  fdl->fd = 0;
	  }

	  fm->deinitProc(fm, &g_filterFuncs);
	  FreeLibrary(fm->hInstModule);
	  if (fm->prev)
		  fm->prev->next = fm->next;
	  if (fm->next)
		  fm->next->prev = fm->prev;
	  delete fm;
  }

  static AVSValue __cdecl Create(AVSValue args, void* user_data, IScriptEnvironment* env) {
    FilterDefinitionList* fdl = (FilterDefinitionList*)user_data;
    return new VirtualdubFilterProxy(args[0].AsClip(), fdl, args, env);
  }
};


FilterDefinition *VDcall FilterAdd(FilterModule *fm, FilterDefinition *pfd, int fd_len) {
  FilterDefinition *fd = new(std::nothrow) FilterDefinition;

  FilterDefinitionList _fdl(fm, fd);
  FilterDefinitionList *fdl = (FilterDefinitionList*)fm->env->SaveString((const char*)&_fdl, sizeof(_fdl));
  fm->fdl = fdl;

  if (fd) {
    memcpy(fd, pfd, min(size_t(fd_len), sizeof(FilterDefinition)));
    fd->module  = fm;
    fd->prev  = NULL;
    fd->next  = NULL;
  }

  const int MAX_PARAMS = 64;
  char converted_paramlist[MAX_PARAMS+1];

  fm->env->AddFunction(fm->avisynth_function_name, "c", VirtualdubFilterProxy::Create, fdl);
  if (fd->script_obj && fd->script_obj->func_list) {
    for (ScriptFunctionDef* i = fd->script_obj->func_list; i->arg_list; i++) {
      // avisynth does not know 'd'ouble or 'l'ong
      // let's fake them to 'f'loat and 'i'nt for avisynth
      char *p_src = i->arg_list + 1;
      char *p_target = converted_paramlist;
      char ch;
      while(ch = *p_src++ && (p_target-converted_paramlist)<MAX_PARAMS) {
        if (ch == 'd') ch = 'f';
        else if (ch == 'l') ch = 'i';
        *p_target++ = ch;
      }
      *p_target = '\0';

      const char* params = fm->env->Sprintf("c%s%s", converted_paramlist, strchr(i->arg_list+1, '.') ? "*" : "");
      // put * if . found

      fm->env->AddFunction(fm->avisynth_function_name, params, VirtualdubFilterProxy::Create, fdl);
    }
  }

  return fd;
}


AVSValue __cdecl LoadVirtualdubPlugin(AVSValue args, void*, IScriptEnvironment* env) {
  const char* const szModule = args[0].AsString();
  const char* const avisynth_function_name = args[1].AsString();
  const int preroll = args[2].AsInt(0);

  HMODULE hmodule = LoadLibrary(szModule);
  if (!hmodule)
    env->ThrowError("LoadVirtualdubPlugin: Error opening \"%s\", error=0x%x", szModule, GetLastError());

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
  fm->fdl = 0;

  int ver_hi = VIRTUALDUB_FILTERDEF_VERSION;
  int ver_lo = VIRTUALDUB_FILTERDEF_COMPATIBLE;
  if (fm->initProc(fm, &g_filterFuncs, ver_hi, ver_lo)) {
    // Neuter any AVS functions that may have been created
    for (FilterDefinitionList* fdl = fm->fdl; fdl; fdl = fdl->fdl) {
      delete fdl->fd;
      fdl->fd = 0;
    }
    FreeLibrary(hmodule);
    delete fm;
    env->ThrowError("LoadVirtualdubPlugin: Error initializing module \"%s\"", szModule);
  }

  if (fm->next)
    fm->next->prev = fm;

  env->SetGlobalVar("$LoadVirtualdubPlugin$", (const char*)fm);

  return AVSValue();
}

const AVS_Linkage * AVS_linkage = 0;
extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit3(IScriptEnvironment* env, const AVS_Linkage* const vectors) {
	AVS_linkage = vectors;

  // clip, base filename, start, end, image format/extension, info
  env->AddFunction("LoadVirtualdubPlugin", "ss[preroll]i", LoadVirtualdubPlugin, 0);

  return "`LoadVirtualdubPlugin' Allows to load and use filters written for VirtualDub.";
}
