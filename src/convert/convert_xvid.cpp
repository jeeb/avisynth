;/*****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *	 mmx yuv planar to rgb conversion
; *
; * Copyright (C) 2001 - Michael Militzer <isibaar@xvid.org>
; *
; *  This file is part of XviD, a free MPEG-4 video encoder/decoder
; *
; *  XviD is free software; you can redistribute it and/or modify it
; *  under the terms of the GNU General Public License as published by
; *  the Free Software Foundation; either version 2 of the License, or
; *  (at your option) any later version.
; *
; *  This program is distributed in the hope that it will be useful,
; *  but WITHOUT ANY WARRANTY; without even the implied warranty of
; *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; *  GNU General Public License for more details.
; *
; *  You should have received a copy of the GNU General Public License
; *  along with this program; if not, write to the Free Software
; *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
; *
; *  Under section 8 of the GNU General Public License, the copyright
; *  holders of XVID explicitly forbid distribution in the following
; *  countries:
; *
; *    - Japan
; *    - United States of America
; *
; *  Linking XviD statically or dynamically with other modules is making a
; *  combined work based on XviD.  Thus, the terms and conditions of the
; *  GNU General Public License cover the whole combination.
; *
; *  As a special exception, the copyright holders of XviD give you
; *  permission to link XviD with independent modules that communicate with
; *  XviD solely through the VFW1.1 and DShow interfaces, regardless of the
; *  license terms of these independent modules, and to copy and distribute
; *  the resulting combined work under terms of your choice, provided that
; *  every copy of the combined work is accompanied by a complete copy of
; *  the source code of XviD (the version of XviD used to produce the
; *  combined work), being distributed under the terms of the GNU General
; *  Public License plus this exception.  An independent module is a module
; *  which is not derived from or based on XviD.
; *
; *  Note that people who make modified versions of XviD are not obligated
; *  to grant this special exception for their modified versions; it is
; *  their choice whether to do so.  The GNU General Public License gives
; *  permission to release a modified version without this exception; this
; *  exception also makes it possible to release a modified version which
; *  carries forward this exception.
; *
; * $Id: convert_xvid.cpp,v 1.1 2003/11/27 09:32:50 sh0dan Exp $
; *
;------------------------------------------------------------------------------
; NB:	n contrary to the c implementation this code does the conversion
;      using direct calculations. Input data width must be a multiple of 8
;      and height must be even.
;      This implementation is less precise than the c version but is
;      more than twice as fast :-)
;------------------------------------------------------------------------------
; ****************************************************************************/
;

#include "stdafx.h"

#include "convert_xvid.h"

__declspec(align(8)) static __int64 Y_SUB = 0x0010001000100010;
__declspec(align(8)) static __int64 U_SUB = 0x0080008000800080;
__declspec(align(8)) static __int64 Y_MUL = 0x004a004a004a004a;
__declspec(align(8)) static __int64 UG_MUL = 0x0019001900190019;
__declspec(align(8)) static __int64 VG_MUL = 0x0034003400340034;
__declspec(align(8)) static __int64 UB_MUL = 0x0081008100810081;
__declspec(align(8)) static __int64 VR_MUL = 0x0066006600660066;



