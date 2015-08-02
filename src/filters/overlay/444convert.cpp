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

#include "444convert.h"



/***** YV12 -> YUV 4:4:4   ******/

void ConvertYV12ChromaTo444(unsigned char *dstp, const unsigned char *srcp,
        const int dst_pitch, const int src_pitch,
        const int src_rowsize, const int src_height)
{
  int dst_pitch2 = dst_pitch * 2;
  __asm {
	push    ebx
    mov     eax,[dstp]
    mov     ebx,[srcp]
    mov     ecx, eax
    add     ecx, [dst_pitch]  // ecx  = 1 line dst offset

    mov     edx,[src_rowsize]
    xor     edi,edi
    mov     esi,[src_height]
    align 16
loopx:
    movd    mm0, [ebx+edi]    // U4U3 U2U1
    movd    mm1, [ebx+edi+4]  // U8U7 U6U5

    punpcklbw mm0, mm0        // U4U4 U3U3 U2U2 U1U1
    punpcklbw mm1, mm1        // U8U8 U7U7 U6U6 U5U5

    movq    [eax+edi*2], mm0
    movq    [eax+edi*2+8], mm1

    movq    [ecx+edi*2], mm0
    movq    [ecx+edi*2+8], mm1

    add     edi,8
    cmp     edi,edx
    jl      loopx

    mov     edi,0
    add     eax,[dst_pitch2]
    add     ecx,[dst_pitch2]
    add     ebx,[src_pitch]
    dec     esi

    jnz     loopx

    emms
	pop     ebx
  }
}

void Convert444FromYV12::ConvertImage(PVideoFrame src, Image444* dst, IScriptEnvironment* env) {
  env->BitBlt(dst->GetPtr(PLANAR_Y), dst->pitch,
    src->GetReadPtr(PLANAR_Y),src->GetPitch(PLANAR_Y), src->GetRowSize(PLANAR_Y), src->GetHeight());

  const BYTE* srcU = src->GetReadPtr(PLANAR_U);
  const BYTE* srcV = src->GetReadPtr(PLANAR_V);

  int srcUVpitch = src->GetPitch(PLANAR_U);

  BYTE* dstU = dst->GetPtr(PLANAR_U);
  BYTE* dstV = dst->GetPtr(PLANAR_V);

  int dstUVpitch = dst->pitch;

  int w = ((src->GetRowSize(PLANAR_U)+7)/8)*8;
  int h = src->GetHeight(PLANAR_U);

  ConvertYV12ChromaTo444(dstU, srcU, dstUVpitch, srcUVpitch, w, h);
  ConvertYV12ChromaTo444(dstV, srcV, dstUVpitch, srcUVpitch, w, h);

}


void Convert444FromYV12::ConvertImageLumaOnly(PVideoFrame src, Image444* dst, IScriptEnvironment* env) {
  env->BitBlt(dst->GetPtr(PLANAR_Y), dst->pitch,
    src->GetReadPtr(PLANAR_Y),src->GetPitch(PLANAR_Y), src->GetRowSize(PLANAR_Y), src->GetHeight());
}

void Convert444FromYV24::ConvertImage(PVideoFrame src, Image444* dst, IScriptEnvironment* env) {
  env->BitBlt(dst->GetPtr(PLANAR_Y), dst->pitch,
    src->GetReadPtr(PLANAR_Y),src->GetPitch(PLANAR_Y), src->GetRowSize(PLANAR_Y), src->GetHeight(PLANAR_Y));
  env->BitBlt(dst->GetPtr(PLANAR_U), dst->pitch,
    src->GetReadPtr(PLANAR_U),src->GetPitch(PLANAR_U), src->GetRowSize(PLANAR_U), src->GetHeight(PLANAR_U));
  env->BitBlt(dst->GetPtr(PLANAR_V), dst->pitch,
    src->GetReadPtr(PLANAR_V),src->GetPitch(PLANAR_V), src->GetRowSize(PLANAR_V), src->GetHeight(PLANAR_V));
}

void Convert444FromYV24::ConvertImageLumaOnly(PVideoFrame src, Image444* dst, IScriptEnvironment* env) {
  env->BitBlt(dst->GetPtr(PLANAR_Y), dst->pitch,
    src->GetReadPtr(PLANAR_Y),src->GetPitch(PLANAR_Y), src->GetRowSize(PLANAR_Y), src->GetHeight(PLANAR_Y));
}

