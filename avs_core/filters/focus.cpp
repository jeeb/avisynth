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

#include "focus.h"
#include <cmath>
#include <cstdlib>
#include <malloc.h>
#include <avs/minmax.h>
#include "../core/internal.h"
#include <emmintrin.h>
#include <avs/alignment.h>

 
/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

extern const AVSFunction Focus_filters[] = {
  { "Blur", "cf[]f[mmx]b", Create_Blur },                     // amount [-1.0 - 1.5849625] -- log2(3)
  { "Sharpen", "cf[]f[mmx]b", Create_Sharpen },               // amount [-1.5849625 - 1.0]
  { "TemporalSoften", "ciii[scenechange]i[mode]i", TemporalSoften::Create }, // radius, luma_threshold, chroma_threshold
  { "SpatialSoften", "ciii", SpatialSoften::Create },   // radius, luma_threshold, chroma_threshold
  { 0 }
};


 


/****************************************
 ***  AdjustFocus helper classes     ***
 ***  Originally by Ben R.G.         ***
 ***  MMX code by Marc FD            ***
 ***  Adaptation and bugfixes sh0dan ***
 ***  Code actually requires ISSE!   ***
 ***  Not anymore - pure MMX    IanB ***
 ***  Implement boundary proc.  IanB ***
 ***  Impl. full 8bit MMX proc. IanB ***
 ***************************************/

AdjustFocusV::AdjustFocusV(double _amount, PClip _child, bool _mmx)
: GenericVideoFilter(_child), amount(int(32768*pow(2.0, _amount)+0.5)), line(NULL), mmx(_mmx) {}

AdjustFocusV::~AdjustFocusV(void) 
{ 
  if (line) delete[] line; 
}

// --------------------------------
// Blur/Sharpen Vertical GetFrame()
// --------------------------------

PVideoFrame __stdcall AdjustFocusV::GetFrame(int n, IScriptEnvironment* env) 
{
	PVideoFrame frame = child->GetFrame(n, env);
	env->MakeWritable(&frame);
	if (!line)
		line = new uc[frame->GetRowSize()+32];
	uc* linea = (uc*)(((int)line+15) & -16); // Align 16

	if (vi.IsPlanar()) {
		for(int cplane=0;cplane<3;cplane++) {
			int plane=0;
			if (cplane==0)  plane = PLANAR_Y;
			if (cplane==1)  plane = PLANAR_U;
			if (cplane==2)  plane = PLANAR_V;
			uc* buf      = frame->GetWritePtr(plane);
			int pitch    = frame->GetPitch(plane);
			int row_size = frame->GetRowSize(plane);
			int height   = frame->GetHeight(plane);
			memcpy(linea, buf, row_size); // First row - map centre as upper
			// All normal cases will have pitch aligned 16, we
			// need 8. If someone works hard enough to override
			// this we can't process the short fall. Use C Code.
#ifdef X86_32
			if (mmx && (pitch >= ((row_size+7) & -8)))
      {
				AFV_MMX(linea, buf, height, pitch, row_size, amount);
			}
      else
#endif
      {
				AFV_C(linea, buf, height, pitch, row_size, amount);
			}
		}

	} else {
		uc* buf      = frame->GetWritePtr();
		int pitch    = frame->GetPitch();
		int row_size = vi.RowSize();
		int height   = vi.height;
		memcpy(linea, buf, row_size); // First row - map centre as upper
#ifdef X86_32
		if (mmx && (pitch >= ((row_size+7) & -8)))
    {
			AFV_MMX(linea, buf, height, pitch, row_size, amount);
		}
    else
#endif
    {
			AFV_C(linea, buf, height, pitch, row_size, amount);
		}
	}
	return frame;
}

// ------------------------------
// Blur/Sharpen Vertical C++ Code
// ------------------------------

void AFV_C(uc* l, uc* p, const int height, const int pitch, const int row_size, const int amount) {
	const int center_weight = amount*2;
	const int outer_weight = 32768-amount;
	for (int y = height-1; y>0; --y) {
		for (int x = 0; x < row_size; ++x) {
			uc a = ScaledPixelClip(p[x] * center_weight + (l[x] + p[x+pitch]) * outer_weight);
			l[x] = p[x];
			p[x] = a;
		}
		p += pitch;
	}
	for (int x = 0; x < row_size; ++x) { // Last row - map centre as lower
		p[x] = ScaledPixelClip(p[x] * center_weight + (l[x] + p[x]) * outer_weight);
	}
}


#ifdef X86_32
// ------------------------------
// Blur/Sharpen Vertical MMX Code
// ------------------------------

void AFV_MMX(const uc* l, const uc* p, const int height, const int pitch, const int row_size, const int amount) 
{
	// round masks
	__declspec(align(8)) const static __int64 r7 = 0x0040004000400040;

	__asm { 
		sub		esp,8				; temp space on stack
		// signed word center weight ((amount+0x100)>>9) x4
		mov		eax,amount
		add		eax,100h
		sar		eax,9               ; [21..128]
		mov		[esp],ax
		mov		[esp+2],ax
		mov		[esp+4],ax
		mov		[esp+6],ax
		movq	mm7,[esp]
		// signed word outer weight 64-((amount+0x100)>>9) x4
		mov		ax,40h
		sub		ax,[esp]            ; [43..-64]
		mov		[esp],ax
		mov		[esp+2],ax
		mov		[esp+4],ax
		mov		[esp+6],ax
		movq	mm6,[esp]

		add		esp,8
		pxor	mm0,mm0
	}
	
	int nloops = __min(pitch, (row_size+7) & -8) >> 3;

	for (int y = height-1; y>0; --y) {

	__asm {
		mov			edx,p			; frame buffer
		mov			ecx,pitch
		mov			eax,edx
		add			eax,ecx
		mov			p,eax			; p += pitch;
		mov			eax,l			; line buffer
		mov			edi,nloops

		align		16
row_loop:
		movq		mm4,[eax]		; 8 Upper pixels (buffered)
		 movq		mm5,[edx+ecx]	; 8 Lower pixels
		movq		mm1,mm4			; Duplicate uppers
		 movq		mm3,mm5			; Duplicate lowers
		punpcklbw	mm4,mm0			; unpack 4 low bytes uppers
		 movq		mm2,[edx]		; 8 Centre pixels
		punpcklbw	mm5,mm0			; unpack 4 low bytes lowers
		 movq		[eax],mm2		; Save centres as next uppers
		paddw		mm5,mm4			; Uppers + Lowers				-- low
		 movq		mm4,mm2			; Duplicate centres
		pmullw		mm5,mm6			; *= outer weight				-- low [-32130, 21930]
		 punpcklbw	mm4,mm0			; unpack 4 low bytes centres
		punpckhbw	mm1,mm0			; unpack 4 high bytes uppers
		 punpckhbw	mm3,mm0			; unpack 4 high bytes lowers
		pmullw		mm4,mm7			; *= centre weight				-- low [32640, 5355]
		 paddw		mm1,mm3			; Uppers + Lowers				-- high
		punpckhbw	mm2,mm0			; unpack 4 high bytes centres
		 pmullw		mm1,mm6			; *= outer weight				-- high
		pmullw		mm2,mm7			; *= centre weight				-- high
		 paddsw		mm5,mm4			; Weighted outers+=0.5*centres	-- low
		paddsw		mm1,mm2			; Weighted outers+=0.5*centres	-- high
		 paddsw		mm5,mm4			; +=0.5*centres					-- low
		paddsw		mm1,mm2			; +=0.5*centres					-- high
 		 paddsw		mm5,r7			; += 0.5						-- low
		paddsw		mm1,r7			; += 0.5						-- high
		 psraw		mm5,7			; /= 128						-- low
		add			eax,8			; upper += 8
		 psraw		mm1,7			; /= 128						-- high
		add			edx,8			; centre += 8
		 packuswb	mm5,mm1			; pack 4 lows with 4 highs
		dec			edi				; count--
		 movq		[edx-8],mm5		; Update 8 pixels
		jnle		row_loop		; 
		}

	} // for (int y = height

	__asm { // Last row - map centre as lower
		mov			edx,p			; frame buffer
		mov			eax,l			; line buffer
		mov			edi,nloops

		align		16
lrow_loop:
		movq		mm5,[eax]		; 8 Upper pixels (buffered)
		 movq		mm4,[edx]		; 8 Centre pixels as lowers
		movq		mm1,mm5			; Duplicate uppers
		 movq		mm3,mm4			; Duplicate lowers (centres)
		punpcklbw	mm5,mm0			; unpack 4 low bytes uppers
		 punpcklbw	mm4,mm0			; unpack 4 low bytes lowers (centres)
		punpckhbw	mm3,mm0			; unpack 4 high bytes lowers (centres)
		 paddw		mm5,mm4			; Uppers + lowers (centres)		-- low
		punpckhbw	mm1,mm0			; unpack 4 high bytes uppers
		 pmullw		mm5,mm6			; *= outer weight				-- low
		paddw		mm1,mm3			; Uppers + lowers (centres)		-- high
		 pmullw		mm4,mm7			; *= centre weight				-- low
		pmullw		mm1,mm6			; *= outer weight				-- high
		 pmullw		mm3,mm7			; *= centre weight				-- high
		paddsw		mm5,mm4			; Weighted outers+=0.5*centres	-- low
		 paddsw		mm1,mm3			; Weighted outers+=0.5*centres	-- high
		paddsw		mm5,mm4			; +=0.5*centres					-- low
		 paddsw		mm1,mm3			; +=0.5*centres					-- high
		paddsw		mm5,r7			; += 0.5						-- low
		 paddsw		mm1,r7			; += 0.5						-- high
		psraw		mm5,7			; /= 128						-- low
		 add		eax,8			; upper += 8
		psraw		mm1,7			; /= 128						-- high
		 add		edx,8			; centre += 8
		packuswb	mm5,mm1			; pack 4 lows with 4 highs
		 dec		edi				; count--
		movq		[edx-8],mm5		; Update 8 pixels
		 jnle		lrow_loop		; 
	}
	__asm emms
}
#endif