void __declspec(naked) yv12_to_rgb32_mmx(BYTE *dst, 
                         int dst_stride, 
                         const BYTE *y_src,
                         const BYTE *u_src,
                         const BYTE *v_src, 
                         int y_stride, int uv_stride,
                         int width, int height) {

#define localsize 72
#define TEMP_Y1  esp
#define TEMP_Y2  esp + 8
#define TEMP_G1  esp + 16
#define TEMP_G2  esp + 24
#define TEMP_B1  esp + 32
#define TEMP_B2  esp + 40
#define y_dif    esp + 48
#define dst_dif	 esp + 52
#define uv_dif   esp + 56
#define height   esp + 60
#define width_8  esp + 64
#define height_2 esp + 68
#define SCALEBITS 6

  __asm {

	push ebx
	push esi
	push edi
	push ebp

	;  local vars alloc
	sub esp, localsize

	; function code 
	mov eax, [esp + 52 + localsize]	          ; height -> eax
	cmp eax, 0x00
	jge near dont_flip		                    ; flip?

	neg eax				; neg height
	mov [height], eax		

	mov esi, [esp + 48 + localsize]	          ; width -> esi

	mov ebp, [esp + 40 + localsize]	          ; y_stride -> ebp
	mov ebx, ebp
	shl ebx, 1			                          ; 2 * y_stride -> ebx
	neg ebx
	sub ebx, esi			                        ; y_dif -> eax

	mov [y_dif], ebx

	sub eax, 1			                          ; height - 1 -> eax
	mul ebp				                            ; (height - 1) * y_stride -> ebp
	mov ecx, eax
	mov eax, [esp + 28 + localsize]	          ; y_src -> eax
	add eax, ecx			                        ; y_src -> eax
	mov ebx, eax
	sub ebx, ebp			                        ; y_src2 -> ebx

	mov ecx, [esp + 24 + localsize]	          ; dst_stride -> ecx
	mov edx, ecx
	shl edx, 3
	mov ecx, edx			                        ; 8 * dst_stride -> ecx
	shl esi, 2
	sub ecx, esi			                        ; 8 * dst_stride - 4 * width -> ecx

	mov [dst_dif], ecx

	mov esi, [esp + 20 + localsize]	          ; dst -> esi
	mov edi, esi
	shr edx, 1
	add edi, edx			                        ; dst2 -> edi

	mov ebp, [esp + 48 + localsize]	          ; width -> ebp
	mov ecx, ebp			                        ; width -> ecx
	shr ecx, 1
	shr ebp, 3			                          ; width / 8 -> ebp
	mov [width_8], ebp

	mov ebp, [esp + 44 + localsize]	; uv_stride -> ebp
	mov edx, ebp
	neg edx
	sub edx, ecx
	mov [uv_dif], edx

	mov edx, ebp
	mov ebp, eax
	mov eax, [height]		; height -> eax
	shr eax, 1			; height / 2 -> eax

	mov ecx, [esp + 32 + localsize]	; u_src -> ecx
	sub eax, 1
	mul edx
	add ecx, eax

	mov edx, [esp + 36 + localsize]	; v_src -> edx
	add edx, eax

	mov eax, ebp

	mov ebp, [height]		; height -> ebp
	shr ebp, 1			; height / 2 -> ebp

	pxor mm7, mm7
	jmp y_loop

  align 16
dont_flip:
	mov esi, [esp + 48 + localsize]	; width -> esi

	mov ebp, [esp + 40 + localsize]	; y_stride -> ebp
	mov ebx, ebp
	shl ebx, 1			; 2 * y_stride -> ebx
	sub ebx, esi			; y_dif -> ebx

	mov [y_dif], ebx

	mov eax, [esp + 28 + localsize]	; y_src -> eax
	mov ebx, eax
	add ebx, ebp			; y_src2 -> ebp

	mov ecx, [esp + 24 + localsize]	; dst_stride -> ecx
	shl ecx, 3
	mov edx, ecx			; 8 * dst_stride -> edx
	shl esi, 2
	sub ecx, esi			; 8 * dst_stride - 4 * width -> ecx

	mov [dst_dif], ecx

	mov esi, [esp + 20 + localsize]	; dst -> esi
	mov edi, esi
	shr edx, 1
	add edi, edx			; dst2 -> edi

	mov ebp, [esp + 48 + localsize]	; width -> ebp
	mov ecx, ebp			; width -> ecx
	shr ecx, 1
	shr ebp, 3			; width / 8 -> ebp
	mov [width_8], ebp

	mov ebp, [esp + 44 + localsize]	; uv_stride -> ebp
	sub ebp, ecx
	mov [uv_dif], ebp

	mov ecx, [esp + 32 + localsize]	; u_src -> ecx
	mov edx, [esp + 36 + localsize]	; v_src -> edx

	mov ebp, [esp + 52 + localsize]	; height -> ebp
	shr ebp, 1			; height / 2 -> ebp

	pxor mm7, mm7

  align 16
y_loop:
	mov [height_2], ebp
	mov ebp, [width_8]

  align 16
x_loop:
	movd mm2, [ecx]
	movd mm3, [edx]

	punpcklbw mm2, mm7		; u3u2u1u0 -> mm2
	punpcklbw mm3, mm7		; v3v2v1v0 -> mm3

	psubsw mm2, [U_SUB]		; U - 128
	psubsw mm3, [U_SUB]		; V - 128

	movq mm4, mm2
	movq mm5, mm3

	pmullw mm2, [UG_MUL]
	pmullw mm3, [VG_MUL]

	movq mm6, mm2			; u3u2u1u0 -> mm6
	punpckhwd mm2, mm2		; u3u3u2u2 -> mm2
	punpcklwd mm6, mm6		; u1u1u0u0 -> mm6

	pmullw mm4, [UB_MUL]		; B_ADD -> mm4

	movq mm0, mm3
	punpckhwd mm3, mm3		; v3v3v2v2 -> mm2
	punpcklwd mm0, mm0		; v1v1v0v0 -> mm6

	paddsw mm2, mm3
	paddsw mm6, mm0

	pmullw mm5, [VR_MUL]		; R_ADD -> mm5

	movq mm0, [eax]			; y7y6y5y4y3y2y1y0 -> mm0

	movq mm1, mm0
	punpckhbw mm1, mm7		; y7y6y5y4 -> mm1
	punpcklbw mm0, mm7		; y3y2y1y0 -> mm0

	psubsw mm0, [Y_SUB]		; Y - Y_SUB
	psubsw mm1, [Y_SUB]		; Y - Y_SUB

	pmullw mm1, [Y_MUL] 
	pmullw mm0, [Y_MUL]

	movq [TEMP_Y2], mm1		; y7y6y5y4 -> mm3
	movq [TEMP_Y1], mm0		; y3y2y1y0 -> mm7

	psubsw mm1, mm2			; g7g6g5g4 -> mm1
	psubsw mm0, mm6			; g3g2g1g0 -> mm0

	psraw mm1, SCALEBITS
	psraw mm0, SCALEBITS

	packuswb mm0, mm1		;g7g6g5g4g3g2g1g0 -> mm0

	movq [TEMP_G1], mm0

	movq mm0, [ebx]			; y7y6y5y4y3y2y1y0 -> mm0

	movq mm1, mm0

	punpckhbw mm1, mm7		; y7y6y5y4 -> mm1
	punpcklbw mm0, mm7		; y3y2y1y0 -> mm0

	psubsw mm0, [Y_SUB]		; Y - Y_SUB
	psubsw mm1, [Y_SUB]		; Y - Y_SUB

	pmullw mm1, [Y_MUL] 
	pmullw mm0, [Y_MUL]

	movq mm3, mm1
	psubsw mm1, mm2			; g7g6g5g4 -> mm1

	movq mm2, mm0
	psubsw mm0, mm6			; g3g2g1g0 -> mm0

	psraw mm1, SCALEBITS
	psraw mm0, SCALEBITS

	packuswb mm0, mm1		; g7g6g5g4g3g2g1g0 -> mm0

	movq [TEMP_G2], mm0

	movq mm0, mm4
	punpckhwd mm4, mm4		; u3u3u2u2 -> mm2
	punpcklwd mm0, mm0		; u1u1u0u0 -> mm6

	movq mm1, mm3			; y7y6y5y4 -> mm1
	paddsw mm3, mm4			; b7b6b5b4 -> mm3

	movq mm7, mm2			; y3y2y1y0 -> mm7

	paddsw mm2, mm0			; b3b2b1b0 -> mm2

	psraw mm3, SCALEBITS
	psraw mm2, SCALEBITS

	packuswb mm2, mm3		; b7b6b5b4b3b2b1b0 -> mm2

	movq [TEMP_B2], mm2

	movq mm3, [TEMP_Y2]
	movq mm2, [TEMP_Y1]

	movq mm6, mm3			; TEMP_Y2 -> mm6
	paddsw mm3, mm4			; b7b6b5b4 -> mm3

	movq mm4, mm2			; TEMP_Y1 -> mm4
	paddsw mm2, mm0			; b3b2b1b0 -> mm2

	psraw mm3, SCALEBITS
	psraw mm2, SCALEBITS

	packuswb mm2, mm3		; b7b6b5b4b3b2b1b0 -> mm2

	movq [TEMP_B1], mm2

	movq mm0, mm5
	punpckhwd mm5, mm5		; v3v3v2v2 -> mm5
	punpcklwd mm0, mm0		; v1v1v0v0 -> mm0

	paddsw mm1, mm5			; r7r6r5r4 -> mm1
	paddsw mm7, mm0			; r3r2r1r0 -> mm7

	psraw mm1, SCALEBITS
	psraw mm7, SCALEBITS

	packuswb mm7, mm1		; r7r6r5r4r3r2r1r0 -> mm7 (TEMP_R2)

	paddsw mm6, mm5			; r7r6r5r4 -> mm6
	paddsw mm4, mm0			; r3r2r1r0 -> mm4

	psraw mm6, SCALEBITS
	psraw mm4, SCALEBITS

	packuswb mm4, mm6		; r7r6r5r4r3r2r1r0 -> mm4 (TEMP_R1)
	
	movq mm0, [TEMP_B1]
	movq mm1, [TEMP_G1]

	movq mm6, mm7

	movq mm2, mm0
	punpcklbw mm2, mm4		; r3b3r2b2r1b1r0b0 -> mm2
	punpckhbw mm0, mm4		; r7b7r6b6r5b5r4b4 -> mm0

	pxor mm7, mm7

	movq mm3, mm1
	punpcklbw mm1, mm7		; 0g30g20g10g0 -> mm1
	punpckhbw mm3, mm7		; 0g70g60g50g4 -> mm3

	movq mm4, mm2
	punpcklbw mm2, mm1		; 0r1g1b10r0g0b0 -> mm2
	punpckhbw mm4, mm1		; 0r3g3b30r2g2b2 -> mm4

	movq mm5, mm0
	punpcklbw mm0, mm3		; 0r5g5b50r4g4b4 -> mm0
	punpckhbw mm5, mm3		; 0r7g7b70r6g6b6 -> mm5

	movq [esi], mm2			
	movq [esi + 8], mm4		
	movq [esi + 16], mm0	
	movq [esi + 24], mm5	

	movq mm0, [TEMP_B2]
	movq mm1, [TEMP_G2]

	movq mm2, mm0
	punpcklbw mm2, mm6		; r3b3r2b2r1b1r0b0 -> mm2
	punpckhbw mm0, mm6		; r7b7r6b6r5b5r4b4 -> mm0

	movq mm3, mm1 
	punpcklbw mm1, mm7		; 0g30g20g10g0 -> mm1
	punpckhbw mm3, mm7		; 0g70g60g50g4 -> mm3

	movq mm4, mm2
	punpcklbw mm2, mm1		; 0r1g1b10r0g0b0 -> mm2
	punpckhbw mm4, mm1		; 0r3g3b30r2g2b2 -> mm4

	movq mm5, mm0
	punpcklbw mm0, mm3		; 0r5g5b50r4g4b4 -> mm0
	punpckhbw mm5, mm3		; 0r7g7b70r6g6b6 -> mm5

	movq [edi], mm2
	movq [edi + 8], mm4
	movq [edi + 16], mm0
	movq [edi + 24], mm5

	add esi, 32
	add edi, 32

	add eax, 8
	add ebx, 8
	add ecx, 4
	add edx, 4

	dec ebp

	jnz near x_loop

	add esi, [dst_dif]
	add edi, [dst_dif]

	add eax, [y_dif]
	add ebx, [y_dif]

	add ecx, [uv_dif]
	add edx, [uv_dif]

	mov ebp, [height_2]
	dec ebp
	jnz near y_loop

	emms

	;; Local vars deallocation
	add esp, localsize
#undef TEMP_Y1
#undef TEMP_Y2
#undef TEMP_G1
#undef TEMP_G2
#undef TEMP_B1
#undef TEMP_B2
#undef y_dif
#undef dst_dif
#undef uv_dif
#undef height
#undef width_8
#undef height_2
#undef localsize
	
	pop ebp
	pop edi
	pop esi
	pop ebx

	ret
  }
}


