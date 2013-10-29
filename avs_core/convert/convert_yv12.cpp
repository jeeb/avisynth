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

#include "convert_yv12.h"
#include <emmintrin.h>

/**************************************************************************
 * WARNING! : These routines are typical of when the compiler forgets to save
 * the ebx register. They have just enough C++ code to cause a slight need for
 * using the ebx register but not enough to stop the optimizer from restructuring
 * the code to avoid using the ebx register. The optimizer having done this 
 * very smugly removes the push ebx from the prologue. It seems totally oblivious
 * that there is still __asm code using the ebx register.
 **************************************************************************/

/*************************************
 * Interlaced YV12 -> YUY2 conversion
 *
 * (c) 2003, Klaus Post.
 *
 * Requires mod 8 pitch.
 *************************************/

__declspec(align(8)) static const __int64 mask1	   = 0x00ff00ff00ff00ff;
__declspec(align(8)) static const __int64 mask2	   = 0xff00ff00ff00ff00;
                                                   
__declspec(align(8)) static const __int64 add_1	   = 0x0001000100010001;
__declspec(align(8)) static const __int64 add_2	   = 0x0002000200020002;
__declspec(align(8)) static const __int64 add_64   = 0x0002000200020002;

__declspec(align(8)) static const __int64 add_ones = 0x0101010101010101;


static inline void copy_yv12_line_to_yuy2_c(const BYTE* srcY, const BYTE* srcU, const BYTE* srcV, BYTE* dstp, int width) {
  for (int x = 0; x < width / 2; ++x) {
    dstp[x*4] = srcY[x*2];
    dstp[x*4+2] = srcY[x*2+1];
    dstp[x*4+1] = srcU[x];
    dstp[x*4+3] = srcV[x];
  }
}

void convert_yv12_to_yuy2_progressive_c(const BYTE* srcY, const BYTE* srcU, const BYTE* srcV, int src_width, int src_pitch_y, int src_pitch_uv, BYTE *dstp, int dst_pitch, int height) {
  //first two lines
  copy_yv12_line_to_yuy2_c(srcY, srcU, srcV, dstp, src_width);
  copy_yv12_line_to_yuy2_c(srcY+src_pitch_y, srcU, srcV, dstp+dst_pitch, src_width);

  //last two lines. Easier to do them here
  copy_yv12_line_to_yuy2_c(
    srcY + src_pitch_y * (height-2), 
    srcU + src_pitch_uv * ((height/2)-1), 
    srcV + src_pitch_uv * ((height/2)-1), 
    dstp + dst_pitch * (height-2), 
    src_width
    );
  copy_yv12_line_to_yuy2_c(
    srcY + src_pitch_y * (height-1), 
    srcU + src_pitch_uv * ((height/2)-1), 
    srcV + src_pitch_uv * ((height/2)-1), 
    dstp + dst_pitch * (height-1), 
    src_width
    );

  srcY += src_pitch_y*2;
  srcU += src_pitch_uv;
  srcV += src_pitch_uv;
  dstp += dst_pitch*2;

  for (int y = 2; y < height-2; y+=2) {
    for (int x = 0; x < src_width / 2; ++x) {
      dstp[x*4] = srcY[x*2];
      dstp[x*4+2] = srcY[x*2+1];

      //avg(avg(a, b)-1, b)
      dstp[x*4+1] = ((((srcU[x-src_pitch_uv] + srcU[x] + 1) / 2) + srcU[x]) / 2);
      dstp[x*4+3] = ((((srcV[x-src_pitch_uv] + srcV[x] + 1) / 2) + srcV[x]) / 2);

      dstp[x*4 + dst_pitch] = srcY[x*2 + src_pitch_y];
      dstp[x*4+2 + dst_pitch] = srcY[x*2+1 + src_pitch_y];

      dstp[x*4+1 + dst_pitch] = ((((srcU[x] + srcU[x+src_pitch_uv] + 1) / 2) + srcU[x]) / 2);
      dstp[x*4+3 + dst_pitch] = ((((srcV[x] + srcV[x+src_pitch_uv] + 1) / 2) + srcV[x]) / 2);
    }
    srcY += src_pitch_y*2;
    dstp += dst_pitch*2;
    srcU += src_pitch_uv;
    srcV += src_pitch_uv;
  }
}

