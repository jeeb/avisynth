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
  { "ImageWriter", "c[file]s[type]s[compression]i", ImageWriter::Create }, // clip, base filename, image format, compression level
  { "ImageReader", "[file]s[type]s", ImageReader::Create }, // base filename, image format
  { 0 }
};



/*****************************
 *******   Image Writer ******
 ****************************/

ImageWriter::ImageWriter(PClip _child, const char * _base_name, const char * _ext, const int _compression)
 : GenericVideoFilter(_child), antialiaser(vi.width, vi.height, "Arial", 192),
   base_name(_base_name), ext(_ext), compression(_compression) 
{  
  if (!lstrcmpi(ext, "bmp")) 
  {
    image = new img_BMP(vi);
  } else if (!lstrcmpi(ext, "png")) 
  {
    image = new img_PNG(vi, compression);
  } else if (!lstrcmpi(ext, "jpeg")) 
  {
    image = new img_JPEG(vi, compression);
  }
}


ImageWriter::~ImageWriter()
{
  delete image;
}



PVideoFrame ImageWriter::GetFrame(int n, IScriptEnvironment* env) 
{
  // check some things
  if (vi.IsPlanar())
    env->ThrowError("ImageWriter: cannot export planar formats");
  
  if (image == NULL)
    env->ThrowError("ImageWriter: invalid format");
  

  // construct filename
  ostringstream fn_oss;
  fn_oss << base_name << setfill('0') << setw(6) << n << '.' << ext;
  string filename = fn_oss.str();

  
  // initialize file object
  ofstream file(filename.c_str(), ios::out | ios::trunc | ios::binary);  
  if (!file)
    env->ThrowError("ImageWriter: could not create file");

  
  // do it
  PVideoFrame frame = child->GetFrame(n, env);
  image->compress(file, frame->GetReadPtr(), frame->GetPitch(), env);
  
  
  // overlay on video output: progress indicator
  ostringstream text;
  text << "Frame " << n << " written to: " << filename;

  HDC hdc = antialiaser.GetDC();
  RECT r = { 32, 16, min(3440,vi.width*8), 768*2 };
  DrawText(hdc, text.str().c_str(), -1, &r, 0);
  GdiFlush();

  env->MakeWritable(&frame);
  antialiaser.Apply(vi, &frame, frame->GetPitch(),
    vi.IsYUV() ? 0xD21092 : 0xFFFF00, vi.IsYUV() ? 0x108080 : 0);

  // cleanup
  file.close();
  
  return frame;
}


AVSValue __cdecl ImageWriter::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  return new ImageWriter(args[0].AsClip(), args[1].AsString("c:\\"), args[2].AsString("png"), args[3].AsInt(0));
}




/*****************************
 *******   Image Reader ******
 ****************************/

ImageReader::ImageReader(const char * _base_name, const char * _ext)
 : base_name(_base_name), ext(_ext)
{  
  if (!lstrcmpi(ext, "bmp")) 
  {
    image = new img_BMP(vi);
  } else if (!lstrcmpi(ext, "png")) 
  {
    image = new img_PNG(vi, 0);
  } else if (!lstrcmpi(ext, "jpeg")) 
  {
    image = new img_JPEG(vi, 0);
  }
}


ImageReader::~ImageReader()
{
  delete image;
}



PVideoFrame ImageReader::GetFrame(int n, IScriptEnvironment* env) 
{
  // check some things  
  if (image == NULL)
    env->ThrowError("ImageReader: invalid format");
  

  // construct filename
  ostringstream fn_oss;
  fn_oss << base_name << setfill('0') << setw(6) << n << '.' << ext;
  string filename = fn_oss.str();

  
  // initialize file object
  ifstream file(filename.c_str(), ios::binary);  
  if (!file)
    env->ThrowError("ImageReader: could not open file");

  
  // do it
  PVideoFrame frame = env->NewVideoFrame(vi);
  image->decompress(file, frame->GetWritePtr(), env);
  
   
  // cleanup
  file.close();
  
  return frame;
}


AVSValue __cdecl ImageReader::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  return new ImageReader(args[1].AsString("c:\\"), args[2].AsString("png"));
}