void __declspec(naked) yv12_to_rgb24_mmx(BYTE *dst, 
                         int dst_stride, 
                         const BYTE *y_src,
                         const BYTE *u_src,
                         const BYTE *v_src, 
                         int y_stride, int uv_stride,
                         int width, int height) {

#define localsize 72
#define TEMP_Y1  esp
#define TEMP_Y2  esp + 8
#define TEMP_G1  esp + 16
#define TEMP_G2  esp + 24
#define TEMP_B1  esp + 32
#define TEMP_B2  esp + 40
#define y_dif    esp + 48
#define dst_dif	 esp + 52
#define uv_dif   esp + 56
#define height   esp + 60
#define width_8  esp + 64
#define height_2 esp + 68
#define SCALEBITS 6

  __asm {

	push ebx
	push esi
	push edi
	push ebp

	;  local vars alloc
	sub esp, localsize


	mov eax, [esp + 52 + localsize]	; height -> eax
	cmp eax, 0x00
	jge near dont_flip		; flip?
	
	neg eax
	mov [height], eax

	mov esi, [esp + 48 + localsize]	; width -> esi

	mov ebp, [esp + 40 + localsize]	; y_stride -> ebp
	mov ebx, ebp
	shl ebx, 1			; 2 * y_stride -> ebx
	neg ebx
	sub ebx, esi			; y_dif -> eax

	mov [y_dif], ebx

	sub eax, 1			; height - 1 -> eax
	mul ebp				; (height - 1) * y_stride -> ebp
	mov ecx, eax
	mov eax, [esp + 28 + localsize]	; y_src -> eax
	add eax, ecx			; y_src -> eax
	mov ebx, eax
	sub ebx, ebp			; y_src2 -> ebx

	mov ecx, [esp + 24 + localsize]	; dst_stride -> ecx
	mov edx, ecx
	add ecx, edx
	shl edx, 2
	add ecx, edx			; 6 * dst_stride -> ecx
	mov edx, ecx
	sub ecx, esi
	shl esi, 1
	sub ecx, esi			; 6 * dst_stride - 3 * width -> ecx

	mov [dst_dif], ecx

	mov esi, [esp + 20 + localsize]	; dst -> esi
	mov edi, esi
	shr edx, 1
	add edi, edx			; dst2 -> edi

	mov ebp, [esp + 48 + localsize]	; width -> ebp
	mov ecx, ebp			; width -> ecx
	shr ecx, 1
	shr ebp, 3			; width / 8 -> ebp
	mov [width_8], ebp

	mov ebp, [esp + 44 + localsize]	; uv_stride -> ebp
	mov edx, ebp
	neg edx
	sub edx, ecx
	mov [uv_dif], edx

	mov edx, ebp
	mov ebp, eax
	mov eax, [height]		; height -> eax
	shr eax, 1			; height / 2 -> eax

	mov ecx, [esp + 32 + localsize]	; u_src -> ecx
	sub eax, 1
	mul edx
	add ecx, eax

	mov edx, [esp + 36 + localsize]	; v_src -> edx
	add edx, eax

	mov eax, ebp

	mov ebp, [height]		; height -> ebp
	shr ebp, 1			; height / 2 -> ebp

	pxor mm7, mm7			; clear mm7
	jmp y_loop


  align 16
dont_flip:
	mov esi, [esp + 48 + localsize]	; width -> esi

	mov ebp, [esp + 40 + localsize]	; y_stride -> ebp
	mov ebx, ebp
	shl ebx, 1			; 2 * y_stride -> ebx
	sub ebx, esi			; y_dif -> ebx

	mov [y_dif], ebx

	mov eax, [esp + 28 + localsize]	; y_src -> eax
	mov ebx, eax
	add ebx, ebp			; y_src2 -> ebp

	mov ecx, [esp + 24 + localsize]	; dst_stride -> ecx
	mov edx, ecx
	add ecx, edx
	shl edx, 2
	add ecx, edx			; 6 * dst_stride -> ecx
	mov edx, ecx
	sub ecx, esi
	shl esi, 1
	sub ecx, esi			; 6 * dst_stride - 3 * width -> ecx

	mov [dst_dif], ecx

	mov esi, [esp + 20 + localsize]	; dst -> esi
	mov edi, esi
	shr edx, 1
	add edi, edx			; dst2 -> edi

	mov ebp, [esp + 48 + localsize]	; width -> ebp
	mov ecx, ebp			; width -> ecx
	shr ecx, 1
	shr ebp, 3			; width / 8 -> ebp
	mov [width_8], ebp

	mov ebp, [esp + 44 + localsize]	; uv_stride -> ebp
	sub ebp, ecx
	mov [uv_dif], ebp

	mov ecx, [esp + 32 + localsize]	; u_src -> ecx
	mov edx, [esp + 36 + localsize]	; v_src -> edx

	mov ebp, [esp + 52 + localsize]	; height -> ebp
	shr ebp, 1			; height / 2 -> ebp

	pxor mm7, mm7

  align 16
y_loop:
	mov [height_2], ebp
	mov ebp, [width_8]

  align 16
x_loop:
	movd mm2, [ecx]
	movd mm3, [edx]

	punpcklbw mm2, mm7		; u3u2u1u0 -> mm2
	punpcklbw mm3, mm7		; v3v2v1v0 -> mm3

	psubsw mm2, [U_SUB]		; U - 128
	psubsw mm3, [U_SUB]		; V - 128

	movq mm4, mm2
	movq mm5, mm3

	pmullw mm2, [UG_MUL]
	pmullw mm3, [VG_MUL]

	movq mm6, mm2			; u3u2u1u0 -> mm6
	punpckhwd mm2, mm2		; u3u3u2u2 -> mm2
	punpcklwd mm6, mm6		; u1u1u0u0 -> mm6

	pmullw mm4, [UB_MUL]		; B_ADD -> mm4

	movq mm0, mm3
	punpckhwd mm3, mm3		; v3v3v2v2 -> mm2
	punpcklwd mm0, mm0		; v1v1v0v0 -> mm6

	paddsw mm2, mm3
	paddsw mm6, mm0

	pmullw mm5, [VR_MUL]		; R_ADD -> mm5

	movq mm0, [eax]			; y7y6y5y4y3y2y1y0 -> mm0

	movq mm1, mm0
	punpckhbw mm1, mm7		; y7y6y5y4 -> mm1
	punpcklbw mm0, mm7		; y3y2y1y0 -> mm0

	psubsw mm0, [Y_SUB]		; Y - Y_SUB
	psubsw mm1, [Y_SUB]		; Y - Y_SUB

	pmullw mm1, [Y_MUL] 
	pmullw mm0, [Y_MUL]

	movq [TEMP_Y2], mm1		; y7y6y5y4 -> mm3
	movq [TEMP_Y1], mm0		; y3y2y1y0 -> mm7

	psubsw mm1, mm2			; g7g6g5g4 -> mm1
	psubsw mm0, mm6			; g3g2g1g0 -> mm0

	psraw mm1, SCALEBITS
	psraw mm0, SCALEBITS

	packuswb mm0, mm1		;g7g6g5g4g3g2g1g0 -> mm0

	movq [TEMP_G1], mm0

	movq mm0, [ebx]			; y7y6y5y4y3y2y1y0 -> mm0

	movq mm1, mm0

	punpckhbw mm1, mm7		; y7y6y5y4 -> mm1
	punpcklbw mm0, mm7		; y3y2y1y0 -> mm0

	psubsw mm0, [Y_SUB]		; Y - Y_SUB
	psubsw mm1, [Y_SUB]		; Y - Y_SUB

	pmullw mm1, [Y_MUL] 
	pmullw mm0, [Y_MUL]

	movq mm3, mm1
	psubsw mm1, mm2			; g7g6g5g4 -> mm1

	movq mm2, mm0
	psubsw mm0, mm6			; g3g2g1g0 -> mm0

	psraw mm1, SCALEBITS
	psraw mm0, SCALEBITS

	packuswb mm0, mm1		; g7g6g5g4g3g2g1g0 -> mm0

	movq [TEMP_G2], mm0

	movq mm0, mm4
	punpckhwd mm4, mm4		; u3u3u2u2 -> mm2
	punpcklwd mm0, mm0		; u1u1u0u0 -> mm6

	movq mm1, mm3			; y7y6y5y4 -> mm1
	paddsw mm3, mm4			; b7b6b5b4 -> mm3

	movq mm7, mm2			; y3y2y1y0 -> mm7

	paddsw mm2, mm0			; b3b2b1b0 -> mm2

	psraw mm3, SCALEBITS
	psraw mm2, SCALEBITS

	packuswb mm2, mm3		; b7b6b5b4b3b2b1b0 -> mm2

	movq [TEMP_B2], mm2

	movq mm3, [TEMP_Y2]
	movq mm2, [TEMP_Y1]

	movq mm6, mm3			; TEMP_Y2 -> mm6
	paddsw mm3, mm4			; b7b6b5b4 -> mm3

	movq mm4, mm2			; TEMP_Y1 -> mm4
	paddsw mm2, mm0			; b3b2b1b0 -> mm2

	psraw mm3, SCALEBITS
	psraw mm2, SCALEBITS

	packuswb mm2, mm3		; b7b6b5b4b3b2b1b0 -> mm2

	movq [TEMP_B1], mm2

	movq mm0, mm5
	punpckhwd mm5, mm5		; v3v3v2v2 -> mm5
	punpcklwd mm0, mm0		; v1v1v0v0 -> mm0

	paddsw mm1, mm5			; r7r6r5r4 -> mm1
	paddsw mm7, mm0			; r3r2r1r0 -> mm7

	psraw mm1, SCALEBITS
	psraw mm7, SCALEBITS

	packuswb mm7, mm1		; r7r6r5r4r3r2r1r0 -> mm7 (TEMP_R2)

	paddsw mm6, mm5			; r7r6r5r4 -> mm6
	paddsw mm4, mm0			; r3r2r1r0 -> mm4

	psraw mm6, SCALEBITS
	psraw mm4, SCALEBITS

	packuswb mm4, mm6		; r7r6r5r4r3r2r1r0 -> mm4 (TEMP_R1)
	
	movq mm0, [TEMP_B1]
	movq mm1, [TEMP_G1]

	movq mm6, mm7

	movq mm2, mm0
	punpcklbw mm2, mm4		; r3b3r2b2r1b1r0b0 -> mm2
	punpckhbw mm0, mm4		; r7b7r6b6r5b5r4b4 -> mm0

	pxor mm7, mm7

	movq mm3, mm1
	punpcklbw mm1, mm7		; 0g30g20g10g0 -> mm1
	punpckhbw mm3, mm7		; 0g70g60g50g4 -> mm3

	movq mm4, mm2
	punpcklbw mm2, mm1		; 0r1g1b10r0g0b0 -> mm2
	punpckhbw mm4, mm1		; 0r3g3b30r2g2b2 -> mm4

	movq mm5, mm0
	punpcklbw mm0, mm3		; 0r5g5b50r4g4b4 -> mm0
	punpckhbw mm5, mm3		; 0r7g7b70r6g6b6 -> mm5

	movd [esi], mm2			
	psrlq mm2, 32

	movd [esi + 3], mm2
	movd [esi + 6], mm4		

	psrlq mm4, 32

	movd [esi + 9], mm4
	movd [esi + 12], mm0	

	psrlq mm0, 32

	movd [esi + 15], mm0
	movd [esi + 18], mm5	

	psrlq mm5, 32

	movd [esi + 21], mm5	

	movq mm0, [TEMP_B2]
	movq mm1, [TEMP_G2]

	movq mm2, mm0
	punpcklbw mm2, mm6		; r3b3r2b2r1b1r0b0 -> mm2
	punpckhbw mm0, mm6		; r7b7r6b6r5b5r4b4 -> mm0

	movq mm3, mm1 
	punpcklbw mm1, mm7		; 0g30g20g10g0 -> mm1
	punpckhbw mm3, mm7		; 0g70g60g50g4 -> mm3

	movq mm4, mm2
	punpcklbw mm2, mm1		; 0r1g1b10r0g0b0 -> mm2
	punpckhbw mm4, mm1		; 0r3g3b30r2g2b2 -> mm4

	movq mm5, mm0
	punpcklbw mm0, mm3		; 0r5g5b50r4g4b4 -> mm0
	punpckhbw mm5, mm3		; 0r7g7b70r6g6b6 -> mm5

	movd [edi], mm2
	psrlq mm2, 32

	movd [edi + 3], mm2
	movd [edi + 6], mm4

	psrlq mm4, 32

	movd [edi + 9], mm4
	movd [edi + 12], mm0

	psrlq mm0, 32

	movd [edi + 15], mm0
	movd [edi + 18], mm5

	psrlq mm5, 32

	movd [edi + 21], mm5

	add esi, 24
	add edi, 24

	add eax, 8
	add ebx, 8
	add ecx, 4
	add edx, 4

	dec ebp

	jnz near x_loop

	add esi, [dst_dif]
	add edi, [dst_dif]

	add eax, [y_dif]
	add ebx, [y_dif]

	add ecx, [uv_dif]
	add edx, [uv_dif]

	mov ebp, [height_2]
	dec ebp
	jnz near y_loop

	emms

	;; Local vars deallocation
	add esp, localsize
#undef TEMP_Y1
#undef TEMP_Y2
#undef TEMP_G1
#undef TEMP_G2
#undef TEMP_B1
#undef TEMP_B2
#undef y_dif
#undef dst_dif
#undef uv_dif
#undef height
#undef width_8
#undef height_2
#undef localsize
	
	pop ebp
	pop edi
	pop esi
	pop ebx

	ret
  }
}



