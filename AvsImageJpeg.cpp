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




//////  JPEG  ///////

const int JPEG_DEFAULT_QUALITY = 70;

// I'm of half a mind to make this 1, since iostreams already do
// buffered I/O...
const size_t OUTPUT_BUF_SIZE = 8192;



img_JPEG::img_JPEG(const VideoInfo & _vi, const int _q) 
  : AvsImage(_vi)
{
  if (_q) {
    quality = _q;
  } else {
    quality = JPEG_DEFAULT_QUALITY;
  }

  // init error handler & jpeg object
  cinfo.err = jpeg_std_error(&jerr);  
  jpeg_create_compress(&cinfo);  

  // set image info
  cinfo.image_width = vi.width;
  cinfo.image_height = vi.height;
  // color channels, i.e. RGB24 -> 3
  cinfo.input_components = vi.RowSize() / (vi.width * (vi.BitsPerPixel() >> 3));
  cinfo.in_color_space = JCS_RGB;

  // compression info
  jpeg_set_defaults(&cinfo);
  jpeg_set_quality(&cinfo, quality, TRUE);

  row_pointers = new BYTE* [vi.height];
}


img_JPEG::~img_JPEG()
{
  jpeg_destroy((jpeg_common_struct*) &cinfo);
  delete [] row_pointers;
}



void img_JPEG::compress(ostream & bufWriter, const BYTE * srcPtr, const int pitch, IScriptEnvironment * env)
{
  // check some things
  if (!vi.IsRGB24())
    env->ThrowError("JPEG: requires RGB24 input");

  if (row_pointers == NULL)
  {     
    jpeg_destroy((jpeg_common_struct*) &cinfo);
    env->ThrowError("JPEG: out of memory");
  }

  // set up custom ostream writer
  jpeg_ostream_dest_init(&cinfo, &bufWriter, env);
  
  // set up row pointers
  for(UINT i = 0, j = vi.height-1; i < vi.height; ++i, --j)
      row_pointers[i] = (BYTE *) srcPtr + j*pitch; // RGB is upside-down


  // do compress
  jpeg_start_compress(&cinfo, TRUE);    

  if (jpeg_write_scanlines(&cinfo, row_pointers, vi.height) != vi.height)
  {
    jpeg_destroy((jpeg_common_struct*) &cinfo);
    env->ThrowError("JPEG: could not write scanlines");
  }

  jpeg_finish_compress(&cinfo);
    
}


void img_JPEG::decompress(const istream & bufReader, BYTE * dstPtr)
{
}



// Custom handler for writing via ostream
void jpeg_ostream_dest_init (j_compress_ptr cinfo, ostream * bufWriter, IScriptEnvironment * env)
{
  my_dest_ptr dest;

  // allocate destination manager via jpeg routine (will be auto-freed)
  if (cinfo->dest == NULL) {	/* first time for this JPEG object? */
    cinfo->dest = (struct jpeg_destination_mgr *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
				  sizeof(my_destination_mgr));
  }

  dest = (my_dest_ptr) cinfo->dest;
  dest->pub.init_destination = init_destination;
  dest->pub.empty_output_buffer = empty_output_buffer;
  dest->pub.term_destination = term_destination;
  dest->bufWriter = bufWriter;
  dest->env = env;
}



void init_destination (j_compress_ptr cinfo)
{
  my_dest_ptr dest = (my_dest_ptr) cinfo->dest;

  // allocate the output buffer via jpeg routine (will be auto-freed)
  dest->buffer = (JOCTET *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				  OUTPUT_BUF_SIZE * sizeof(JOCTET));

  dest->pub.next_output_byte = dest->buffer;
  dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
}



boolean empty_output_buffer (j_compress_ptr cinfo)
{
  my_dest_ptr dest = (my_dest_ptr) cinfo->dest;  
  dest->bufWriter->write( reinterpret_cast<char *> (dest->buffer), OUTPUT_BUF_SIZE );
  if (!(dest->bufWriter))
    dest->env->ThrowError("JPEG: write error");

  // reset
  dest->pub.next_output_byte = dest->buffer;
  dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
  return TRUE;
}



void term_destination (j_compress_ptr cinfo)
{
  my_dest_ptr dest = (my_dest_ptr) cinfo->dest;
  size_t datacount = OUTPUT_BUF_SIZE - dest->pub.free_in_buffer;

  /* Write any data remaining in the buffer */
  if (datacount > 0) 
    dest->bufWriter->write( reinterpret_cast<char *> (dest->buffer), datacount);
  
  dest->bufWriter->flush();  
}