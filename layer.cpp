// Avisynth v1.0 beta.  Copyright 2000 Ben Rudiak-Gould.
// http://www.math.berkeley.edu/~benrg/avisynth.html

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


#include "layer.h"



/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

AVSFunction Layer_filters[] = {
  { "Mask", "cc", Mask::Create },     // clip, mask
  { "Layer", "ccs[x]iii[threshold]i[use_chroma]b", Layer::Create },
  /**
    * Layer(clip, overlayclip, amount, xpos, ypos, [threshold=0], [use_chroma=true])
   **/     
  { "Subtract", "cc", Subtract::Create },
  { 0,0,0 }
};





/******************************
 *******   Mask Filter   ******
 ******************************/

Mask::Mask(PClip _child1, PClip _child2, IScriptEnvironment* env)
  : child1(_child1), child2(_child2)
{
  const VideoInfo& vi1 = child1->GetVideoInfo();
  const VideoInfo& vi2 = child2->GetVideoInfo();
  if (vi1.width != vi2.width || vi1.height != vi2.height)
    env->ThrowError("Mask error: image dimensions don't match");
  if (!vi1.IsRGB32() | !vi2.IsRGB32())
    env->ThrowError("Mask error: sources must be RGB32");

  vi = vi1;
  mask_frames = vi2.num_frames;
}


PVideoFrame __stdcall Mask::GetFrame(int n, IScriptEnvironment* env) 
{
  PVideoFrame src1 = child1->GetFrame(n, env);
  PVideoFrame src2 = child2->GetFrame(min(n,mask_frames-1), env);

  env->MakeWritable(&src1);

	BYTE* src1p = src1->GetWritePtr();
	const BYTE* src2p = src2->GetReadPtr();

	const int pitch = src1->GetPitch();

	const int cyb = int(0.114*32768+0.5);
	const int cyg = int(0.587*32768+0.5);
	const int cyr = int(0.299*32768+0.5);

	const int myx = vi.width;
	const int myy = vi.height;

	__int64 rgb2lum = ((__int64)cyb << 32) | (cyg << 16) | cyr;
	__int64 alpha_mask=0x00ffffff00ffffff;
	__int64 color_mask=0xff000000ff000000;
/*
  for (int y=0; y<vi.height; ++y) 
  {
	  for (int x=0; x<vi.width; ++x)
		  src1p[x*4+3] = (cyb*src2p[x*src2_pixels] + cyg*src2p[x*src2_pixels+1] + 
                    cyr*src2p[x*src2_pixels+2] + 0x8000) >> 16; 
    
    src1p += src1_pitch;
    src2p += src2_pitch;
  }
*/
 		__asm {
		mov			edi, src1p
		mov			esi, src2p
		mov			ebx, myy
		movq		mm1,rgb2lum
		movq		mm2, alpha_mask
		movq		mm3, color_mask
		xor         ecx, ecx
		pxor		mm0,mm0
		mov         edx, myx

		mask_mmxloop:

						movd		mm6, [esi + ecx*4] ; pipeline in next mask pixel RGB

						movq		mm5,mm1					;get rgb2lum

						movd		mm4, [edi + ecx*4]	;get color RGBA

						punpcklbw		mm6,mm0		;mm6= 00aa|00bb|00gg|00rr [src2]
						pmaddwd			mm6,mm5		;partial monochrome result

						mov		eax, ecx		;remember this pointer for the queue... aka pipline overhead

						inc         ecx				;point to next - aka loop counter

						punpckldq		mm5,mm6		;ready to add

						paddd			mm6, mm5			;32 bit result
						psrlq			mm6, 23				;8 bit result

						pand			mm4, mm2			;strip out old alpha
						pand			mm6, mm3			;clear any possible junk

						cmp			ecx, edx

						por				mm6, mm4			;merge new alpha and original color
						movd        [edi + eax*4],mm6		;store'em where they belong (at ecx-1)

				jnz         mask_mmxloop

		add			edi, pitch
		add			esi, pitch
		mov       edx, myx
		xor         ecx, ecx
		dec		ebx
		jnz		mask_mmxloop
		emms
		}
 return src1;
}