void Convert444FromY8::ConvertImage(PVideoFrame src, Image444* dst, IScriptEnvironment* env) {
  env->BitBlt(dst->GetPtr(PLANAR_Y), dst->pitch,
    src->GetReadPtr(PLANAR_Y),src->GetPitch(PLANAR_Y), src->GetRowSize(PLANAR_Y), src->GetHeight(PLANAR_Y));
  memset((void *)dst->GetPtr(PLANAR_U), 0x80, dst->pitch * dst->h());
  memset((void *)dst->GetPtr(PLANAR_V), 0x80, dst->pitch * dst->h());
}

void Convert444FromY8::ConvertImageLumaOnly(PVideoFrame src, Image444* dst, IScriptEnvironment* env) {
  env->BitBlt(dst->GetPtr(PLANAR_Y), dst->pitch,
    src->GetReadPtr(PLANAR_Y),src->GetPitch(PLANAR_Y), src->GetRowSize(PLANAR_Y), src->GetHeight(PLANAR_Y));
}

/***** YUY2 -> YUV 4:4:4   ******/


void Convert444FromYUY2::ConvertImage(PVideoFrame src, Image444* dst, IScriptEnvironment* env) {

  const BYTE* srcP = src->GetReadPtr();
  int srcPitch = src->GetPitch();

  BYTE* dstY = dst->GetPtr(PLANAR_Y);
  BYTE* dstU = dst->GetPtr(PLANAR_U);
  BYTE* dstV = dst->GetPtr(PLANAR_V);

  int dstPitch = dst->pitch;

  int w = dst->w();
  int h = dst->h();

  for (int y=0; y<h; y++) {
    for (int x=0; x<w; x+=2) {
      int x2 = x<<1;
      dstY[x]   = srcP[x2];
      dstU[x]   = dstU[x+1] = srcP[x2+1];
      dstV[x]   = dstV[x+1] = srcP[x2+3];
      dstY[x+1] = srcP[x2+2];
    }
    srcP+=srcPitch;

    dstY+=dstPitch;
    dstU+=dstPitch;
    dstV+=dstPitch;
  }
}


void Convert444FromYUY2::ConvertImageLumaOnly(PVideoFrame src, Image444* dst, IScriptEnvironment* env) {

  const BYTE* srcP = src->GetReadPtr();
  int srcPitch = src->GetPitch();

  BYTE* dstY = dst->GetPtr(PLANAR_Y);

  int dstPitch = dst->pitch;

  int w = dst->w();
  int h = dst->h();

  for (int y=0; y<h; y++) {
    for (int x=0; x<w; x+=2) {
      int x2 = x<<1;
      dstY[x]   = srcP[x2];
      dstY[x+1] = srcP[x2+2];
    }
    srcP+=srcPitch;
    dstY+=dstPitch;
  }
}

/****** YUV 4:4:4 -> YUV 4:4:4   - Perhaps the easiest job in the world ;)  *****/
PVideoFrame Convert444ToYV24::ConvertImage(Image444* src, PVideoFrame dst, IScriptEnvironment* env) {
  env->MakeWritable(&dst);

  env->BitBlt(dst->GetWritePtr(PLANAR_Y), dst->GetPitch(PLANAR_Y),
    src->GetPtr(PLANAR_Y), src->pitch, dst->GetRowSize(PLANAR_Y), dst->GetHeight(PLANAR_Y));
  env->BitBlt(dst->GetWritePtr(PLANAR_U), dst->GetPitch(PLANAR_U),
    src->GetPtr(PLANAR_U), src->pitch, dst->GetRowSize(PLANAR_U), dst->GetHeight(PLANAR_U));
  env->BitBlt(dst->GetWritePtr(PLANAR_V), dst->GetPitch(PLANAR_V),
    src->GetPtr(PLANAR_V), src->pitch, dst->GetRowSize(PLANAR_V), dst->GetHeight(PLANAR_V));
  return dst;
}

PVideoFrame Convert444ToY8::ConvertImage(Image444* src, PVideoFrame dst, IScriptEnvironment* env) {
  env->MakeWritable(&dst);

  env->BitBlt(dst->GetWritePtr(PLANAR_Y), dst->GetPitch(PLANAR_Y),
    src->GetPtr(PLANAR_Y), src->pitch, dst->GetRowSize(PLANAR_Y), dst->GetHeight());
  return dst;
}

