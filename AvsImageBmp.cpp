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



///// General BMP I/O handler /////

img_BMP::img_BMP (const VideoInfo & _vi) 
  : AvsImage(_vi)
{}


void img_BMP::compress(ostream & bufWriter, const BYTE * srcPtr, const int pitch) 
{  
  // construct file header  
  fileHeader.bfType = ('M' << 8) + 'B'; // little-endian sucks my balls
  fileHeader.bfSize = vi.height * (vi.RowSize() + (vi.RowSize() % 4)); // don't forget padding
  fileHeader.bfReserved1 = 0;
  fileHeader.bfReserved2 = 0;
  fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
  fileHeader.bfSize += fileHeader.bfOffBits;

  // construct info header
  infoHeader.biSize = sizeof(BITMAPINFOHEADER);
  infoHeader.biWidth = vi.width;
  infoHeader.biHeight = vi.height;
  infoHeader.biPlanes = 1;
  infoHeader.biBitCount = vi.BitsPerPixel();
  infoHeader.biCompression = 0;
  infoHeader.biSizeImage = fileHeader.bfSize - fileHeader.bfOffBits;
  infoHeader.biXPelsPerMeter = 0;
  infoHeader.biYPelsPerMeter = 0;
  infoHeader.biClrUsed = 0;
  infoHeader.biClrImportant = 0;

  // write headers
  bufWriter.write(reinterpret_cast<char *>( &fileHeader ), sizeof(BITMAPFILEHEADER));
  bufWriter.write(reinterpret_cast<char *>( &infoHeader ), sizeof(BITMAPINFOHEADER));
    
  // write raster
  int dummy = 0;  
  for(UINT i=0; i < vi.height; ++i)
  {
    bufWriter.write(reinterpret_cast<const char *>( srcPtr ), vi.RowSize());
    bufWriter.write(reinterpret_cast<char *>( &dummy ), vi.RowSize() % 4); // pad with 0's
    srcPtr += pitch;
  }
  bufWriter.flush();

}


void img_BMP::decompress(const istream & bufReader, BYTE * dstPtr)
{
  return;
}