AVSValue __cdecl Mask::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  return new Mask(args[0].AsClip(), args[1].AsClip(), env);
}









/*******************************
 *******   Layer Filter   ******
 *******************************/

Layer::Layer( PClip _child1, PClip _child2, const char _op[], int _lev, int _x, int _y, 
              int _t, bool _chroma, IScriptEnvironment* env )
  : child1(_child1), levelA(255-_lev), child2(_child2), levelB(_lev), ofsX(_x), ofsY(_y), Op(_op), 
    T(_t), chroma(_chroma)
{
	const VideoInfo& vi1 = child1->GetVideoInfo();
  const VideoInfo& vi2 = child2->GetVideoInfo();
    
    if (vi1.pixel_type != vi2.pixel_type)
      env->ThrowError("Layer: image formats don't match");

  vi = vi1;

//	ofsY = vi.height-vi2.height-ofsY; //RGB is upside down

	if (vi.IsRGB32()) ofsY = vi.height-vi2.height-ofsY; //RGB is upside down
	else ofsX = ofsX & 0xFFFFFFFE; //YUV must be aligned on even pixels

	xdest=(ofsX < 0)? 0: ofsX;
	ydest=(ofsY < 0)? 0: ofsY;

	xsrc=(ofsX < 0)? (0-ofsX): 0;
	ysrc=(ofsY < 0)? (0-ofsY): 0;

	xcount = (vi.width < (ofsX + vi2.width))? (vi.width-xdest) : (vi2.width - xsrc);
	ycount = (vi.height <  (ofsY + vi2.height))? (vi.height-ydest) : (vi2.height - ysrc);

  overlay_frames = vi2.num_frames;

	for (int A=0; A<256; A++)
		map1[A] = levelA * A;	

	for (int B=0; B<256; B++)
		map2[B] = levelB * B;	
}