/******  YUV 4:4:4 -> YV12  *****/

// ISSE_Convert444ChromaToYV12:
// by Klaus Post, Copyright 2004.
// and Ian Brabham Copyright 2007.
//
// src_rowsize must be mod 16 (dst_rowsize mod 8)
// Operates on 16x2 input pixels per loop for best possible pairability.

// Note! (((a+b+1) >> 1) + ((c+d+1) >> 1) + 1) >> 1 = (a+b+c+d+3) >> 2

/*
      should be (a+b+c+d+2) >> 2
      average_round_down(average_round_up(a, b), average_round_up(c, d))
    = average_round_down(pavgb(a, b), pavgb(c, d))
    = ~pavgb(~pavgb(a, b), ~pavgb(c, d))
*/

void ISSE_Convert444ChromaToYV12(unsigned char *dstp, const unsigned char *srcp,
        const int dst_pitch, const int src_pitch,
        const int src_rowsize, const int src_height)
{
  int src_pitch2 = src_pitch * 2;
  __asm {
	push     ebx
    mov      eax,[dstp]
    mov      ebx,[srcp]
    mov      ecx, ebx
    add      ecx, [src_pitch]  // ecx  = 1 line src offset

    mov      edx,[src_rowsize]
    xor      edi,edi
    mov      esi,[src_height]
    pcmpeqb  mm6,mm6         // ffff ffff ffff ffff
    pcmpeqb  mm7,mm7
    psrlw    mm7, 8          // 00ff 00ff 00ff 00ff
    align    16
loopx:
    movq     mm0, [ebx+edi*2]    // u4U4 u3U3 u2U2 u1U1
    movq     mm2, [ebx+edi*2+8]  // u8U8 u7U7 U6U6 u5U5

    pavgb    mm0, [ecx+edi*2]    // u4U4 u3U3 u2U2 u1U1  (Next line)
    pavgb    mm2, [ecx+edi*2+8]  // u8U8 u7U7 u6U6 u5U5  (Next line)

    pxor     mm0,mm6         // ~u4~U4 ~u3~U3 ~u2~U2 ~u1~U1
    pxor     mm2,mm6         // ~u8~U8 ~u7~U7 ~u6~U6 ~u5~U5

    movq     mm1,mm0
    psrlw    mm0, 8          // ~(00u4 00u3 00u2 00u1)
    movq     mm3,mm2
    psrlw    mm2, 8          // ~(00u8 00u7 00u6 00u5)
    pavgb    mm0,mm1         // ~(xxU4 xxU3 xxU2 xxU1)
    pavgb    mm2,mm3         // ~(xxU8 xxU7 xxU6 xxU5)
    pand     mm0,mm7         // ~(00U4 00U3 00U2 00U1)
    pand     mm2,mm7         // ~(00U8 00U7 00U6 00U5)

    add      edi,8
    packuswb mm0,mm2         // ~U8~U7~U6~U5~U4~U3~U2~U1

    cmp      edi,edx
    pxor     mm0,mm6         // U8U7U6U5U4U3U2U1
    movq     [eax+edi-8],mm0

    jl       loopx

    mov      edi,0
    add      eax,[dst_pitch]
    add      ecx,[src_pitch2]
    add      ebx,[src_pitch2]
    dec      esi

    jnz      loopx

    emms
	pop      ebx
  }
}

// MMX_Convert444ChromaToYV12:
// by Ian Brabham Copyright 2007.
//
// src_rowsize must be mod 16 (dst_rowsize mod 8)
// Operates on 16x2 input pixels per loop for best possible pairability.