void convert_yv12_to_yuy2_interlaced_c(const BYTE* srcY, const BYTE* srcU, const BYTE* srcV, int src_width, int src_pitch_y, int src_pitch_uv, BYTE *dstp, int dst_pitch, int height) {
  //first four lines
  copy_yv12_line_to_yuy2_c(srcY, srcU, srcV, dstp, src_width);
  copy_yv12_line_to_yuy2_c(srcY + src_pitch_y*2, srcU, srcV, dstp + dst_pitch*2, src_width);
  copy_yv12_line_to_yuy2_c(srcY + src_pitch_y, srcU + src_pitch_uv, srcV + src_pitch_uv, dstp + dst_pitch, src_width);
  copy_yv12_line_to_yuy2_c(srcY + src_pitch_y*3, srcU + src_pitch_uv, srcV + src_pitch_uv, dstp + dst_pitch*3, src_width);

  //last four lines. Easier to do them here
  copy_yv12_line_to_yuy2_c(
    srcY + src_pitch_y * (height-4), 
    srcU + src_pitch_uv * ((height/2)-2), 
    srcV + src_pitch_uv * ((height/2)-2), 
    dstp + dst_pitch * (height-4), 
    src_width
    );
  copy_yv12_line_to_yuy2_c(
    srcY + src_pitch_y * (height-2), 
    srcU + src_pitch_uv * ((height/2)-2), 
    srcV + src_pitch_uv * ((height/2)-2), 
    dstp + dst_pitch * (height-2), 
    src_width
    );
  copy_yv12_line_to_yuy2_c(
    srcY + src_pitch_y * (height-3), 
    srcU + src_pitch_uv * ((height/2)-1), 
    srcV + src_pitch_uv * ((height/2)-1), 
    dstp + dst_pitch * (height-3), 
    src_width
    );
  copy_yv12_line_to_yuy2_c(
    srcY + src_pitch_y * (height-1), 
    srcU + src_pitch_uv * ((height/2)-1), 
    srcV + src_pitch_uv * ((height/2)-1), 
    dstp + dst_pitch * (height-1), 
    src_width
    );

  srcY += src_pitch_y * 4;
  srcU += src_pitch_uv * 2;
  srcV += src_pitch_uv * 2;
  dstp += dst_pitch * 4;

  for (int y = 4; y < height-4; y+= 2) {
    for (int x = 0; x < src_width / 2; ++x) {
      dstp[x*4] = srcY[x*2];
      dstp[x*4+2] = srcY[x*2+1];

      dstp[x*4+1] = ((((srcU[x-src_pitch_uv*2] + srcU[x] + 1) / 2) + srcU[x]) / 2);
      dstp[x*4+3] = ((((srcV[x-src_pitch_uv*2] + srcV[x] + 1) / 2) + srcV[x]) / 2);

      dstp[x*4 + dst_pitch*2] = srcY[x*2 + src_pitch_y*2];
      dstp[x*4+2 + dst_pitch*2] = srcY[x*2+1 + src_pitch_y*2];

      dstp[x*4+1 + dst_pitch*2] = ((((srcU[x] + srcU[x+src_pitch_uv*2] + 1) / 2) + srcU[x]) / 2);
      dstp[x*4+3 + dst_pitch*2] = ((((srcV[x] + srcV[x+src_pitch_uv*2] + 1) / 2) + srcV[x]) / 2);
    }

    if (y % 4 == 0) {
      //top field processed, jumb to the bottom
      srcY += src_pitch_y;
      dstp += dst_pitch;
      srcU += src_pitch_uv;
      srcV += src_pitch_uv;
    } else {
      //bottom field processed, jump to the next top
      srcY += src_pitch_y*3;
      srcU += src_pitch_uv;
      srcV += src_pitch_uv;
      dstp += dst_pitch*3;
    }
  }
}


