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
  { 0 }
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
  if (!vi1.IsRGB32() | !vi2.IsRGB())
    env->ThrowError("Mask error: sources must be RGB and dest must be RGB32");

  vi = vi1;
  vi.num_frames = max(vi1.num_frames, vi2.num_frames);
  vi.num_audio_samples = max(vi1.num_audio_samples, vi2.num_audio_samples);
}


PVideoFrame __stdcall Mask::GetFrame(int n, IScriptEnvironment* env) 
{
  PVideoFrame src1 = child1->GetFrame(n, env);
  PVideoFrame src2 = child2->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrame(vi);

	const unsigned char* src1p = src1->GetReadPtr();
  const unsigned char* src2p = src2->GetReadPtr();
  unsigned char* dstp = dst->GetWritePtr();

  const int src1_pitch = src1->GetPitch();
  const int src2_pitch = src2->GetPitch();
  const int dst_pitch = dst->GetPitch();

  const int src2_row_size = src2->GetRowSize();
	const int src2_pixels = src2_row_size / vi.width;
  const int row_size = src1->GetRowSize();

  BitBlt(dstp, dst_pitch, src1p, src1_pitch, row_size, vi.height);   

	const int cyb = int(0.114*219/255*65536+0.5);
	const int cyg = int(0.587*219/255*65536+0.5);
	const int cyr = int(0.299*219/255*65536+0.5);

  for (int y=0; y<vi.height; ++y) 
  {
	  for (int x=0; x<vi.width; ++x)
		dstp[x*4+3] = ( cyb*src2p[x*src2_pixels] + cyg*src2p[x*src2_pixels+1] +
                    cyr*src2p[x*src2_pixels+2] + 0x108000 ) >> 16; 
    dstp += dst_pitch;
    src1p += src1->GetPitch();
    src2p += src2->GetPitch();
  }

  return dst;
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
    
  if (!vi1.IsRGB32() | !vi2.IsRGB32())
    env->ThrowError("Layer sources must be RGB32");

  vi = vi1;

	ofsY = vi.height-(vi2.height-ofsY); //RGB is upside down

	xdest=(ofsX < 0)? 0: ofsX;
	ydest=(ofsY < 0)? 0: ofsY;

	xsrc=(ofsX < 0)? (0-ofsX): 0;
	ysrc=(ofsY < 0)? (0-ofsY): 0;

	xcount = (vi.width < (ofsX + vi2.width))? (vi.width-xdest) : (vi2.width - xsrc);
	ycount = (vi.height <  (ofsY + vi2.height))? (vi.height-ydest) : (vi2.height - ysrc);

  vi.num_frames = max(vi1.num_frames, vi2.num_frames);
  vi.num_audio_samples = max(vi1.num_audio_samples, vi2.num_audio_samples);
}


