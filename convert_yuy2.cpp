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



	/*****************************	
	 * MMX code by Klaus Post
	 * - Notes on MMX:
	 * Fractions are one bit less than integer code,
	 *  but otherwise the algorithm is the same, except
	 *  r_y and b_y are calculated at the same time.
	 * Order of executin has been changed much for better pairing possibilities.
	 * It is important that the 64bit values are 8 byte-aligned
	 *  otherwise it will give a huge penalty when accessing them.
   * Instructions pair rather ok, instructions from the top is merged
	 *  into last part, to avoid dependency stalls.
	 *  (paired instrucions are indented by a space)
	 *****************************/


void mmx_ConvertRGB32toYUY2(unsigned int *src,unsigned int *dst,int src_pitch, int dst_pitch,int w, int h) {
	__declspec(align(8)) static const __int64 rgb_mask = 0x00ffffff00ffffff;
	__declspec(align(8)) static const __int64 fraction = 0x0000000000084000;    //= 0x108000/2 = 0x84000
	__declspec(align(8)) static const __int64 add_32 = 0x000000000000000020;    //= 32 shifted 15 up
	__declspec(align(8)) static const __int64 rb_mask = 0x0000ffff0000ffff;    //=Mask for unpacked R and B
  __declspec(align(8)) static const __int64 y1y2_mult = 0x0000000000004A85;
	__declspec(align(8)) static const __int64 fpix_add =	0x0080800000808000;
	__declspec(align(8)) static const __int64 fpix_mul =  0x000000282000001fb;
	__declspec(align(8)) static const __int64 chroma_mask =  0x000000000ff00ff00;
	__declspec(align(8)) static const __int64 low32_mask =  0x000000000ffffffff;
  //  const int cyb = int(0.114*219/255*32768+0.5);
  //  const int cyg = int(0.587*219/255*32768+0.5);
  //  const int cyr = int(0.299*219/255*32768+0.5);
  //	__declspec(align(8)) const __int64 cybgr_64 = (__int64)cyb|(((__int64)cyg)<<16)|(((__int64)cyr)<<32);
	__declspec(align(8)) const __int64 cybgr_64 = 0x000020DE40870c88;

	int lwidth_bytes = w<<2;    // Width in bytes
	src+=src_pitch*(h-1);       // ;Move source to bottom line (read top->bottom)


#define SRC eax
#define DST edi
#define RGBOFFSET ecx
#define YUVOFFSET edx

	for (int y=0;y<h;y++) {
	__asm {
		mov SRC,src
		mov DST,dst
		mov RGBOFFSET,0
		mov YUVOFFSET,0
		cmp       RGBOFFSET,[lwidth_bytes]
		jge       outloop		; Jump out of loop if true (width==0?? - somebody brave should remove this test)
		movq mm3,[rgb_mask]
		movq mm0,[SRC+RGBOFFSET]		; mm0= XXR2 G2B2 XXR1 G1B1
		pand mm0,mm3								; mm0= 00R2 G2B2 00R1 G1B1
		punpcklbw mm1,mm0						; mm1= 0000 R100 G100 B100
		movq mm4,[cybgr_64]
		align 16
re_enter:
		punpckhbw mm2,mm0				; mm2= 0000 R200 G200 B200
		 movq mm3,[fraction]
		psrlw mm1,8							; mm1= 0000 00R1 00G1 00B1
	  psrlw mm2,8							; mm2= 0000 00R2 00G2 00B2 
		 movq mm6,mm1						; mm6= 0000 00R1 00G1 00B1 (shifter unit stall)
		pmaddwd mm1,mm4						; mm1= v2v2 v2v2 v1v1 v1v1   y1 //(cyb*rgb[0] + cyg*rgb[1] + cyr*rgb[2] + 0x108000)
		 movq mm7,mm2						; mm7= 0000 00R2 00G2 00B2		 
		  movq mm0,[rb_mask]
		pmaddwd mm2,mm4						; mm1= w2w2 w2w2 w1w1 w1w1   y2 //(cyb*rgbnext[0] + cyg*rgbnext[1] + cyr*rgbnext[2] + 0x108000)
		 paddd mm1,mm3						; Add rounding fraction
		  paddw mm6,mm7					; mm6 = accumulated RGB values (for b_y and r_y) 
		paddd mm2,mm3						; Add rounding fraction (lower dword only)
		 movq mm4,mm1
		movq mm5,mm2
		pand mm1,[low32_mask]
		 psrlq mm4,32
		pand mm6,mm0						; Clear out accumulated G-value mm6= 0000 RRRR 0000 BBBB
		pand mm2,[low32_mask]
		 psrlq mm5,32
		paddd mm1,mm4				    ;mm1 Contains final y1 value (shifted 15 up)
		 psllq mm6, 14					; Shift up accumulated R and B values (<<15 in C)
		paddd mm2,mm5					  ;mm2 Contains final y2 value (shifted 15 up)
		 psrlq mm1,15
		movq mm3,mm1
		 psrlq mm2,15
		movq mm4,[add_32]
		 paddd mm3,mm2					;mm3 = y1+y2
		movq mm5,[y1y2_mult]
		 psubd mm3,mm4					; mm3 = y1+y2-32 		mm0,mm4,mm5,mm7 free
    movq mm0,[fpix_add]			; Constant that should be added to final UV pixel
	   pmaddwd mm3,mm5				; mm3=scaled_y (latency 2 cycles)
      movq mm4,[fpix_mul]		; Constant that should be multiplied to final UV pixel	  
		  psllq mm2,16					; mm2 Y2 shifted up (to clear fraction) mm2 ready
		punpckldq mm3,mm3						; Move scaled_y to upper dword mm3=SCAL ED_Y SCAL ED_Y 
		psubd mm6,mm3								; mm6 = b_y and r_y (stall)
		psrld mm6,9									; Shift down b_y and r_y (>>10 in C-code) 
		 por mm1,mm2								; mm1 = 0000 0000 00Y2 00Y1
		pmaddwd mm6,mm4							; Mult b_y and r_y 
		 pxor mm2,mm2
		 movq mm7,[chroma_mask]
		paddd mm6, mm0							; Add 0x800000 to r_y and b_y 
		 add RGBOFFSET,8
		psrld mm6,9									; Move down, so fraction is only 7 bits
		 cmp       RGBOFFSET,[lwidth_bytes]
		jge       outloop						; Jump out of loop if true
		packssdw mm6,mm2						; mm6 = 0000 0000 VVVV UUUU (7 bits fraction) (values above 0xff are saturated)
		 movq mm3,[rgb_mask]				;														[From top (to get better pairing)]
		psllq mm6,1									; Move up, so fraction is 8 bit
		 movq mm0,[SRC+RGBOFFSET]		; mm0= XXR2 G2B2 XXR1 G1B1	[From top (to get better pairing)]
		pand mm6,mm7					; Clear out fractions
		 pand mm0,mm3								; mm0= 00R2 G2B2 00R1 G1B1  [From top (to get better pairing)]
		por mm6,mm1									; Or luma and chroma together			
		 movq mm4,[cybgr_64]        ;                           [From top (to get better pairing)]
		movd [DST+YUVOFFSET],mm6		; Store final pixel						
		 punpcklbw mm1,mm0					; mm1= 0000 R100 G100 B100
		add YUVOFFSET,4			// Two pixels (packed)
		jmp re_enter
outloop:
		// Do store without loading next pixel
		packssdw mm6,mm2			; mm6 = 0000 0000 VVVV UUUU (7 bits fraction) (values above 0xff are saturated)
		psllq mm6,1						; Move up, so fraction is 8 bit
		pand mm6,mm7					; Clear out fractions
		por mm1,mm6						; Or luma and chroma together
		movd [DST+YUVOFFSET],mm1	; Store final pixel
		} // end asm
		src -= src_pitch;
		dst += dst_pitch;
	} // end for y

#undef SRC
#undef DST
#undef RGBOFFSET
#undef YUVOFFSET
 
}