#ifdef X86_32
void convert_yv12_to_yuy2_interlaced_isse(const BYTE* srcY, const BYTE* srcU, const BYTE* srcV, int src_rowsize, int src_pitch, int src_pitch_uv, 
                    BYTE* dst, int dst_pitch,
                    int height) 
{
  const BYTE** srcp= new const BYTE*[3];
  int src_pitch_uv2 = src_pitch_uv*2;
  int src_pitch_uv4 = src_pitch_uv*4;
  int skipnext = 0;

  int dst_pitch2=dst_pitch*2;
  int src_pitch2 = src_pitch*2;

  int dst_pitch4 = dst_pitch*4;
  int src_pitch4 = src_pitch*4;

  
  /**** Do first and last lines - NO interpolation:   *****/
  // MMX loop relies on C-code to adjust the lines for it.
  const BYTE* _srcY=srcY;
  const BYTE* _srcU=srcU;
  const BYTE* _srcV=srcV;
  BYTE* _dst=dst;
//
  for (int i=0;i<8;i++) {
    switch (i) {
    case 1:
      _srcY+=src_pitch2;  // Same chroma as in 0
      _dst+=dst_pitch2;
      break;
    case 2:
      _srcY-=src_pitch;  // Next field
      _dst-=dst_pitch;
      _srcU+=src_pitch_uv;
      _srcV+=src_pitch_uv;
      break;
    case 3:
      _srcY+=src_pitch2;  // Same  chroma as in 2
      _dst+=dst_pitch2;
      break;
    case 4: // Now we process the bottom four lines of the picture. 
      _srcY=srcY+(src_pitch*(height-4));
      _srcU=srcU+(src_pitch_uv*((height>>1)-2));
      _srcV=srcV+(src_pitch_uv*((height>>1)-2));
      _dst = dst+(dst_pitch*(height-4));
      break;
    case 5: // Same chroma as in 4
      _srcY += src_pitch2;
      _dst += dst_pitch2;
      break;
    case 6:  // Next field
      _srcY -= src_pitch;
      _dst -= dst_pitch;
      _srcU+=src_pitch_uv;
      _srcV+=src_pitch_uv;
      break;
    case 7:  // Same chroma as in 6
      _srcY += src_pitch2;
      _dst += dst_pitch2;
      default:  // Nothing, case 0
        break;
    }

    __asm {
	push ebx    // stupid compiler forgets to save ebx!!
    mov edi, [_dst]
    mov eax, [_srcY]
    mov ebx, [_srcU]
    mov ecx, [_srcV]
    mov edx,0
    pxor mm7,mm7
    jmp xloop_test_p
xloop_p:
    movq mm0,[eax]    //Y
      movd mm1,[ebx]  //U
    movq mm3,mm0  
     movd mm2,[ecx]   //V
    punpcklbw mm0,mm7  // Y low
     punpckhbw mm3,mm7   // Y high
    punpcklbw mm1,mm7   // 00uu 00uu
     punpcklbw mm2,mm7   // 00vv 00vv
    movq mm4,mm1
     movq mm5,mm2
    punpcklbw mm1,mm7   // 0000 00uu low
     punpcklbw mm2,mm7   // 0000 00vv low
    punpckhbw mm4,mm7   // 0000 00uu high
     punpckhbw mm5,mm7   // 0000 00vv high
    pslld mm1,8
     pslld mm4,8
    pslld mm2,24
     pslld mm5,24
    por mm0, mm1
     por mm3, mm4
    por mm0, mm2
     por mm3, mm5
    movq [edi],mm0
     movq [edi+8],mm3
    add eax,8
    add ebx,4
    add ecx,4
    add edx,8
    add edi, 16
xloop_test_p:
      cmp edx,[src_rowsize]
      jl xloop_p
	  pop ebx
    }
  }

/****************************************
 * Conversion main loop.
 * The code properly interpolates UV from
 * interlaced material.
 * We process two lines in the same field
 * in the same loop, to avoid reloading
 * chroma each time.
 *****************************************/

  height-=8;

  dst+=dst_pitch4;
  srcY+=src_pitch4;
  srcU+=src_pitch_uv2;
  srcV+=src_pitch_uv2;

  srcp[0] = srcY;
  srcp[1] = srcU-src_pitch_uv2;
  srcp[2] = srcV-src_pitch_uv2;

  int y=0;
  int x=0;
  
  __asm {
  push ebx    // stupid compiler forgets to save ebx!!
    mov esi, [srcp]
    mov edi, [dst]

    mov eax,[esi]
    mov ebx,[esi+4]
    mov ecx,[esi+8]
    mov edx,0
    jmp yloop_test
    align 16
yloop:
    mov edx,0               // x counter
    jmp xloop_test
    align 16
xloop:
    mov edx, src_pitch_uv2
      movq mm6, [add_ones]
    movq mm0,[eax]          // mm0 = Y current line
     pxor mm7,mm7
    movd mm2,[ebx+edx]            // mm2 = U top field
     movd mm3, [ecx+edx]          // mm3 = V top field
    movd mm4,[ebx]            // U prev top field
     movq mm1,mm0             // mm1 = Y current line
    movd mm5,[ecx]            // V prev top field
     pavgb mm4,mm2            // interpolate chroma U 
    pavgb mm5,mm3             // interpolate chroma V
     psubusb mm4, mm6         // Better rounding (thanks trbarry!)
    psubusb mm5, mm6
     pavgb mm4,mm2            // interpolate chroma U 
    pavgb mm5,mm3             // interpolate chroma V    
    punpcklbw mm0,mm7        // Y low
    punpckhbw mm1,mm7         // Y high*
     punpcklbw mm4,mm7        // U 00uu 00uu 00uu 00uu
    punpcklbw mm5,mm7         // V 00vv 00vv 00vv 00vv
     pxor mm6,mm6
    punpcklbw mm6,mm4         // U 0000 uu00 0000 uu00 (low)
     punpckhbw mm7,mm4         // V 0000 uu00 0000 uu00 (high
    por mm0,mm6
     por mm1,mm7
    movq mm6,mm5
     punpcklbw mm5,mm5          // V 0000 vvvv 0000 vvvv (low)
    punpckhbw mm6,mm6           // V 0000 vvvv 0000 vvvv (high)
     pslld mm5,24
    pslld mm6,24
     por mm0,mm5
    por mm1,mm6
    mov edx, src_pitch_uv4
     movq [edi],mm0
    movq [edi+8],mm1

    //Next line in same field
     movq mm6, [add_ones]     
    movd mm4,[ebx+edx]        // U next top field
     movd mm5,[ecx+edx]       // V prev top field
    mov edx, [src_pitch2]
     movq mm0,[eax+edx]        // Next Y-line
    pavgb mm4,mm2            // interpolate chroma U
     pavgb mm5,mm3             // interpolate chroma V
    psubusb mm4, mm6         // Better rounding (thanks trbarry!)
     psubusb mm5, mm6
    pavgb mm4,mm2            // interpolate chroma U
     pavgb mm5,mm3             // interpolate chroma V
    pxor mm7,mm7
    movq mm1,mm0             // mm1 = Y current line
     punpcklbw mm0,mm7        // Y low
    punpckhbw mm1,mm7         // Y high*
     punpcklbw mm4,mm7        // U 00uu 00uu 00uu 00uu
    punpcklbw mm5,mm7         // V 00vv 00vv 00vv 00vv
     pxor mm6,mm6
    punpcklbw mm6,mm4         // U 0000 uu00 0000 uu00 (low)
     punpckhbw mm7,mm4         // V 0000 uu00 0000 uu00 (high
    por mm0,mm6
     por mm1,mm7
    movq mm6,mm5
     punpcklbw mm5,mm5          // V 0000 vvvv 0000 vvvv (low)
    punpckhbw mm6,mm6           // V 0000 vvvv 0000 vvvv (high)
     pslld mm5,24
    mov edx,[dst_pitch2]
    pslld mm6,24
     por mm0,mm5
    por mm1,mm6
     movq [edi+edx],mm0
    movq [edi+edx+8],mm1
     add edi,16
    mov edx, [x]
     add eax, 8
    add ebx, 4
     add edx, 8
    add ecx, 4
xloop_test:
    cmp edx,[src_rowsize]
    mov x,edx
    jl xloop
    mov edi, dst
    mov eax,[esi]
    mov ebx,[esi+4]
    mov ecx,[esi+8]

    mov edx,skipnext
    cmp edx,1
    je dont_skip
    add edi,[dst_pitch]
    add eax,[src_pitch]
    add ebx,[src_pitch_uv]
    add ecx,[src_pitch_uv]
    mov [skipnext],1
    jmp yloop  // Never out of loop, if not skip
    align 16
dont_skip:
    add edi,[dst_pitch4]
    add eax,[src_pitch4]
    add ebx,[src_pitch_uv2]
    add ecx,[src_pitch_uv2]
    mov [skipnext],0
    mov edx, [y]
    mov [esi],eax
    mov [esi+4],ebx
    mov [esi+8],ecx
    mov [dst],edi
    add edx, 4

yloop_test:
    cmp edx,[height]
    mov [y],edx
    jl yloop
    sfence
    emms
    pop ebx
  }
   delete[] srcp;
}
#endif

