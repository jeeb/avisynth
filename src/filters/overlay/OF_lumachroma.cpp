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
#include "overlayfunctions.h"

void OL_BlendLumaImage::BlendImageMask(Image444* base, Image444* overlay, Image444* mask) {
  BYTE* baseY = base->GetPtr(PLANAR_Y);

  BYTE* ovY = overlay->GetPtr(PLANAR_Y);
  
  BYTE* maskY = mask->GetPtr(PLANAR_Y);
  
  int w = base->w();
  int h = base->h();

  if (opacity == 256) {
    for (int y = 0; y < h; y++) {
      for (int x = 0; x < w; x++) {
        baseY[x] = (BYTE)(((baseY[x]*(256-maskY[x])) + (ovY[x]*maskY[x]+128))>>8);
      } // for x
      baseY += base->pitch;

      ovY += overlay->pitch;

      maskY += mask->pitch;
    } // for y
  } else {
    for (int y = 0; y < h; y++) {
      for (int x = 0; x < w; x++) {
        int mY = (opacity*maskY[x])>>8;
        baseY[x] = (BYTE)((((256-mY)*baseY[x]) + (mY*ovY[x])+128)>>8);
      }
      baseY += base->pitch;

      ovY += overlay->pitch;

      maskY += mask->pitch;
    } // for x
  } // for y
}


void OL_BlendLumaImage::BlendImage(Image444* base, Image444* overlay) {
  BYTE* baseY = base->GetPtr(PLANAR_Y);

  BYTE* ovY = overlay->GetPtr(PLANAR_Y);

  int w = base->w();
  int h = base->h();

  if (opacity == 256) {
    env->BitBlt(baseY, base->pitch, ovY, overlay->pitch, w, h);
  } else {
    if (!(w&3) && (env->GetCPUFlags() & CPUF_MMX)) {
      int weight = (opacity*32767+128)>>8;
      int invweight = 32767-weight;

      mmx_weigh_planar(baseY, ovY, base->pitch, overlay->pitch, w, h, weight, invweight);

    } else {
      for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
          baseY[x] = (BYTE)(((inv_opacity*baseY[x]) + (opacity*ovY[x]+128))>>8);
        }
        baseY += base->pitch;

        ovY += overlay->pitch;
      } // for x
    } // for y
  }// if !mmx
}


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

void OL_BlendLumaImage::mmx_weigh_planar(BYTE *p1, const BYTE *p2, int p1_pitch, int p2_pitch,int rowsize, int height, int weight, int invweight) {
  __int64 weight64  = (__int64)weight | (((__int64)invweight)<<16) | (((__int64)weight)<<32) |(((__int64)invweight)<<48);
	__int64 rounder = 0x0000400000004000;		// (0.5)<<15 in each dword

  __asm {
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
  } // end asm
}


void OL_BlendChromaImage::BlendImageMask(Image444* base, Image444* overlay, Image444* mask) {
  BYTE* baseU = base->GetPtr(PLANAR_U);
  BYTE* baseV = base->GetPtr(PLANAR_V);

  BYTE* ovU = overlay->GetPtr(PLANAR_U);
  BYTE* ovV = overlay->GetPtr(PLANAR_V);
  
  BYTE* maskU = mask->GetPtr(PLANAR_U);
  BYTE* maskV = mask->GetPtr(PLANAR_V);

  int w = base->w();
  int h = base->h();

  if (opacity == 256) {
    for (int y = 0; y < h; y++) {
      for (int x = 0; x < w; x++) {
        baseU[x] = (BYTE)(((baseU[x]*(256-maskU[x])) + (ovU[x]*maskU[x]+128))>>8);
        baseV[x] = (BYTE)(((baseV[x]*(256-maskV[x])) + (ovV[x]*maskV[x]+128))>>8);
      } // for x
      baseU += base->pitch;
      baseV += base->pitch;

      ovU += overlay->pitch;
      ovV += overlay->pitch;

      maskU += mask->pitch;
      maskV += mask->pitch;
    } // for y
  } else {
    for (int y = 0; y < h; y++) {
      for (int x = 0; x < w; x++) {
        int mU = (opacity*maskU[x])>>8;
        int mV = (opacity*maskV[x])>>8;
        baseU[x] = (BYTE)((((256-mU)*baseU[x]) + (mU*ovU[x])+128)>>8);
        baseV[x] = (BYTE)((((256-mV)*baseV[x]) + (mV*ovV[x])+128)>>8);
      }
      baseU += base->pitch;
      baseV += base->pitch;

      ovU += overlay->pitch;
      ovV += overlay->pitch;

      maskU += mask->pitch;
      maskV += mask->pitch;
    } // for x
  } // for y
}

void OL_BlendChromaImage::BlendImage(Image444* base, Image444* overlay) {
  BYTE* baseU = base->GetPtr(PLANAR_U);
  BYTE* baseV = base->GetPtr(PLANAR_V);

  BYTE* ovU = overlay->GetPtr(PLANAR_U);
  BYTE* ovV = overlay->GetPtr(PLANAR_V);
  
  int w = base->w();
  int h = base->h();

  if (opacity == 256) {
    env->BitBlt(baseU, base->pitch, ovU, overlay->pitch, w, h);
    env->BitBlt(baseV, base->pitch, ovV, overlay->pitch, w, h);
  } else {
    if (!(w&3) && (env->GetCPUFlags() & CPUF_MMX)) {
      int weight = (opacity*32767+128)>>8;
      int invweight = 32767-weight;

      mmx_weigh_planar(baseU, ovU, base->pitch, overlay->pitch, w, h, weight, invweight);
      mmx_weigh_planar(baseV, ovV, base->pitch, overlay->pitch, w, h, weight, invweight);

    } else {
      for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
          baseU[x] = (BYTE)(((inv_opacity*baseU[x]) + (opacity*ovU[x]+128))>>8);
          baseV[x] = (BYTE)(((inv_opacity*baseV[x]) + (opacity*ovV[x]+128))>>8);
        }
        baseU += base->pitch;
        baseV += base->pitch;

        ovU += overlay->pitch;
        ovV += overlay->pitch;
      } // for x
    } // for y
  }// if !mmx
}


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

void OL_BlendChromaImage::mmx_weigh_planar(BYTE *p1, const BYTE *p2, int p1_pitch, int p2_pitch,int rowsize, int height, int weight, int invweight) {
  __int64 weight64  = (__int64)weight | (((__int64)invweight)<<16) | (((__int64)weight)<<32) |(((__int64)invweight)<<48);
	__int64 rounder = 0x0000400000004000;		// (0.5)<<15 in each dword

  __asm {
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
  } // end asm
}