AdjustFocusH::AdjustFocusH(double _amount, PClip _child, bool _mmx)
: GenericVideoFilter(_child), amount(int(32768*pow(2.0, _amount)+0.5)), mmx(_mmx) {}

// ----------------------------------
// Blur/Sharpen Horizontal GetFrame()
// ----------------------------------

PVideoFrame __stdcall AdjustFocusH::GetFrame(int n, IScriptEnvironment* env) 
{
	PVideoFrame frame = child->GetFrame(n, env);
	env->MakeWritable(&frame); // Damn! This screws FillBorder

	if (vi.IsPlanar()) {
		for(int cplane=0;cplane<3;cplane++) {
			int plane=0;
			if (cplane==0) plane = PLANAR_Y;
			if (cplane==1) plane = PLANAR_U;
			if (cplane==2) plane = PLANAR_V;
			const int row_size = frame->GetRowSize(plane);
			uc* q = frame->GetWritePtr(plane);
			const int pitch = frame->GetPitch(plane);
			int height = frame->GetHeight(plane);
#ifdef X86_32
			if (mmx && (pitch >= ((row_size+7) & -8)))
      {
				AFH_YV12_MMX(q,height,pitch,row_size,amount);
			}
      else
#endif
      {
				AFH_YV12_C(q,height,pitch,row_size,amount);
			} 
		}
	} else {
		uc* q = frame->GetWritePtr();
		const int pitch = frame->GetPitch();
		if (vi.IsYUY2()) {
#ifdef X86_32
			if (mmx)
      {
				AFH_YUY2_MMX(q,vi.height,pitch,vi.width,amount);
			}
      else
#endif
      {
				AFH_YUY2_C(q,vi.height,pitch,vi.width,amount);
			}
		} 
		else if (vi.IsRGB32()) {
#ifdef X86_32
			if (mmx)
      {
				AFH_RGB32_MMX(q,vi.height,pitch,vi.width,amount);
			}
      else
#endif
      {
				AFH_RGB32_C(q,vi.height,pitch,vi.width,amount);
			}
		} 
		else { //rgb24
			AFH_RGB24_C(q,vi.height,pitch,vi.width,amount);
		}
	}

	return frame;
}

// --------------------------------------
// Blur/Sharpen Horizontal RGB32 C++ Code
// --------------------------------------

void AFH_RGB32_C(uc* p, int height, const int pitch, const int width, const int amount) {
	const int center_weight = amount*2;
	const int outer_weight = 32768-amount;
	for (int y = height; y>0; --y) 
	{
		uc bb = p[0];
		uc gg = p[1];
		uc rr = p[2];
		uc aa = p[3];
		int x;
		for (x = 0; x < width-1; ++x) 
		{
			uc b = ScaledPixelClip(p[x*4+0] * center_weight + (bb + p[x*4+4]) * outer_weight);
			bb = p[x*4+0]; p[x*4+0] = b;
			uc g = ScaledPixelClip(p[x*4+1] * center_weight + (gg + p[x*4+5]) * outer_weight);
			gg = p[x*4+1]; p[x*4+1] = g;
			uc r = ScaledPixelClip(p[x*4+2] * center_weight + (rr + p[x*4+6]) * outer_weight);
			rr = p[x*4+2]; p[x*4+2] = r;
			uc a = ScaledPixelClip(p[x*4+3] * center_weight + (aa + p[x*4+7]) * outer_weight);
			aa = p[x*4+3]; p[x*4+3] = a;
		}
		p[x*4+0] = ScaledPixelClip(p[x*4+0] * center_weight + (bb + p[x*4+0]) * outer_weight);
		p[x*4+1] = ScaledPixelClip(p[x*4+1] * center_weight + (gg + p[x*4+1]) * outer_weight);
		p[x*4+2] = ScaledPixelClip(p[x*4+2] * center_weight + (rr + p[x*4+2]) * outer_weight);
		p[x*4+3] = ScaledPixelClip(p[x*4+3] * center_weight + (aa + p[x*4+3]) * outer_weight);
		p += pitch;
	}
}

#ifdef X86_32
// --------------------------------------
// Blur/Sharpen Horizontal RGB32 MMX Code
// --------------------------------------

void AFH_RGB32_MMX(const uc* p, const int height, const int pitch, const int width, const int amount) 
{
	// round masks
	__declspec(align(8)) const __int64 r7 = 0x0040004000400040;
	// weights
	__declspec(align(8)) __int64 cw;
	__declspec(align(8)) __int64 ow;
	__asm { 
		// signed word center weight ((amount+0x100)>>9) x4
		mov		eax,amount
		add		eax,100h
		sar		eax,9
		lea		edx,cw
		mov		[edx],ax
		mov		[edx+2],ax
		mov		[edx+4],ax
		mov		[edx+6],ax
		// signed word outer weight 64-((amount+0x100)>>9) x4
		mov		ax,40h
		sub		ax,[edx]
		lea		edx,ow
		mov		[edx],ax
		mov		[edx+2],ax
		mov		[edx+4],ax
		mov		[edx+6],ax
	}
	for (int y=0;y<height;y++) {
	__asm {
		mov			ecx,p
		mov			edi,width

		movq		mm1,[ecx]		; trash + left pixel
		pxor		mm0,mm0			; zeros
		movq		mm2,mm1			; centre + right pixel
		psllq		mm1,32			; left + zero

		align		16
row_loop:
		dec			edi
		jle			odd_end

		movq		mm7,mm2			; duplicate right pixel
		punpckhbw	mm1,mm0			; unpack left pixel
		punpckhbw	mm7,mm0			; unpack right pixel
		movq		mm4,mm2			; duplicate centre pixel
		paddsw		mm7,mm1			; right + left
		punpcklbw	mm4,mm0			; unpack centre pixel
		pmullw		mm7,ow			; *= outer weight
		pmullw		mm4,cw			; *= centre weight
		movq		mm1,mm2			; left + centre pixel
		paddsw		mm7,mm4			; Weighted centres + outers
		dec			edi
		paddsw		mm7,mm4			; Weighted centres + outers
		paddsw		mm7,r7			; += 0.5
		psraw		mm7,7			; /= 32768
		jle			even_end

		movq		mm6,mm1			; duplicate left pixel
		movq		mm2,[ecx+8]		; right + trash pixel
		punpcklbw	mm6,mm0			; unpack left pixel
		movq		mm5,mm2			; duplicate right pixel
		movq		mm4,mm1			; duplicate centre pixel
		punpcklbw	mm5,mm0			; unpack right pixel
		punpckhbw	mm4,mm0			; unpack centre pixel
		paddsw		mm6,mm5			; left + right
		pmullw		mm4,cw			; *= centre weight
		pmullw		mm6,ow			; *= outer weight
		paddsw		mm6,mm4			; Weighted centres + outers
		paddsw		mm6,mm4			; Weighted centres + outers
		paddsw		mm6,r7			; += 0.5
		psraw		mm6,7			; /= 32768
		add			ecx,8
		packuswb	mm7,mm6			; pack low with high
		movq		[ecx-8],mm7		; Update 2 centre pixels
		jmp			row_loop

		align		16
odd_end:
		punpckhbw	mm1,mm0			; unpack left pixel
		punpcklbw	mm2,mm0			; unpack centre pixel
		paddsw		mm1,mm2			; left + centre
		pmullw		mm2,cw			; *= centre weight
		pmullw		mm1,ow			; *= outer weight
		paddsw		mm1,mm2			; Weighted centres + outers
		paddsw		mm1,mm2			; Weighted centres + outers
		paddsw		mm1,r7			; += 0.5
		psraw		mm1,7			; /= 32768
		packuswb	mm1,mm1			; pack low with high
		movd		[ecx],mm1		; Update 1 centre pixels
		jmp			next_loop

		align		16
even_end:
		punpckhbw	mm2,mm0			; unpack centre pixel
		punpcklbw	mm1,mm0			; unpack left pixel
		paddsw		mm1,mm2			; left + centre
		pmullw		mm2,cw			; *= centre weight
		pmullw		mm1,ow			; *= outer weight
		paddsw		mm1,mm2			; Weighted centres + outers
		paddsw		mm1,mm2			; Weighted centres + outers
		paddsw		mm1,r7			; += 0.5
		psraw		mm1,7			; /= 32768
		packuswb	mm7,mm1			; pack low with high
		movq		[ecx],mm7		; Update 2 centre pixels

next_loop:
		}
		p += pitch;
	}
	__asm emms
}
#endif