void MMX_Convert444ChromaToYV12(unsigned char *dstp, const unsigned char *srcp,
        const int dst_pitch, const int src_pitch,
        const int src_rowsize, const int src_height)
{
#if 1
  static const __int64 sevenfB = 0x7f7f7f7f7f7f7f7f;
  int src_pitch2 = src_pitch * 2;
  __asm {
	push     ebx
    mov      eax, [dstp]
    mov      ebx, [srcp]
    mov      ecx, ebx
    add      ecx, [src_pitch]  // ecx  = 1 line src offset

    mov      edx, [src_rowsize]
    xor      edi, edi
    mov      esi, [src_height]
    pcmpeqb  mm7, mm7
    psrlw    mm7, 8          // 00ff 00ff 00ff 00ff
    movq     mm6, [sevenfB]
    align    16

// (a + b + 1) >> 1 = (a | b) - ((a ^ b) >> 1)

loopx:
    movq     mm4, [ebx+edi*2]    // u4U4 u3U3 u2U2 u1U1
    movq     mm5, [ebx+edi*2+8]  // u8U8 u7U7 U6U6 u5U5
    movq     mm1, [ecx+edi*2]    // u4U4 u3U3 u2U2 u1U1  (Next line)
    movq     mm3, [ecx+edi*2+8]  // u8U8 u7U7 u6U6 u5U5  (Next line)
    movq     mm0, mm4
    movq     mm2, mm5
    pxor     mm4, mm1            // Average with next line
    pxor     mm5, mm3            // Average with next line
    psrlq    mm4, 1              // Fuck Intel! Where is psrlb
    por      mm0, mm1
    psrlq    mm5, 1              // Fuck Intel! Where is psrlb
    por      mm2, mm3
    pand     mm4, mm6
    pand     mm5, mm6
    psubb    mm0, mm4
    psubb    mm2, mm5
    movq     mm1, mm0
    psrlw    mm0, 8              // 00u4 00u3 00u2 00u1
    movq     mm3, mm2
    psrlw    mm2, 8              // 00u8 00u7 00u6 00u5
    pand     mm1, mm7            // 00U4 00U3 00U2 00U1
    pand     mm3, mm7            // 00U8 00U7 00U6 00U5
    paddw    mm0, mm1            // xU4. xU3. xU2. xU1.
    paddw    mm2, mm3            // xU8. xU7. xU6. xU5.
    psrlw    mm0, 1              // 00U4 00U3 00U2 00U1
    psrlw    mm2, 1              // 00U8 00U7 00U6 00U5

    add      edi,8
    packuswb mm0,mm2             // U8U7 U6U5 U4U3 U2U1

    cmp      edi,edx
    movq     [eax+edi-8],mm0

    jl       loopx

    mov      edi,0
    add      eax,[dst_pitch]
    add      ecx,[src_pitch2]
    add      ebx,[src_pitch2]
    dec      esi

    jnz      loopx

    emms
	pop       ebx
  }
}
#else
  static const __int64 onesW = 0x0001000100010001;
  static const __int64 twosW = 0x0002000200020002;
  int src_pitch2 = src_pitch * 2;
  __asm {
	push      ebx
    mov       eax,[dstp]
    mov       ebx,[srcp]
    mov       ecx, ebx
    add       ecx, [src_pitch]  // ecx  = 1 line src offset

    mov       edx,[src_rowsize]
    xor       edi,edi
    mov       esi,[src_height]
    pxor      mm7,mm7
    movq      mm6,[onesW]
    movq      mm5,[twosW]

    align     16
loopx:
    movq      mm0, [ebx+edi*2]    // u4U4 u3U3 u2U2 u1U1
    movq      mm1, [ecx+edi*2]    // u4U4 u3U3 u2U2 u1U1  (Next line)
    movq      mm2,mm0
    movq      mm3,mm1

    punpcklbw mm0, mm7      // 00u2 00U2 00u1 00U1
    punpcklbw mm1, mm7      // 00u2 00U2 00u1 00U1  (Next line)
    punpckhbw mm2, mm7      // 00u4 00U4 00u3 00U3
    paddw     mm0, mm1      // Add with next line
    punpckhbw mm3, mm7      // 00u4 00U4 00u3 00U3  (Next line)
    pmaddwd   mm0, mm6      // 0000 0U2. 0000 0U1.
    paddw     mm2, mm3      // Add with next line
     movq      mm1, [ebx+edi*2+8]  // u8U8 u7U7 U6U6 u5U5
    pmaddwd   mm2, mm6      // 0000 0U4. 0000 0U3.
     movq      mm4, [ecx+edi*2+8]  // u8U8 u7U7 u6U6 u5U5  (Next line)
    packssdw  mm0, mm2      // 0U4. 0U3. 0U2. 0U1.
     movq      mm3,mm1
     movq      mm2,mm4
     punpcklbw mm1, mm7      // 00u6 00U6 00u5 00U5
     punpcklbw mm2, mm7      // 00u6 00U6 00u5 00U5  (Next line)
     punpckhbw mm3, mm7      // 00u8 00U8 00u7 00U7
     paddw     mm1, mm2      // Add with next line
     punpckhbw mm4, mm7      // 00u8 00U8 00u7 00U7  (Next line)
     pmaddwd   mm1, mm6      // 0000 0U6. 0000 0U5.
     paddw     mm3, mm4      // Add with next line
     pmaddwd   mm3, mm6      // 0000 0U8. 0000 0U7.
     packssdw  mm1, mm3      // 0U8. 0U7. 0U6. 0U5.
    paddw     mm0, mm5      // Add rounder
     paddw     mm1, mm5      // Add rounder
    psrlw     mm0, 2        // 00U4 00U3 00U2 00U1
     psrlw     mm1, 2        // 00U8 00U7 00U6 00U5
    add       edi,8
    packuswb  mm0,mm1       // U8U7 U6U5 U4U3 U2U1
    cmp       edi,edx
    movq      [eax+edi-8],mm0
    jl        loopx

    mov       edi,0
    add       eax,[dst_pitch]
    add       ecx,[src_pitch2]
    add       ebx,[src_pitch2]
    dec       esi

    jnz       loopx

    emms
	pop       ebx
  }
}
#endif