/*************************************
 * Progessive YV12 -> YUY2 conversion
 *
 * (c) 2003, Klaus Post.
 *
 * Requires mod 8 pitch.
 *************************************/

#ifdef X86_32

void convert_yv12_to_yuy2_progressive_isse(const BYTE* srcY, const BYTE* srcU, const BYTE* srcV, int src_rowsize, int src_pitch, int src_pitch_uv, 
                    BYTE* dst, int dst_pitch,
                    int height)
{
  const BYTE** srcp= new const BYTE*[3];
  int src_pitch_uv2 = src_pitch_uv*2;
//  int src_pitch_uv4 = src_pitch_uv*4;

  int dst_pitch2=dst_pitch*2;
  int src_pitch2 = src_pitch*2;

  
  /**** Do first and last lines - NO interpolation:   *****/
  // MMX loop relies on C-code to adjust the lines for it.
  const BYTE* _srcY=srcY;
  const BYTE* _srcU=srcU;
  const BYTE* _srcV=srcV;
  BYTE* _dst=dst;

  for (int i=0;i<4;i++) {
    switch (i) {
    case 1:
      _srcY+=src_pitch;  // Same chroma as in 0
      _dst+=dst_pitch;
      break;
    case 2:
      _srcY=srcY+(src_pitch*(height-2));
      _srcU=srcU+(src_pitch_uv*((height>>1)-1));
      _srcV=srcV+(src_pitch_uv*((height>>1)-1));
      _dst = dst+(dst_pitch*(height-2));
      break;
    case 3: // Same chroma as in 4
      _srcY += src_pitch;
      _dst += dst_pitch;
      break;
    default:  // Nothing, case 0
        break;
    }

    __asm {
	push ebx    // stupid compiler forgets to save ebx!!
    mov edi, [_dst]
    mov eax, [_srcY]
    mov ebx, [_srcU]
    mov ecx, [_srcV]
    mov edx,0
    pxor mm7,mm7
    jmp xloop_test_p
xloop_p:
    movq mm0,[eax]    //Y
      movd mm1,[ebx]  //U
    movq mm3,mm0  
     movd mm2,[ecx]   //V
    punpcklbw mm0,mm7  // Y low
     punpckhbw mm3,mm7   // Y high
    punpcklbw mm1,mm7   // 00uu 00uu
     punpcklbw mm2,mm7   // 00vv 00vv
    movq mm4,mm1
     movq mm5,mm2
    punpcklbw mm1,mm7   // 0000 00uu low
     punpcklbw mm2,mm7   // 0000 00vv low
    punpckhbw mm4,mm7   // 0000 00uu high
     punpckhbw mm5,mm7   // 0000 00vv high
    pslld mm1,8
     pslld mm4,8
    pslld mm2,24
     pslld mm5,24
    por mm0, mm1
     por mm3, mm4
    por mm0, mm2
     por mm3, mm5
    movq [edi],mm0
     movq [edi+8],mm3
    add eax,8
    add ebx,4
    add ecx,4
    add edx,8
    add edi, 16
xloop_test_p:
      cmp edx,[src_rowsize]
      jl xloop_p
	  pop ebx
    }
  }

/****************************************
 * Conversion main loop.
 * The code properly interpolates UV from
 * interlaced material.
 * We process two lines in the same field
 * in the same loop, to avoid reloading
 * chroma each time.
 *****************************************/

  height-=4;

  dst+=dst_pitch2;
  srcY+=src_pitch2;
  srcU+=src_pitch_uv;
  srcV+=src_pitch_uv;

  srcp[0] = srcY;
  srcp[1] = srcU-src_pitch_uv;
  srcp[2] = srcV-src_pitch_uv;

  int y=0;
  int x=0;

  __asm {
  push ebx    // stupid compiler forgets to save ebx!!
    mov esi, [srcp]
    mov edi, [dst]

    mov eax,[esi]
    mov ebx,[esi+4]
    mov ecx,[esi+8]
    mov edx,0
    jmp yloop_test
    align 16
yloop:
    mov edx,0               // x counter
    jmp xloop_test
    align 16
xloop:
    movq mm6,[add_ones]
    mov edx, src_pitch_uv
    movq mm0,[eax]          // mm0 = Y current line
     pxor mm7,mm7
    movd mm2,[ebx+edx]            // mm2 = U top field
     movd mm3, [ecx+edx]          // mm3 = V top field
    movd mm4,[ebx]        // U prev top field
     movq mm1,mm0             // mm1 = Y current line
    movd mm5,[ecx]        // V prev top field
     pavgb mm4,mm2            // interpolate chroma U  (25/75)
    pavgb mm5,mm3             // interpolate chroma V  (25/75)
     psubusb mm4, mm6         // Better rounding (thanks trbarry!)
    psubusb mm5, mm6
     pavgb mm4,mm2            // interpolate chroma U 
    pavgb mm5,mm3             // interpolate chroma V
     punpcklbw mm0,mm7        // Y low
    punpckhbw mm1,mm7         // Y high*
     punpcklbw mm4,mm7        // U 00uu 00uu 00uu 00uu
    punpcklbw mm5,mm7         // V 00vv 00vv 00vv 00vv
     pxor mm6,mm6
    punpcklbw mm6,mm4         // U 0000 uu00 0000 uu00 (low)
     punpckhbw mm7,mm4         // V 0000 uu00 0000 uu00 (high
    por mm0,mm6
     por mm1,mm7
    movq mm6,mm5
     punpcklbw mm5,mm5          // V 0000 vvvv 0000 vvvv (low)
    punpckhbw mm6,mm6           // V 0000 vvvv 0000 vvvv (high)
     pslld mm5,24
    pslld mm6,24
     por mm0,mm5
    por mm1,mm6
    mov edx, src_pitch_uv2
     movq [edi],mm0
    movq [edi+8],mm1

    //Next line
     
     movq mm6,[add_ones]
    movd mm4,[ebx+edx]        // U next top field
     movd mm5,[ecx+edx]       // V prev top field
    mov edx, [src_pitch]
     pxor mm7,mm7
    movq mm0,[eax+edx]        // Next U-line
     pavgb mm4,mm2            // interpolate chroma U 
    movq mm1,mm0             // mm1 = Y current line
    pavgb mm5,mm3             // interpolate chroma V
     psubusb mm4, mm6         // Better rounding (thanks trbarry!)
    psubusb mm5, mm6
     pavgb mm4,mm2            // interpolate chroma U 
    pavgb mm5,mm3             // interpolate chroma V
     punpcklbw mm0,mm7        // Y low
    punpckhbw mm1,mm7         // Y high*
     punpcklbw mm4,mm7        // U 00uu 00uu 00uu 00uu
    punpcklbw mm5,mm7         // V 00vv 00vv 00vv 00vv
     pxor mm6,mm6
    punpcklbw mm6,mm4         // U 0000 uu00 0000 uu00 (low)
     punpckhbw mm7,mm4         // V 0000 uu00 0000 uu00 (high
    por mm0,mm6
     por mm1,mm7
    movq mm6,mm5
     punpcklbw mm5,mm5          // V 0000 vvvv 0000 vvvv (low)
    punpckhbw mm6,mm6           // V 0000 vvvv 0000 vvvv (high)
     pslld mm5,24
    mov edx,[dst_pitch]
    pslld mm6,24
     por mm0,mm5
    por mm1,mm6
     movq [edi+edx],mm0
    movq [edi+edx+8],mm1
     add edi,16
    mov edx, [x]
     add eax, 8
    add ebx, 4
     add edx, 8
    add ecx, 4
xloop_test:
    cmp edx,[src_rowsize]
    mov x,edx
    jl xloop
    mov edi, dst
    mov eax,[esi]
    mov ebx,[esi+4]
    mov ecx,[esi+8]

    add edi,[dst_pitch2]
    add eax,[src_pitch2]
    add ebx,[src_pitch_uv]
    add ecx,[src_pitch_uv]
    mov edx, [y]
    mov [esi],eax
    mov [esi+4],ebx
    mov [esi+8],ecx
    mov [dst],edi
    add edx, 2

yloop_test:
    cmp edx,[height]
    mov [y],edx
    jl yloop
    sfence
    emms
    pop ebx
  }
   delete[] srcp;
}
#endif