// -------------------------------------
// Blur/Sharpen Horizontal YUY2 C++ Code
// -------------------------------------

void AFH_YUY2_C(uc* p, int height, const int pitch, const int width, const int amount) {
	const int center_weight = amount*2;
	const int outer_weight = 32768-amount;
	for (int y = height; y>0; --y) 
	{
		uc yy = p[0];
		uc uv = p[1];
		uc vu = p[3];
		int x;
		for (x = 0; x < width-2; ++x) 
		{
			uc y = ScaledPixelClip(p[x*2+0] * center_weight + (yy + p[x*2+2]) * outer_weight);
			yy   = p[x*2+0];
			p[x*2+0] = y;
			uc w = ScaledPixelClip(p[x*2+1] * center_weight + (uv + p[x*2+5]) * outer_weight);
			uv   = vu;
			vu   = p[x*2+1];
			p[x*2+1] = w;
		}
		uc y     = ScaledPixelClip(p[x*2+0] * center_weight + (yy + p[x*2+2]) * outer_weight);
		yy       = p[x*2+0];
		p[x*2+0] = y;
		p[x*2+1] = ScaledPixelClip(p[x*2+1] * center_weight + (uv + p[x*2+1]) * outer_weight);
		p[x*2+2] = ScaledPixelClip(p[x*2+2] * center_weight + (yy + p[x*2+2]) * outer_weight);
		p[x*2+3] = ScaledPixelClip(p[x*2+3] * center_weight + (vu + p[x*2+3]) * outer_weight);
   
		p += pitch;
	}
}


#ifdef X86_32
// -------------------------------------
// Blur/Sharpen Horizontal YUY2 MMX Code
// -------------------------------------