PVideoFrame Convert444ToYV12::ConvertImage(Image444* src, PVideoFrame dst, IScriptEnvironment* env) {
  env->MakeWritable(&dst);

  env->BitBlt(dst->GetWritePtr(PLANAR_Y), dst->GetPitch(PLANAR_Y),
    src->GetPtr(PLANAR_Y), src->pitch, dst->GetRowSize(PLANAR_Y), dst->GetHeight());

  const BYTE* srcU = src->GetPtr(PLANAR_U);
  const BYTE* srcV = src->GetPtr(PLANAR_V);

  int srcUVpitch = src->pitch;

  BYTE* dstU = dst->GetWritePtr(PLANAR_U);
  BYTE* dstV = dst->GetWritePtr(PLANAR_V);

  int dstUVpitch = dst->GetPitch(PLANAR_U);

  int w = ((dst->GetRowSize(PLANAR_U)+7)/8)*8;
  int h = dst->GetHeight(PLANAR_U);

  if (env->GetCPUFlags() & CPUF_INTEGER_SSE) {

	ISSE_Convert444ChromaToYV12(dstU, srcU, dstUVpitch, srcUVpitch, w, h);
	ISSE_Convert444ChromaToYV12(dstV, srcV, dstUVpitch, srcUVpitch, w, h);

  } else if (env->GetCPUFlags() & CPUF_MMX) {

	MMX_Convert444ChromaToYV12(dstU, srcU, dstUVpitch, srcUVpitch, w, h);
	MMX_Convert444ChromaToYV12(dstV, srcV, dstUVpitch, srcUVpitch, w, h);

  } else {
    for (int y=0; y<h; y++) {
      for (int x=0; x<w; x++) {
        int x2 = x<<1;
        dstU[x] = (srcU[x2] + srcU[x2+1] + srcU[x2+srcUVpitch] + srcU[x2+srcUVpitch+1] + 2)>>2;
        dstV[x] = (srcV[x2] + srcV[x2+1] + srcV[x2+srcUVpitch] + srcV[x2+srcUVpitch+1] + 2)>>2;
      }
      srcU+=srcUVpitch*2;
      srcV+=srcUVpitch*2;
      dstU+=dstUVpitch;
      dstV+=dstUVpitch;
    }
  }
  return dst;
}

/*****   YUV 4:4:4 -> YUY2   *******/

PVideoFrame Convert444ToYUY2::ConvertImage(Image444* src, PVideoFrame dst, IScriptEnvironment* env) {
  env->MakeWritable(&dst);

  const BYTE* srcY = src->GetPtr(PLANAR_Y);
  const BYTE* srcU = src->GetPtr(PLANAR_U);
  const BYTE* srcV = src->GetPtr(PLANAR_V);

  int srcPitch = src->pitch;

  BYTE* dstP = dst->GetWritePtr();

  int dstPitch = dst->GetPitch();

  int w = src->w();
  int h = src->h();

  for (int y=0; y<h; y++) {
    for (int x=0; x<w; x+=2) {
      int x2 = x<<1;
      dstP[x2]   = srcY[x];
      dstP[x2+1] = (srcU[x] + srcU[x+1] + 1)>>1;
      dstP[x2+2] = srcY[x+1];
      dstP[x2+3] = (srcV[x] + srcV[x+1] + 1)>>1;
    }
    srcY+=srcPitch;
    srcU+=srcPitch;
    srcV+=srcPitch;
    dstP+=dstPitch;
  }
  return dst;
}