/*===========================================================================
;
; void yv12_to_uyvy_mmx(
;				uint8_t * dst,
;				int dst_stride,
;				uint8_t * y_src,
;				uint8_t * u_src,
;				uint8_t * v_src,
;				int y_stride,
;				int uv_stride,
;				int width,
;				int height);
;
;	width must be multiple of 8
;	~10% faster than plain c
;
;===========================================================================*/


void __declspec(naked) yv12_to_uyvy_mmx(BYTE * dst,
				int dst_stride,
				const BYTE * y_src,
				const BYTE * u_src,
				const BYTE * v_src,
				int y_stride,
				int uv_stride,
				int width,
        int height) {

  __asm {
		push ebx
		push ecx
		push esi
		push edi			
		push ebp		; STACK BASE = 20

		; global constants

		mov ebx, [esp + 20 + 32]	; width
		mov eax, [esp + 20 + 8]		; dst_stride
		sub eax, ebx				; 
		add eax, eax				; eax = 2*(dst_stride - width)
		push eax					; [esp + 4] = dst_dif
						; STACK BASE = 24

		shr ebx, 3					; ebx = width / 8
		mov edi, [esp + 24 + 4]		; dst


		; --------- flip -------------

		mov	ebp, [esp + 24 + 36]
		test ebp, ebp
		jl flip

		mov esi, [esp + 24 + 12]	; y_src
		mov ecx, [esp + 24 + 16]	; u_src
		mov edx, [esp + 24 + 20]	; v_src
		shr ebp, 1					; y = height / 2
		jmp yloop


  align 16
flip:
		neg ebp			; height = -height
		
		mov	eax, [esp + 24 + 24]	; y_stride
		lea	edx, [ebp - 1]			; edx = height - 1
		mul	edx
		mov esi, [esp + 24 + 12]	; y_src
		add esi, eax				; y_src += (height - 1) * y_stride

		shr ebp, 1					; y = height / 2
		mov	eax, [esp + 24 + 28]	; uv_stride
		lea	edx, [ebp - 1]			; edx = height/2 - 1
		mul	edx

		mov ecx, [esp + 24 + 16]	; u_src
		mov edx, [esp + 24 + 20]	; v_src
		add ecx, eax				; u_src += (height/2 - 1) * uv_stride
		add edx, eax				; v_src += (height/2 - 1) * uv_stride

		neg	[esp + 24 + 24]	; y_stride = -y_stride
		neg [esp + 24 + 28]	; uv_stride = -uv_stride
	
  align 16
yloop:
		xor eax, eax			; x = 0;

  align 16
xloop1:
				movd mm0, [ecx+4*eax]		; [    |uuuu]
				movd mm1, [edx+4*eax]		; [    |vvvv]
				movq mm2, [esi+8*eax]		; [yyyy|yyyy]

				punpcklbw mm0, mm1			; [vuvu|vuvu]
				movq mm1, mm0
				punpcklbw mm0, mm2			; [yvyu|yvyu]
				punpckhbw mm1, mm2			; [yvyu|yvyu]
				movq [edi], mm0
				movq [edi+8], mm1

				inc eax
				add edi, 16
						
				cmp eax, ebx
				jb	xloop1

		add edi, dword ptr [esp + 0]		; dst += dst_dif
		add esi, [esp + 24 + 24]	; y_src += y_stride
		
		xor eax, eax

  align 16
xloop2:
				movd mm0, [ecx+4*eax]		; [    |uuuu]
				movd mm1, [edx+4*eax]		; [    |vvvv]
				movq mm2, [esi+8*eax]		; [yyyy|yyyy]

				punpcklbw mm0, mm1			; [vuvu|vuvu]
				movq mm1, mm0
				punpcklbw mm0, mm2			; [yvyu|yvyu]
				punpckhbw mm1, mm2			; [yvyu|yvyu]

				movq [edi], mm0
				movq [edi+8], mm1

				inc eax
				add edi, 16
										
				cmp eax, ebx
				jb	xloop2

		add edi, dword ptr [esp + 0]			; dst += dst_dif
		add esi, [esp + 24 + 24]	; y_src += y_stride
		add ecx, [esp + 24 + 28]	; u_src += uv_stride
		add edx, [esp + 24 + 28]	; v_src += uv_stride

		dec	ebp				; y--
		jnz	yloop

		emms

		add esp, 4
		pop ebp
		pop edi
		pop esi
		pop ecx
		pop ebx

		ret
  }
}