void AFH_YUY2_MMX(const uc* p, const int height, const int pitch, const int width, const int amount) 
{
	// round masks
	__declspec(align(8)) const __int64 r7 = 0x0040004000400040;
	// YY and UV masks
	__declspec(align(8)) const __int64 ym = 0x00FF00FF00FF00FF;
	__declspec(align(8)) const __int64 cm = 0xFF00FF00FF00FF00;

	__declspec(align(8)) const __int64 rightmask = 0x00FF000000000000;
	// weights
	__declspec(align(8)) __int64 cw;
	__declspec(align(8)) __int64 ow;

	__asm { 
		// signed word center weight ((amount+0x100)>>9) x4
		mov		eax,amount
		add		eax,100h
		sar		eax,9
		lea		edx,cw
		mov		[edx],ax
		mov		[edx+2],ax
		mov		[edx+4],ax
		mov		[edx+6],ax
		// signed word outer weight 64-((amount+0x100)>>9) x4
		mov		ax,40h
		sub		ax,[edx]
		lea		edx,ow
		mov		[edx],ax
		mov		[edx+2],ax
		mov		[edx+4],ax
		mov		[edx+6],ax
	}

	for (int y=0;y<height;y++) {
	__asm {
		mov			ecx,p
		 movq		mm1,ym			; 0x00FF00FF00FF00FF
		movq		mm2,[ecx]		; [2v 3y 2u 2y 0v 1y 0u 0y]	-- Centre
		 movq		mm3,cm			; 0xFF00FF00FF00FF00
		pand		mm1,mm2			; [00 3y 00 2y 00 1y 00 0y]
		 pand		mm3,mm2			; [2v 00 2u 00 0v 00 0u 00]
		psllq		mm1,48			; [00 0y 00 00 00 00 00 00]
		 psllq		mm3,32			; [0v 00 0u 00 00 00 00 00]
		mov			edi,width
		 por		mm1,mm3			; [0v 0y 0u 00 00 00 00 00]	-- Left
		sub			edi,2

		align		16
row_loop:
		jle			odd_end
		sub			edi,2
		jle			even_end

		movq		mm0,mm1			;   [-2v -1y -2u -2y -4v -3y -4u -4y]
		 movq		mm3,[ecx+8]		; [6v 7y 6u 6y 4v 5y 4u 4y]	-- Right
		movq		mm5,ym			; 0x00FF00FF00FF00FF
		 movq		mm6,mm3			;   [6v 7y 6u 6y 4v 5y 4u 4y]
		pand		mm0,mm5			;   [00-1y 00-2y 00-3y 00-4y]
		 pand		mm6,mm5			;   [007y 006y 005y 004y]
		psrlq		mm0,48			;   [0000  0000  0000  00-1y]
		 pand		mm5,mm2			; [003y 002y 001y 000y]		-- Centre
		psllq		mm6,48			;   [004y 0000 0000 0000]
		 movq		mm7,mm5			;   [003y 002y 001y 000y]
		movq		mm4,mm5			;   [003y 002y 001y 000y]
		 psllq		mm7,16			;   [002y 001y 000y 0000]
		psrlq		mm5,16			;   [0000 003y 002y 001y]
		 por		mm0,mm7			; [002y 001y 000y 00-1y]	-- Left
		por			mm6,mm5			; [004y 003y 002y 001y]		-- Right
		 pmullw		mm4,cw			; *= center weight
		paddsw		mm0,mm6			; left += right 
		 movq		mm6,cm			; 0xFF00FF00FF00FF00
		pmullw		mm0,ow			; *= outer weight
		 pand		mm1,mm6			;   [-2v00 -2u00 -4v00 -4u00]
		movq		mm5,mm2			;   [2v 3y 2u 2y 0v 1y 0u 0y]
		 paddsw		mm0,mm4			; Weighted centres + outers
		psrlq		mm1,40			;   [0000 0000 00-2v 00-2u]
		 pand		mm5,mm6			;   [2v00 2u00 0v00 0u00]
		paddsw		mm0,mm4			; Weighted centres + outers
		 psrlq		mm5,8			; [002v 002u 000v 000u]		-- Centre
		pand		mm6,mm3			;   [6v00 6u00 4v00 4u00]
		 movq		mm7,mm5			;   [002v 002u 000v 000u]
		movq		mm4,mm5			;   [002v 002u 000v 000u]
		 psllq		mm6,24			;   [004v 004u 0000 0000]
		psllq		mm7,32			;   [000v 000u 0000 0000]
		 psrlq		mm4,32			;   [0000 0000 002v 002u]
		por			mm7,mm1			; [000v 000u 00-2v 00-2u]	-- Left
		 por		mm6,mm4			; [004v 004u 002v 002u]		-- Right
		 paddsw		mm7,mm6			; left += right
		pmullw		mm5,cw			; *= center weight
		 pmullw		mm7,ow			; *= outer weight
		add			ecx,8			; 
		 paddsw		mm7,mm5			; Weighted centres + outers
		paddsw		mm0,r7			; += 0.5
		 paddsw		mm7,mm5			; Weighted centres + outers
		psraw		mm0,7			; /= 32768
		 paddsw		mm7,r7			; += 0.5
		packuswb	mm0,mm0			; [3y 2y 1y 0y 3y 2y 1y 0y] -- Unsign Saturated
		 psraw		mm7,7			; /= 32768
		movq		mm1,mm2			; 
		 packuswb	mm7,mm7			; [2v 2u 0v 0u 2v 2u 0v 0u] -- Unsign Saturated
		movq		mm2,mm3			; 
		 punpcklbw	mm0,mm7			; [2v 3y 2u 2y 0v 1y 0v 0y]
		sub			edi,2
		 movq		[ecx-8],mm0		; Update 4 centre pixels
		jmp			row_loop		; 

		align		16
odd_end:
		movq		mm5,ym			; 0x00FF00FF00FF00FF
		 movq		mm0,mm1			;   [-2v -1y -2u -2y -4v -3y -4u -4y]
	;stall
		 pand		mm0,mm5			;   [00-1y 00-2y 00-3y 00-4y]
		pand		mm5,mm2			; [00xx 00xx 001y 000y]		-- Centre
		 psrlq		mm0,48			;   [0000  0000  0000  00-1y]
		movq		mm7,mm5			;   [00xx 00xx 001y 000y]
		 movq		mm6,mm5			;   [00xx 00xx 001y 000y]
		psllq		mm7,16			;   [00xx 001y 000y 0000]
		 psrlq		mm6,16			;   [0000 00xx 00xx 001y]
		por			mm0,mm7			; [00xx 001y 000y 00-1y]	-- Left
		 punpcklwd	mm6,mm6			; [00xx 00xx 001y 001y]		-- Right
		pmullw		mm5,cw			; *= center weight YY
		 paddsw		mm0,mm6			; left += right  YY
		movq		mm3,cm			; 0xFF00FF00FF00FF00
		 pmullw		mm0,ow			; *= outer weight YY
		pand		mm1,mm3			;   [-2v00 -2u00 -4v00 -4u00]
		pand		mm3,mm2			;   [xx00 xx00 0v00 0u00]
		 psrlq		mm1,40			; [0000 0000 00-2v 00-2u]	-- Left
		psrlq		mm3,8			; [0000 0000 000v 000u]		-- Centre
		paddsw		mm1,mm3			; left += centre UV
		 pmullw		mm3,cw			; *= center weight UV
		pmullw		mm1,ow			; *= outer weight UV
		 paddsw		mm0,mm5			; Weighted centres + outers YY
		paddsw		mm1,mm3			; Weighted centres + outers UV
		 paddsw		mm0,mm5			; Weighted centres + outers YY
		paddsw		mm1,mm3			; Weighted centres + outers UV
		 paddsw		mm0,r7			; += 0.5 YY
		paddsw		mm1,r7			; += 0.5 UV
		 psraw		mm0,7			; /= 32768 YY
		psraw		mm1,7			; /= 32768 UV
		 packuswb	mm0,mm0			; [xx xx 1y 0y xx xx 1y 0y] -- Unsign Saturated
		packuswb	mm1,mm1			; [xx xx 0v 0u xx xx 0v 0u] -- Unsign Saturated
		punpcklbw	mm0,mm1			; [xx xx xx xx 0v 1y 0v 0y]
		movd		[ecx],mm0		; Update 2 centre pixels
		jmp			next_loop

		align		16
even_end:

		 movq		mm5,ym			; 0x00FF00FF00FF00FF
		movq		mm0,mm1			;   [-2v -1y -2u -2y -4v -3y -4u -4y]
		 movq		mm6,rightmask	; 0x00FF000000000000
		pand		mm0,mm5			;   [00-1y 00-2y 00-3y 00-4y]
		 pand		mm6,mm2			;   [003y 0000 0000 0000]
		pand		mm5,mm2			; [003y 002y 001y 000y]		-- Centre
		 psrlq		mm0,48			;   [0000  0000  0000  00-1y]
		movq		mm7,mm5			;   [003y 002y 001y 000y]
		 movq		mm4,mm5			;   [003y 002y 001y 000y]
		psllq		mm7,16			;   [002y 001y 000y 0000]
		 psrlq		mm4,16			;   [0000 003y 002y 001y]
		por			mm0,mm7			; [002y 001y 000y 00-1y]	-- Left
		 por		mm6,mm4			; [003y 003y 002y 001y]		-- Right
		movq		mm3,cm			; 0xFF00FF00FF00FF00
		 paddsw		mm0,mm6			; left += right 
		pmullw		mm5,cw			; *= center weight
		 pmullw		mm0,ow			; *= outer weight
		 pand		mm1,mm3			;   [-2v00 -2u00 -4v00 -4u00]
		pand		mm3,mm2			;   [2v00 2u00 0v00 0u00]
		 psrlq		mm1,40			;   [0000 0000 00-2v 00-2u]
		movq		mm4,mm3			;   [2v00 2u00 0v00 0u00]
		 movq		mm7,mm3			;   [2v00 2u00 0v00 0u00]
		psrlq		mm4,40			;   [0000 0000 002v 002u]
		 psllq		mm7,24			;   [000v 000u 0000 0000]
		movq		mm6,mm4			;   [0000 0000 002v 002u]
		 por		mm1,mm7			; [000v 000u 00-2v 00-2u]	-- Left
		psllq		mm6,32			;   [002v 002u 0000 0000]
		por			mm6,mm4			; [002v 002u 002v 002u]		-- Right
		 psrlq		mm3,8			; [002v 002u 000v 000u]		-- Centre
		paddsw		mm1,mm6			; left += right
		 pmullw		mm3,cw			; *= center weight
		pmullw		mm1,ow			; *= outer weight
		 paddsw		mm0,mm5			; Weighted centres + outers
		paddsw		mm1,mm3			; Weighted centres + outers
		 paddsw		mm0,mm5			; Weighted centres + outers
		paddsw		mm1,mm3			; Weighted centres + outers
		 paddsw		mm0,r7			; += 0.5
		paddsw		mm1,r7			; += 0.5
		 psraw		mm0,7			; /= 32768
		psraw		mm1,7			; /= 32768
		 packuswb	mm0,mm0			; [3y 2y 1y 0y 3y 2y 1y 0y] -- Unsign Saturated
		packuswb	mm1,mm1			; [2v 2u 0v 0u 2v 2u 0v 0u] -- Unsign Saturated
		punpcklbw	mm0,mm1			; [2v 3y 2u 2y 0v 1y 0v 0y]
		movq		[ecx],mm0		; Update 4 centre pixels

next_loop:
		}
	p += pitch;
	}
	__asm emms
}
#endif

// --------------------------------------
// Blur/Sharpen Horizontal RGB24 C++ Code
// --------------------------------------

void AFH_RGB24_C(uc* p, int height, const int pitch, const int width, const int amount) {
  const int center_weight = amount*2;
  const int outer_weight = 32768-amount;
  for (int y = height; y>0; --y) 
    {

      uc bb = p[0];
      uc gg = p[1];
      uc rr = p[2];
      int x;
	  for (x = 0; x < width-1; ++x) 
      {
        uc b = ScaledPixelClip(p[x*3+0] * center_weight + (bb + p[x*3+3]) * outer_weight);
        bb = p[x*3+0]; p[x*3+0] = b;
        uc g = ScaledPixelClip(p[x*3+1] * center_weight + (gg + p[x*3+4]) * outer_weight);
        gg = p[x*3+1]; p[x*3+1] = g;
        uc r = ScaledPixelClip(p[x*3+2] * center_weight + (rr + p[x*3+5]) * outer_weight);
        rr = p[x*3+2]; p[x*3+2] = r;
      }
      p[x*3+0] = ScaledPixelClip(p[x*3+0] * center_weight + (bb + p[x*3+0]) * outer_weight);
      p[x*3+1] = ScaledPixelClip(p[x*3+1] * center_weight + (gg + p[x*3+1]) * outer_weight);
      p[x*3+2] = ScaledPixelClip(p[x*3+2] * center_weight + (rr + p[x*3+2]) * outer_weight);
      p += pitch;
    }
}

// -------------------------------------
// Blur/Sharpen Horizontal YV12 C++ Code
// -------------------------------------

