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

// Overlay (c) 2003, 2004 by Klaus Post


#include "stdafx.h"
#include "blend_asm.h"


/*******************
 * Blends two planes.
 * A weight between the two planes are given.
 * Has rather ok pairing, 
 * and has very little memory usage.
 * Processes four pixels per loop, so rowsize must be mod 4.
 * Thanks to ARDA for squeezing out a bit more performance.
 * 
 * Weights must be multipled by 32767
 * Returns the blended plane in p1;
 * (c) 2002 by sh0dan.
 ********/

void mmx_weigh_planar(BYTE *p1, const BYTE *p2, int p1_pitch, int p2_pitch,int rowsize, int height, int weight, int invweight) {
  __int64 weight64  = (__int64)weight | (((__int64)invweight)<<16) | (((__int64)weight)<<32) |(((__int64)invweight)<<48);
  __int64 rounder = 0x0000400000004000;   // (0.5)<<15 in each dword

  __asm {
	  push ebx
      movq mm5,[rounder]
      pxor mm6,mm6
      movq mm7,[weight64]
      mov ebx,[rowsize]
      mov esi,[p1]
      mov edi,[p2]
      xor ecx, ecx  // Height
      mov edx,[height]
      align 16
yloopback:
      cmp ecx, edx
      jge outy
      xor eax, eax
      align 16 
testloop:
      cmp ebx, eax
      jle outloop
      punpcklbw mm0,[esi+eax]  // 4 pixels
       pxor mm3,mm3
      punpcklbw mm1,[edi+eax]  // y300 y200 y100 y000
       psrlw mm0,8              // 00y3 00y2 00y1 00y0
      psrlw mm1,8              // 00y3 00y2 00y1 00y0  
       pxor mm2,mm2
      movq mm4,mm1
       punpcklwd mm2,mm0
      punpckhwd mm3,mm0  
       punpcklwd mm4,mm6
      punpckhwd mm1,mm6
       por mm2,mm4
      por mm3,mm1
       pmaddwd mm2,mm7     // Multiply pixels by weights.  pixel = img1*weight + img2*invweight (twice)
      pmaddwd mm3,mm7      // Stalls 1 cycle (multiply unit stall)
       paddd mm2,mm5       // Add rounder
      paddd mm3,mm5
       psrld mm2,15        // Shift down, so there is no fraction.
      psrld mm3,15        
      packssdw mm2,mm3
      packuswb mm2,mm6 
      movd [esi+eax],mm2
      add eax,4
      jmp testloop
      align 16
outloop:
      inc ecx
      add esi, [p1_pitch];
      add edi, [p2_pitch];
      jmp yloopback
outy:
      emms
	  pop ebx
  } // end asm
}

// From MaskedMerge - Weighed merge of video planes depending on masks
// MaskedMerge (C) 2003 Kurosu (kurosu@inforezo.org)

void MMerge_MMX(unsigned char *dstp, const unsigned char *srcp,
        const unsigned char *maskp, const int dst_pitch, const int src_pitch,
        const int mask_pitch, const int row_size, const int height)
{
  static const __int64 Addi = 0x0080008000800080i64;

  __asm {
	push    ebx
    mov     eax,[dstp]
    mov     ebx,[srcp]
    mov     ecx,[maskp]
    mov     edx,[row_size]
    mov     edi,0
    mov     esi,[height]
    movq    mm7,[Addi]   // sh0dan: Stored in register to avoid having to read from cache.
    pxor    mm6, mm6
    align 16
loopx:
    movq    mm0, [eax+edi]
     movq    mm1, [ebx+edi]
    movq    mm2, [ecx+edi]
     movq    mm3, mm0
    movq    mm4, mm1
     movq    mm5, mm2  // sh0dan: No longer loading & unpacking the same data.

    punpcklbw mm0, mm6
     punpcklbw   mm1, mm6
    punpcklbw mm2, mm6
     punpckhbw   mm3, mm6
    punpckhbw mm4, mm6
     punpckhbw   mm5, mm6

    psubw   mm1, mm0
     psubw     mm4, mm3

    pmullw    mm1, mm2
     pmullw    mm4, mm5

    psllw   mm0, 8
     psllw     mm3, 8

    pxor    mm0, mm7
     pxor    mm3, mm7

    paddw   mm0, mm1
     paddw     mm3, mm4

    psrlw   mm0, 8
     psrlw     mm3, 8

    packuswb  mm0, mm3

    movq    [eax+edi],mm0

    add     edi,8
    cmp     edi,edx
    jl      loopx

    mov     edi,0
    add     eax,[dst_pitch]
    add     ebx,[src_pitch]
    add     ecx,[mask_pitch]
    dec     esi

    jnz     loopx

    emms
	pop     ebx
  }
}


