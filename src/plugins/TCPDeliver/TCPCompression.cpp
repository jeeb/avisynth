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


#include "TCPCompression.h"


int TCPCompression::CompressImage(BYTE* image, int rowsize, int h, int pitch) {
  dst = image;
  inplace = true;
  return pitch*h;
}

int TCPCompression::DeCompressImage(BYTE* image, int rowsize, int h, int pitch, int data_size) {
  dst = image;
  inplace = true;
  return pitch*h;
}

PredictDownLZO::PredictDownLZO() {
  compression_type = ServerFrameInfo::COMPRESSION_DELTADOWN_LZO;
  wrkmem = (lzo_bytep) malloc(LZO1X_1_MEM_COMPRESS);
}

PredictDownLZO::~PredictDownLZO(void) {
  free(wrkmem);
}
/******************************
 * Downwards deltaencoded.
 ******************************/

int PredictDownLZO::CompressImage(BYTE* image, int rowsize, int h, int pitch) {
  // Pitch mod 16
  // Height > 2
  rowsize = (rowsize+15)&~15;
  inplace = false;    
  __asm {
    xor eax, eax        // x offset
    mov ecx, [pitch]
    mov edx, rowsize
xloopback:
    mov ebx, [h]
    mov esi, [image]    // src
    pxor mm4, mm4   // left
    pxor mm5, mm5   // left2
yloopback:
    movq mm0, [esi+eax]   // temp
    movq mm1, [esi+eax+8]   // temp
    movq mm2, mm0
    movq mm3, mm1
    psubb mm0, mm4  // left - temp
    psubb mm1, mm5  // left - temp
    movq [esi+eax], mm0
    movq [esi+eax+8], mm1
    movq mm4, mm2  // left = temp
    movq mm5, mm3  // left = temp
    add esi, ecx  // Next line
    dec ebx
    jnz yloopback

    add eax, 16
    cmp eax, edx
    jl xloopback

    emms
  }
  int in_size = pitch*h;
  int out_size = -1;
  dst = (BYTE*)_aligned_malloc(in_size + (in_size >>6) + 16 + 3, 16);
  lzo1x_1_compress(image, in_size ,(unsigned char *)dst, (unsigned int *)&out_size , wrkmem);
  _RPT2(0, "TCPCompression: Compressed %d bytes into %d bytes.\n", in_size, out_size);
  return out_size;
}
 
int PredictDownLZO::DeCompressImage(BYTE* image, int rowsize, int h, int pitch, int in_size) {
  // Pitch mod 16
  // Height > 2
  inplace = false;    
  unsigned int dst_size = pitch*h;
  dst = (BYTE*)_aligned_malloc(dst_size, 64);
  lzo1x_decompress(image, in_size, dst, &dst_size, wrkmem);

  if ((int)dst_size != pitch*h) {
    _RPT0(1,"TCPCompression: Size did NOT match");
  }
    
  rowsize = (rowsize+15)&~15;
  image = dst;
    
  __asm {
    xor eax, eax        // x offset
    mov ecx, [pitch]
    mov edx, rowsize
xloopback:
    mov ebx, [h]
    mov esi, [image]    // src
    pxor mm4, mm4   // left
    pxor mm5, mm5   // left2
yloopback:
    movq mm0, [esi+eax]   // src
    movq mm1, [esi+eax+8]   // src2
    paddb mm4, mm0  // left + src
    paddb mm5, mm1  // left + src
    movq [esi+eax], mm4
    movq [esi+eax+8], mm5
    add esi, ecx  // Next line
    dec ebx
    jnz yloopback

    add eax, 16
    cmp eax, edx
    jl xloopback

    emms
  }
  _RPT2(0, "TCPCompression: Decompressed %d bytes into %d bytes.\n", in_size, dst_size);
  return dst_size;
}



PredictDownHuffman::PredictDownHuffman() {
  compression_type = ServerFrameInfo::COMPRESSION_DELTADOWN_HUFFMAN;
}

PredictDownHuffman::~PredictDownHuffman(void) {
}
/******************************
 * Downwards deltaencoded.
 ******************************/

int PredictDownHuffman::CompressImage(BYTE* image, int rowsize, int h, int pitch) {
  // Pitch mod 16
  // Height > 2
  inplace = false;    
  rowsize = (rowsize+15)&~15;
    
  __asm {
    xor eax, eax        // x offset
    mov ecx, [pitch]
    mov edx, rowsize
xloopback:
    mov ebx, [h]
    mov esi, [image]    // src
    pxor mm4, mm4   // left
    pxor mm5, mm5   // left2
yloopback:
    movq mm0, [esi+eax]   // temp
    movq mm1, [esi+eax+8]   // temp
    movq mm2, mm0
    movq mm3, mm1
    psubb mm0, mm4  // left - temp
    psubb mm1, mm5  // left - temp
    movq [esi+eax], mm0
    movq [esi+eax+8], mm1
    movq mm4, mm2  // left = temp
    movq mm5, mm3  // left = temp
    add esi, ecx  // Next line
    dec ebx
    jnz yloopback

    add eax, 16
    cmp eax, edx
    jl xloopback

    emms
  }

  int in_size = pitch*h;
  unsigned int out_size = in_size*2;
  dst = (BYTE*)_aligned_malloc(out_size, 16);
  out_size = Huffman_Compress(image, dst, in_size );

  _RPT2(0, "TCPCompression: Compressed %d bytes into %d bytes.(Huffman)\n", in_size, out_size);
  return out_size;
}
 