//yv12_to_yuyv_mmx

void __declspec(naked) yv12_to_yuyv_mmx(BYTE * dst,
				int dst_stride,
				const BYTE * y_src,
				const BYTE * u_src,
				const BYTE * v_src,
				int y_stride,
				int uv_stride,
				int width,
        int height) {

  __asm {

		push ebx
		push ecx
		push esi
		push edi			
		push ebp		; STACK BASE = 20

		; global constants

		mov ebx, [esp + 20 + 32]	; width
		mov eax, [esp + 20 + 8]		; dst_stride
		sub eax, ebx				; 
		add eax, eax				; eax = 2*(dst_stride - width)
		push eax					; [esp + 4] = dst_dif
						; STACK BASE = 24

		shr ebx, 3					; ebx = width / 8
		mov edi, [esp + 24 + 4]		; dst


		; --------- flip -------------

		mov	ebp, [esp + 24 + 36]
		test ebp, ebp
		jl flip

		mov esi, [esp + 24 + 12]	; y_src
		mov ecx, [esp + 24 + 16]	; u_src
		mov edx, [esp + 24 + 20]	; v_src
		shr ebp, 1					; y = height / 2
		jmp yloop


  align 16
flip:
		neg ebp			; height = -height
		
		mov	eax, [esp + 24 + 24]	; y_stride
		lea	edx, [ebp - 1]			; edx = height - 1
		mul	edx
		mov esi, [esp + 24 + 12]	; y_src
		add esi, eax				; y_src += (height - 1) * y_stride

		shr ebp, 1					; y = height / 2
		mov	eax, [esp + 24 + 28]	; uv_stride
		lea	edx, [ebp - 1]			; edx = height/2 - 1
		mul	edx

		mov ecx, [esp + 24 + 16]	; u_src
		mov edx, [esp + 24 + 20]	; v_src
		add ecx, eax				; u_src += (height/2 - 1) * uv_stride
		add edx, eax				; v_src += (height/2 - 1) * uv_stride

		neg	[esp + 24 + 24]	; y_stride = -y_stride
		neg [esp + 24 + 28]	; uv_stride = -uv_stride
	
  align 16
yloop:
		xor eax, eax			; x = 0;

  align 16
xloop1:
				movd mm0, [ecx+4*eax]		; [    |uuuu]
				movd mm1, [edx+4*eax]		; [    |vvvv]
				movq mm2, [esi+8*eax]		; [yyyy|yyyy]

				punpcklbw mm0, mm1			; [vuvu|vuvu]
				movq mm3, mm2
				punpcklbw mm2, mm0			; [vyuy|vyuy]
				punpckhbw mm3, mm0			; [vyuy|vyuy]
				movq [edi], mm2
				movq [edi+8], mm3

				inc eax
				add edi, 16
						
				cmp eax, ebx
				jb	xloop1

		add edi, dword ptr [esp + 0]		; dst += dst_dif
		add esi, [esp + 24 + 24]	; y_src += y_stride
		
		xor eax, eax

  align 16
xloop2:
				movd mm0, [ecx+4*eax]		; [    |uuuu]
				movd mm1, [edx+4*eax]		; [    |vvvv]
				movq mm2, [esi+8*eax]		; [yyyy|yyyy]

				punpcklbw mm0, mm1			; [vuvu|vuvu]
				movq mm3, mm2
				punpcklbw mm2, mm0			; [vyuy|vyuy]
				punpckhbw mm3, mm0			; [vyuy|vyuy]
				movq [edi], mm2
				movq [edi+8], mm3

				inc eax
				add edi, 16
										
				cmp eax, ebx
				jb	xloop2

		add edi, dword ptr [esp + 0]			; dst += dst_dif
		add esi, [esp + 24 + 24]	; y_src += y_stride
		add ecx, [esp + 24 + 28]	; u_src += uv_stride
		add edx, [esp + 24 + 28]	; v_src += uv_stride

		dec	ebp				; y--
		jnz	near yloop

		emms

		add esp, 4
		pop ebp
		pop edi
		pop esi
		pop ecx
		pop ebx

		ret
    }
 }