void mmx_darken_planar(BYTE *p1, BYTE *p1U, BYTE *p1V, const BYTE *p2, const BYTE *p2U, const BYTE *p2V, int p1_pitch, int p2_pitch,int rowsize, int height) {
  static const __int64 AllOnes = 0xffffffffffffffff;
  const BYTE* bpp[6];
  bpp[0] = p1; bpp[1] = p2; bpp[2] = p1U; bpp[3] = p2U; bpp[4] = p1V; bpp[5] = p2V;

  __asm {
	  push ebx
      movq mm7,[AllOnes]
      pxor mm6,mm6
      mov ebx,[rowsize]
      mov esi,[p1]
      mov edi,[p2]
      xor ecx, ecx  // Height
      lea edx, bpp
      align 16
yloopback:
      cmp ecx, [height]
      jge outy
      xor eax, eax
      align 16 
testloop:
      cmp ebx, eax
      jle outloop
      mov esi,[edx+0]   // Y
      mov edi,[edx+4]
      movq mm0,[eax+esi]
      movq mm1,[eax+edi]
      movq mm2, mm1
      psubusb mm2,mm0
      pcmpeqb mm2,mm6
      
      movq mm3,mm2   
      pxor mm2,mm7   // Inverted
      pand mm0,mm2
      pand mm1,mm3
      por mm0,mm1
      movq [eax+esi], mm0

      mov esi,[edx+8]  // U
      mov edi,[edx+12]
      movq mm4,[eax+esi]
      movq mm5,[eax+edi]
      pand mm4,mm2
      pand mm5,mm3
      por mm4,mm5
      movq [eax+esi], mm4

      mov esi,[edx+16]  // V
      mov edi,[edx+20]
      movq mm0,[eax+esi]
      movq mm1,[eax+edi]
      pand mm0,mm2
      pand mm1,mm3
      por mm0,mm1
      movq [eax+esi], mm0

      add eax,8
      jmp testloop
      align 16
outloop:
      inc ecx
      mov esi,[edx+0]
      mov edi,[edx+4]
      add esi, [p1_pitch];
      add edi, [p2_pitch];
      mov [edx+0], esi
      mov [edx+4], edi

      mov esi,[edx+8]
      mov edi,[edx+12]
      add esi, [p1_pitch];
      add edi, [p2_pitch];
      mov [edx+8], esi
      mov [edx+12], edi

      mov esi,[edx+16]
      mov edi,[edx+20]
      add esi, [p1_pitch];
      add edi, [p2_pitch];
      mov [edx+16], esi
      mov [edx+20], edi

      jmp yloopback
outy:
      emms
	  pop ebx
  } // end asm
}

void mmx_lighten_planar(BYTE *p1, BYTE *p1U, BYTE *p1V, const BYTE *p2, const BYTE *p2U, const BYTE *p2V, int p1_pitch, int p2_pitch,int rowsize, int height) {
  static const __int64 AllOnes = 0xffffffffffffffff;
  const BYTE* bpp[6];
  bpp[0] = p1; bpp[1] = p2; bpp[2] = p1U; bpp[3] = p2U; bpp[4] = p1V; bpp[5] = p2V;

  __asm {
	  push ebx
      movq mm7,[AllOnes]
      pxor mm6,mm6
      mov ebx,[rowsize]
      mov esi,[p1]
      mov edi,[p2]
      xor ecx, ecx  // Height
      lea edx, bpp
      align 16
yloopback:
      cmp ecx, [height]
      jge outy
      xor eax, eax
      align 16 
testloop:
      cmp ebx, eax
      jle outloop
      mov esi,[edx+0]   // Y
      mov edi,[edx+4]
      movq mm0,[eax+esi]
      movq mm1,[eax+edi]
      movq mm2, mm0
      psubusb mm2,mm1
      pcmpeqb mm2,mm6
      
      movq mm3,mm2   
      pxor mm2,mm7   // Inverted
      pand mm0,mm2
      pand mm1,mm3
      por mm0,mm1
      movq [eax+esi], mm0

      mov esi,[edx+8]  // U
      mov edi,[edx+12]
      movq mm4,[eax+esi]
      movq mm5,[eax+edi]
      pand mm4,mm2
      pand mm5,mm3
      por mm4,mm5
      movq [eax+esi], mm4

      mov esi,[edx+16]  // V
      mov edi,[edx+20]
      movq mm0,[eax+esi]
      movq mm1,[eax+edi]
      pand mm0,mm2
      pand mm1,mm3
      por mm0,mm1
      movq [eax+esi], mm0

      add eax,8
      jmp testloop
      align 16
outloop:
      inc ecx
      mov esi,[edx+0]
      mov edi,[edx+4]
      add esi, [p1_pitch];
      add edi, [p2_pitch];
      mov [edx+0], esi
      mov [edx+4], edi

      mov esi,[edx+8]
      mov edi,[edx+12]
      add esi, [p1_pitch];
      add edi, [p2_pitch];
      mov [edx+8], esi
      mov [edx+12], edi

      mov esi,[edx+16]
      mov edi,[edx+20]
      add esi, [p1_pitch];
      add edi, [p2_pitch];
      mov [edx+16], esi
      mov [edx+20], edi

      jmp yloopback
outy:
      emms
	  pop ebx
  } // end asm
}