int PredictDownHuffman::DeCompressImage(BYTE* image, int rowsize, int h, int pitch, int in_size) {
  // Pitch mod 16
  // Height > 2
  inplace = false;    
  unsigned int dst_size = pitch*h;
  dst = (BYTE*)_aligned_malloc(dst_size, 64);

  Huffman_Uncompress(image, dst, in_size, dst_size);

  rowsize = (rowsize+15)&~15;
  image = dst;
    
  __asm {
    xor eax, eax        // x offset
    mov ecx, [pitch]
    mov edx, rowsize
xloopback:
    mov ebx, [h]
    mov esi, [image]    // src
    pxor mm4, mm4   // left
    pxor mm5, mm5   // left2
yloopback:
    movq mm0, [esi+eax]   // src
    movq mm1, [esi+eax+8]   // src2
    paddb mm4, mm0  // left + src
    paddb mm5, mm1  // left + src
    movq [esi+eax], mm4
    movq [esi+eax+8], mm5
    add esi, ecx  // Next line
    dec ebx
    jnz yloopback

    add eax, 16
    cmp eax, edx
    jl xloopback

    emms
  }
  _RPT2(0, "TCPCompression: Decompressed %d bytes into %d bytes.(Huffmann)\n", in_size, dst_size);
  return dst_size;
}


PredictDownGZip::PredictDownGZip() {
  compression_type = ServerFrameInfo::COMPRESSION_DELTADOWN_GZIP;
  z = (z_stream_s*)malloc(sizeof(z_stream_s));
}

PredictDownGZip::~PredictDownGZip(void) {
  free(z);
}
/******************************
 * Downwards deltaencoded.
 ******************************/

int PredictDownGZip::CompressImage(BYTE* image, int rowsize, int h, int pitch) {
  // Pitch mod 16
  // Height > 2
  inplace = false;    
  rowsize = (rowsize+15)&~15;
    
  __asm {
    xor eax, eax        // x offset
    mov ecx, [pitch]
    mov edx, rowsize
xloopback:
    mov ebx, [h]
    mov esi, [image]    // src
    pxor mm4, mm4   // left
    pxor mm5, mm5   // left2
yloopback:
    movq mm0, [esi+eax]   // temp
    movq mm1, [esi+eax+8]   // temp
    movq mm2, mm0
    movq mm3, mm1
    psubb mm0, mm4  // left - temp
    psubb mm1, mm5  // left - temp
    movq [esi+eax], mm0
    movq [esi+eax+8], mm1
    movq mm4, mm2  // left = temp
    movq mm5, mm3  // left = temp
    add esi, ecx  // Next line
    dec ebx
    jnz yloopback

    add eax, 16
    cmp eax, edx
    jl xloopback

    emms
  }

  int in_size = pitch*h;
  unsigned int out_size = in_size*2;
  dst = (BYTE*)_aligned_malloc(out_size, 16);
  memset(z, 0, sizeof(z_stream_s));

  z->avail_in = in_size;
  z->next_in = image;
  z->total_in = 0;

  z->avail_out = out_size;
  z->next_out = dst;
  z->total_out = 0;
  
  z->data_type = Z_BINARY;
  deflateInit2(z, Z_BEST_SPEED, Z_DEFLATED, 15, 8, Z_HUFFMAN_ONLY);
//  deflateInit(z, Z_BEST_SPEED);
  int i = deflate(z, Z_FINISH);

  deflateEnd(z);
  out_size = z->total_out;//uLong
  unsigned int* dstint = (unsigned int*)&dst[out_size];
  dstint[0]= z->adler;

  out_size+=4;
  _RPT2(0, "TCPCompression: Compressed %d bytes into %d bytes (GZIP).\n", in_size, out_size);
  return out_size;
}
 
int PredictDownGZip::DeCompressImage(BYTE* image, int rowsize, int h, int pitch, int in_size) {
  // Pitch mod 16
  // Height > 2
  inplace = false;    
  unsigned int dst_size = pitch*h;
  dst = (BYTE*)_aligned_malloc(dst_size, 64);
  memset(z, 0, sizeof(z_stream_s));

  unsigned int* dstint = (unsigned int*)&image[in_size-4];
  z->adler = dstint[0]; 
  in_size-=4;

  z->avail_in = in_size;
  z->next_in = image;
  z->total_in = 0;

  z->avail_out = dst_size;
  z->next_out = dst;
  z->total_out = 0;

  z->data_type = Z_BINARY;


  inflateInit(z);
  int i = inflate(z, Z_FINISH);
  inflateEnd(z);

  rowsize = (rowsize+15)&~15;
  image = dst;
    
  __asm {
    xor eax, eax        // x offset
    mov ecx, [pitch]
    mov edx, rowsize
xloopback:
    mov ebx, [h]
    mov esi, [image]    // src
    pxor mm4, mm4   // left
    pxor mm5, mm5   // left2
yloopback:
    movq mm0, [esi+eax]   // src
    movq mm1, [esi+eax+8]   // src2
    paddb mm4, mm0  // left + src
    paddb mm5, mm1  // left + src
    movq [esi+eax], mm4
    movq [esi+eax+8], mm5
    add esi, ecx  // Next line
    dec ebx
    jnz yloopback

    add eax, 16
    cmp eax, edx
    jl xloopback

    emms
  }
  _RPT2(0, "TCPCompression: Decompressed %d bytes into %d bytes. (GZIP)\n", in_size, dst_size);
  return dst_size;
}