void mmx_ConvertRGB32toYUY2_Dup(unsigned int *src,unsigned int *dst,int src_pitch, int dst_pitch,int w, int h) {
	__declspec(align(8)) static const __int64 rgb_mask = 0x00ffffff00ffffff;
	__declspec(align(8)) static const __int64 fraction = 0x0000000000084000;    //= 0x108000/2 = 0x84000
	__declspec(align(8)) static const __int64 add_32 = 0x000000000000000020;    //= 32 shifted 15 up
	__declspec(align(8)) static const __int64 rb_mask = 0x0000ffff0000ffff;    //=Mask for unpacked R and B
  __declspec(align(8)) static const __int64 y1y2_mult = 0x0000000000004A85;
	__declspec(align(8)) static const __int64 fpix_add =	0x00807fff00807fff;
	__declspec(align(8)) static const __int64 fpix_mul =  0x000000282000001fb;
	__declspec(align(8)) static const __int64 chroma_mask =  0x000000000ff00ff00;
	__declspec(align(8)) static const __int64 low32_mask =  0x000000000ffffffff;
	__declspec(align(8)) const __int64 cybgr_64 = 0x000020DE40870c88;

	int lwidth_bytes = w<<2;    // Width in bytes
	src+=src_pitch*(h-1);       // ;Move source to bottom line (read top->bottom)



#define SRC eax
#define DST edi
#define RGBOFFSET ecx
#define YUVOFFSET edx

	for (int y=0;y<h;y++) {
	__asm {
		mov SRC,src
		mov DST,dst
		mov RGBOFFSET,0
		mov YUVOFFSET,0
		cmp       RGBOFFSET,[lwidth_bytes]
		jge       outloop		; Jump out of loop if true (width==0?? - somebody brave should remove this test)
		movq mm3,[rgb_mask]
		movq mm0,[SRC+RGBOFFSET]		; mm0= XXR2 G2B2 XXR1 G1B1
		pand mm0,mm3								; mm0= 00R2 G2B2 00R1 G1B1
		punpcklbw mm1,mm0						; mm1= 0000 R100 G100 B100
		movq mm4,[cybgr_64]
		align 16
re_enter:
		punpckhbw mm2,mm0				; mm2= 0000 R200 G200 B200
		 movq mm3,[fraction]
		psrlw mm1,8							; mm1= 0000 00R1 00G1 00B1
	  psrlw mm2,8							; mm2= 0000 00R2 00G2 00B2 (shifter unit stall)
		 movq mm6,mm1						; mm6= 0000 00R1 00G1 00B1 
		pmaddwd mm1,mm4						; mm1= v2v2 v2v2 v1v1 v1v1   y1 //(cyb*rgb[0] + cyg*rgb[1] + cyr*rgb[2] + 0x108000)
			pand mm6,[rb_mask]			; Clear out accumulated G-value mm6= 0000 RRRR 0000 BBBB
		pmaddwd mm2,mm4						; mm1= w2w2 w2w2 w1w1 w1w1   y2 //(cyb*rgbnext[0] + cyg*rgbnext[1] + cyr*rgbnext[2] + 0x108000)
		 paddd mm1,mm3						; Add rounding fraction
		paddd mm2,mm3						; Add rounding fraction (lower dword only)
		 movq mm4,mm1
		movq mm5,mm2
		pand mm1,[low32_mask]
		 psrlq mm4,32
		pand mm2,[low32_mask]
		 psrlq mm5,32
		paddd mm1,mm4				    ;mm1 Contains final y1 value (shifted 15 up)
		 psllq mm6, 15					; Shift up first pixe R and B values (<<16 in C)
		paddd mm2,mm5					  ;mm2 Contains final y2 value (shifted 15 up)
		 psrlq mm1,15
		movq mm3,mm1
		psrlq mm2,15
		movq mm4,[add_32]
			pslld mm3,1						; mm3=y1*2
		movq mm5,[y1y2_mult]
		 psubd mm3,mm4					; mm3 = y1+y2-32 		mm0,mm4,mm5,mm7 free
    movq mm0,[fpix_add]			; Constant that should be added to final UV pixel
	   pmaddwd mm3,mm5				; mm3=scaled_y (latency 2 cycles)
      movq mm4,[fpix_mul]		; Constant that should be multiplied to final UV pixel	  
		  psllq mm2,16					; mm2 Y2 shifted up (to clear fraction) mm2 ready
		punpckldq mm3,mm3						; Move scaled_y to upper dword mm3=SCAL ED_Y SCAL ED_Y 
		psubd mm6,mm3								; mm6 = b_y and r_y (stall)
		psrld mm6,9									; Shift down b_y and r_y (>>10 in C-code) 
		 por mm1,mm2								; mm1 = 0000 0000 00Y2 00Y1
		pmaddwd mm6,mm4							; Mult b_y and r_y 
		 pxor mm2,mm2
		 movq mm7,[chroma_mask]
		paddd mm6, mm0							; Add 0x800000 to r_y and b_y 
		 add RGBOFFSET,8
		psrld mm6,9									; Move down, so fraction is only 7 bits
		 cmp       RGBOFFSET,[lwidth_bytes]
		jge       outloop						; Jump out of loop if true
		packssdw mm6,mm2						; mm6 = 0000 0000 VVVV UUUU (7 bits fraction) (values above 0xff are saturated)
		 movq mm3,[rgb_mask]				;														[From top (to get better pairing)]
		psllq mm6,1									; Move up, so fraction is 8 bit
		 movq mm0,[SRC+RGBOFFSET]		; mm0= XXR2 G2B2 XXR1 G1B1	[From top (to get better pairing)]
		pand mm6,mm7					; Clear out fractions
		 pand mm0,mm3								; mm0= 00R2 G2B2 00R1 G1B1  [From top (to get better pairing)]
		por mm6,mm1									; Or luma and chroma together			
		 movq mm4,[cybgr_64]        ;                           [From top (to get better pairing)]
		movd [DST+YUVOFFSET],mm6		; Store final pixel						
		 punpcklbw mm1,mm0					; mm1= 0000 R100 G100 B100
		add YUVOFFSET,4			// Two pixels (packed)
		jmp re_enter
outloop:
		// Do store without loading next pixel
		packssdw mm6,mm2			; mm6 = 0000 0000 VVVV UUUU (7 bits fraction) (values above 0xff are saturated)
		psllq mm6,1						; Move up, so fraction is 8 bit
		pand mm6,mm7					; Clear out fractions
		por mm1,mm6						; Or luma and chroma together
		movd [DST+YUVOFFSET],mm1	; Store final pixel
		} // end asm
		src -= src_pitch;
		dst += dst_pitch;
	} // end for y

#undef SRC
#undef DST
#undef RGBOFFSET
#undef YUVOFFSET
 
}