void convert_yuy2_to_yv12_progressive_c(const BYTE* src, int src_width, int src_pitch, BYTE* dstY, BYTE* dstU, BYTE* dstV, int dst_pitchY, int dst_pitchUV, int height) {
  //src_width is twice the luma width of yv12 frame
  const BYTE* srcp = src;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < src_width / 2 ; ++x) {
      dstY[x] = srcp[x*2];
    }
    dstY += dst_pitchY;
    srcp += src_pitch;
  }


  for (int y = 0; y < height / 2; ++y) {
    for (int x = 0; x < src_width / 4; ++x) {
      dstU[x] = (src[x*4+1] + src[x*4+1+src_pitch] + 1) / 2;
      dstV[x] = (src[x*4+3] + src[x*4+3+src_pitch] + 1) / 2;
    }
    dstU += dst_pitchUV;
    dstV += dst_pitchUV;
    src += src_pitch * 2;
  }
}

void convert_yuy2_to_yv12_interlaced_c(const BYTE* src, int src_width, int src_pitch, BYTE* dstY, BYTE* dstU, BYTE* dstV, int dst_pitchY, int dst_pitchUV, int height) {
  const BYTE* srcp = src;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < src_width / 2 ; ++x) {
      dstY[x] = srcp[x*2];
    }
    dstY += dst_pitchY;
    srcp += src_pitch;
  }

  for (int y = 0; y < height / 2; y+=2) {
    for (int x = 0; x < src_width / 4; ++x) {
      dstU[x] = ((src[x*4+1] + src[x*4+1+src_pitch*2] + 1) / 2 + src[x*4+1]) / 2;
      dstV[x] = ((src[x*4+3] + src[x*4+3+src_pitch*2] + 1) / 2 + src[x*4+3]) / 2;
    }
    dstU += dst_pitchUV;
    dstV += dst_pitchUV;
    src += src_pitch;

    for (int x = 0; x < src_width / 4; ++x) {
      dstU[x] = ((src[x*4+1] + src[x*4+1+src_pitch*2] + 1) / 2 + src[x*4+1+src_pitch*2]) / 2;
      dstV[x] = ((src[x*4+3] + src[x*4+3+src_pitch*2] + 1) / 2 + src[x*4+3+src_pitch*2]) / 2;
    }
    dstU += dst_pitchUV;
    dstV += dst_pitchUV;
    src += src_pitch*3;
  }
}