#define Y_ADD	16
#define UV_ADD	128


 __declspec(align(8)) static short y_mul[4]	= {25,129,66,0};
 __declspec(align(8)) static short u_mul[4]	= {112,-74,-38,0};
 __declspec(align(8)) static short v_mul[4]	= {-18,-94,112,0};


/*;===========================================================================
;
;	void rgb24_to_yv12_mmx(uint8_t * const y_out,
;						uint8_t * const u_out,
;						uint8_t * const v_out,
;						const uint8_t * const src,
;						const uint32_t width,
;						const uint32_t height,
;						const uint32_t stride)
;
; always flips
;
;===========================================================================*/

void __declspec(naked) rgb24_to_yv12_mmx (BYTE * const y_out,
						BYTE * const u_out,
						BYTE * const v_out,
						const BYTE * const src,
						const unsigned int width,
						const unsigned int height,
            const unsigned int stride) {
  __asm {
		push ebx
		push ecx
		push esi
		push edi			
		push ebp			; STACK BASE = 20

		; global consants

		mov eax, [esp + 20 + 28]	; stride
		mov ecx, [esp + 20 + 20]	; width
		mov ebx, eax
		sub ebx, ecx				
		shr ebx, 1					; ebx = (stride-width) / 2;
		push ebx					; [esp + 20] = uv_dif
							; STACK BASE = 24

		add eax, eax
		sub eax, ecx				; eax = 2*stride - width
		push eax					; [esp + 16] = y_dif
							; STACK BASE = 28

		mov ebx, ecx				; 
		shr ebx, 1					;
		push ebx					; [esp + 12] = width/2
							; STACK BASE = 32

		mov edx, ecx
		add ecx, edx
		add ecx, edx				; ecx = 3*width   (use 4 for rgb32)
		push ecx					; [esp + 8] = width3
							; STACK BASE = 36

		mov edx, ecx
		add edx, ecx
		add edx, ecx				; edx = 3*width3
		push edx					; [esp + 4] = src_dif
							; STACK BASE = 40

		mov esi, [esp + 40 + 16]	; src
		mov ebp, [esp + 40 + 24]	; eax = height
		mov eax, ebp
		sub eax, 2
		mul ecx
		add esi, eax				; src += (height-2) * width3

		mov edi, [esp + 40 + 4]		; y_out
		mov ecx, [esp + 40 + 8]		; u_out
		mov edx, [esp + 40 + 12]	; v_out
		movq mm7, [y_mul]		

		shr ebp, 1				; ebp = height / 2
		push ebp				; [esp+0] = tmp
								; STACK BASE = 44

  align 16
yloop:
		mov ebp, [esp + 12]		; ebp = width /2 

  align 16
xloop:
			; y_out

			mov ebx, [esp + 8]			; ebx = width3

			pxor mm4, mm4
			pxor mm5, mm5
			movd mm0, [esi]			; src[0...]
			movd mm2, [esi+ebx]		; src[width3...]
			punpcklbw mm0, mm4		; [  |b |g |r ]
			punpcklbw mm2, mm5		; [  |b |g |r ]
			movq mm6, mm0			; = [  |b4|g4|r4]
			paddw mm6, mm2			; +[  |b4|g4|r4]
			pmaddwd mm0, mm7		; *= Y_MUL
			pmaddwd mm2, mm7		; *= Y_MUL
			movq mm4, mm0			; [r]
			movq mm5, mm2			; [r]
			psrlq mm4, 32			; +[g]
			psrlq mm5, 32			; +[g]
			paddd mm0, mm4			; +[b]
			paddd mm2, mm5			; +[b]

			pxor mm4, mm4
			pxor mm5, mm5
			movd mm1, [esi+3]		; src[4...]
			movd mm3, [esi+ebx+3]	; src[width3+4...]
			punpcklbw mm1, mm4		; [  |b |g |r ]
			punpcklbw mm3, mm5		; [  |b |g |r ]
			paddw mm6, mm1			; +[  |b4|g4|r4]
			paddw mm6, mm3			; +[  |b4|g4|r4]
			pmaddwd mm1, mm7		; *= Y_MUL
			pmaddwd mm3, mm7		; *= Y_MUL
			movq mm4, mm1			; [r]
			movq mm5, mm3			; [r]
			psrlq mm4, 32			; +[g]
			psrlq mm5, 32			; +[g]
			paddd mm1, mm4			; +[b]
			paddd mm3, mm5			; +[b]

			mov ebx, [esp + 44 + 28]	; stride

			movd eax, mm0
			shr eax, 8
			add eax, Y_ADD
			mov [edi + ebx], al

			movd eax, mm1
			shr eax, 8
			add eax, Y_ADD
			mov [edi + ebx + 1], al

			movd eax, mm2
			shr eax, 8
			add eax, Y_ADD
			mov [edi], al

			movd eax, mm3
			shr eax, 8
			add eax, Y_ADD
			mov [edi + 1], al

			; u_out, v_out

			movq mm0, mm6			; = [  |b4|g4|r4]
			pmaddwd mm6, [v_mul]		; *= V_MUL
			pmaddwd mm0, [u_mul]		; *= U_MUL
			movq mm1, mm0
			movq mm2, mm6
			psrlq mm1, 32
			psrlq mm2, 32
			paddd mm0, mm1
			paddd mm2, mm6

			movd eax, mm0
			shr eax, 10
			add eax, UV_ADD
			mov [ecx], al

			movd eax, mm2
			shr eax, 10
			add eax, UV_ADD
			mov [edx], al

			add esi, 2 * 3			; (use 4 for rgb32)
			add edi, 2
			inc ecx
			inc edx

			dec ebp
			jnz near xloop

		sub esi, [esp + 4]			; src  -= src_dif
		add edi, [esp + 16]			; y_out += y_dif
		add ecx, [esp + 20]			; u_out += uv_dif
		add edx, [esp + 20]			; v_out += uv_dif

		dec dword ptr [esp+0]
		jnz near yloop

		emms

		add esp, 24
		pop ebp
		pop edi
		pop esi
		pop ecx
		pop ebx

		ret
  }
}


