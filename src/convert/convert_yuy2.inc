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

	/*****************************	
	 * MMX code by Klaus Post
	 * Updated for PC Levels/Rec.709
     *   by Ian Brabham Nov 2004
     *
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

#ifdef RGB24
#ifdef DUPL

//void mmx_ConvertRGB??toYUY2(unsigned int *src,unsigned int *dst,int src_pitch, int dst_pitch,int w, int h, int matrix) {
#if RGB24
  int lwidth_bytes = w*3;    // Width in bytes
#else
  int lwidth_bytes = w<<2;    // Width in bytes
#endif
  src+=src_pitch*(h-1);       // ;Move source to bottom line (read top->bottom)

#define SRC eax
#define DST edi
#define MATRIX esi
#define RGBOFFSET ecx
#define YUVOFFSET edx

	for (int y=0;y<h;y++) {
	__asm {
		mov         SRC,src
		mov         DST,dst
		mov         MATRIX,matrix
		mov         RGBOFFSET,0
		mov         YUVOFFSET,0
        
		movd        mm0,[fraction+MATRIX*4]
		movq        mm7,[cybgr_64+MATRIX*8]
		movd        mm5,[y1y2_mult+MATRIX*4]

		movq        mm2,[SRC+RGBOFFSET] ; mm2= XXR2 G2B2 XXR1 G1B1
		cmp         RGBOFFSET,[lwidth_bytes]
		punpcklbw   mm1,mm2             ; mm1= XXxx R1xx G1xx B1xx
#if RGB24
		psllq       mm2,8               ; Compensate for RGB24
#endif

		align 16
re_enter:
		 punpckhbw   mm2,mm0             ; mm2= 00XX 00R2 00G2 00B2 
		psrlw       mm1,8               ; mm1= 00XX 00R1 00G1 00B1
		 jge         outloop             ; Jump out of loop if true (width==0)

		movq        mm6,mm1             ; mm6= 00XX 00R1 00G1 00B1
		 pmaddwd     mm1,mm7             ; mm1= v2v2 v2v2 v1v1 v1v1   y1 //(cyb*rgb[0] + cyg*rgb[1] + cyr*rgb[2] + 0x108000)
#if DUPL
		paddw       mm6,mm6             ; mm6 = accumulated RGB values (for b_y and r_y)
#else
		paddw       mm6,mm2             ; mm6 = accumulated RGB values (for b_y and r_y)
#endif
		 pmaddwd     mm2,mm7             ; mm2= w2w2 w2w2 w1w1 w1w1   y2 //(cyb*rgbnext[0] + cyg*rgbnext[1] + cyr*rgbnext[2] + 0x108000)
		paddd       mm1,mm0             ; Add rounding fraction (16.5)<<15 to lower dword only
		 paddd       mm2,mm0             ; Add rounding fraction (16.5)<<15 to lower dword only
		movq        mm3,mm1
		 movq        mm4,mm2
		psrlq       mm3,32
		 pand        mm6,[rb_mask]       ; Clear out accumulated G-value mm6= 0000 RRRR 0000 BBBB
		psrlq       mm4,32
		 paddd       mm1,mm3
		paddd       mm2,mm4
		 psrld       mm1,15              ; mm1= xxxx xxxx 0000 00y1 final value
		movd        mm3,[sub_32+MATRIX*4]; mm3 = -32
		 psrld       mm2,15              ; mm2= xxxx xxxx 0000 00y2 final value
		paddw       mm3,mm1
		 pslld       mm6,14              ; Shift up accumulated R and B values (<<15 in C)
#if DUPL
		paddw       mm3,mm1             ; mm3 = y1+y1-32
#else
		paddw       mm3,mm2             ; mm3 = y1+y2-32
#endif
		 psllq       mm2,16              ; mm2 Y2 shifted up (to clear fraction) mm2 ready
		pmaddwd     mm3,mm5             ; mm3=scaled_y (latency 2 cycles)
		 por         mm1,mm2             ; mm1 = 0000 0000 00Y2 00Y1
		punpckldq   mm3,mm3             ; Move scaled_y to upper dword mm3=SCAL ED_Y SCAL ED_Y 
		 movq        mm2,[fpix_mul+MATRIX*8]
		psubd       mm6,mm3             ; mm6 = b_y and r_y
		 movq        mm4,[fpix_add]
		psrad       mm6,14              ; Shift down b_y and r_y (>>10 in C-code) 
		 movq        mm3,[chroma_mask2]
		pmaddwd     mm6,mm2             ; Mult b_y and r_y 
		 add         YUVOFFSET,4         ; Two pixels (packed)
		paddd       mm6,mm4             ; Add 0x808000 to r_y and b_y 
#if RGB24
		 add         RGBOFFSET,6
#else
		 add         RGBOFFSET,8
#endif
		pand        mm6,mm3             ; Clear out fractions
		 movq        mm2,[SRC+RGBOFFSET] ; mm2= XXR2 G2B2 XXR1 G1B1
		packuswb    mm6,mm6             ; mm6 = VV00 UU00 VV00 UU00
		 cmp         RGBOFFSET,[lwidth_bytes]
		por         mm6,mm1             ; Or luma and chroma together			
		 punpcklbw   mm1,mm2             ; mm1= XXxx R1xx G1xx B1xx
		movd        [DST+YUVOFFSET-4],mm6 ; Store final pixel						
#if RGB24
		 psllq       mm2,8               ; Compensate for RGB24
#endif
		jmp         re_enter            ; loop if true
        
        align 16
outloop:
		} // end asm
		src -= src_pitch;
		dst += dst_pitch;
	} // end for y

#undef SRC
#undef DST
#undef RGBOFFSET
#undef YUVOFFSET

#undef RGB24
#undef DUPL

#endif
#endif