#ifdef X86_32

void convert_yuy2_to_yv12_progressive_isse(const BYTE* src, int src_width, int src_pitch, BYTE* dstY, BYTE* dstU, BYTE* dstV, int dst_pitchY, int dst_pitchUV, int height)
{
  __m64 luma_mask = _mm_set1_pi16(0x00FF);
  for (int y = 0; y < height/2; ++y) { 
    for (int x = 0; x < (src_width+3) / 4; x+=4) {
      __m64 src_lo_line0 = *reinterpret_cast<const __m64*>(src+x*4); //VYUY VYUY
      __m64 src_lo_line1 = *reinterpret_cast<const __m64*>(src+x*4+src_pitch);

      __m64 src_hi_line0 = *reinterpret_cast<const __m64*>(src+x*4+8);
      __m64 src_hi_line1 = *reinterpret_cast<const __m64*>(src+x*4+src_pitch+8);

      __m64 src_lo_line0_luma = _mm_and_si64(src_lo_line0, luma_mask);
      __m64 src_lo_line1_luma = _mm_and_si64(src_lo_line1, luma_mask);
      __m64 src_hi_line0_luma = _mm_and_si64(src_hi_line0, luma_mask);
      __m64 src_hi_line1_luma = _mm_and_si64(src_hi_line1, luma_mask);

      __m64 src_luma_line0 = _mm_packs_pu16(src_lo_line0_luma, src_hi_line0_luma);
      __m64 src_luma_line1 = _mm_packs_pu16(src_lo_line1_luma, src_hi_line1_luma);

      *reinterpret_cast<__m64*>(dstY + x*2) = src_luma_line0;
      *reinterpret_cast<__m64*>(dstY + x*2 + dst_pitchY) = src_luma_line1;

      __m64 avg_chroma_lo = _mm_avg_pu8(src_lo_line0, src_lo_line1);
      __m64 avg_chroma_hi = _mm_avg_pu8(src_hi_line0, src_hi_line1);

      __m64 chroma_lo = _mm_srli_si64(avg_chroma_lo, 8);
      __m64 chroma_hi = _mm_srli_si64(avg_chroma_hi, 8);

      chroma_lo = _mm_and_si64(luma_mask, chroma_lo); //0V0U 0V0U
      chroma_hi = _mm_and_si64(luma_mask, chroma_hi); //0V0U 0V0U

      __m64 chroma = _mm_packs_pu16(chroma_lo, chroma_hi); //VUVU VUVU

      __m64 chroma_u = _mm_and_si64(luma_mask, chroma); //0U0U 0U0U
      __m64 chroma_v = _mm_andnot_si64(luma_mask, chroma); //V0V0 V0V0
      chroma_v = _mm_srli_si64(chroma_v, 8); //0V0V 0V0V
      
      chroma_u = _mm_packs_pu16(chroma_u, luma_mask);
      chroma_v = _mm_packs_pu16(chroma_v, luma_mask);

      *reinterpret_cast<int*>(dstU+x) = _mm_cvtsi64_si32(chroma_u);
      *reinterpret_cast<int*>(dstV+x) = _mm_cvtsi64_si32(chroma_v);
    }

    src += src_pitch*2;
    dstY += dst_pitchY * 2;
    dstU += dst_pitchUV;
    dstV += dst_pitchUV;
  }
  _mm_empty();
}