PVideoFrame __stdcall Layer::GetFrame(int n, IScriptEnvironment* env) 
{
  PVideoFrame src1 = child1->GetFrame(n, env);
  PVideoFrame src2 = child2->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrame(vi);

  const unsigned char* src1p = src1->GetReadPtr();
  const unsigned char* src2p = src2->GetReadPtr();
  unsigned char* dstp = dst->GetWritePtr();
  const int src1_pitch = src1->GetPitch();
  const int src2_pitch = src2->GetPitch();
  const int dst_pitch = dst->GetPitch();
  const int src2_row_size = src2->GetRowSize();
  const int row_size = src1->GetRowSize();

  BitBlt(dstp, dst_pitch, src1p, src1_pitch, row_size, vi.height);

	for (int A=0; A<256; A++){
		map1[A] = (levelA * A) >> 8;
	}

	for (int B=0; B<256; B++){
		map2[B] = levelB * B;
	}

	const int cyb = int(0.114*219/255*32768+0.5);
	const int cyg = int(0.587*219/255*32768+0.5);
	const int cyr = int(0.299*219/255*32768+0.5);
	const unsigned int rgb2lum = 0 | ((cyb << 32) | (cyg << 16) | cyr);

	src1p = src1->GetReadPtr() + (src1->GetPitch() * ydest) + (xdest * 4);
	src2p = src2->GetReadPtr() + (src2->GetPitch() * ysrc) + (xsrc * 4);
	dstp = dst->GetWritePtr() + (dst->GetPitch() * ydest) + (xdest * 4);

	const int dstpitch = dst_pitch;
	const int src1pitch = src1->GetPitch();
	const int src2pitch = src2->GetPitch();
	const int myx=xcount-1;
	int myy=ycount;
	const int mylevel = levelB;

	if (!lstrcmpi(Op, "Add"))
	{
		if (chroma)
		{
			__asm {
			mov         edi, dstp
			mov			esi, src1p
			mov			eax, src2p
			mov			ebx, myy
			movd		mm1, mylevel			;alpha
			pxor		mm0,mm0
			
			add32loop:
					mov         edx, myx
					xor         ecx, ecx

					add32xloop:
							movd		mm6, [eax + ecx*4] ;src2
							movq		mm2,mm6

					//----- extract alpha into four channels

							psrlq		mm2,24		;mm2= 0000|0000|0000|00aa
							pmullw		mm2,mm1		;mm2= pixel alpha * script alpha

							movd		mm7, [esi + ecx*4] ;src1/dest		;what a mess...
							
							psrlw		mm2,8		;mm2= 0000|0000|0000|00aa*
							punpcklwd		mm2,mm2		;mm2= 0000|0000|00aa*|00aa*
							punpckldq		mm2, mm2		;mm2=00aa*|00aa*|00aa*|00aa*

					//----- alpha mask now in all four channels of mm2

							punpcklbw		mm7,mm0		;mm7= 00aa|00bb|00gg|00rr [src1]
							punpcklbw		mm6,mm0		;mm6= 00aa|00bb|00gg|00rr [src2]

					//----- begin the fun stuff
							
							psubsw		mm6, mm7
							pmullw		mm6,mm2		;mm6=scaled difference*255
							psrlw		mm6,8		;scale result
							paddb		mm6,mm7		;add src1

					//----- end the fun stuff...

							packuswb			mm6,mm0
							movd        [edi + ecx*4],mm6

							inc         ecx
							cmp         ecx, edx
					jnz         add32xloop

					add				esi, src1pitch
					add				eax, src2pitch
					add				edi, dstpitch
			dec		ebx
			jnz		add32loop
			emms
			}
		} else {
			__asm {
			mov         edi, dstp
			mov			esi, src1p
			mov			eax, src2p
			mov			ebx, myy
			movd		mm1, mylevel
			pxor		mm0,mm0
					
			add32yloop:
					mov         edx, myx
					xor         ecx, ecx

					add32yxloop:
							movd		mm6, [eax + ecx*4] ;src2
							movq		mm2,mm6

					//----- extract alpha into four channels

							psrlq		mm2,24		;mm2= 0000|0000|0000|00aa
							pmullw		mm2,mm1		;mm2= pixel alpha * script alpha

							movd		mm7, [esi + ecx*4] ;src1/dest		;what a mess...
							
							psrlw		mm2,8		;mm2= 0000|0000|0000|00aa*
							punpcklwd		mm2,mm2		;mm2= 0000|0000|00aa*|00aa*
							punpckldq		mm2, mm2		;mm2=00aa*|00aa*|00aa*|00aa*

							movd			mm3, rgb2lum		;another spaced out load

					//----- alpha mask now in all four channels of mm3

							punpcklbw		mm7,mm0		;mm7= 00aa|00bb|00gg|00rr [src1]
							punpcklbw		mm6,mm0		;mm6= 00aa|00bb|00gg|00rr [src2]

					//----- begin the fun stuff

					//----- start rgb -> monochrome
							pmaddwd			mm6,mm3			;partial monochrome result
							punpckhdq		mm3,mm6					;ready to add
							paddd			mm6, mm3		;32 bit result
							psrld			mm6, 15				;8 bit result
							punpcklwd		mm6, mm6		;propogate words
							punpckldq		mm6, mm6
					//----- end rgb -> monochrome

							psubsw		mm6, mm7
							pmullw		mm6,mm2		;mm6=scaled difference*255
							psrlw		mm6,8		;scale result
							paddb		mm6,mm7		;add src1

					//----- end the fun stuff...

							packuswb			mm6,mm0
							movd        [edi + ecx*4],mm6

							inc         ecx
							cmp         ecx, edx
					jnz         add32yxloop

					add				esi, src1pitch
					add				eax, src2pitch
					add				edi, dstpitch
			dec		ebx
			jnz		add32yloop
			emms
			}
		}
	}
	if (!lstrcmpi(Op, "Subtract"))
	{
		if (chroma)
		{
			__asm {
			mov         edi, dstp
			mov			esi, src1p
			mov			eax, src2p
			mov			ebx, myy
			movd		mm1, mylevel
			pxor		mm0,mm0
			pcmpeqb		mm4, mm4
			punpcklbw	mm4, mm0		;0x00ff00ff00ff00ff
					
			sub32loop:
					mov         edx, myx
					xor         ecx, ecx

					sub32xloop:
							movd	mm6, [eax + ecx*4] ;src2	
							movq		mm2,mm6

					//----- extract alpha into four channels

							psrlq		mm2,24		;mm2= 0000|0000|0000|00aa
							pmullw		mm2,mm1		;mm2= pixel alpha * script alpha

							movd		mm7, [esi + ecx*4] ;src1/dest		;what a mess...
							
							psrlw		mm2,8		;mm2= 0000|0000|0000|00aa*
							punpcklwd		mm2,mm2		;mm2= 0000|0000|00aa*|00aa*
							punpckldq		mm2, mm2		;mm2=00aa*|00aa*|00aa*|00aa*

					//----- alpha mask now in all four channels of mm2

							punpcklbw		mm7,mm0		;mm7= 00aa|00bb|00gg|00rr [src1]
							punpcklbw		mm6,mm0		;mm6= 00aa|00bb|00gg|00rr [src2]
							pandn				mm6, mm4	;mm6 = mm6*

					//----- begin the fun stuff
							
							psubsw		mm6, mm7
							pmullw		mm6,mm2		;mm6=scaled difference*255
							psrlw		mm6,8		;scale result
							paddb		mm6,mm7		;add src1

					//----- end the fun stuff...

							packuswb			mm6,mm0
							movd        [edi + ecx*4],mm6

							inc         ecx
							cmp         ecx, edx
					jnz         sub32xloop

					add				esi, src1pitch
					add				eax, src2pitch
					add				edi, dstpitch
			dec		ebx
			jnz		sub32loop
			emms
			}
		} else {
			__asm {
			mov         edi, dstp
			mov			esi, src1p
			mov			eax, src2p
			mov			ebx, myy
			movd		mm1, mylevel
			pxor		mm0,mm0
			pcmpeqb		mm4, mm4
			punpcklbw	mm4, mm0		;0x00ff00ff00ff00ff
					
			sub32yloop:
					mov         edx, myx
					xor         ecx, ecx

					sub32yxloop:
							movd		mm6, [eax + ecx*4] ;src2
							movq		mm2,mm6

					//----- extract alpha into four channels

							psrlq		mm2,24		;mm2= 0000|0000|0000|00aa
							pmullw		mm2,mm1		;mm2= pixel alpha * script alpha

							movd		mm7, [esi + ecx*4] ;src1/dest		;what a mess...
							
							psrlw		mm2,8		;mm2= 0000|0000|0000|00aa*
							punpcklwd		mm2,mm2		;mm2= 0000|0000|00aa*|00aa*
							punpckldq		mm2, mm2		;mm2=00aa*|00aa*|00aa*|00aa*

							movd			mm3, rgb2lum		;another spaced out load

					//----- alpha mask now in all four channels of mm3

							punpcklbw		mm7,mm0		;mm7= 00aa|00bb|00gg|00rr [src1]
							punpcklbw		mm6,mm0		;mm6= 00aa|00bb|00gg|00rr [src2]
							pandn		mm6, mm4

					//----- begin the fun stuff

					//----- start rgb -> monochrome
							pmaddwd			mm6,mm3			;partial monochrome result
							punpckhdq		mm3,mm6					;ready to add
							paddd			mm6, mm3		;32 bit result
							psrld			mm6, 15				;8 bit result
							punpcklwd		mm6, mm6		;propogate words
							punpckldq		mm6, mm6
					//----- end rgb -> monochrome

							psubsw		mm6, mm7
							pmullw		mm6,mm2		;mm6=scaled difference*255
							psrlw		mm6,8		;scale result
							paddb		mm6,mm7		;add src1

					//----- end the fun stuff...

							packuswb			mm6,mm0
							movd        [edi + ecx*4],mm6

							inc         ecx
							cmp         ecx, edx
					jnz         sub32yxloop

					add				esi, src1pitch
					add				eax, src2pitch
					add				edi, dstpitch
			dec		ebx
			jnz		sub32yloop
			emms
			}
		}
	}
	if (!lstrcmpi(Op, "Lighten"))
	{
		for (int y=0; y<ycount; ++y) {
			for (int x=0; x<xcount; ++x) {
				int inx = (x + xdest) *4;
				int overx = (x + xsrc) *4;
				int lumin = int (cyb*src1p[inx] + cyg*src1p[inx+1] + cyr*src1p[inx+2] + 0x108000) >> 16; 
				int lumover = int (cyb*src2p[overx] + cyg*src2p[overx+1] + cyr*src2p[overx+2] + 0x108000) >> 16; 
				int _temp1 = (map2[lumover] + map1[lumin]) >>8;
				if (  _temp1 > lumin) 
				{
					if (chroma){
						dstp[inx] = (map1[src1p[inx]] + map2[src2p[overx]])>>8;	inx++; overx++;
						dstp[inx] = (map1[src1p[inx]] + map2[src2p[overx]])>>8;	inx++; overx++;
						dstp[inx] = (map1[src1p[inx]] + map2[src2p[overx]])>>8;
					} else {
						dstp[inx] = (map1[src1p[inx]] + lumover)>>8;	inx++;
						dstp[inx] = (map1[src1p[inx]] + lumover)>>8;	inx++;
						dstp[inx] = (map1[src1p[inx]] + lumover)>>8;
					}
				}
			}
		  dstp += dst_pitch;
		  src1p += src1->GetPitch();
		  src2p += src2->GetPitch();
		}
	}
	if (!lstrcmpi(Op, "Darken"))
	{
		for (int y=0; y<ycount; ++y) {
			for (int x=0; x<xcount; ++x) {
				int inx = (x + xdest) *4;
				int overx = (x + xsrc) *4;
				int lumin = int (cyb*src1p[inx] + cyg*src1p[inx+1] + cyr*src1p[inx+2] + 0x108000) >> 16; 
				int lumover = int (cyb*src2p[overx] + cyg*src2p[overx+1] + cyr*src2p[overx+2] + 0x108000) >> 16; 
				int _temp1 = (map2[lumover] + map1[lumin]) >>8;
				if ( _temp1 < lumin) 
				{
					if (chroma){
						dstp[inx] = (map1[src1p[inx]] + map2[src2p[overx]])>>8;	inx++; overx++;
						dstp[inx] = (map1[src1p[inx]] + map2[src2p[overx]])>>8;	inx++; overx++;
						dstp[inx] = (map1[src1p[inx]] + map2[src2p[overx]])>>8;
					} else {
						dstp[inx] = (map1[src1p[inx]] + map2[lumover])>>8;	inx++;
						dstp[inx] = (map1[src1p[inx]] + map2[lumover])>>8;	inx++;
						dstp[inx] = (map1[src1p[inx]] + map2[lumover])>>8;
					}
				}
			}
		  dstp += dst_pitch;
		  src1p += src1->GetPitch();
		  src2p += src2->GetPitch();
		}
	}
	return dst;
}

  

AVSValue __cdecl Layer::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  return new Layer( args[0].AsClip(), args[1].AsClip(), args[2].AsString("Add"), args[3].AsInt(), 
                    args[4].AsInt(), args[5].AsInt(), args[6].AsInt(0), args[7].AsBool(true), env );
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
  const unsigned char* src1p = src1->GetReadPtr();
  const unsigned char* src2p = src2->GetReadPtr();
  unsigned char* dstp = (dst ? dst : src1)->GetWritePtr();
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
