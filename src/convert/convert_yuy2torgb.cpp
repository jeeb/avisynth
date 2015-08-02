// Avisynth v2.5.  Copyright 2002 Ben Rudiak-Gould et al.
// http://www.avisynth.org
//
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

// Convert_YUY2toRGB (c) 2015 by Ian Brabham


#include "stdafx.h"

#include "convert_yuy2torgb.h"


#define USE_DYNAMIC_COMPILER true


YUY2toRGBGenerator::YUY2toRGBGenerator() { }


YUY2toRGBGenerator::~YUY2toRGBGenerator() {
  assembly.Free();
}

         
struct yuv2rgb_constants {
  __int64 x0000_0000_0010_0010;
  __int64 x0080_0080_0080_0080;
  __int64 x00002000_00002000;
  __int64 xFF000000_FF000000;

  __int64 cy;
  __int64 crv;
  __int64 cgu_cgv;
  __int64 cbu;
};

__declspec(align(64)) struct yuv2rgb_constants yuv2rgb_constants_rec601 = {
  0x00000000000100010,  //    16
  0x00080008000800080,  //   128
  0x00000200000002000,  //  8192        = (0.5)<<14
  0x0FF000000FF000000,  //

  0x000004A8500004A85,  // 19077        = (255./219.)<<14+0.5
  0x03313000033130000,  // 13075        = ((1-0.299)*255./112.)<<13+0.5
  0x0E5FCF377E5FCF377,  // -6660, -3209 = ((K-1)*K/0.587*255./112.)<<13-0.5, K=(0.299, 0.114)
  0x00000408D0000408D   //        16525 = ((1-0.114)*255./112.)<<13+0.5
};

__declspec(align(64)) struct yuv2rgb_constants yuv2rgb_constants_PC_601 = {
  0x00000000000000000,  //     0       
  0x00080008000800080,  //   128       
  0x00000200000002000,  //  8192        = (0.5)<<14
  0x0FF000000FF000000,  //             

  0x00000400000004000,  // 16384        = (1.)<<14+0.5                                
  0x02D0B00002D0B0000,  // 11531        = ((1-0.299)*255./127.)<<13+0.5                      
  0x0E90FF4F2E90FF4F2,  // -5873, -2830 = (((K-1)*K/0.587)*255./127.)<<13-0.5, K=(0.299, 0.114)
  0x0000038ED000038ED   //        14573 = ((1-0.114)*255./127.)<<13+0.5                      
};

__declspec(align(64)) struct yuv2rgb_constants yuv2rgb_constants_rec709 = {
  0x00000000000100010,  //    16       
  0x00080008000800080,  //   128       
  0x00000200000002000,  //  8192        = (0.5)<<14
  0x0FF000000FF000000,  //             

  0x000004A8500004A85,  // 19077        = (255./219.)<<14+0.5
  0x0395E0000395E0000,  // 14686        = ((1-0.2126)*255./112.)<<13+0.5
  0x0EEF2F92DEEF2F92D,  // -4366, -1747 = ((K-1)*K/0.7152*255./112.)<<13-0.5, K=(0.2126, 0.0722)
  0x00000439900004399   //        17305        = ((1-0.0722)*255./112.)<<13+0.5       
};

__declspec(align(64)) struct yuv2rgb_constants yuv2rgb_constants_PC_709 = {
  0x00000000000000000,  //     0       
  0x00080008000800080,  //   128       
  0x00000200000002000,  //  8192        = (0.5)<<14
  0x0FF000000FF000000,  //             

  0x00000400000004000,  // 16384        = (1.)<<14+0.5                                
  0x03298000032980000,  // 12952        = ((1-0.2126)*255./127.)<<13+0.5                      
  0x0F0F6F9FBF0F6F9FB,  // -3850, -1541 = (((K-1)*K/0.7152)*255./127.)<<13-0.5, K=(0.2126, 0.0722)
  0x000003B9D00003B9D   //        15261 = ((1-0.0722)*255./127.)<<13+0.5                      
};