void AFH_YV12_C(uc* p, int height, const int pitch, const int row_size, const int amount) 
{
	const int center_weight = amount*2;
	const int outer_weight = 32768-amount;
	uc pp,l;
	for (int y = height; y>0; --y) {
		l = p[0];
		int x;
		for (x = 1; x < row_size-1; ++x) {
			pp = ScaledPixelClip(p[x] * center_weight + (l + p[x+1]) * outer_weight);
			l=p[x]; p[x]=pp;
		}
		p[x] = ScaledPixelClip(p[x] * center_weight + (l + p[x]) * outer_weight);
		p += pitch;
	}
}

#ifdef X86_32
// -------------------------------------
// Blur/Sharpen Horizontal YV12 MMX Code
// -------------------------------------

#define scale(mmAA,mmBB,mmCC,mmA,mmB,mmC,zeros)	\
__asm	movq		mmC,mmCC	/* Right 8 pixels      */\
__asm	 movq		mmA,mmAA	/* Left     "          */\
__asm	movq		mmB,mmBB	/* Centre   "          */\
__asm	 punpcklbw	mmC,zeros	/* Low 4 right         */\
__asm	punpcklbw	mmA,zeros	/* Low 4 left          */\
__asm	 punpcklbw	mmB,zeros	/* Low 4 centre        */\
__asm	paddsw		mmA,mmC		/* Low 4 left + right  */\
__asm	 pmullw		mmB,cw		/* *= centre weight    */\
__asm	pmullw		mmA,ow		/* *= outer weight     */\
__asm	 punpckhbw	mmCC,zeros	/* High 4 Right        */\
__asm	punpckhbw	mmAA,zeros	/* High 4 Left         */\
__asm	 punpckhbw	mmBB,zeros	/* High 4 Centre       */\
__asm	paddsw		mmAA,mmCC	/* High 4 left + right */\
__asm	 pmullw		mmBB,cw		/* *= centre weight    */\
__asm	pmullw		mmAA,ow		/* *= outer weight     */\
__asm	 paddsw		mmA,mmB		/* += weighed low 4    */\
__asm	paddsw		mmAA,mmBB	/* += weighed high 4   */\
__asm	 paddsw		mmA,mmB		/* += weighed low 4    */\
__asm	paddsw		mmAA,mmBB	/* += weighed high 4   */\
__asm	 paddsw		mmA,r7		/* += 0.5              */\
__asm	paddsw		mmAA,r7		/* += 0.5              */\
__asm	 psraw		mmA,7		/* /= 128              */\
__asm	psraw		mmAA,7		/* /= 128              */\
__asm	 add		ecx,8		/* p += 8              */\
__asm	packuswb	mmA,mmAA	/* Packed new 8 pixels */
#endif


#ifdef X86_32
// 
// Planer MMX blur/sharpen - process 8 pixels at a time
//   FillBorder::Create(_child)) ensures the right edge is repeated to mod 8 width
//   frame->GetRowSize(plane|PLANAR_ALIGNED) ensured rowsize is also mod 8
// For pitch less than 8 C code is used (unlikely, I couldn't force a case)
// 
void AFH_YV12_MMX(uc* p, int height, const int pitch, const int row_size, const int amount) 
{
	// weights
	__declspec(align(8)) __int64 cw;
	__declspec(align(8)) __int64 ow;
	// round masks
	__declspec(align(8)) const __int64 r7 = 0x0040004000400040;
	// edge masks
	__declspec(align(8)) const __int64 leftmask  = 0x00000000000000FF;
	__declspec(align(8)) const __int64 rightmask = 0xFF00000000000000;

	__asm { 
		// signed word center weight ((amount+0x100)>>9) x4
		mov		eax,amount
		add		eax,100h
		sar		eax,9
		lea		edx,cw
		mov		[edx],ax
		mov		[edx+2],ax
		mov		[edx+4],ax
		mov		[edx+6],ax
		// signed word outer weight 64-((amount+0x100)>>9) x4
		mov		ax,40h
		sub		ax,[edx]
		lea		edx,ow
		mov		[edx],ax
		mov		[edx+2],ax
		mov		[edx+4],ax
		mov		[edx+6],ax
	}

	for (int y=0;y<height;y++) {

	__asm {
		mov			ecx,p
		mov			edi,pitch
		add			edi,ecx
		mov			p,edi			; p += pitch;

		mov			edi,row_size
		test		edi,7
		jz			nopad
		mov			al,[ecx+edi-1]	; Pad edge pixel if needed
		mov			[ecx+edi],al
nopad:
		add			edi,7
		shr			edi,3

		pxor		mm0,mm0
; first row
		 movq		mm2,[ecx]		; Centre 8 pixels
		movq		mm1,leftmask	; 0x00000000000000ff
		 movq		mm3,mm2			; Duplicate for left
		pand		mm1,mm2			; Left edge pixel
		 psllq		mm3,8			; Left 7 other pixels
		dec			edi
		 por		mm1,mm3			; Left pixels, left most repeated
		jz			out_row_loop_ex	; 8 or less pixels per line

		movq		mm3,[ecx+1]		; Right 8 pixels

		scale(mm1,mm2,mm3,mm4,mm5,mm6,mm0)

		dec			edi
		jz			out_row_loop	; 9 to 16 pixels per line

		align 16
row_loop:
		movq		mm1,[ecx-1]		; Pickup left 8th before it's updated
		 movq		[ecx-8],mm4		; update current 8 pixels
		movq		mm2,[ecx]		; Centre 8 pixels
		 movq		mm3,[ecx+1]		; Right 8 pixels

		scale(mm1,mm2,mm3,mm4,mm5,mm6,mm0)

		dec			edi
		jnz			row_loop		; more ?

out_row_loop:
		movq		mm1,[ecx-1]		; Pickup left 8th before it's updated
		 movq		[ecx-8],mm4		; update current 8 pixels
		movq		mm2,[ecx]		; Centre 8 pixels
out_row_loop_ex:
		 movq		mm3,rightmask	; 0xFF00000000000000
		movq		mm4,mm2			; Duplicate for right
		 pand		mm3,mm2			; Right edge pixel
		psrlq		mm4,8			; Right 7 other pixels
		por			mm3,mm4			; Right pixles, right most repeated

		scale(mm1,mm2,mm3,mm4,mm5,mm6,mm0)

		movq		[ecx-8],mm4		; update current 8 pixels
		}
	}
	__asm emms
}
#endif


/************************************************
 *******   Sharpen/Blur Factory Methods   *******
 ***********************************************/

AVSValue __cdecl Create_Sharpen(AVSValue args, void*, IScriptEnvironment* env) 
{
  const double amountH = args[1].AsFloat(), amountV = args[2].AsDblDef(amountH);
  const bool mmx = args[3].AsBool(true) && (env->GetCPUFlags() & CPUF_MMX);

  if (amountH < -1.5849625 || amountH > 1.0 || amountV < -1.5849625 || amountV > 1.0) // log2(3)
    env->ThrowError("Sharpen: arguments must be in the range -1.58 to 1.0");

  if (fabs(amountH) < 0.00002201361136) { // log2(1+1/65536)
    if (fabs(amountV) < 0.00002201361136) {
      return args[0].AsClip();
    }
    else {
      return new AdjustFocusV(amountV, args[0].AsClip(), mmx);
    }
  }
  else {
    if (fabs(amountV) < 0.00002201361136) {
      return new AdjustFocusH(amountH, args[0].AsClip(), mmx);
    }
    else {
      return new AdjustFocusH(amountH, new AdjustFocusV(amountV, args[0].AsClip(), mmx), mmx);
    }
  }
}

AVSValue __cdecl Create_Blur(AVSValue args, void*, IScriptEnvironment* env) 
{
  const double amountH = args[1].AsFloat(), amountV = args[2].AsDblDef(amountH);
  const bool mmx = args[3].AsBool(true) && (env->GetCPUFlags() & CPUF_MMX);

  if (amountH < -1.0 || amountH > 1.5849625 || amountV < -1.0 || amountV > 1.5849625) // log2(3)
    env->ThrowError("Blur: arguments must be in the range -1.0 to 1.58");

  if (fabs(amountH) < 0.00002201361136) { // log2(1+1/65536)
    if (fabs(amountV) < 0.00002201361136) {
      return args[0].AsClip();
    }
    else {
      return new AdjustFocusV(-amountV, args[0].AsClip(), mmx);
    }
  }
  else {
    if (fabs(amountV) < 0.00002201361136) {
      return new AdjustFocusH(-amountH, args[0].AsClip(), mmx);
    }
    else {
      return new AdjustFocusH(-amountH, new AdjustFocusV(-amountV, args[0].AsClip(), mmx), mmx);
    }
  }
}