/*;===========================================================================
;
;	void rgb32_to_yv12mmx(uint8_t * const y_out,
;						uint8_t * const u_out,
;						uint8_t * const v_out,
;						const uint8_t * const src,
;						const uint32_t width,
;						const uint32_t height,
;						const uint32_t stride)
;
; always flips
;
;===========================================================================*/

void __declspec(naked) rgb32_to_yv12_mmx(BYTE * const y_out,
						BYTE * const u_out,
						BYTE * const v_out,
						const BYTE * const src,
						const unsigned int width,
						const unsigned int height,
            const unsigned int stride) {
  __asm {

		push ebx
		push ecx
		push esi
		push edi			
		push ebp			; STACK BASE = 20

		; global consants

		mov eax, [esp + 20 + 28]	; stride
		mov ecx, [esp + 20 + 20]	; width
		mov ebx, eax
		sub ebx, ecx				
		shr ebx, 1					; ebx = (stride-width) / 2;
		push ebx					; [esp + 20] = uv_dif
							; STACK BASE = 24

		add eax, eax
		sub eax, ecx				; eax = 2*stride - width
		push eax					; [esp + 16] = y_dif
							; STACK BASE = 28

		mov ebx, ecx				; 
		shr ebx, 1					;
		push ebx					; [esp + 12] = width/2
							; STACK BASE = 32

		mov edx, ecx
		shl ecx, 2					; ecx = 4*width   (use 4 for rgb32)
		push ecx					; [esp + 8] = width4
							; STACK BASE = 36

		mov edx, ecx
		add edx, ecx
		add edx, ecx				; edx = 3*width4
		push edx					; [esp + 4] = src_dif
							; STACK BASE = 40

		mov esi, [esp + 40 + 16]	; src
		mov ebp, [esp + 40 + 24]	; eax = height
		mov eax, ebp
		sub eax, 2
		mul ecx
		add esi, eax				; src += (height-2) * width4

		mov edi, [esp + 40 + 4]		; y_out
		mov ecx, [esp + 40 + 8]		; u_out
		mov edx, [esp + 40 + 12]	; v_out
		movq mm7, [y_mul]		

		shr ebp, 1				; ebp = height / 2
		push ebp				; [esp+0] = tmp
								; STACK BASE = 44

  align 16
yloop:
		mov ebp, [esp + 12]		; ebp = width /2 

  align 16
xloop:
			; y_out

			mov ebx, [esp + 8]			; ebx = width4

			pxor mm4, mm4
			movq mm0, [esi]			; src[4...       |0...     ]
			movq mm2, [esi+ebx]		; src[width4+4...|width4...]
			movq mm1, mm0
			movq mm3, mm2
			punpcklbw mm0, mm4		; [  |b |g |r ]
			punpcklbw mm2, mm4		; [  |b |g |r ]
			punpckhbw mm1, mm4		; [  |b |g |r ]
			punpckhbw mm3, mm4		; [  |b |g |r ]

			movq mm6, mm0			; = [  |b4|g4|r4]
			paddw mm6, mm2			; +[  |b4|g4|r4]
			pmaddwd mm0, mm7		; *= Y_MUL
			pmaddwd mm2, mm7		; *= Y_MUL
			movq mm4, mm0			; [r]
			movq mm5, mm2			; [r]
			psrlq mm4, 32			; +[g]
			psrlq mm5, 32			; +[g]
			paddd mm0, mm4			; +[b]
			paddd mm2, mm5			; +[b]

			paddw mm6, mm1			; +[  |b4|g4|r4]
			paddw mm6, mm3			; +[  |b4|g4|r4]
			pmaddwd mm1, mm7		; *= Y_MUL
			pmaddwd mm3, mm7		; *= Y_MUL
			movq mm4, mm1			; [r]
			movq mm5, mm3			; [r]
			psrlq mm4, 32			; +[g]
			psrlq mm5, 32			; +[g]
			paddd mm1, mm4			; +[b]
			paddd mm3, mm5			; +[b]

			mov ebx, [esp + 44 + 28]	; stride

			movd eax, mm0
			shr eax, 8
			add eax, Y_ADD
			mov [edi + ebx], al

			movd eax, mm1
			shr eax, 8
			add eax, Y_ADD
			mov [edi + ebx + 1], al

			movd eax, mm2
			shr eax, 8
			add eax, Y_ADD
			mov [edi], al

			movd eax, mm3
			shr eax, 8
			add eax, Y_ADD
			mov [edi + 1], al

			; u_out, v_out

			movq mm0, mm6			; = [  |b4|g4|r4]
			pmaddwd mm6, [v_mul]		; *= V_MUL
			pmaddwd mm0, [u_mul]		; *= U_MUL
			movq mm1, mm0
			movq mm2, mm6
			psrlq mm1, 32
			psrlq mm2, 32
			paddd mm0, mm1
			paddd mm2, mm6

			movd eax, mm0
			shr eax, 10
			add eax, UV_ADD
			mov [ecx], al

			movd eax, mm2
			shr eax, 10
			add eax, UV_ADD
			mov [edx], al

			add esi, 2 * 4			; (use 4 for rgb32)
			add edi, 2
			inc ecx
			inc edx

			dec ebp
      jnz xloop

		sub esi, [esp + 4]			; src  -= src_dif
		add edi, [esp + 16]			; y_out += y_dif
		add ecx, [esp + 20]			; u_out += uv_dif
		add edx, [esp + 20]			; v_out += uv_dif
    
		dec dword ptr [esp+0]
		jnz yloop

		emms

		add esp, 24
		pop ebp
		pop edi
		pop esi
		pop ecx
		pop ebx

		ret
  }
}