/*****   YUV 4:4:4 -> RGB24/32   *******/

#define Kr 0.299
#define Kg 0.587
#define Kb 0.114

PVideoFrame Convert444ToRGB::ConvertImage(Image444* src, PVideoFrame dst, IScriptEnvironment* env) {
  const int crv = int(2*(1-Kr)       * 255.0/224.0 * 65536+0.5);
  const int cgv = int(2*(1-Kr)*Kr/Kg * 255.0/224.0 * 65536+0.5);
  const int cgu = int(2*(1-Kb)*Kb/Kg * 255.0/224.0 * 65536+0.5);
  const int cbu = int(2*(1-Kb)       * 255.0/224.0 * 65536+0.5);

  env->MakeWritable(&dst);

  const BYTE* srcY = src->GetPtr(PLANAR_Y);
  const BYTE* srcU = src->GetPtr(PLANAR_U);
  const BYTE* srcV = src->GetPtr(PLANAR_V);

  int srcPitch = src->pitch;

  BYTE* dstP = dst->GetWritePtr();
  int dstPitch = dst->GetPitch();

  int w = src->w();
  int h = src->h();

  dstP += h*dstPitch-dstPitch;
  int bpp = dst->GetRowSize()/w;

  for (int y=0; y<h; y++) {
    int xRGB = 0;
    for (int x=0; x<w; x++) {
      const int Y = (srcY[x] -  16) * int(255.0/219.0*65536+0.5);
      const int U =  srcU[x] - 128;
      const int V =  srcV[x] - 128;

      dstP[xRGB+0] = ScaledPixelClip(Y + U * cbu);
      dstP[xRGB+1] = ScaledPixelClip(Y - U * cgu - V * cgv);
      dstP[xRGB+2] = ScaledPixelClip(Y           + V * crv);
      xRGB += bpp;
    }
    srcY+=srcPitch;
    srcU+=srcPitch;
    srcV+=srcPitch;
    dstP-=dstPitch;
  }
  return dst;
}

PVideoFrame Convert444NonCCIRToRGB::ConvertImage(Image444* src, PVideoFrame dst, IScriptEnvironment* env) {
  const int crv = int(2*(1-Kr)       * 65536+0.5);
  const int cgv = int(2*(1-Kr)*Kr/Kg * 65536+0.5);
  const int cgu = int(2*(1-Kb)*Kb/Kg * 65536+0.5);
  const int cbu = int(2*(1-Kb)       * 65536+0.5);

  env->MakeWritable(&dst);

  const BYTE* srcY = src->GetPtr(PLANAR_Y);
  const BYTE* srcU = src->GetPtr(PLANAR_U);
  const BYTE* srcV = src->GetPtr(PLANAR_V);

  int srcPitch = src->pitch;

  BYTE* dstP = dst->GetWritePtr();
  int dstPitch = dst->GetPitch();

  int w = src->w();
  int h = src->h();

  dstP += h*dstPitch-dstPitch;
  int bpp = dst->GetRowSize()/w;

  for (int y=0; y<h; y++) {
    int xRGB = 0;
    for (int x=0; x<w; x++) {
      const int Y = srcY[x]*65536;
      const int U = srcU[x] - 128;
      const int V = srcV[x] - 128;

      dstP[xRGB+0] = ScaledPixelClip(Y + U * cbu);
      dstP[xRGB+1] = ScaledPixelClip(Y - U * cgu - V * cgv);
      dstP[xRGB+2] = ScaledPixelClip(Y           + V * crv);
      xRGB += bpp;
    }
    srcY+=srcPitch;
    srcU+=srcPitch;
    srcV+=srcPitch;
    dstP-=dstPitch;
  }
  return dst;
}


/******* RGB 24/32 -> YUV444   *******/

