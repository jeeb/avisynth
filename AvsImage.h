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


#ifndef __AVSIMAGE_H__
#define __AVSIMAGE_H__

#include<iostream>
using namespace std;

#include "internal.h"

#include "..\libpng\png.h"
#include "..\libjpeg\include\jpeglib.h"


// abstract base class for reading/writing images to/from Avisynth buffers
class AvsImage
{
public:
  AvsImage(const VideoInfo & _vi) : vi(_vi) {};
  virtual ~AvsImage() {};
  
  virtual void compress(ostream & bufWriter, const BYTE * srcPtr, const int pitch, IScriptEnvironment * env) = 0;
  virtual void decompress(const istream & bufReader, BYTE * dstPtr) = 0;

protected:
  VideoInfo vi;
};



///// Compressed formats /////

class img_PNG : public AvsImage
{
public:  
  img_PNG(const VideoInfo & _vi, const int compression);
  virtual ~img_PNG();
  
  void compress(ostream & bufWriter, const BYTE * srcPtr, const int pitch, IScriptEnvironment * env);
  void decompress(const istream & bufReader, BYTE * dstPtr);
private:
  int compression;  // 0..9

  png_structp png_ptr;
  png_infop info_ptr; 

  BYTE ** row_pointers;
};


class img_JPEG : public AvsImage
{
public:  
  img_JPEG(const VideoInfo & _vi, const int quality);
  virtual ~img_JPEG();

  void compress(ostream & bufWriter, const BYTE * srcPtr, const int pitch, IScriptEnvironment * env);
  void decompress(const istream & bufReader, BYTE * dstPtr);
private:
  int quality;      // 0..100

  jpeg_compress_struct cinfo;
  jpeg_error_mgr jerr;
  
  BYTE ** row_pointers;
};




///// BMP formats /////

// class for simple formats written as BMPs
class img_BMP : public AvsImage
{
public:
  img_BMP(const VideoInfo & _vi);
  virtual ~img_BMP() {};

  void compress(ostream & bufWriter, const BYTE * srcPtr, const int pitch, IScriptEnvironment * env);
  void decompress(const istream & bufReader, BYTE * dstPtr);

protected:
  BITMAPFILEHEADER fileHeader;
  BITMAPINFOHEADER infoHeader;
};



// free functions for libPNG callbacks
void __cdecl writePng(png_structp png_ptr, png_bytep data, 
                png_size_t length);
void __cdecl flushPng(png_structp png_ptr);



// free functions for libJPEG callbacks
void jpeg_ostream_dest_init (j_compress_ptr cinfo, ostream * bufWriter, IScriptEnvironment * env);
void __cdecl init_destination (j_compress_ptr cinfo);
boolean __cdecl empty_output_buffer (j_compress_ptr cinfo);
void __cdecl term_destination (j_compress_ptr cinfo);

// custom data destination object for libJPEG -> ostream output
typedef struct {
  struct jpeg_destination_mgr pub; /* public fields */

  ostream * bufWriter;		/* target stream */
  JOCTET * buffer;		/* start of buffer */
  
  IScriptEnvironment * env;

} my_destination_mgr;

typedef my_destination_mgr * my_dest_ptr;




#endif // __AVSIMAGE_H__