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

// TCPDeliver (c) 2004 by Klaus Post

#ifndef TCP_Compression_h
#define TCP_Compression_h

#define ZLIB_WINAPI

#include "lzo/include/lzoconf.h"
#include "lzo/include/lzo1x.h"
#include "huffman.h"
#include "TCPCommon.h"
#include "avisynth.h"
#include <malloc.h>
#include "zlib/include/zlib.h" 

/*******************************************************************************
  This is a generic compression class for implementing different types of
  compression.

  Each compression type has it's own class and TCPCompression as it's 
  superclass.

  Basicly for compression and decompression you:

  - Create an instance of the compression class.
  - Call CompressImage or DeCompressImage with your data.
  - The result is placed in the "dst" pointer.
  - The number of bytes in the output is returned by the function.
  - If "inplace" is false you must free the dst data using "_aligned_free".
    - otherwise dst is the same as your source.

  You can get a unique ID of the compression type, by reading "compression_type".
 ******************************************************************************/

class TCPCompression {
public:
  TCPCompression() { 
    dst = 0;
    compression_type = ServerFrameInfo::COMPRESSION_NONE;
  }
  virtual ~TCPCompression(void) {};

  virtual int CompressImage(BYTE* image, int rowsize, int h, int pitch);  // returns new size
  virtual int DeCompressImage(BYTE* image, int rowsize, int h, int pitch, int data_size); // returns new size


  BYTE* dst;      // Must always be deallocated using _aligned_free().
  int compression_type;
  bool inplace;   // Do NOT free dst, when true.
};


class PredictDownLZO : public TCPCompression {
public:
  PredictDownLZO();
  virtual ~PredictDownLZO(void);

  int CompressImage(BYTE* image, int rowsize, int h, int pitch);
  int DeCompressImage(BYTE* image, int rowsize, int h, int pitch, int data_size);
private:
  lzo_bytep wrkmem; 

};

class PredictDownHuffman : public TCPCompression {
public:
  PredictDownHuffman();
  virtual ~PredictDownHuffman(void);

  int CompressImage(BYTE* image, int rowsize, int h, int pitch);
  int DeCompressImage(BYTE* image, int rowsize, int h, int pitch, int data_size);
};

class PredictDownGZip : public TCPCompression {
public:
  PredictDownGZip();
  virtual ~PredictDownGZip(void);

  int CompressImage(BYTE* image, int rowsize, int h, int pitch);
  int DeCompressImage(BYTE* image, int rowsize, int h, int pitch, int data_size);
private:
  z_stream_s *z;
};

#endif