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

#ifndef __Text_overlay_H__
#define __Text_overlay_H__

#include "../internal.h"
#include "../convert/convert.h"


/********************************************************************
********************************************************************/



class Antialiaser 
/**
  * Helper class to anti-alias text
 **/
{  
public:
  Antialiaser(int width, int height, const char fontname[], int size);
  virtual ~Antialiaser();
  HDC GetDC();
  
  void Apply(const VideoInfo& vi, PVideoFrame* frame, int pitch, int textcolor, int halocolor);
  void ApplyYV12(BYTE* buf, int pitch, int textcolor, int halocolor, int UVpitch,BYTE* bufV,BYTE* bufU);
  void ApplyYUY2(BYTE* buf, int pitch, int textcolor, int halocolor);
  void ApplyRGB24(BYTE* buf, int pitch, int textcolor, int halocolor);
  void ApplyRGB32(BYTE* buf, int pitch, int textcolor, int halocolor);  

private:
  const int w, h;
  HDC hdcAntialias;
  HBITMAP hbmAntialias;
  void* lpAntialiasBits;
  HFONT hfontDefault;
  HBITMAP hbmDefault;
  char* alpha_bits;
  bool dirty;

  void GetAlphaRect();
};



class ShowFrameNumber : public GenericVideoFilter 
/**
  * Class to display frame number on a video clip
 **/
{  
public:
  ShowFrameNumber(PClip _child, bool _scroll);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);

private:
  Antialiaser antialiaser;
  bool scroll;
};



class ShowSMPTE : public GenericVideoFilter 
/**
  * Class to display SMPTE codes on a video clip
 **/
{
public:
  ShowSMPTE(PClip _child, double _rate, IScriptEnvironment* env);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);


private:
  Antialiaser antialiaser;
  char rate;
  bool dropframe;
};




class Subtitle : public GenericVideoFilter 
/**
  * Subtitle creation class
 **/
{
public:
  Subtitle( PClip _child, const char _text[], int _x, int _y, int _firstframe, int _lastframe, 
            const char _fontname[], int _size, int _textcolor, int _halocolor, int _align, int _spc );
  virtual ~Subtitle(void);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
  
  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);  

private:
  void InitAntialiaser(void);
  
  const int x, y, firstframe, lastframe, size;
  /*const*/ int textcolor, halocolor, align, spc;
  const char* const fontname;
  const char* const text;
  Antialiaser* antialiaser;  
};


class FilterInfo : public GenericVideoFilter 
/**
  * FilterInfo creation class
 **/
{
public:
  FilterInfo( PClip _child);
  virtual ~FilterInfo(void);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
  
  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);  

private:
  HDC hdcAntialias;
  Antialiaser antialiaser;  
};


class Compare : public GenericVideoFilter
/**
  * Compare two clips frame by frame and display fidelity measurements (with optionnal logging to file)
 **/
{
public:
  Compare(PClip _child1, PClip _child2, const char* channels, const char *fname, bool _show_graph, IScriptEnvironment* env);
  ~Compare();
  static AVSValue __cdecl Create(AVSValue args, void* , IScriptEnvironment* env);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
private:
  Antialiaser antialiaser;
  PClip child2;
  DWORD mask;
  int masked_bytes;
  FILE* log;
  int* psnrs;
  bool show_graph;
  double PSNR_min, PSNR_tot, PSNR_max;
  double MAD_min, MAD_tot, MAD_max;
  double MD_min, MD_tot, MD_max;
  double bytecount_overall, SSD_overall;
  int framecount;
};



/**** Helper functions ****/

void ApplyMessage( PVideoFrame* frame, const VideoInfo& vi, const char* message, int size, 
                   int textcolor, int halocolor, int bgcolor, IScriptEnvironment* env );

bool GetTextBoundingBox( const char* text, const char* fontname, int size, bool bold, 
                         bool italic, int align, int* width, int* height );




/**** Inline helper functions ****/

inline static HFONT LoadFont(const char name[], int size, bool bold, bool italic) 
{
  return CreateFont( size, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL,
                     italic, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                     CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE | DEFAULT_PITCH, name );
}

inline char* MyStrdup(const char* s) {
  return lstrcpy(new char[(lstrlen(s)+1)], s);
}



#endif  // __Text_overlay_H__