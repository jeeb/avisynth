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



#include "AvsImage.h"




//////  PNG  ///////

#define PNG_DEFAULT_COMPRESSION 3


img_PNG::img_PNG(const VideoInfo & _vi, const int _c)
 : AvsImage(_vi)
{
  if (_c) {
    compression = _c;
  } else {
    compression = PNG_DEFAULT_COMPRESSION;
  }
  
  // create main png struct
  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

  // create info struct
  info_ptr = png_create_info_struct(png_ptr);

  // Set image info
  int bits_per_channel = vi.BitsPerPixel() / (vi.RowSize() / vi.width);
  png_set_IHDR(png_ptr, info_ptr, vi.width, vi.height, bits_per_channel, PNG_COLOR_TYPE_RGB_ALPHA,
    PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

  // Set compression level
  png_set_compression_level(png_ptr, compression);

  // Swap pixels from Avisynth's BGRA -> libpng's ARGB
  png_set_bgr(png_ptr);

  // Swap bytes of 16-bit pixels from Intel's little-endian -> libpng's big-endian
  if (bits_per_channel > 8)
    png_set_packswap(png_ptr);

  // Allocate space for row pointers
  row_pointers = new BYTE* [vi.height];   
}


img_PNG::~img_PNG()
{
  png_destroy_write_struct(&png_ptr, &info_ptr);
  delete [] row_pointers;
}


void img_PNG::compress(ostream & bufWriter, const BYTE * srcPtr, const int pitch, IScriptEnvironment * env)
{
  // check some things
  if (!vi.IsRGB32())
    env->ThrowError("PNG: requires RGB32 input");

  if (png_ptr == NULL)   
    env->ThrowError("PNG: could not initialize write_struct");      
    
  if (info_ptr == NULL)
  {      
    png_destroy_write_struct(&png_ptr,  png_infopp_NULL);
    env->ThrowError("PNG: could not initialize info_struct");   
  }
   
  if (setjmp(png_jmpbuf(png_ptr)))
  {
    // If we get here, we had a problem reading the file
    png_destroy_write_struct(&png_ptr, &info_ptr);
    env->ThrowError("PNG: error reading the framebuffer");   
  }

  if (row_pointers == NULL)
  {     
    png_destroy_write_struct(&png_ptr, &info_ptr);
    env->ThrowError("PNG: out of memory");   
  }   


  // Override write I/O functions to use ostream
  ostream_struct * my_ostream = new ostream_struct;
  my_ostream->bufWriter = &bufWriter;
  png_set_write_fn(png_ptr, my_ostream, writePng, flushPng);

  // Write the file header
  png_write_info(png_ptr, info_ptr);  
   
  // Prepare row info for main IO operation    
  for(UINT i = 0, j = vi.height-1; i < vi.height; ++i, --j)
    row_pointers[i] = (BYTE *) srcPtr + j*pitch; // RGB is upside-down

  // Do yo thing
  png_write_image(png_ptr, row_pointers);   
  png_write_end(png_ptr, info_ptr); 
  
  delete my_ostream;
}



void img_PNG::decompress(const istream & bufReader, BYTE * dstPtr, IScriptEnvironment * env)
{
}



void PNGAPI writePng(png_structp png_ptr, png_bytep data, 
              png_size_t length)
{
  ostream_struct * my_ostream = reinterpret_cast<ostream_struct *> ( png_get_io_ptr(png_ptr) );
  my_ostream->bufWriter->write( reinterpret_cast<const char *> ( data ), length);
}



void PNGAPI flushPng(png_structp png_ptr)
{
  ostream_struct * my_ostream = reinterpret_cast<ostream_struct *> ( png_get_io_ptr(png_ptr) );
  my_ostream->bufWriter->flush();
}