void Convert444FromRGB::ConvertImageLumaOnly(PVideoFrame src, Image444* dst, IScriptEnvironment* env) {

  const BYTE* srcP = src->GetReadPtr();
  int srcPitch = src->GetPitch();

  BYTE* dstY = dst->GetPtr(PLANAR_Y);

  int dstPitch = dst->pitch;

  int w = dst->w();
  int h = dst->h();

  int bpp = src->GetRowSize()/w;
  srcP += h*srcPitch-srcPitch;

  for (int y=0; y<h; y++) {
    int RGBx = 0;
    for (int x=0; x<w; x++) {
      dstY[x] = srcP[RGBx]; // Blue channel only ???
      RGBx+=bpp;
    }
    srcP-=srcPitch;
    dstY+=dstPitch;
  }
}

void Convert444FromRGB::ConvertImage(PVideoFrame src, Image444* dst, IScriptEnvironment* env) {
  const int cyr = int(Kr * 219/255 * 65536 + 0.5);
  const int cyg = int(Kg * 219/255 * 65536 + 0.5);
  const int cyb = int(Kb * 219/255 * 65536 + 0.5);

  const int kv = int(32768 / (2*(1-Kr) * 255.0/224.0) + 0.5); // 20531
  const int ku = int(32768 / (2*(1-Kb) * 255.0/224.0) + 0.5); // 16244

  const BYTE* srcP = src->GetReadPtr();
  int srcPitch = src->GetPitch();

  BYTE* dstY = dst->GetPtr(PLANAR_Y);
  BYTE* dstU = dst->GetPtr(PLANAR_U);
  BYTE* dstV = dst->GetPtr(PLANAR_V);

  const int dstPitch = dst->pitch;

  const int w = dst->w();
  const int h = dst->h();

  const int bpp = src->GetRowSize()/w;
  srcP += h*srcPitch-srcPitch;

  for (int y=0; y<h; y++) {
    int RGBx = 0;
    for (int x=0; x<w; x++) {
      const int b = srcP[RGBx+0];
      const int g = srcP[RGBx+1];
      const int r = srcP[RGBx+2];

      const int y = (cyb*b + cyg*g + cyr*r + 0x108000) >> 16; // 0x108000 = 16.5 * 65536
      const int scaled_y = (y - 16) * int(255.0/219.0*65536+0.5);
      const int b_y = (b << 16) - scaled_y;
      const int r_y = (r << 16) - scaled_y;

      dstY[x] = BYTE(y);
      dstU[x] = BYTE(((b_y>>11) * ku + 0x8080000)>>20); // 0x8080000 = 128.5 << 20
      dstV[x] = BYTE(((r_y>>11) * kv + 0x8080000)>>20);

      RGBx += bpp;
    }

    srcP-=srcPitch;

    dstY+=dstPitch;
    dstU+=dstPitch;
    dstV+=dstPitch;
  }
}

void Convert444NonCCIRFromRGB::ConvertImage(PVideoFrame src, Image444* dst, IScriptEnvironment* env) {
  const int cyb = int(Kb * 65536 + 0.5);
  const int cyg = int(Kg * 65536 + 0.5);
  const int cyr = int(Kr * 65536 + 0.5);

  const int kv = int(65536 / (2*(1-Kr)) + 0.5);
  const int ku = int(65536 / (2*(1-Kb)) + 0.5);

  const BYTE* srcP = src->GetReadPtr();
  const int srcPitch = src->GetPitch();

  BYTE* dstY = dst->GetPtr(PLANAR_Y);
  BYTE* dstU = dst->GetPtr(PLANAR_U);
  BYTE* dstV = dst->GetPtr(PLANAR_V);

  const int dstPitch = dst->pitch;

  const int w = dst->w();
  const int h = dst->h();

  const int bpp = src->GetRowSize()/w;
  srcP += h*srcPitch-srcPitch;

  for (int y=0; y<h; y++) {
    int RGBx = 0;
    for (int x=0; x<w; x++) {
      const int b = srcP[RGBx+0];
      const int g = srcP[RGBx+1];
      const int r = srcP[RGBx+2];

      const int y = (cyb*b + cyg*g + cyr*r + 0x8000) >> 16; // 0x8000 = 0.5 * 65536

      dstY[x] = BYTE(y);
      dstU[x] = BYTE(((b - y) * ku + 0x808000)>>16); // 0x808000 = 128.5 * 65536
      dstV[x] = BYTE(((r - y) * kv + 0x808000)>>16);

      RGBx+=bpp;
    }
    srcP-=srcPitch;

    dstY+=dstPitch;
    dstU+=dstPitch;
    dstV+=dstPitch;
  }
}