PVideoFrame __stdcall Layer::GetFrame(int n, IScriptEnvironment* env) 
{
  PVideoFrame src1 = child1->GetFrame(n, env);
  
  if (xcount<=0 || ycount<=0) return src1;
  
	PVideoFrame src2 = child2->GetFrame(min(n,overlay_frames-1), env);	

	env->MakeWritable(&src1);

	const int src1_pitch = src1->GetPitch();
	const int src2_pitch = src2->GetPitch();
	const int src2_row_size = src2->GetRowSize();
	const int row_size = src1->GetRowSize();
	const int mylevel = levelB;
	const int myy = ycount;

		__int64 oxooffooffooffooff=0x00ff00ff00ff00ff;  // Luma mask
		__int64 oxffooffooffooffoo=0xff00ff00ff00ff00;  // Chroma mask
		__int64 oxoo80oo80oo80oo80=0x0080008000800080;  // Null Chroma
		__int64 ox7f7f7f7f7f7f7f7f=0x7f7f7f7f7f7f7f7f;  // FAST shift mask
		__int64	 ox0101010101010101=0x0101010101010101;// FAST lsb mask

	if(vi.IsYUV()){

		BYTE* src1p = src1->GetWritePtr();
		const BYTE* src2p = src2->GetReadPtr();
		src1p += (src1_pitch * ydest) + (xdest * 2);
		src2p += (src2_pitch * ysrc) + (xsrc * 2);
		const int myx = xcount >> 1;

		  __asm {
				movq mm3, oxffooffooffooffoo     ; Chroma mask
				movq mm2, oxooffooffooffooff     ; Luma mask
				movd		mm1, mylevel			;alpha
				punpcklwd		mm1,mm1	  ;mm1= 0000|0000|00aa*|00aa*
				punpckldq		mm1, mm1	;mm1= 00aa*|00aa*|00aa*|00aa*
				pxor		mm0,mm0
		}
		if (!lstrcmpi(Op, "Add"))
		{
			if (chroma)
			{
				__asm {
				mov			edi, src1p
				mov			esi, src2p
				mov			ebx, myy
				
				addyuy32loop:
						mov         edx, myx
						xor         ecx, ecx

						addyuy32xloop:
							//---- fetch src1/dest
									
							movd		mm7, [edi + ecx*4] ;src1/dest;
							movd		mm6, [esi + ecx*4] ;src2
							movq		mm4,mm7					;temp mm4=mm7
							pand		mm7,mm2					;mask for luma
							pand		mm4,mm3					;mask for chroma
							movq		mm5,mm6					;temp mm5=mm6
							psrlw		mm4,8							;line up chroma
							pand		mm6,mm2					;mask for luma
							pand		mm5,mm3					;mask for chroma
							psrlw		mm5,8							;line'em up

							//----- begin the fun (luma) stuff
							psubsw	mm6, mm7
							pmullw	mm6, mm1 	  	;mm6=scaled difference*255
							psrlw		mm6, 8		    ;scale result
							paddb		mm6, mm7		  ;add src1
							//----- end the fun stuff...

							//----- begin the fun (chroma) stuff
							psubsw	mm5, mm4
							pmullw	mm5, mm1 	  	;mm5=scaled difference*255
							psrlw		mm5, 8		    ;scale result
							paddb		mm5, mm4		  ;add src1
							//----- end the fun stuff...

							psllw		mm5,8				;line up chroma
							por			mm6,mm5		;and merge'em back	

							movd        [edi + ecx*4],mm6

							inc         ecx
							cmp         ecx, edx
						jnz         addyuy32xloop

						add			edi, src1_pitch
						add			esi, src2_pitch
				dec		ebx
				jnz		addyuy32loop
				emms
				}
			} else {
				__asm {
				mov			edi, src1p
				mov			esi, src2p
				mov			ebx, myy
						
				addy032loop:
						mov         edx, myx
						xor         ecx, ecx

						addy032xloop:
							//---- fetch src1/dest
									
							movd		mm7, [edi + ecx*4] ;src1/dest;
							movd		mm6, [esi + ecx*4] ;src2
							movq		mm4,mm7					;temp mm4=mm7
							pand		mm7,mm2					;mask for luma
							pand		mm4,mm3					;mask for chroma
							movq		mm5,mm6					;temp mm5=mm6
							psrlw		mm4,8							;line up chroma
							pand		mm6,mm2					;mask for luma
							movq		mm5,oxoo80oo80oo80oo80	 					;get null chroma

							//----- begin the fun (luma) stuff
							psubsw	mm6, mm7
							pmullw	mm6, mm1 	  	;mm6=scaled difference*255
							psrlw		mm6, 8		    ;scale result
							paddb		mm6, mm7		  ;add src1
							//----- end the fun stuff...

							//----- begin the fun (chroma) stuff
							psubsw	mm5, mm4
							pmullw	mm5, mm1 	  	;mm5=scaled difference*255
							psrlw		mm5, 8		    ;scale result
							paddb		mm5, mm4		  ;add src1
							//----- end the fun stuff...

							psllw		mm5,8				;line up chroma
							por			mm6,mm5		;and merge'em back	

							movd        [edi + ecx*4],mm6

							inc         ecx
							cmp         ecx, edx
						jnz         addy032xloop

						add			edi, src1_pitch
						add			esi, src2_pitch
				dec		ebx
				jnz		addy032loop
				emms
				}
			}
		}
		if (!lstrcmpi(Op, "Fast"))
		{
			if (chroma)
			{
				__asm {
				mov			edi, src1p
				mov			esi, src2p
				mov			ebx, myy
				movq			mm0, ox7f7f7f7f7f7f7f7f	;get shift mask
				movq			mm1, ox0101010101010101 ;lsb mask
				
				fastyuy32loop:
						mov         edx, myx
						xor         ecx, ecx
						shr			edx,1

						fastyuy32xloop:
							//---- fetch src1/dest
									
							movq		mm7, [edi + ecx*8] ;src1/dest;
							movq		mm6, [esi + ecx*8] ;src2
							movq		mm3, mm1
							pand		mm3, mm7
							psrlq		mm6,1
							psrlq		mm7,1
							pand		mm6,mm0
							pand		mm7,mm0

						//----- begin the fun stuff
								
							paddb		mm6, mm7		  ;fast src1
							paddb		mm6, mm3		  ;fast lsb
						//----- end the fun stuff...

							movq        [edi + ecx*8],mm6

							inc         ecx
							cmp         ecx, edx
						jnz         fastyuy32xloop

						add			edi, src1_pitch
						add			esi, src2_pitch
				dec		ebx
				jnz		fastyuy32loop
				emms
				}
			} else {
			      env->ThrowError("Layer: this mode not allowed in FAST; use ADD instead");
			}
		}
		if (!lstrcmpi(Op, "Subtract"))
		{
			if (chroma)
			{
				__asm {
				mov			edi, src1p
				mov			esi, src2p
				mov			ebx, myy
				
				subyuy32loop:
						mov         edx, myx
						xor         ecx, ecx

						subyuy32xloop:
							//---- fetch src1/dest
									
							movd		mm7, [edi + ecx*4] ;src1/dest;
							movd		mm6, [esi + ecx*4] ;src2
							movq		mm4,mm7					;temp mm4=mm7
							pand		mm7,mm2					;mask for luma
							pand		mm4,mm3					;mask for chroma
							pcmpeqb	mm5, mm5					;mm5 will be sacrificed 
							psrlw		mm4,8							;line up chroma
							psubb		mm5, mm6					;mm5 = 255-mm6
							movq		mm6,mm5					;temp mm6=mm5
							pand		mm6,mm2					;mask for luma
							pand		mm5,mm3					;mask for chroma
							psrlw		mm5,8							;line'em up

							//----- begin the fun (luma) stuff
							psubsw	mm6, mm7
							pmullw	mm6, mm1 	  	;mm6=scaled difference*255
							psrlw		mm6, 8		    ;scale result
							paddb		mm6, mm7		  ;add src1
							//----- end the fun stuff...

							//----- begin the fun (chroma) stuff
							psubsw	mm5, mm4
							pmullw	mm5, mm1 	  	;mm5=scaled difference*255
							psrlw		mm5, 8		    ;scale result
							paddb		mm5, mm4		  ;add src1
							//----- end the fun stuff...

							psllw		mm5,8				;line up chroma
							por			mm6,mm5		;and merge'em back	

							movd        [edi + ecx*4],mm6

							inc         ecx
							cmp         ecx, edx
						jnz        subyuy32xloop

						add			edi, src1_pitch
						add			esi, src2_pitch
				dec		ebx
				jnz		subyuy32loop
				emms
				}
			} else {
				__asm {
				mov			edi, src1p
				mov			esi, src2p
				mov			ebx, myy
						
				suby032loop:
						mov         edx, myx
						xor         ecx, ecx

						suby032xloop:
							//---- fetch src1/dest
									
							movd		mm7, [edi + ecx*4] ;src1/dest;
							movd		mm6, [esi + ecx*4] ;src2
							movq		mm4,mm7					;temp mm4=mm7
							pand		mm7,mm2					;mask for luma
							pand		mm4,mm3					;mask for chroma
							pcmpeqb	mm5, mm5					;mm5 will be sacrificed 
							psrlw		mm4,8							;line up chroma
							psubb		mm5, mm6					;mm5 = 255-mm6
							movq		mm6,mm5					;temp mm6=mm5
							pand		mm6,mm2					;mask for luma
							movq		mm5,oxoo80oo80oo80oo80	 					;get null chroma

							//----- begin the fun (luma) stuff
							psubsw	mm6, mm7
							pmullw	mm6, mm1 	  	;mm6=scaled difference*255
							psrlw		mm6, 8		    ;scale result
							paddb		mm6, mm7		  ;add src1
							//----- end the fun stuff...

							//----- begin the fun (chroma) stuff
							psubsw	mm5, mm4
							pmullw	mm5, mm1 	  	;mm5=scaled difference*255
							psrlw		mm5, 8		    ;scale result
							paddb		mm5, mm4		  ;add src1
							//----- end the fun stuff...

							psllw		mm5,8				;line up chroma
							por			mm6,mm5		;and merge'em back	

							movd        [edi + ecx*4],mm6

							inc         ecx
							cmp         ecx, edx
						jnz         suby032xloop

						add			edi, src1_pitch
						add			esi, src2_pitch
				dec		ebx
				jnz		suby032loop
				emms
				}
			}
		}
		if (!lstrcmpi(Op, "Window"))
			{
			for (int y=0; y<myy; ++y) {
				for (int x=0; x<myx; ++x) {
					if (IsClose(src1p[x], src2p[x], T)) 
					{
					src1p[x]=(map1[src1p[x]] + map2[src2p[x]]) >> 8;
					if (chroma) 
						{
						if (IsClose(src1p[x+1], src2p[x+1], T))  src1p[x+1]=(map1[src1p[x+1]] + map2[src2p[x+1]]) >> 8;
						}
					else 
						{
						src1p[x+1]=(map1[src1p[x+1]] + map2[128]) >> 8;
						}
					} 
				}
			  src1p += src1_pitch;
			  src2p += src2_pitch;
			}
		}
		if (!lstrcmpi(Op, "Lighten"))
		{
			for (int y=0; y<myy; ++y) {
				for (int x=0; x<myx; ++x) {
					int _temp1 = (map2[src2p[x]] + map1[src1p[x]]) >>8;
					if (  _temp1 > (T + src1p[x])) 
					{
						src1p[x]= _temp1;
						if (chroma) src1p[x + 1]=(map1[src1p[x + 1]] + map2[src2p[x+1]]) >> 8;
					} 
				}
			  src1p += src1_pitch;
			  src2p += src2_pitch;
			}
		}
		if (!lstrcmpi(Op, "Darken"))
		{
			for (int y=0; y<myy; ++y) {
				for (int x=0; x<myx; ++x) {
					int _temp1 = (map2[src2p[x]] + map1[src1p[x]]) >>8;
					if (  _temp1 < (src1p[x]-T)) 
					{
						src1p[x]= _temp1;
						if (chroma) src1p[x + 1]=(map1[src1p[x + 1]] + map2[src2p[x+1]]) >> 8;
					} 
				}
			  src1p += src1_pitch;
			  src2p += src2_pitch;
			}
		}
	}
	else if (vi.IsRGB32())
	{
		const int cyb = int(0.114*32768+0.5);
		const int cyg = int(0.587*32768+0.5);
		const int cyr = int(0.299*32768+0.5);
		const unsigned __int64 rgb2lum = ((__int64)cyb << 32) | (cyg << 16) | cyr;

		BYTE* src1p = src1->GetWritePtr();
		const BYTE* src2p = src2->GetReadPtr();
		const int myx = xcount;

		src1p += (src1_pitch * ydest) + (xdest * 4);
		src2p += (src2_pitch * ysrc) + (xsrc * 4);

		if (!lstrcmpi(Op, "Add"))
		{
			if (chroma)
			{
				__asm {
				mov			edi, src1p
				mov			esi, src2p
				mov			ebx, myy
				movd		mm1, mylevel			;alpha
				pxor		mm0,mm0
				
				add32loop:
						mov         edx, myx
						xor         ecx, ecx

						add32xloop:
								movd		mm6, [esi + ecx*4] ;src2
								movq		mm2,mm6
								movd		mm7, [edi + ecx*4] ;src1/dest		;what a mess...

						//----- extract alpha into four channels

								psrlq		mm2,24		    ;mm2= 0000|0000|0000|00aa
								pmullw	mm2,mm1		    ;mm2= pixel alpha * script alpha
								
								psrlw		mm2,8		      ;mm2= 0000|0000|0000|00aa*
								punpcklwd		mm2,mm2	  ;mm2= 0000|0000|00aa*|00aa*
								punpckldq		mm2, mm2	;mm2= 00aa*|00aa*|00aa*|00aa*

						//----- alpha mask now in all four channels of mm2

								punpcklbw		mm6,mm0		;mm6= 00aa|00bb|00gg|00rr [src2]
								punpcklbw		mm7, mm0	;mm7= 00aa|00bb|00gg|00rr [src1]

						//----- begin the fun stuff
								
								psubsw	mm6, mm7
								pmullw	mm6, mm2 	  	;mm6=scaled difference*255
								psrlw		mm6, 8		    ;scale result
								paddb		mm6, mm7		  ;add src1

						//----- end the fun stuff...

								packuswb			mm6,mm0
								movd        [edi + ecx*4],mm6

								inc         ecx
								cmp         ecx, edx
						jnz         add32xloop

						add			edi, src1_pitch
						add			esi, src2_pitch
				dec		ebx
				jnz		add32loop
				emms
				}
			} else {
				__asm {
				mov			edi, src1p
				mov			esi, src2p
				mov			ebx, myy
				movd		mm1, mylevel
				pxor		mm0,mm0
						
				add32yloop:
						mov         edx, myx
						xor         ecx, ecx

						add32yxloop:
								movd		mm6, [esi + ecx*4] ;src2
								movq		mm2,mm6
								movd		mm7, [edi + ecx*4] ;src1/dest		;what a mess...

						//----- extract alpha into four channels

								psrlq		mm2,24		      ;mm2= 0000|0000|0000|00aa
								pmullw		mm2,mm1		    ;mm2= pixel alpha * script alpha

								psrlw		mm2,8		        ;mm2= 0000|0000|0000|00aa*
								punpcklwd		mm2,mm2		  ;mm2= 0000|0000|00aa*|00aa*
								punpckldq		mm2, mm2		;mm2= 00aa*|00aa*|00aa*|00aa*

								movq			mm3, rgb2lum	;another spaced out load

						//----- alpha mask now in all four channels of mm3

								punpcklbw		mm6,mm0		  ;mm6= 00aa|00bb|00gg|00rr [src2]
								punpcklbw		mm7,mm0		  ;mm7= 00aa|00bb|00gg|00rr [src1]

						//----- begin the fun stuff

						//----- start rgb -> monochrome
								pmaddwd			mm6,mm3			;partial monochrome result
								punpckldq		mm3,mm6			;ready to add
								paddd			mm6, mm3		  ;32 bit result
								psrlq			mm6, 47				;8 bit result
								punpcklwd		mm6, mm6		;propagate words
								punpckldq		mm6, mm6
						//----- end rgb -> monochrome

								psubsw		mm6, mm7
								pmullw		mm6,mm2		;mm6=scaled difference*255
								psrlw		  mm6,8		  ;scale result
								paddb		  mm6,mm7		;add src1

						//----- end the fun stuff...

								packuswb		mm6,mm0
								movd        [edi + ecx*4],mm6

								inc         ecx
								cmp         ecx, edx
						jnz         add32yxloop

						add				edi, src1_pitch
						add				esi, src2_pitch
				dec		ebx
				jnz		add32yloop
				emms
				}
			}
		}
		if (!lstrcmpi(Op, "Fast"))
		{
			if (chroma)
			{
				__asm {
				mov			edi, src1p
				mov			esi, src2p
				mov			ebx, myy
				movq			mm0, ox7f7f7f7f7f7f7f7f	;get shift mask
				movq			mm1, ox0101010101010101 ;lsb mask

				
				fastrgb32loop:
						mov         edx, myx
						xor         ecx, ecx
						shr			edx,1

						fastrgb32xloop:
							//---- fetch src1/dest
									
							movq		mm7, [edi + ecx*8] ;src1/dest;
							movq		mm6, [esi + ecx*8] ;src2
							movq		mm3, mm1
							pand		mm3, mm7
							psrlq		mm6,1
							psrlq		mm7,1
							pand		mm6,mm0
							pand		mm7,mm0

						//----- begin the fun stuff
								
							paddb		mm6, mm7		  ;fast src1
							paddb		mm6, mm3		  ;fast lsb
						//----- end the fun stuff...

							movq        [edi + ecx*8],mm6

							inc         ecx
							cmp         ecx, edx
						jnz         fastrgb32xloop

						add			edi, src1_pitch
						add			esi, src2_pitch
				dec		ebx
				jnz		fastrgb32loop
				emms
				}
			} else {
			      env->ThrowError("Layer: this mode not allowed in FAST; use ADD instead");
			}
		}
		if (!lstrcmpi(Op, "Subtract"))
		{
			if (chroma)
			{
				__asm {
				mov			edi, src1p
				mov			esi, src2p
				mov			ebx, myy
				movd		mm1, mylevel
				pxor		mm0, mm0
				pcmpeqb		mm4, mm4
				punpcklbw	mm4, mm0		;0x00ff00ff00ff00ff
						
				sub32loop:
						mov         edx, myx
						xor         ecx, ecx

						sub32xloop:
								movd	  mm6, [esi + ecx*4] ;src2	
								movq		mm2,mm6
								movd		mm7, [edi + ecx*4] ;src1/dest		;what a mess...

						//----- extract alpha into four channels

								psrlq		mm2,24		  ;mm2= 0000|0000|0000|00aa
								pmullw		mm2,mm1		;mm2= pixel alpha * script alpha
								
								psrlw		mm2,8		        ;mm2= 0000|0000|0000|00aa*
								punpcklwd		mm2,mm2		  ;mm2= 0000|0000|00aa*|00aa*
								punpckldq		mm2, mm2		;mm2=00aa*|00aa*|00aa*|00aa*

						//----- alpha mask now in all four channels of mm2

								punpcklbw		mm6,mm0		;mm6= 00aa|00bb|00gg|00rr [src2]
								pandn				mm6, mm4	;mm6 = mm6*
								punpcklbw		mm7,mm0		;mm7= 00aa|00bb|00gg|00rr [src1]

						//----- begin the fun stuff
								
								psubsw	mm6, mm7
								pmullw	mm6,mm2		;mm6=scaled difference*255
								psrlw		mm6,8		  ;scale result
								paddb		mm6,mm7		;add src1

						//----- end the fun stuff...

								packuswb			mm6,mm0
								movd        [edi + ecx*4],mm6

								inc         ecx
								cmp         ecx, edx
						jnz         sub32xloop

						add				edi, src1_pitch
						add				esi, src2_pitch
				dec		ebx
				jnz		sub32loop
				emms
				}
			} else {
				__asm {
				mov			edi, src1p
				mov			esi, src2p
				mov			eax, src2p
				mov			ebx, myy
				movd		mm1, mylevel
				pxor		mm0, mm0
				pcmpeqb		mm4, mm4
				punpcklbw	mm4, mm0		;0x00ff00ff00ff00ff
						
				sub32yloop:
						mov         edx, myx
						xor         ecx, ecx

						sub32yxloop:
								movd		mm6, [esi + ecx*4] ;src2
								movq		mm2,mm6
								movd		mm7, [edi + ecx*4] ;src1/dest		;what a mess...
								
						//----- extract alpha into four channels

								psrlq		mm2,24		;mm2= 0000|0000|0000|00aa
								pmullw	mm2,mm1		;mm2= pixel alpha * script alpha

								psrlw		mm2,8		        ;mm2= 0000|0000|0000|00aa*
								punpcklwd		mm2,mm2		  ;mm2= 0000|0000|00aa*|00aa*
								punpckldq		mm2, mm2		;mm2=00aa*|00aa*|00aa*|00aa*

								movq			mm3, rgb2lum		;another spaced out load

						//----- alpha mask now in all four channels of mm3

								punpcklbw		mm6,mm0		;mm6= 00aa|00bb|00gg|00rr [src2]
								pandn				mm6, mm4	;mm6 = mm6*
								punpcklbw		mm7,mm0		;mm7= 00aa|00bb|00gg|00rr [src1]

						//----- begin the fun stuff

						//----- start rgb -> monochrome
								pmaddwd			mm6,mm3			;partial monochrome result
								punpckldq		mm3,mm6			;ready to add
								paddd			mm6, mm3		  ;32 bit result
								psrlq			mm6, 47				;8 bit result
								punpcklwd		mm6, mm6		;propagate words
								punpckldq		mm6, mm6
						//----- end rgb -> monochrome

								psubsw		mm6, mm7
								pmullw		mm6,mm2		;mm6=scaled difference*255
								psrlw		  mm6,8		  ;scale result
								paddb		  mm6,mm7		;add src1

								packuswb		mm6,mm0
								movd        [edi + ecx*4],mm6
						//----- end the fun stuff...

								inc         ecx
								cmp         ecx, edx
						jnz         sub32yxloop

						add				edi, src1_pitch
						add				esi, src2_pitch
				dec		ebx
				jnz		sub32yloop
				emms
				}
			}
		}
	}
	return src1;
}

  