//75% of the first argument and 25% of the second one. 
static __forceinline __m64 convert_yuy2_to_yv12_merge_chroma_isse(const __m64 &line75p, const __m64 &line25p, const __m64 &one, const __m64 &luma_mask) {
  __m64 avg_chroma_lo = _mm_avg_pu8(line75p, line25p);
  avg_chroma_lo = _mm_subs_pu8(avg_chroma_lo, one);
  avg_chroma_lo = _mm_avg_pu8(avg_chroma_lo, line75p);
  __m64 chroma_lo = _mm_srli_si64(avg_chroma_lo, 8);
  return _mm_and_si64(luma_mask, chroma_lo); //0V0U 0V0U
}

void convert_yuy2_to_yv12_interlaced_isse(const BYTE* src, int src_width, int src_pitch, BYTE* dstY, BYTE* dstU, BYTE* dstV, int dst_pitchY, int dst_pitchUV, int height) {
  __m64 one = _mm_set1_pi8(1);
  __m64 luma_mask = _mm_set1_pi16(0x00FF);

  for (int y = 0; y < height / 2; y+=2) {
    for (int x = 0; x < src_width / 4; x+=4) {
      __m64 src_lo_line0 = *reinterpret_cast<const __m64*>(src+x*4); //VYUY VYUY
      __m64 src_lo_line1 = *reinterpret_cast<const __m64*>(src+x*4+src_pitch*2);

      __m64 src_hi_line0 = *reinterpret_cast<const __m64*>(src+x*4+8);
      __m64 src_hi_line1 = *reinterpret_cast<const __m64*>(src+x*4+src_pitch*2+8);

      __m64 chroma_lo = convert_yuy2_to_yv12_merge_chroma_isse(src_lo_line0, src_lo_line1, one, luma_mask);
      __m64 chroma_hi = convert_yuy2_to_yv12_merge_chroma_isse(src_hi_line0, src_hi_line1, one, luma_mask);

      __m64 chroma = _mm_packs_pu16(chroma_lo, chroma_hi); //VUVU VUVU

      __m64 chroma_u = _mm_and_si64(luma_mask, chroma); //0U0U 0U0U
      __m64 chroma_v = _mm_andnot_si64(luma_mask, chroma); //V0V0 V0V0
      chroma_v = _mm_srli_si64(chroma_v, 8); //0V0V 0V0V

      chroma_u = _mm_packs_pu16(chroma_u, luma_mask);
      chroma_v = _mm_packs_pu16(chroma_v, luma_mask);

      *reinterpret_cast<int*>(dstU+x) = _mm_cvtsi64_si32(chroma_u);
      *reinterpret_cast<int*>(dstV+x) = _mm_cvtsi64_si32(chroma_v);

      __m64 src_lo_line0_luma = _mm_and_si64(src_lo_line0, luma_mask);
      __m64 src_lo_line1_luma = _mm_and_si64(src_lo_line1, luma_mask);
      __m64 src_hi_line0_luma = _mm_and_si64(src_hi_line0, luma_mask);
      __m64 src_hi_line1_luma = _mm_and_si64(src_hi_line1, luma_mask);

      __m64 src_luma_line0 = _mm_packs_pu16(src_lo_line0_luma, src_hi_line0_luma);
      __m64 src_luma_line1 = _mm_packs_pu16(src_lo_line1_luma, src_hi_line1_luma);

      *reinterpret_cast<__m64*>(dstY + x*2) = src_luma_line0;
      *reinterpret_cast<__m64*>(dstY + x*2 + dst_pitchY*2) = src_luma_line1;
    }
    dstU += dst_pitchUV;
    dstV += dst_pitchUV;
    dstY += dst_pitchY;
    src += src_pitch;

    for (int x = 0; x < src_width / 4; x+=4) {
      __m64 src_lo_line0 = *reinterpret_cast<const __m64*>(src+x*4); //VYUY VYUY
      __m64 src_lo_line1 = *reinterpret_cast<const __m64*>(src+x*4+src_pitch*2);

      __m64 src_hi_line0 = *reinterpret_cast<const __m64*>(src+x*4+8);
      __m64 src_hi_line1 = *reinterpret_cast<const __m64*>(src+x*4+src_pitch*2+8);

      __m64 chroma_lo = convert_yuy2_to_yv12_merge_chroma_isse(src_lo_line1, src_lo_line0, one, luma_mask);
      __m64 chroma_hi = convert_yuy2_to_yv12_merge_chroma_isse(src_hi_line1, src_hi_line0, one, luma_mask);

      __m64 chroma = _mm_packs_pu16(chroma_lo, chroma_hi); //VUVU VUVU

      __m64 chroma_u = _mm_and_si64(luma_mask, chroma); //0U0U 0U0U
      __m64 chroma_v = _mm_andnot_si64(luma_mask, chroma); //V0V0 V0V0
      chroma_v = _mm_srli_si64(chroma_v, 8); //0V0V 0V0V

      chroma_u = _mm_packs_pu16(chroma_u, luma_mask);
      chroma_v = _mm_packs_pu16(chroma_v, luma_mask);

      *reinterpret_cast<int*>(dstU+x) = _mm_cvtsi64_si32(chroma_u);
      *reinterpret_cast<int*>(dstV+x) = _mm_cvtsi64_si32(chroma_v);

      __m64 src_lo_line0_luma = _mm_and_si64(src_lo_line0, luma_mask);
      __m64 src_lo_line1_luma = _mm_and_si64(src_lo_line1, luma_mask);
      __m64 src_hi_line0_luma = _mm_and_si64(src_hi_line0, luma_mask);
      __m64 src_hi_line1_luma = _mm_and_si64(src_hi_line1, luma_mask);

      __m64 src_luma_line0 = _mm_packs_pu16(src_lo_line0_luma, src_hi_line0_luma);
      __m64 src_luma_line1 = _mm_packs_pu16(src_lo_line1_luma, src_hi_line1_luma);

      *reinterpret_cast<__m64*>(dstY + x*2) = src_luma_line0;
      *reinterpret_cast<__m64*>(dstY + x*2 + dst_pitchY*2) = src_luma_line1;
    }
    dstU += dst_pitchUV;
    dstV += dst_pitchUV;
    dstY += dst_pitchY*3;
    src += src_pitch*3;
  }
  _mm_empty();
}

#endif
