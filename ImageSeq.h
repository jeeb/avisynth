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

#ifndef __Image_Sequence_H__
#define __Image_Sequence_H__

#include<iostream>
#include<iomanip>
#include<sstream>
#include<fstream>
using namespace std;

#include "internal.h"
#include "text-overlay.h"
#include "AvsImage.h"

#include "distrib\il\il.h"
#include "distrib\il\ilu.h"


class ImageWriter : public GenericVideoFilter 
/**
  * Class to write video as a sequence of images
 **/
{  
public:
  ImageWriter(PClip _child, const char * _base_name, const int start, const int end, 
              const char * _ext, const int compression);
  ~ImageWriter();

  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
  void fileWrite(ostream & file, const BYTE * srcPtr, int pitch, int row_size, int height);

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);

private:
  Antialiaser antialiaser;
  const char * base_name;
  const char * ext;
  const int start;
  const int end;
  const int compression;

  BITMAPFILEHEADER fileHeader;
  BITMAPINFOHEADER infoHeader;
};


class ImageReader : public IClip
/**
  * Class to read image sequences into video buffers
 **/
{
public:
  ImageReader(const char * _base_name, const char * _ext);
  ~ImageReader();

  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
  
  void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) {
    memset(buf, 0, vi.BytesFromAudioSamples(count));
  }
  const VideoInfo& __stdcall GetVideoInfo() { return vi; }
  bool __stdcall GetParity(int n) { return vi.IsFieldBased() ? (n&1) : false; }
  void __stdcall SetCacheHints(int cachehints,int frame_range) { };
  
  
  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);

private:
  const char * base_name;
  const char * ext;
    
  VideoInfo vi;
};


#endif // __Image_Sequence_H__