AVSValue __cdecl Layer::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  return new Layer( args[0].AsClip(), args[1].AsClip(), args[2].AsString("Add"), args[3].AsInt(255), 
                    args[4].AsInt(0), args[5].AsInt(0), args[6].AsInt(0), args[7].AsBool(true), env );  
}








/**********************************
 *******   Subtract Filter   ******
 *********************************/

Subtract::Subtract(PClip _child1, PClip _child2, IScriptEnvironment* env)
  : child1(_child1), child2(_child2)
{
  const VideoInfo& vi1 = child1->GetVideoInfo();
  const VideoInfo& vi2 = child2->GetVideoInfo();
  if (vi1.width != vi2.width || vi1.height != vi2.height)
    env->ThrowError("Subtract: image dimensions don't match");
  if (vi1.pixel_type != vi2.pixel_type)
    env->ThrowError("Subtract: image formats don't match");

  vi = vi1;
  vi.num_frames = max(vi1.num_frames, vi2.num_frames);
  vi.num_audio_samples = max(vi1.num_audio_samples, vi2.num_audio_samples);
}


PVideoFrame __stdcall Subtract::GetFrame(int n, IScriptEnvironment* env) 
{
  PVideoFrame src1 = child1->GetFrame(n, env);
  PVideoFrame src2 = child2->GetFrame(n, env);
  PVideoFrame dst=0;
  if (!src1->IsWritable())
    dst = env->NewVideoFrame(vi);
  const BYTE* src1p = src1->GetReadPtr();
  const BYTE* src2p = src2->GetReadPtr();
  BYTE* dstp = (dst ? dst : src1)->GetWritePtr();
  const int dst_pitch = (dst ? dst : src1)->GetPitch();

  const int row_size = src1->GetRowSize();

  for (int y=0; y<vi.height; ++y) 
  {
    // For YUY2, 50% gray is about (126,128,128) instead of (128,128,128).  Grr...
    if (vi.IsYUY2()) {
      for (int x=0; x<row_size; x+=2) {
        dstp[x] = src1p[x] - src2p[x] + 126;
        dstp[x+1] = src1p[x+1] - src2p[x+1] + 128;
      }
    } else {
      for (int x=0; x<row_size; ++x)
        dstp[x] = src1p[x] - src2p[x] + 128;
    }
    dstp += dst_pitch;
    src1p += src1->GetPitch();
    src2p += src2->GetPitch();
  }

  return (dst ? dst : src1);
}



AVSValue __cdecl Subtract::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  return new Subtract(args[0].AsClip(), args[1].AsClip(), env);
}