/***************************
 ****  TemporalSoften  *****
 **************************/

 /**
 *  YV12 / YUY2 code (c) by Klaus Post // sh0dan 2002 - 2003
 *
 * - All frames are loaded (we rely on the cache for this, which is why it has to be rewritten)
 * - Pointer array is given to the mmx function for planes.
 * - One line of each planes is one after one compared to the current frame. 
 * - Accumulated values are stored in an arrays. (as shorts)
 * - The divisor is stored in a separate array (as bytes)
 * - The divisor is looked up.
 * - Result is stored.
 * Mode 2:
 * - Works by storing the pixels of the original plane, 
 *     when pixels of the tested frames are larger than threshold.
 **/



TemporalSoften::TemporalSoften( PClip _child, unsigned radius, unsigned luma_thresh, 
                                unsigned chroma_thresh, int _scenechange, IScriptEnvironment* env )
  : GenericVideoFilter  (_child),
    chroma_threshold    (min(chroma_thresh,255u)),
    luma_threshold      (min(luma_thresh,255u)),
    kernel              (2*min(radius,(unsigned int)MAX_RADIUS)+1),
    scenechange (_scenechange)
{

  child->SetCacheHints(CACHE_WINDOW,kernel);

  if (vi.IsRGB24()) {
    env->ThrowError("TemporalSoften: RGB24 Not supported, use ConvertToRGB32().");
  }

  if ((vi.IsRGB32()) && (vi.width&1)) {
    env->ThrowError("TemporalSoften: RGB32 source must be multiple of 2 in width.");
  }

  if ((vi.IsYUY2()) && (vi.width&3)) {
    env->ThrowError("TemporalSoften: YUY2 source must be multiple of 4 in width.");
  }

  if (scenechange >= 255) {
    scenechange = 0;
  }
  if (scenechange>0 && vi.IsRGB32()) {
      env->ThrowError("TemporalSoften: Scenechange not available on RGB32");
  }

  scenechange *= ((vi.width/32)*32)*vi.height*vi.BytesFromPixels(1);
  
  planes = new int[8];
  planeP = new const BYTE*[16];
  planeP2 = new const BYTE*[16];
  planePitch = new int[16];
  planePitch2 = new int[16];
  planeDisabled = new bool[16];

  int c = 0;
  if (vi.IsPlanar()) {
    if (luma_thresh>0) {planes[c++] = PLANAR_Y; planes[c++] = luma_thresh;}
    if (chroma_thresh>0) { planes[c++] = PLANAR_V;planes[c++] =chroma_thresh; planes[c++] = PLANAR_U;planes[c++] = chroma_thresh;}
  } else if (vi.IsYUY2()) {
    planes[c++]=0;
    planes[c++]=luma_thresh|(chroma_thresh<<8);
  } else if (vi.IsRGB()) {  // For RGB We use Luma.
    planes[c++]=0;
    planes[c++]=luma_thresh;
  }
  planes[c]=0;
  frames = new PVideoFrame[kernel];
}



TemporalSoften::~TemporalSoften(void) 
{
    delete[] planes;
    delete[] planeP;
    delete[] planeP2;
    delete[] planePitch;
    delete[] planePitch2;
    delete[] planeDisabled;
    delete[] frames;
}

//offset is the initial value of x. Used when C routine processes only parts of frames after SSE/MMX paths do their job.
static void accumulate_line_c(BYTE* c_plane, const BYTE** planeP, int planes, int offset, size_t width, BYTE threshold, int div) {
  for (size_t x = offset; x < width; ++x) {
    BYTE current = c_plane[x];
    size_t sum = current;

    for (int plane = planes - 1; plane >= 0; plane--) {
      BYTE p = planeP[plane][x];
      size_t absdiff = std::abs(current - p);

      if (absdiff <= threshold) {
        sum += p;
      } else {
        sum += current;
      }
    }

    c_plane[x] = (BYTE)((sum * div + 16384) >> 15);
  }
}

static void accumulate_line_yuy2_c(BYTE* c_plane, const BYTE** planeP, int planes, size_t width, BYTE threshold_luma, BYTE threshold_chroma, int div) {
  for (size_t x = 0; x < width; x+=2) {
    BYTE current_y = c_plane[x];
    BYTE current_c = c_plane[x+1];
    size_t sum_y = current_y;
    size_t sum_c = current_c;

    for (int plane = planes - 1; plane >= 0; plane--) {
      BYTE p_y = planeP[plane][x];
      BYTE p_c = planeP[plane][x+1];
      size_t absdiff_y = std::abs(current_y - p_y);
      size_t absdiff_c = std::abs(current_c - p_c);

      if (absdiff_y <= threshold_luma) {
        sum_y += p_y;
      } else {
        sum_y += current_y;
      }

      if (absdiff_c <= threshold_chroma) {
        sum_c += p_c;
      } else {
        sum_c += current_c;
      }
    }

    c_plane[x] = (BYTE)((sum_y * div + 16384) >> 15);
    c_plane[x+1] = (BYTE)((sum_c * div + 16384) >> 15);
  }
}


static __forceinline __m128i ts_multiply_repack_sse2(__m128i &src, __m128i &div, __m128i &halfdiv, __m128i &zero) {
  __m128i acc = _mm_madd_epi16(src, div);
  acc = _mm_add_epi32(acc, halfdiv);
  acc = _mm_srli_epi32(acc, 15);
  acc = _mm_packs_epi32(acc, acc);
  return _mm_packus_epi16(acc, zero);
}


static void accumulate_line_sse2(BYTE* c_plane, const BYTE** planeP, int planes, size_t width, int threshold, int div) {
  for (size_t x = 0; x < width; x+=16) {
    __m128i current = _mm_load_si128(reinterpret_cast<const __m128i*>(c_plane+x));
    __m128i zero = _mm_setzero_si128();
    __m128i low = _mm_unpacklo_epi8(current, zero);
    __m128i high = _mm_unpackhi_epi8(current, zero);
    __m128i thresh = _mm_set1_epi16(threshold);
    __m128i sum = current;

    for(int plane = planes-1; plane >= 0; --plane) {
      __m128i p = _mm_load_si128(reinterpret_cast<const __m128i*>(planeP[plane]+x));

      __m128i adiff_h = _mm_subs_epu8(p, current);
      __m128i adiff_l = _mm_subs_epu8(current, p);
      __m128i adiff = _mm_or_si128(adiff_h, adiff_l); //abs(p-c)

      __m128i over_thresh = _mm_subs_epu8(adiff, thresh);
      __m128i leq_thresh = _mm_cmpeq_epi8(over_thresh, zero); //abs diff lower or equal to threshold

      __m128i andop = _mm_and_si128(leq_thresh, p);
      __m128i andnop = _mm_andnot_si128(leq_thresh, current);
      __m128i blended = _mm_or_si128(andop, andnop); //abs(p-c) <= thresh ? p : c

      __m128i add_low = _mm_unpacklo_epi8(blended, zero);
      __m128i add_high = _mm_unpackhi_epi8(blended, zero);

      low = _mm_adds_epu16(low, add_low);
      high = _mm_adds_epu16(high, add_high);
    }

    __m128i halfdiv_vector = _mm_set1_epi32(16384);
    __m128i div_vector = _mm_set1_epi16(div);

    __m128i low_low   = ts_multiply_repack_sse2(_mm_unpacklo_epi16(low, zero), div_vector, halfdiv_vector, zero);
    __m128i low_high  = ts_multiply_repack_sse2(_mm_unpackhi_epi16(low, zero), div_vector, halfdiv_vector, zero);
    __m128i high_low  = ts_multiply_repack_sse2(_mm_unpacklo_epi16(high, zero), div_vector, halfdiv_vector, zero);
    __m128i high_high = ts_multiply_repack_sse2(_mm_unpackhi_epi16(high, zero), div_vector, halfdiv_vector, zero);

    __m128i mask = _mm_set_epi32(0, 0, 0, 0xFFFF);
    low_low = _mm_and_si128(low_low, mask);
    low_high = _mm_and_si128(low_high, mask);
    high_low = _mm_and_si128(high_low, mask);

    low_high = _mm_slli_si128(low_high, 16);
    high_low = _mm_slli_si128(high_low, 32);
    high_high = _mm_slli_si128(high_high, 48);

    __m128i acc = _mm_or_si128(low_low, low_high);
    acc = _mm_or_si128(acc, high_low);
    acc = _mm_or_si128(acc, high_high);

    _mm_store_si128(reinterpret_cast<__m128i*>(c_plane+x), acc);
  }
}

