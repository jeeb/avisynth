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

#include "convert_yuy2.h"


//  const int cyb = int(0.114*219/255*32768+0.5); // 0x0C88
//  const int cyg = int(0.587*219/255*32768+0.5); // 0x4087
//  const int cyr = int(0.299*219/255*32768+0.5); // 0x20DE
//__declspec(align(8)) const __int64 cybgr_64 = (__int64)cyb|(((__int64)cyg)<<16)|(((__int64)cyr)<<32);
  __declspec(align(8)) static const __int64 cybgr_64[4] ={0x000020DE40870C88,
                                                          0x0000175C4EA507ED,
                                                          0x000026464B230E97,
                                                          0x00001B335B92093B};

  __declspec(align(8)) static const __int64 fpix_mul[4] ={0x0000503300003F74,    //=(1/((1-0.299)*255/112)<<15+0.5),  (1/((1-0.114)*255/112)<<15+0.5)
                                                          0x0000476400003C97,    //=(1/((1-0.2125)*255/112)<<15+0.5), (1/((1-0.0721)*255/112)<<15+0.5)
                                                          0x00005AF1000047F4,    //=(1/((1-0.299)*255/127)<<15+0.5),  (1/((1-0.114)*255/127)<<15+0.5)
                                                          0x000050F3000044B4};   //=(1/((1-0.2125)*255/127)<<15+0.5), (1/((1-0.0721)*255/127)<<15+0.5)

  __declspec(align(8)) static const __int64 rb_mask     = 0x0000ffff0000ffff;    //=Mask for unpacked R and B
  __declspec(align(8)) static const __int64 fpix_add    = 0x0080800000808000;    //=(128.5) << 16
  __declspec(align(8)) static const __int64 chroma_mask2= 0xffff0000ffff0000;

  static const int y1y2_mult[4]={0x00004A85,    //=(255./219.) << 14
                                 0x00004A85,
                                 0x00004000,    //=1 << 14
                                 0x00004000};

  static const int fraction[4] ={0x00084000,    //=(16.5) << 15 = 0x84000
                                 0x00084000,
                                 0x00004000,    //=(0.5) << 15 = 0x4000
                                 0x00004000};

  static const int sub_32[4]   ={0x0000FFE0,    //=-16*2
                                 0x0000FFE0,
                                 0x00000000,    //=0
                                 0x00000000};


/*******************************
 * MMX RGB32 version
 *******************************/

void mmx_ConvertRGB32toYUY2(unsigned int *src,unsigned int *dst,int src_pitch, int dst_pitch,int w, int h, int matrix) {
#define RGB24 0
#define DUPL 0

#include "convert_yuy2.inc"

}


/*******************************
 * MMX RGB24 version
 *******************************/

void mmx_ConvertRGB24toYUY2(unsigned int *src,unsigned int *dst,int src_pitch, int dst_pitch,int w, int h, int matrix) {
#define RGB24 1
#define DUPL 0

#include "convert_yuy2.inc"
 
}


/*******************************
 * MMX RGB32 left pixel only version
 *******************************/

void mmx_ConvertRGB32toYUY2_Dup(unsigned int *src,unsigned int *dst,int src_pitch, int dst_pitch,int w, int h, int matrix) {
#define RGB24 0
#define DUPL 1

#include "convert_yuy2.inc"
 
}


/*******************************
 * MMX RGB24 left pixel only version
 *******************************/

void mmx_ConvertRGB24toYUY2_Dup(unsigned int *src,unsigned int *dst,int src_pitch, int dst_pitch,int w, int h, int matrix) {
#define RGB24 1
#define DUPL 1

#include "convert_yuy2.inc"
 
}