/*;===========================================================================
;
;	void yuyv_to_yv12_mmx(uint8_t * const y_out,
;						uint8_t * const u_out,
;						uint8_t * const v_out,
;						const uint8_t * const src,
;						const uint32_t width,
;						const uint32_t height,
;						const uint32_t stride);
;
;	width must be multiple of 8
;	does not flip
;	~30% faster than plain c
;
;===========================================================================*/
__declspec(align(8)) static __int64 mask1	= 0x00ff00ff00ff00ff;
__declspec(align(8)) static __int64 mask2	= 0xff00ff00ff00ff00;


void __declspec(naked) yuyv_to_yv12_mmx(BYTE * const y_out,
						BYTE * const u_out,
						BYTE * const v_out,
						const BYTE * const src,
						const int width,
						const int height,
						const int stride) {
  __asm {
		push ebx
		push ecx
		push esi
		push edi			
		push ebp		; STACK BASE = 20

		; some global consants

		mov ecx, [esp + 20 + 20]	; width
		mov eax, [esp + 20 + 28]	; stride
		sub eax, ecx				; eax = stride - width
		mov edx, eax
		add edx, [esp + 20 + 28]	; edx = y_dif + stride
		push edx					; [esp + 12] = y_dif

		shr eax, 1
		push eax					; [esp + 8] = uv_dif

		shr ecx, 3
		push ecx					; [esp + 4] = width/8

		sub	esp, 4					; [esp + 0] = tmp_height_counter
						; STACK_BASE = 36
		
		movq mm6, [mask1]
		movq mm7, [mask2]

		mov edi, [esp + 36 + 4]		; y_out
		mov ebx, [esp + 36 + 8]		; u_out
		mov edx, [esp + 36 + 12]	; v_out
		mov esi, [esp + 36 + 16]	; src

		mov eax, [esp + 36 + 20]
		mov ebp, [esp + 36 + 24]
		mov ecx, [esp + 36 + 28]	; ecx = stride
		shr ebp, 1					; ebp = height /= 2
		add eax, eax				; eax = 2 * width

    align 16
yloop:
		mov [esp], ebp
		mov ebp, [esp + 4]			; width/8

    align 16
xloop:
		movq mm2, [esi]				; y 1st row
		movq mm3, [esi + 8]
		movq mm0, mm2
		movq mm1, mm3
		pand mm2, mm6 ; mask1
		pand mm3, mm6 ; mask1
		pand mm0, mm7 ; mask2
		pand mm1, mm7 ; mask2
		packuswb mm2, mm3
		psrlq mm0, 8
		psrlq mm1, 8
		movq [edi], mm2

		movq mm4, [esi + eax]		; y 2nd row
		movq mm5, [esi + eax + 8]
		movq mm2, mm4
		movq mm3, mm5
		pand mm4, mm6 ; mask1
		pand mm5, mm6 ; mask1
		pand mm2, mm7 ; mask2
		pand mm3, mm7 ; mask2
		packuswb mm4, mm5
		psrlq mm2, 8
		psrlq mm3, 8
		movq [edi + ecx], mm4

		paddw mm0, mm2			; uv avg 1st & 2nd
		paddw mm1, mm3
		psrlw mm0, 1
		psrlw mm1, 1
		packuswb mm0, mm1
		movq mm2, mm0
		pand mm0, mm6 ; mask1
		pand mm2, mm7 ; mask2
		packuswb mm0, mm0
		psrlq mm2, 8
		movd [ebx], mm0
		packuswb mm2, mm2
		movd [edx], mm2

		add	esi, 16
		add	edi, 8
		add	ebx, 4
		add	edx, 4
		dec ebp
		jnz xloop

		mov ebp, [esp]

		add esi, eax			; += width2
		add edi, [esp + 12]		; += y_dif + stride
		add ebx, [esp + 8]		; += uv_dif
		add edx, [esp + 8]		; += uv_dif

		dec ebp
		jnz yloop

		emms

		add esp, 16
		pop ebp
		pop edi
		pop esi
		pop ecx
		pop ebx

		ret
    }
 }