#ifdef X86_32

static __forceinline __m64 ts_multiply_repack_mmx(__m64 &src, __m64 &div, __m64 &halfdiv, __m64 &zero) {
  __m64 acc = _mm_madd_pi16(src, div);
  acc = _mm_add_pi32(acc, halfdiv);
  acc = _mm_srli_pi32(acc, 15);
  acc = _mm_packs_pi32(acc, acc);
  return _mm_packs_pu16(acc, zero);
}

//thresh and div must always be 16-bit integers. Thresh is 2 packed bytes and div is a single 16-bit number
static void accumulate_line_mmx(BYTE* c_plane, const BYTE** planeP, int planes, size_t width, int threshold, int div) {
  for (size_t x = 0; x < width; x+=8) {
    __m64 current = *reinterpret_cast<const __m64*>(c_plane+x);
    __m64 zero = _mm_setzero_si64();
    __m64 low = _mm_unpacklo_pi8(current, zero);
    __m64 high = _mm_unpackhi_pi8(current, zero);
    __m64 thresh = _mm_set1_pi16(threshold);
    __m64 sum = current;

    for(int plane = planes-1; plane >= 0; --plane) {
      __m64 p = *reinterpret_cast<const __m64*>(planeP[plane]+x);

      __m64 adiff_h = _mm_subs_pu8(p, current);
      __m64 adiff_l = _mm_subs_pu8(current, p);
      __m64 adiff = _mm_or_si64(adiff_h, adiff_l); //abs(p-c)

      __m64 over_thresh = _mm_subs_pu8(adiff, thresh);
      __m64 leq_thresh = _mm_cmpeq_pi8(over_thresh, zero); //abs diff lower or equal to threshold

      __m64 andop = _mm_and_si64(leq_thresh, p);
      __m64 andnop = _mm_andnot_si64(leq_thresh, current);
      __m64 blended = _mm_or_si64(andop, andnop); //abs(p-c) <= thresh ? p : c

      __m64 add_low = _mm_unpacklo_pi8(blended, zero);
      __m64 add_high = _mm_unpackhi_pi8(blended, zero);

      low = _mm_adds_pu16(low, add_low);
      high = _mm_adds_pu16(high, add_high);
    }

    __m64 halfdiv_vector = _mm_set1_pi32(16384);
    __m64 div_vector = _mm_set1_pi16(div);
    
    __m64 low_low   = ts_multiply_repack_mmx(_mm_unpacklo_pi16(low, zero), div_vector, halfdiv_vector, zero);
    __m64 low_high  = ts_multiply_repack_mmx(_mm_unpackhi_pi16(low, zero), div_vector, halfdiv_vector, zero);
    __m64 high_low  = ts_multiply_repack_mmx(_mm_unpacklo_pi16(high, zero), div_vector, halfdiv_vector, zero);
    __m64 high_high = ts_multiply_repack_mmx(_mm_unpackhi_pi16(high, zero), div_vector, halfdiv_vector, zero);

    __m64 mask = _mm_set_pi32(0, 0xFFFF);
    low_low = _mm_and_si64(low_low, mask);
    low_high = _mm_and_si64(low_high, mask);
    high_low = _mm_and_si64(high_low, mask);

    low_high = _mm_slli_si64(low_high, 16);
    high_low = _mm_slli_si64(high_low, 32);
    high_high = _mm_slli_si64(high_high, 48);

    __m64 acc = _mm_or_si64(low_low, low_high);
    acc = _mm_or_si64(acc, high_low);
    acc = _mm_or_si64(acc, high_high);

    *reinterpret_cast<__m64*>(c_plane+x) = acc;
  }
  _mm_empty();
}

#endif

static void accumulate_line_yuy2(BYTE* c_plane, const BYTE** planeP, int planes, size_t width, BYTE threshold_luma, BYTE threshold_chroma, int div, bool aligned16, IScriptEnvironment* env) {
  if ((env->GetCPUFlags() & CPUF_SSE2) && aligned16 && width >= 16) {
    accumulate_line_sse2(c_plane, planeP, planes, width, threshold_luma | (threshold_chroma << 8), div);
  } else
#ifdef X86_32
  if ((env->GetCPUFlags() & CPUF_MMX) && width >= 8) {
    accumulate_line_mmx(c_plane, planeP, planes, width, threshold_luma | (threshold_chroma << 8), div); //yuy2 is always at least mod8
  } else 
#endif
    accumulate_line_yuy2_c(c_plane, planeP, planes, width, threshold_luma, threshold_chroma, div);
}

static void accumulate_line(BYTE* c_plane, const BYTE** planeP, int planes, size_t width, BYTE threshold, int div, bool aligned16, IScriptEnvironment* env) {
  if ((env->GetCPUFlags() & CPUF_SSE2) && aligned16 && width >= 16) {
    accumulate_line_sse2(c_plane, planeP, planes, width, threshold | (threshold << 8), div);
  } else
#ifdef X86_32
  if ((env->GetCPUFlags() & CPUF_MMX) && width >= 8) {
    size_t mod8_width = width / 8 * 8;
    accumulate_line_mmx(c_plane, planeP, planes, width, threshold | (threshold << 8), div);

    if (mod8_width != width) {
      accumulate_line_c(c_plane, planeP, planes, mod8_width, width - mod8_width, threshold, div);
    }
  } else 
#endif
    accumulate_line_c(c_plane, planeP, planes, 0, width, threshold, div);
}


static int calculate_sad_sse2(const BYTE* cur_ptr, const BYTE* other_ptr, int cur_pitch, int other_pitch, size_t width, size_t height)
{
  size_t mod16_width = width / 16 * 16;
  int result = 0;
  __m128i sum = _mm_setzero_si128();
  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < mod16_width; x+=16) {
      __m128i cur = _mm_load_si128(reinterpret_cast<const __m128i*>(cur_ptr + x));
      __m128i other = _mm_load_si128(reinterpret_cast<const __m128i*>(other_ptr + x));
      __m128i sad = _mm_sad_epu8(cur, other);
      sum = _mm_add_epi32(sum, sad);
    }
    if (mod16_width != width) {
      for (size_t x = mod16_width; x < width; ++x) {
        result += std::abs(cur_ptr[x] - other_ptr[x]);
      }
    }
    cur_ptr += cur_pitch;
    other_ptr += other_pitch;
  }
  __m128i upper = _mm_castps_si128(_mm_movehl_ps(_mm_setzero_ps(), _mm_castsi128_ps(sum)));
  sum = _mm_add_epi32(sum, upper);
  result += _mm_cvtsi128_si32(sum);
  return result;
}

#ifdef X86_32
static int calculate_sad_isse(const BYTE* cur_ptr, const BYTE* other_ptr, int cur_pitch, int other_pitch, size_t width, size_t height)
{
  size_t mod8_width = width / 8 * 8;
  int result = 0;
  __m64 sum = _mm_setzero_si64();
  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < mod8_width; x+=8) {
      __m64 cur = *reinterpret_cast<const __m64*>(cur_ptr + x);
      __m64 other = *reinterpret_cast<const __m64*>(other_ptr + x);
      __m64 sad = _mm_sad_pu8(cur, other);
      sum = _mm_add_pi32(sum, sad);
    }
    if (mod8_width != width) {
      for (size_t x = mod8_width; x < width; ++x) {
        result += std::abs(cur_ptr[x] - other_ptr[x]);
      }
    }

    cur_ptr += cur_pitch;
    other_ptr += other_pitch;
  }
  result += _mm_cvtsi64_si32(sum);
  _mm_empty();
  return result;
}
#endif

static int calculate_sad_c(const BYTE* cur_ptr, const BYTE* other_ptr, int cur_pitch, int other_pitch, size_t width, size_t height)
{
  size_t sum = 0;
  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < width; ++x) {
      sum += std::abs(cur_ptr[x] - other_ptr[x]);
    }
    cur_ptr += cur_pitch;
    other_ptr += other_pitch;
  }
  return sum;
}

