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

#include "ImageSeq.h"




/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

AVSFunction Image_filters[] = {
  { "ImageWriter", "c[file]s[start]i[end]i[type]s[compression]i", ImageWriter::Create }, // clip, base filename, image format, compression level
  { "ImageReader", "[file]s[type]s", ImageReader::Create }, // base filename, image format
  { 0 }
};



/*****************************
 *******   Image Writer ******
 ****************************/

ImageWriter::ImageWriter(PClip _child, const char * _base_name, const int _start, const int _end,
                         const char * _ext, const int _compression)
 : GenericVideoFilter(_child), antialiaser(vi.width, vi.height, "Arial", 192), base_name(_base_name), start(_start),
    end(_end), ext(_ext), compression(_compression) 
{  
  if (!lstrcmpi(ext, "bmp")) 
  {
    // construct file header  
    fileHeader.bfType = ('M' << 8) + 'B'; // I hate little-endian
    fileHeader.bfSize = vi.BMPSize(); // includes 4-byte padding
    fileHeader.bfReserved1 = 0;
    fileHeader.bfReserved2 = 0;
    fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    fileHeader.bfSize += fileHeader.bfOffBits;
  
    // construct info header
    infoHeader.biSize = sizeof(BITMAPINFOHEADER);
    infoHeader.biWidth = vi.width;
    infoHeader.biHeight = vi.height;
    infoHeader.biPlanes = vi.IsPlanar() ? 3 : 1;
    infoHeader.biBitCount = vi.BitsPerPixel();
    infoHeader.biCompression = 0;
    infoHeader.biSizeImage = fileHeader.bfSize - fileHeader.bfOffBits;
    infoHeader.biXPelsPerMeter = 0;
    infoHeader.biYPelsPerMeter = 0;
    infoHeader.biClrUsed = 0;
    infoHeader.biClrImportant = 0;
  }
  else {
    
  } 
}


ImageWriter::~ImageWriter()
{
  
}



PVideoFrame ImageWriter::GetFrame(int n, IScriptEnvironment* env) 
{
  PVideoFrame frame = child->GetFrame(n, env);
  
  // check bounds (where end=0 implies no upper bound)
  if (n < start || (end > 0 && n > end) )
    return frame;  

  // construct filename
  ostringstream fn_oss;
  fn_oss << base_name << setfill('0') << setw(6) << n << '.' << ext;
  string filename = fn_oss.str();

  if (!lstrcmpi(ext, "bmp"))
  {
    // initialize file object
    ofstream file(filename.c_str(), ios::out | ios::trunc | ios::binary);  
    if (!file)
      env->ThrowError("ImageWriter: could not create file");

    // write headers
    file.write(reinterpret_cast<const char *>( &fileHeader ), sizeof(BITMAPFILEHEADER));
    file.write(reinterpret_cast<const char *>( &infoHeader ), sizeof(BITMAPINFOHEADER));
    
    // write raster
    const BYTE * srcPtr = frame->GetReadPtr();
    int pitch = frame->GetPitch(); 
    int row_size = frame->GetRowSize();
    int height = frame->GetHeight();    
    
    fileWrite(file, srcPtr, pitch, row_size, height);

    if (vi.IsYV12())
    {
      srcPtr = frame->GetReadPtr(PLANAR_U);
      pitch = frame->GetPitch(PLANAR_U); 
      row_size = frame->GetRowSize(PLANAR_U);
      height = frame->GetHeight(PLANAR_U);
      fileWrite(file, srcPtr, pitch, row_size, height);

      srcPtr = frame->GetReadPtr(PLANAR_V);
      fileWrite(file, srcPtr, pitch, row_size, height);
    }
    file.close();
  }
  else {
    // Check colorspace
    if (!vi.IsRGB())
      env->ThrowError("ImageWrite: DevIL requires RGB input");

    // Set up DevIL
    ilInit();
    ILuint myImage;
    ilGenImages(1, &myImage);
    ilBindImage(myImage);
    
    // Set image parameters
    ilTexImage(vi.width, vi.height, 1, vi.BitsPerPixel() / 8, vi.IsRGB24 ? IL_BGR : IL_BGRA, IL_UNSIGNED_BYTE, NULL);
    ilSetData((void *) frame->GetReadPtr());

    // Save to disk (format automatically inferred from extension)
    ilSaveImage(const_cast<char * const> (filename.c_str()) );

    // Get errors if any
    ILenum err = ilGetError();
    if (err != IL_NO_ERROR)
      env->ThrowError("ImageWriter: error encountered in DevIL library");
    
    // Clean up
    ilDeleteImages(1, &myImage);
  }  
    
  
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

  
  return frame;
}


void ImageWriter::fileWrite(ostream & file, const BYTE * srcPtr, int pitch, int row_size, int height)
{
  int dummy = 0;      
  int padding = (4 - (row_size % 4)) % 4;

  for(UINT i=0; i < height; ++i)
  {
    file.write(reinterpret_cast<const char *>( srcPtr ), row_size);
    file.write(reinterpret_cast<char *>( &dummy ), padding); // pad with 0's to mod-4
    srcPtr += pitch;
  }
}


AVSValue __cdecl ImageWriter::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  return new ImageWriter(args[0].AsClip(), args[1].AsString("c:\\"), args[2].AsInt(0), args[3].AsInt(0),
                         args[4].AsString("png"), args[5].AsInt(0));
}




/*****************************
 *******   Image Reader ******
 ****************************/

ImageReader::ImageReader(const char * _base_name, const char * _ext)
 : base_name(_base_name), ext(_ext)
{  
  if (!lstrcmpi(ext, "bmp")) 
  {
    
  
  }
}


ImageReader::~ImageReader()
{

}



PVideoFrame ImageReader::GetFrame(int n, IScriptEnvironment* env) 
{

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

   
  // cleanup
  file.close();
  
  return frame;
}


AVSValue __cdecl ImageReader::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  return new ImageReader(args[1].AsString("c:\\"), args[2].AsString("png"));
}