enum {
  ofs_x0000_0000_0010_0010 = 0,
  ofs_x0080_0080_0080_0080 = 8,
  ofs_x00002000_00002000   = 16,
  ofs_xFF000000_FF000000   = 24,

  ofs_cy = 32,
  ofs_crv = 40,
  ofs_cgu_cgv = 48,
  ofs_cbu = 56
};

// ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

void YUY2toRGBGenerator::Get_Y(Assembler &x86, MMREG mm_A, int uyvy) {
  if (uyvy) {
    x86.psrlw(      mm_A, 8);
  }
  else {
    x86.pand(       mm_A, mm7);
  }
}


void YUY2toRGBGenerator::Get_UV(Assembler &x86, MMREG mm_A, int uyvy) {
    Get_Y(x86, mm_A, 1-uyvy);
}


void YUY2toRGBGenerator::InnerLoop(Assembler &x86, int uyvy, int rgb32, int no_next_pixel, bool Y_16) {
/*
;; This YUV422->RGB conversion code uses only four MMX registers per
;; source dword, so I convert two dwords in parallel.  Lines corresponding
;; to the "second pipe" are indented an extra space.  There's almost no
;; overlap, except at the end and in the three lines marked ***.
;; revised 4july, 2002 to properly set alpha in rgb32 to default "on" & other small memory optimizations
*/
// mm0 mm1 mm2 mm3 mm4 mm5 mm6 mm7

    x86.movd(       mm0, dword_ptr[esi]);
     x86.pcmpeqw(   mm7, mm7);                     // 0xFFFFFFFFFFFFFFFFi64
    x86.movd(       mm5, dword_ptr[esi+4]);
     x86.psrlw(     mm7, 8);                       // 0x00FF00FF00FF00FFi64
    x86.movq(       mm1, mm0);
  Get_Y(x86,        mm0, uyvy);                    // mm0 = __________Y1__Y0
     x86.movq(      mm4, mm5);
  Get_UV(x86,       mm1, uyvy);                    // mm1 = __________V0__U0
  Get_Y(x86,        mm4, uyvy);                    // mm4 = __________Y3__Y2
    x86.movq(       mm2, mm5);                     // *** avoid reload from [esi+4]
  Get_UV(x86,       mm5, uyvy);                    // mm5 = __________V2__U2
  if (Y_16)
    x86.psubw(      mm0, qword_ptr[edx+ofs_x0000_0000_0010_0010]);                   // (Y-16)
     x86.movd(      mm6, dword_ptr[esi+8-4*(no_next_pixel)]);
  Get_UV(x86,       mm2, uyvy);                    // mm2 = __________V2__U2
  if (Y_16)
     x86.psubw(     mm4, qword_ptr[edx+ofs_x0000_0000_0010_0010]);                   // (Y-16)
    x86.paddw(      mm2, mm1);                     // 2*UV1=UV0+UV2
  Get_UV(x86,       mm6, uyvy);                    // mm6 = __________V4__U4
    x86.psubw(      mm1, qword_ptr[edx+ofs_x0080_0080_0080_0080]);                   // (UV-128)
     x86.paddw(     mm6, mm5);                     // 2*UV3=UV2+UV4
    x86.psllq(      mm2, 32);
     x86.psubw(     mm5, qword_ptr[edx+ofs_x0080_0080_0080_0080]);                   // (UV-128)
    x86.punpcklwd(  mm0, mm2);                     // mm0 = ______Y1______Y0
     x86.psllq(     mm6, 32);
    x86.pmaddwd(    mm0, qword_ptr[edx+ofs_cy]);            // (Y-16)*(255./219.)<<14
     x86.punpcklwd( mm4, mm6);
    x86.paddw(      mm1, mm1);                     // 2*UV0=UV0+UV0
     x86.pmaddwd(   mm4, qword_ptr[edx+ofs_cy]);
     x86.paddw(     mm5, mm5);                     // 2*UV2=UV2+UV2
    x86.paddw(      mm1, mm2);                     // mm1 = __V1__U1__V0__U0 * 2
    x86.paddd(      mm0, qword_ptr[edx+ofs_x00002000_00002000]);                     // +=0.5<<14
     x86.paddw(     mm5, mm6);                     // mm5 = __V3__U3__V2__U2 * 2
    x86.movq(       mm2, mm1);
     x86.paddd(     mm4, qword_ptr[edx+ofs_x00002000_00002000]);                     // +=0.5<<14
    x86.movq(       mm3, mm1);
     x86.movq(      mm6, mm5);
    x86.pmaddwd(    mm1, qword_ptr[edx+ofs_crv]);
     x86.movq(      mm7, mm5);
    x86.paddd(      mm1, mm0);
     x86.pmaddwd(   mm5, qword_ptr[edx+ofs_crv]);
    x86.psrad(      mm1, 14);                      // mm1 = RRRRRRRRrrrrrrrr
     x86.paddd(     mm5, mm4);
    x86.pmaddwd(    mm2, qword_ptr[edx+ofs_cgu_cgv]);
     x86.psrad(     mm5, 14);
    x86.paddd(      mm2, mm0);
     x86.pmaddwd(   mm6, qword_ptr[edx+ofs_cgu_cgv]);
    x86.psrad(      mm2, 14);                      // mm2 = GGGGGGGGgggggggg
     x86.paddd(     mm6, mm4);
    x86.pmaddwd(    mm3, qword_ptr[edx+ofs_cbu]);
     x86.psrad(     mm6, 14);
    x86.paddd(      mm3, mm0);
     x86.pmaddwd(   mm7, qword_ptr[edx+ofs_cbu]);
    x86.add(        esi, 8);
     x86.add(       edi, 12+4*rgb32);

  if (no_next_pixel == 0) {
    x86.cmp(        esi, ecx);
  }
    x86.psrad(      mm3, 14);                      // mm3 = BBBBBBBBbbbbbbbb
     x86.paddd(     mm7, mm4);
    x86.pxor(       mm0, mm0);
     x86.psrad(     mm7, 14);
    x86.packssdw(   mm3, mm2);                     // mm3 = GGGGggggBBBBbbbb
     x86.packssdw(  mm7, mm6);
    x86.packssdw(   mm1, mm0);                     // mm1 = ________RRRRrrrr
     x86.packssdw(  mm5, mm0);                     // *** avoid pxor mm4, mm4
    x86.movq(       mm2, mm3);
     x86.movq(      mm6, mm7);
    x86.punpcklwd(  mm2, mm1);                     // mm2 = RRRRBBBBrrrrbbbb
     x86.punpcklwd( mm6, mm5);
    x86.punpckhwd(  mm3, mm1);                     // mm3 = ____GGGG____gggg
     x86.punpckhwd( mm7, mm5);
    x86.movq(       mm0, mm2);
     x86.movq(      mm4, mm6);
    x86.punpcklwd(  mm0, mm3);                     // mm0 = ____rrrrggggbbbb
     x86.punpcklwd( mm4, mm7);
  if (rgb32 == 0) {  // rgb24
    x86.psllq(      mm0, 16);
     x86.psllq(     mm4, 16);
  }
    x86.punpckhwd(  mm2, mm3);                     // mm2 = ____RRRRGGGGBBBB
     x86.punpckhwd( mm6, mm7);
    x86.packuswb(   mm0, mm2);                     // mm0 = __RRGGBB__rrggbb <- ta dah!
     x86.packuswb(  mm4, mm6);
  if (rgb32 != 0) {  // rgb32
    x86.por(        mm0, qword_ptr[edx+ofs_xFF000000_FF000000]); // set alpha channels "on"
     x86.por(       mm4, qword_ptr[edx+ofs_xFF000000_FF000000]);
    x86.movq(       qword_ptr[edi-16], mm0);                // store the quadwords independently
     x86.movq(      qword_ptr[edi-8], mm4);
  }
  else {  // rgb24
    x86.psrlq(      mm0, 8);                       // pack the two quadwords into 12 bytes
    x86.psllq(      mm4, 8);                       // (note: the two shifts above leave
    x86.movd(       dword_ptr[edi-12], mm0);       // mm0,4 = __RRGGBBrrggbb__)
    x86.psrlq(      mm0, 32);
    x86.por(        mm4, mm0);
    x86.movd(       dword_ptr[edi-8], mm4);
    x86.psrlq(      mm4, 32);
    x86.movd(       dword_ptr[edi-4], mm4);
  }
}

// ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

// void __cdecl procname(
//        [esp+ 4] const BYTE* src,
//        [esp+ 8] BYTE* dst,
//        [esp+12] const BYTE* src_end,
//        [esp+16] int src_pitch,
//        [esp+20] int row_size

// ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

void YUY2toRGBGenerator::YUV2RGB_PROC(Assembler &x86, int uyvy, int rgb32, int theMatrix) {
  enum {             // Argument offsets
    src       = 0,
    dstp      = 4,
    src_end   = 8,
    src_pitch = 12,
    row_size  = 16,
  };
  bool Y_16;

    x86.push(       esi);
    x86.push(       edi);
    x86.push(       ebx);

    x86.mov(        eax, dword_ptr[ebp+src_pitch]);// src_pitch
    x86.mov(        esi, dword_ptr[ebp+src_end]);  // src_end - read source bottom-up
    x86.mov(        edi, dword_ptr[ebp+dstp]);     // dstp
    x86.mov(        ebx, dword_ptr[ebp+row_size]); // row_size

  switch (theMatrix) {
    case 1:
      Y_16 =                 !!yuv2rgb_constants_rec709.x0000_0000_0010_0010;
      x86.mov(      edx, (int)&yuv2rgb_constants_rec709);
      break;
    case 3:
      Y_16 =                 !!yuv2rgb_constants_PC_601.x0000_0000_0010_0010;
      x86.mov(      edx, (int)&yuv2rgb_constants_PC_601);
      break;
    case 7:
      Y_16 =                 !!yuv2rgb_constants_PC_709.x0000_0000_0010_0010;
      x86.mov(      edx, (int)&yuv2rgb_constants_PC_709);
      break;
    default:
      Y_16 =                 !!yuv2rgb_constants_rec601.x0000_0000_0010_0010;
      x86.mov(      edx, (int)&yuv2rgb_constants_rec601);
  }
//loop0:
    x86.align(      16);
    x86.label(      "loop0");
    x86.sub(        esi, eax);
    x86.lea(        ecx, dword_ptr[esi+ebx-8]);

//loop1:
    x86.align(      16);
    x86.label(      "loop1");

  InnerLoop(x86, uyvy, rgb32, 0, Y_16);

    x86.jb(         "loop1");

  InnerLoop(x86, uyvy, rgb32, 1, Y_16);

    x86.sub(        esi, ebx);
    x86.cmp(        esi, dword_ptr[ebp+src]);      // src
    x86.ja(         "loop0");

    x86.emms();

    x86.pop(        ebx);
    x86.pop(        edi);
    x86.pop(        esi);
}


void YUY2toRGBGenerator::Generate(bool isRGB32, int theMatrix, IScriptEnvironment* env) {

  Assembler x86;   // This is the class that assembles the code.

    x86.push(       ebp);
    x86.mov(        ebp, dword_ptr[esp+4+4]);      // Pointer to args list

  YUV2RGB_PROC(x86, 0, (int)isRGB32, theMatrix);

    x86.xor(        eax, eax);
    x86.pop(        ebp);
    x86.ret();

  assembly = DynamicAssembledCode(x86, env, "mmx_YUY2toRGB Dynamic MMX code could not be compiled.");
}