static int calculate_sad(const BYTE* cur_ptr, const BYTE* other_ptr, int cur_pitch, int other_pitch, size_t width, size_t height, IScriptEnvironment* env) {
  if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(cur_pitch, 16) && IsPtrAligned(other_ptr, 16) && width >= 16) {
    return calculate_sad_sse2(cur_ptr, other_ptr, cur_pitch, other_pitch, width, height);
  }
#ifdef X86_32
  if ((env->GetCPUFlags() & CPUF_INTEGER_SSE) && width >= 8) {
    return calculate_sad_isse(cur_ptr, other_ptr, cur_pitch, other_pitch, width, height);
  }
#endif
  return calculate_sad_c(cur_ptr, other_ptr, cur_pitch, other_pitch, width, height);
}


PVideoFrame TemporalSoften::GetFrame(int n, IScriptEnvironment* env) 
{
  int radius = (kernel-1) / 2;

#ifdef X86_32
  int threshold = 0;
  int c=0;
  
  // Just skip if silly settings

  if ((!luma_threshold) && (!chroma_threshold) || (!radius))
    return child->GetFrame(n,env);
    

  {for (int p=0; p<16; p++) {
    planeDisabled[p]=false;
  }}
  
  {for (int p=n-radius; p<=n+radius; p++) {
    frames[p+radius-n] = child->GetFrame(clamp(p, 0, vi.num_frames-1), env);
  }}

  env->MakeWritable(&frames[radius]);

  do {
    int c_thresh = planes[c+1];  // Threshold for current plane.
    int d=0;
    {for (int i = 0; i<radius; i++) { // Fetch all planes sequencially
      planePitch[d] = frames[i]->GetPitch(planes[c]);
      planeP[d++] = frames[i]->GetReadPtr(planes[c]);
    }}

    BYTE* c_plane= frames[radius]->GetWritePtr(planes[c]);

    {for (int i = 1; i<=radius; i++) { // Fetch all planes sequencially
      planePitch[d] = frames[radius+i]->GetPitch(planes[c]);
      planeP[d++] = frames[radius+i]->GetReadPtr(planes[c]);
    }}
    
    int rowsize=frames[radius]->GetRowSize(planes[c]|PLANAR_ALIGNED);
    int h = frames[radius]->GetHeight(planes[c]);
    int pitch = frames[radius]->GetPitch(planes[c]);

    if (scenechange>0) {
      int d2=0;
      bool skiprest = false;
      for (int i = radius-1; i>=0; i--) { // Check frames backwards
        if ((!skiprest) && (!planeDisabled[i])) {
          int sad = calculate_sad(c_plane, planeP[i], pitch,  planePitch[i],  frames[radius]->GetRowSize(planes[c]), h, env);
          if (sad < scenechange) {
            planePitch2[d2] =  planePitch[i];
            planeP2[d2++] = planeP[i]; 
          } else {
            skiprest = true;
          }
          planeDisabled[i] = skiprest;  // Disable this frame on next plane (so that Y can affect UV)
        } else {
          planeDisabled[i] = true;
        }
      }
      skiprest = false;
      for (int i = radius; i < 2*radius; i++) { // Check forward frames
        if ((!skiprest)  && (!planeDisabled[i]) ) {   // Disable this frame on next plane (so that Y can affect UV)
          int sad = calculate_sad(c_plane, planeP[i], pitch,  planePitch[i],  frames[radius]->GetRowSize(planes[c]), h, env);
          if (sad < scenechange) {
            planePitch2[d2] =  planePitch[i];
            planeP2[d2++] = planeP[i];
          } else {
            skiprest = true;
          }
          planeDisabled[i] = skiprest;
        } else {
          planeDisabled[i] = true;
        }
      }
      
      //Copy back
      for (int i=0;i<d2;i++) {
        planeP[i]=planeP2[i];
        planePitch[i]=planePitch2[i];
      }
      d = d2;
    }

    if (d<1) 
      return frames[radius];

   
    int c_div = 32768/(d+1);  // We also have the tetplane included, thus d+1.
    if (c_thresh) {
      bool aligned16 = IsPtrAligned(c_plane, 16);
      if ((env->GetCPUFlags() & CPUF_SSE2) && aligned16) {
        for (int i = 0; i < d; ++i) {
          aligned16 = aligned16 && IsPtrAligned(planes[i], 16);
        }
      }

      for (int y=0;y<h;y++) { // One line at the time
        if (vi.IsYUY2()) {
          accumulate_line_yuy2(c_plane, planeP, d, rowsize, luma_threshold, chroma_threshold, c_div, aligned16, env);
        } else {
          accumulate_line(c_plane, planeP, d, rowsize, c_thresh, c_div, aligned16, env);
        }
        for (int p=0;p<d;p++)
          planeP[p] += planePitch[p];
        c_plane += pitch;
      }
    } else { // Just maintain the plane
    }
    c+=2;
  } while (planes[c]);
#else
  //TODO
  env->ThrowError("TemporalSoften::GetFrame is not yet ported to 64-bit.");
#endif

  return frames[radius];
}


AVSValue __cdecl TemporalSoften::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  return new TemporalSoften( args[0].AsClip(), args[1].AsInt(), args[2].AsInt(), 
                             args[3].AsInt(), args[4].AsInt(0),/*args[5].AsInt(1),*/env ); //ignore mode parameter
}



/****************************
 ****  Spatial Soften   *****
 ***************************/

SpatialSoften::SpatialSoften( PClip _child, int _radius, unsigned _luma_threshold, 
                              unsigned _chroma_threshold, IScriptEnvironment* env )
  : GenericVideoFilter(_child), diameter(_radius*2+1),
    luma_threshold(_luma_threshold), chroma_threshold(_chroma_threshold)
{
  if (!vi.IsYUY2())
    env->ThrowError("SpatialSoften: requires YUY2 input");
}


PVideoFrame SpatialSoften::GetFrame(int n, IScriptEnvironment* env) 
{
  PVideoFrame src = child->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrame(vi);

  const BYTE* srcp = src->GetReadPtr();
  BYTE* dstp = dst->GetWritePtr();
  int src_pitch = src->GetPitch();
  int dst_pitch = dst->GetPitch();
  int row_size = src->GetRowSize();

  for (int y=0; y<vi.height; ++y) 
  {
    const BYTE* line[65];    // better not make diameter bigger than this...
    for (int h=0; h<diameter; ++h)
      line[h] = &srcp[src_pitch * clamp(y+h-(diameter>>1), 0, vi.height-1)];
    int x;

    int edge = (diameter+1) & -4;
    for (x=0; x<edge; ++x)  // diameter-1 == (diameter>>1) * 2
      dstp[y*dst_pitch + x] = srcp[y*src_pitch + x];
    for (; x < row_size - edge; x+=2) 
    {
      int cnt=0, _y=0, _u=0, _v=0;
      int xx = x | 3;
      int Y = srcp[y*src_pitch + x], U = srcp[y*src_pitch + xx - 2], V = srcp[y*src_pitch + xx];
      for (int h=0; h<diameter; ++h) 
      {
        for (int w = -diameter+1; w < diameter; w += 2) 
        {
          int xw = (x+w) | 3;
          if (IsClose(line[h][x+w], Y, luma_threshold) && IsClose(line[h][xw-2], U,
                      chroma_threshold) && IsClose(line[h][xw], V, chroma_threshold)) 
          {
            ++cnt; _y += line[h][x+w]; _u += line[h][xw-2]; _v += line[h][xw];
          }
        }
      }
      dstp[y*dst_pitch + x] = (_y + (cnt>>1)) / cnt;
      if (!(x&3)) {
        dstp[y*dst_pitch + x+1] = (_u + (cnt>>1)) / cnt;
        dstp[y*dst_pitch + x+3] = (_v + (cnt>>1)) / cnt;
      }
    }
    for (; x<row_size; ++x)
      dstp[y*dst_pitch + x] = srcp[y*src_pitch + x];
  }

  return dst;
}


AVSValue __cdecl SpatialSoften::Create(AVSValue args, void*, IScriptEnvironment* env)
{
  return new SpatialSoften( args[0].AsClip(), args[1].AsInt(), args[2].AsInt(), 
                            args[3].AsInt(), env );
}
