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



#include "ImageSeq.h"




/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

AVSFunction Image_filters[] = {
  { "ImageSequence", "c[file]s[type]s", ImageSequence::Create }, // clip, filename
  { 0 }
};



/**********************************
 *******   Image Sequence    ******
 *********************************/

ImageSequence::ImageSequence(PClip _child, const char * _base_name, const char * _ext)
 : GenericVideoFilter(_child), antialiaser(vi.width, vi.height, "Arial", 192),
   base_name(_base_name), ext(_ext) {}


PVideoFrame ImageSequence::GetFrame(int n, IScriptEnvironment* env) 
{
  PVideoFrame frame = child->GetFrame(n, env);
  env->MakeWritable(&frame);

  // construct filename
  ostringstream fn_oss;
  fn_oss << base_name << setfill('0') << setw(6) << n << '.' << ext;
  string filename = fn_oss.str();
  
  // construct file & image objects appropriately
  ofstream file(filename.c_str(), ios::out | ios::trunc | ios::binary);
  AvsImage * image;
  if (!lstrcmpi(ext, "bmp")) {
    image = new img_BMP(vi);
  }

  // do it
  image->compress(file, frame->GetReadPtr(), frame->GetPitch());
  
  
  // overlay on video output: progress indicator
  ostringstream text;
  text << "Frame " << n << " written to: " << filename;

  HDC hdc = antialiaser.GetDC();
  RECT r = { 32, 16, min(3440,vi.width*8), 768*2 };
  DrawText(hdc, text.str().c_str(), -1, &r, 0);
  GdiFlush();

  antialiaser.Apply(vi, &frame, frame->GetPitch(),
    vi.IsYUV() ? 0xD21092 : 0xFFFF00, vi.IsYUV() ? 0x108080 : 0);


  // cleanup
  file.close();
  delete image;

  return frame;
}


AVSValue __cdecl ImageSequence::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  return new ImageSequence(args[0].AsClip(), args[1].AsString(""), args[2].AsString("bmp"));
}