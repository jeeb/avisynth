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



// Avisynth filter: Layer 
// by "poptones" (poptones@myrealbox.com)

#include "stdafx.h"

#include "layer.h"



/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

AVSFunction Layer_filters[] = {
  { "Mask", "cc", Mask::Create },     // clip, mask
  { "ColorKeyMask", "cii", ColorKeyMask::Create },    // clip, color, tolerance
  { "ResetMask", "c", ResetMask::Create },
  { "Invert", "c[channels]s", Invert::Create },
  { "ShowAlpha", "c[pixel_type]s", ShowAlpha::Create },
  { "Layer", "cc[op]s[level]i[x]i[y]i[threshold]i[use_chroma]b", Layer::Create },
  /**
    * Layer(clip, overlayclip, operation, amount, xpos, ypos, [threshold=0], [use_chroma=true])
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

	__declspec(align(8)) static __int64 rgb2lum = ((__int64)cyb << 32) | (cyg << 16) | cyr;
	__declspec(align(8)) static __int64 alpha_mask=0x00ffffff00ffffff;
	__declspec(align(8)) static __int64 color_mask=0xff000000ff000000;
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









/**************************************
 *******   ColorKeyMask Filter   ******
 **************************************/


ColorKeyMask::ColorKeyMask(PClip _child, int _color, int _tolerance, IScriptEnvironment *env)
  : GenericVideoFilter(_child), color(_color & 0xffffff), tol(_tolerance)
{
  if (!vi.IsRGB32())
    env->ThrowError("ColorKeyMask: requires RGB32 input");
}

PVideoFrame __stdcall ColorKeyMask::GetFrame(int n, IScriptEnvironment *env)
{
  PVideoFrame frame = child->GetFrame(n, env);
  env->MakeWritable(&frame);

  BYTE* pf = frame->GetWritePtr();
  const int pitch = frame->GetPitch();
  const int rowsize = frame->GetRowSize();

  if (!(env->GetCPUFlags() & CPUF_MMX) || vi.width==1) {
    const int R = color >> 16;
    const int G = (color & 0xff00) >> 8;
    const int B = color & 0xff;

    for (int y=0; y<vi.height; y++) {
      for (int x=0; x<rowsize; x+=4) {
        if (IsClose(pf[x],B,tol) && IsClose(pf[x+1],B,tol) && IsClose(pf[x+2],R,tol))
          pf[x+3]=0;
      }
      pf += pitch;
    }
  } else { // MMX
    const int height = vi.height;
    const __int64 col8 = (__int64)color << 32 | color;
    const __int64 tol8 = ((__int64)tol * 0x0001010100010101i64) | 0xff000000ff000000i64;
    const int xloopcount = -(rowsize & -8);
    pf -= xloopcount;
    __asm {
      mov       esi, pf
      mov       edx, height
      pxor      mm0, mm0
      movq      mm1, col8
      movq      mm2, tol8

yloop:
      mov       ecx, xloopcount
xloop:
      movq      mm3, [esi+ecx]
      movq      mm4, mm1
      movq      mm5, mm3
      psubusb   mm4, mm3
      psubusb   mm5, mm1
      por       mm4, mm5
      psubusb   mm4, mm2
      add       ecx, 8
      pcmpeqd   mm4, mm0
      pslld     mm4, 24
      pandn     mm4, mm3
      movq      [esi+ecx-8], mm4
      jnz       xloop

      mov       ecx, rowsize
      and       ecx, 7
      jz        not_odd
      ; process last pixel
      movd      mm3, [esi]
      movq      mm4, mm1
      movq      mm5, mm3
      psubusb   mm4, mm3
      psubusb   mm5, mm1
      por       mm4, mm5
      psubusb   mm4, mm2
      pcmpeqd   mm4, mm0
      pslld     mm4, 24
      pandn     mm4, mm3
      movd      [esi], mm4

not_odd:
      add       esi, pitch
      dec       edx
      jnz       yloop
      emms
    }
  }

  return frame;
}

AVSValue __cdecl ColorKeyMask::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  return new ColorKeyMask(args[0].AsClip(), args[1].AsInt(0), args[2].AsInt(10), env);
}








/********************************
 ******  ResetMask filter  ******
 ********************************/


ResetMask::ResetMask(PClip _child, IScriptEnvironment* env)
  : GenericVideoFilter(_child)
{
  if (!vi.IsRGB32())
    env->ThrowError("ResetMask: RGB32 data only");
}


PVideoFrame ResetMask::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame f = child->GetFrame(n, env);
  env->MakeWritable(&f);

  BYTE* pf = f->GetWritePtr();
  int pitch = f->GetPitch();
  int rowsize = f->GetRowSize();
  int height = f->GetHeight();

  for (int i=0; i<height; i++) {
    for (int j=3; j<rowsize; j+=4)
      pf[j] = 255;
    pf += pitch;
  }

  return f;
}


AVSValue ResetMask::Create(AVSValue args, void*, IScriptEnvironment* env)
{
  return new ResetMask(args[0].AsClip(), env);
}




/********************************
 ******  Invert filter  ******
 ********************************/


Invert::Invert(PClip _child, const char * _channels, IScriptEnvironment* env)
  : GenericVideoFilter(_child), channels(_channels)
{

}


PVideoFrame Invert::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame f = child->GetFrame(n, env);
  env->MakeWritable(&f);

  BYTE* pf = f->GetWritePtr();
  int pitch = f->GetPitch();
  int rowsize = f->GetRowSize();
  int height = f->GetHeight();

  
  bool doB = false;
  bool doG = false;
  bool doR = false;
  bool doA = false;

  bool doY = false;
  bool doU = false;
  bool doV = false;
  char ch = 1;

  for (int k=0; ch!='\0'; ++k) {
    ch = tolower(channels[k]);
    if (ch == 'b')
      doB = true;
    if (ch == 'g')
      doG = true;
    if (ch == 'r')
      doR = true;
    if (ch == 'a')
      doA = true;

    if (ch == 'y')
      doY = true;
    if (ch == 'u')
      doU = true;
    if (ch == 'v')
      doV = true;
  }

  if (vi.IsYUY2()) {
    int mask = doY ? 0x00ff00ff : 0;
    mask |= doU ? 0x0000ff00 : 0;
    mask |= doV ? 0xFF000000 : 0;
    ConvertFrame(pf, pitch, rowsize, height, mask);
  }

  if (vi.IsRGB32()) {
    int mask = doB ? 0xff : 0;
    mask |= doG ? 0xff00 : 0;
    mask |= doR ? 0xff0000 : 0;
    mask |= doA ? 0xff000000 : 0;
    ConvertFrame(pf, pitch, rowsize, height, mask);
  }

  if (vi.IsYV12()) {
    if (doY)
      ConvertFrame(pf, pitch, f->GetRowSize(PLANAR_Y_ALIGNED), height, 0xffffffff);
    if (doU)
      ConvertFrame(f->GetWritePtr(PLANAR_U), f->GetPitch(PLANAR_U), f->GetRowSize(PLANAR_U_ALIGNED), f->GetHeight(PLANAR_U), 0xffffffff);
    if (doV)
      ConvertFrame(f->GetWritePtr(PLANAR_V), f->GetPitch(PLANAR_V), f->GetRowSize(PLANAR_V_ALIGNED), f->GetHeight(PLANAR_V), 0xffffffff);
  }

  if (vi.IsRGB24()) {
    for (int i=0; i<height; i++) {
      for (int j=0; j<rowsize; j+=3) {
        if (doB)
          pf[j] = 255 - pf[j];
        if (doG)
          pf[j+1] = 255 - pf[j+1];
        if (doR)
          pf[j+2] = 255 - pf[j+2];
      }
      pf += pitch;
    }
  }

  return f;
}

void Invert::ConvertFrame(BYTE* frame, int pitch, int rowsize, int height, int mask) {
  __asm {
      movd mm7,[mask]
      mov ebx,[rowsize]
      mov esi,[frame]
      xor ecx, ecx  // Height
      mov edx,[height]
      punpckldq mm7,mm7
      align 16
yloopback:
      cmp ecx, edx
      jge outy
      xor eax, eax
      align 16 
testloop:
      cmp ebx, eax
      jle outloop

      movq mm0,[esi+eax]  // 8 pixels
      pxor mm0, mm7
      movq [esi+eax], mm0

      add eax,8
      jmp testloop
      align 16
outloop:
      inc ecx
      add esi, [pitch];
      jmp yloopback
outy:
      emms
  } // end asm
}



AVSValue Invert::Create(AVSValue args, void*, IScriptEnvironment* env)
{
  return new Invert(args[0].AsClip(), args[0].AsClip()->GetVideoInfo().IsRGB() ? args[1].AsString("RGBA") : args[1].AsString("YUV"), env);
}




/********************************
 ******  ShowAlpha filter  ******
 ********************************/


ShowAlpha::ShowAlpha(PClip _child, const char * _pixel_type, IScriptEnvironment* env)
  : GenericVideoFilter(_child), pixel_type(_pixel_type)
{
  if (!vi.IsRGB32())
    env->ThrowError("ShowAlpha: RGB32 data only");

  if (!lstrcmpi(pixel_type, "rgb")) {
    vi.pixel_type = VideoInfo::CS_BGR32;
  } 
  else if (!lstrcmpi(pixel_type, "yuy2")) {
    if (vi.width & 1) {
      env->ThrowError("ShowAlpha: width must be mod 2 for yuy2");
    }
    vi.pixel_type = VideoInfo::CS_YUY2;
  }
  else if (!lstrcmpi(pixel_type, "yv12")) {
    if (vi.width & 1) {
      env->ThrowError("ShowAlpha: width must be mod 2 for yv12");
    }
    if (vi.height & 1) {
      env->ThrowError("ShowAlpha: height must be mod 2 for yv12");
    }
    vi.pixel_type = VideoInfo::CS_YV12;
  }

  else {
    env->ThrowError("ShowAlpha supports the following output pixel types: RGB, YUY2, or YV12");
  }
}


PVideoFrame ShowAlpha::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame f = child->GetFrame(n, env);

  const BYTE* pf = f->GetReadPtr();
  int height = f->GetHeight();
  int pitch = f->GetPitch();
  int rowsize = f->GetRowSize();
  
  if (!lstrcmpi(pixel_type, "rgb"))
  {
    // we can do it in-place
    env->MakeWritable(&f);
    BYTE* dstp = f->GetWritePtr();
    
    for (int i=0; i<height; ++i) {
      for (int j=0; j<rowsize; j+=4) {        
        dstp[j + 0] = dstp[j + 1] = dstp[j + 2] = dstp[j + 3];
      }
      dstp += pitch;
    }
  
    return f;
  }
  else if (!lstrcmpi(pixel_type, "yuy2"))
  {    
    PVideoFrame dst = env->NewVideoFrame(vi);
    BYTE * dstp = dst->GetWritePtr();
    int dstpitch = dst->GetPitch();
    int dstrowsize = dst->GetRowSize();

    // RGB is upside-down
    pf += (height-1) * pitch;

    for (int i=0; i<height; ++i) {
      for (int j=0; j<dstrowsize; j+=2) {        
        dstp[j + 0] = pf[j*2 + 3];
        dstp[j + 1] = 128;        
      }
      pf -= pitch;
      dstp += dstpitch;
    }      

    return dst;
  }
  else if (!lstrcmpi(pixel_type, "yv12"))
  {
    int i, j;  // stupid VC6

    PVideoFrame dst = env->NewVideoFrame(vi);
    BYTE * dstp = dst->GetWritePtr();
    int dstpitch = dst->GetPitch();
    int dstrowsize = dst->GetRowSize();

    // RGB is upside-down
    pf += (height-1) * pitch;

    for (i=0; i<height; ++i) {
      for (j=0; j<dstrowsize; ++j) {
        dstp[j] = pf[j*4 + 3]; 
      }
      pf -= pitch;
      dstp += dstpitch;
    }

    dstpitch = dst->GetPitch(PLANAR_U);
    dstrowsize = dst->GetRowSize(PLANAR_U);
    int dstheight = dst->GetHeight(PLANAR_U);
    BYTE * dstpu = dst->GetWritePtr(PLANAR_U);
    BYTE * dstpv = dst->GetWritePtr(PLANAR_V);
    for (i=0; i<dstheight; ++i) {      
      for (j=0; j<dstrowsize/4; ++j) {
        ((unsigned int*) dstpu)[j] = ((unsigned int*) dstpv)[j] = 0x80808080;
      }
      dstpu += dstpitch;
      dstpv += dstpitch;
    }

    return dst;
  }
  
  env->ThrowError("ShowAlpha: unexpected end of function");
  return f;
}


AVSValue ShowAlpha::Create(AVSValue args, void*, IScriptEnvironment* env)
{
  return new ShowAlpha(args[0].AsClip(), args[1].AsString("RGB"), env);
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

	if (! (vi1.IsRGB32() | vi1.IsYUY2()) ) 
		env->ThrowError("Layer only support RGB32 and YUY2 formats");

  vi = vi1;

	if (vi.IsRGB32()) ofsY = vi.height-vi2.height-ofsY; //RGB is upside down
	else ofsX = ofsX & 0xFFFFFFFE; //YUV must be aligned on even pixels

	xdest=(ofsX < 0)? 0: ofsX;
	ydest=(ofsY < 0)? 0: ofsY;

	xsrc=(ofsX < 0)? (0-ofsX): 0;
	ysrc=(ofsY < 0)? (0-ofsY): 0;

	xcount = (vi.width < (ofsX + vi2.width))? (vi.width-xdest) : (vi2.width - xsrc);
	ycount = (vi.height <  (ofsY + vi2.height))? (vi.height-ydest) : (vi2.height - ysrc);

  if (!( !lstrcmpi(Op, "Mul") || !lstrcmpi(Op, "Add") || !lstrcmpi(Op, "Fast") || 
         !lstrcmpi(Op, "Subtract") || !lstrcmpi(Op, "Add") || !lstrcmpi(Op, "Lighten") ||
         !lstrcmpi(Op, "Darken") ))
    env->ThrowError("Layer supports the following ops: Fast, Lighten, Darken, Add, Subtract, Mul");

  overlay_frames = vi2.num_frames;
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


		__declspec(align(8)) static __int64 oxooffooffooffooff=0x00ff00ff00ff00ff;  // Luma mask
		__declspec(align(8)) static __int64 oxffooffooffooffoo=0xff00ff00ff00ff00;  // Chroma mask
		__declspec(align(8)) static __int64 oxoo80oo80oo80oo80=0x0080008000800080;  // Null Chroma
		__declspec(align(8)) static __int64 ox7f7f7f7f7f7f7f7f=0x7f7f7f7f7f7f7f7f;  // FAST shift mask
		__declspec(align(8)) static __int64	 ox0101010101010101=0x0101010101010101;// FAST lsb mask

	if(vi.IsYUY2()){

		BYTE* src1p = src1->GetWritePtr();
		const BYTE* src2p = src2->GetReadPtr();
		src1p += (src1_pitch * ydest) + (xdest * 2);
		src2p += (src2_pitch * ysrc) + (xsrc * 2);
		const int myx = xcount >> 1;

		__int64	 thresh=0x0000000000000000 | ((T & 0xFF) <<48)| ((T & 0xFF) <<32)| ((T & 0xFF) <<16)| (T & 0xFF);

		if (!lstrcmpi(Op, "Mul"))
		{
			if (chroma)
			{
				__asm {
				mov			edi, src1p
				mov			esi, src2p
				mov			ebx, myy
				
				movq mm3, oxffooffooffooffoo     ; Chroma mask
				movq mm2, oxooffooffooffooff     ; Luma mask
				movd		mm1, mylevel			;alpha
				punpcklwd		mm1,mm1	  ;mm1= 0000|0000|00aa*|00aa*
				punpckldq		mm1, mm1	;mm1= 00aa*|00aa*|00aa*|00aa*
				movq					mylevel, mm1
				pxor		mm0,mm0

				mulyuy32loop:
						mov         edx, myx
						xor         ecx, ecx
		align 16
						mulyuy32xloop:
							//---- fetch src1/dest
									
							movd		mm7, [edi + ecx*4] ;src1/dest;
							movd		mm6, [esi + ecx*4] ;src2
							movq		mm4,mm7					;temp mm4=mm7
							pand		mm7,mm2					;mask for luma
							pand		mm4,mm3					;mask for chroma
							movq		mm5,mm6					;temp mm5=mm6
							pand		mm6,mm2					;mask for luma

							//----- begin the fun (luma) stuff
							pmullw	mm6,mm7

							pand		mm5,mm3					;mask for chroma
							psrlw		mm5,8							;line'em up

							psrlw		mm6,8
							psubsw	mm6, mm7
							pmullw	mm6, mm1 	  	;mm6=scaled difference*255

							psrlw		mm4,8							;line up chroma
							psubsw	mm5, mm4

							psrlw		mm6, 8		    ;scale result
							paddb		mm6, mm7		  ;add src1
							//----- end the fun stuff...

							//----- begin the fun (chroma) stuff
							pmullw	mm5, mm1 	  	;mm5=scaled difference*255

							mov		eax, ecx
							inc         ecx
							cmp         ecx, edx

							psrlw		mm5, 8		    ;scale result
							paddb		mm5, mm4		  ;add src1
							//----- end the fun stuff...

							psllw		mm5,8				;line up chroma
							por			mm6,mm5		;and merge'em back	

							movd        [edi + eax*4],mm6

						jnz         mulyuy32xloop

						add			edi, src1_pitch
						add			esi, src2_pitch
				dec		ebx
				jnz		mulyuy32loop
				emms
				}
			} else {
				__asm {
				mov			edi, src1p
				mov			esi, src2p
				mov			ebx, myy
						
				movq mm3, oxffooffooffooffoo     ; Chroma mask
				movq mm2, oxooffooffooffooff     ; Luma mask
				movd		mm1, mylevel			;alpha
				punpcklwd		mm1,mm1	  ;mm1= 0000|0000|00aa*|00aa*
				punpckldq		mm1, mm1	;mm1= 00aa*|00aa*|00aa*|00aa*
				movq					mylevel, mm1
				pxor		mm0,mm0

				muly032loop:
						mov         edx, myx
						xor         ecx, ecx

				align 16
						muly032xloop:
							//---- fetch src1/dest
									
							movd		mm7, [edi + ecx*4] ;src1/dest;
							movd		mm6, [esi + ecx*4] ;src2
							movq		mm4,mm7					;temp mm4=mm7
							pand		mm7,mm2					;mask for luma
							movq		mm5,mm6					;temp mm5=mm6
							pand		mm6,mm2					;mask for luma

							//----- begin the fun (luma) stuff
							pmullw	mm6,mm7
							
							pand		mm4,mm3					;mask for chroma

							psrlw		mm6,8
							psubsw	mm6, mm7
							pmullw	mm6, mm1 	  	;mm6=scaled difference*255

							psrlw		mm4,8							;line up chroma
							movq		mm5,oxoo80oo80oo80oo80	 					;get null chroma

							psrlw		mm6, 8		    ;scale result
							paddb		mm6, mm7		  ;add src1
							//----- end the fun stuff...

							//----- begin the fun (chroma) stuff
							movq		mm7,mm1
							psrlw		mm7,1
							psubsw	mm5, mm4
							pmullw	mm5, mm7 	  	;mm5=scaled difference*255

							mov		eax, ecx
							inc         ecx
							cmp         ecx, edx

							psrlw		mm5, 8		    ;scale result
							paddb		mm5, mm4		  ;add src1
							//----- end the fun stuff...

							psllw		mm5,8				;line up chroma
							por			mm6,mm5		;and merge'em back	

							movd        [edi + eax*4],mm6

						jnz         muly032xloop

						add			edi, src1_pitch
						add			esi, src2_pitch
				dec		ebx
				jnz		muly032loop
				emms
				}
			}
		}
		if (!lstrcmpi(Op, "Add"))
		{
			if (chroma)
			{
				__asm {
				mov			edi, src1p
				mov			esi, src2p
				mov			ebx, myy
				
				movq mm3, oxffooffooffooffoo     ; Chroma mask
				movq mm2, oxooffooffooffooff     ; Luma mask
				movd		mm1, mylevel			;alpha
				punpcklwd		mm1,mm1	  ;mm1= 0000|0000|00aa*|00aa*
				punpckldq		mm1, mm1	;mm1= 00aa*|00aa*|00aa*|00aa*
				movq					mylevel, mm1
				pxor		mm0,mm0

				addyuy32loop:
						mov         edx, myx
						xor         ecx, ecx

				align 16
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

							//----- begin the fun (luma) stuff
							psubsw	mm6, mm7
							pmullw	mm6, mm1 	  	;mm6=scaled difference*255

							pand		mm5,mm3					;mask for chroma
							psrlw		mm5,8							;line'em up
							
							psrlw		mm6, 8		    ;scale result
							paddb		mm6, mm7		  ;add src1
							//----- end the fun stuff...

							//----- begin the fun (chroma) stuff
							psubsw	mm5, mm4
							pmullw	mm5, mm1 	  	;mm5=scaled difference*255

							mov		eax, ecx
							inc         ecx
							cmp         ecx, edx

							psrlw		mm5, 8		    ;scale result
							paddb		mm5, mm4		  ;add src1
							//----- end the fun stuff...

							psllw		mm5,8				;line up chroma
							por			mm6,mm5		;and merge'em back	

							movd        [edi + eax*4],mm6

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
						
				movq mm3, oxffooffooffooffoo     ; Chroma mask
				movq mm2, oxooffooffooffooff     ; Luma mask
				movd		mm1, mylevel			;alpha
				punpcklwd		mm1,mm1	  ;mm1= 0000|0000|00aa*|00aa*
				punpckldq		mm1, mm1	;mm1= 00aa*|00aa*|00aa*|00aa*
				movq					mylevel, mm1
				pxor		mm0,mm0

				addy032loop:
						mov         edx, myx
						xor         ecx, ecx

				align 16
						addy032xloop:
							//---- fetch src1/dest
									
							movd		mm7, [edi + ecx*4] ;src1/dest;
							movd		mm6, [esi + ecx*4] ;src2
							movq		mm4,mm7					;temp mm4=mm7
							pand		mm7,mm2					;mask for luma
							pand		mm4,mm3					;mask for chroma
							movq		mm5,mm6					;temp mm5=mm6
							pand		mm6,mm2					;mask for luma

							//----- begin the fun (luma) stuff
							psubsw	mm6, mm7
							pmullw	mm6, mm1 	  	;mm6=scaled difference*255

							movq		mm5,oxoo80oo80oo80oo80	 					;get null chroma
							psrlw		mm4,8							;line up chroma
							
							psrlw		mm6, 8		    ;scale result
							paddb		mm6, mm7		  ;add src1
							//----- end the fun stuff...

							//----- begin the fun (chroma) stuff
							psubsw	mm5, mm4
							pmullw	mm5, mm1 	  	;mm5=scaled difference*255

							mov		eax, ecx
							inc         ecx
							cmp         ecx, edx

							psrlw		mm5, 8		    ;scale result
							paddb		mm5, mm4		  ;add src1
							//----- end the fun stuff...

							psllw		mm5,8				;line up chroma
							por			mm6,mm5		;and merge'em back	

							movd        [edi + eax*4],mm6
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
				movq			mm3, oxffooffooffooffoo     ; Chroma mask
				movq			mm2, oxooffooffooffooff     ; Luma mask

				fastyuy32loop:
						mov         edx, myx
						xor         ecx, ecx
						shr			edx,1

				    align 16
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
				
				movq mm3, oxffooffooffooffoo     ; Chroma mask
				movq mm2, oxooffooffooffooff     ; Luma mask
				movd		mm1, mylevel			;alpha
				punpcklwd		mm1,mm1	  ;mm1= 0000|0000|00aa*|00aa*
				punpckldq		mm1, mm1	;mm1= 00aa*|00aa*|00aa*|00aa*
				movq					mylevel, mm1
				pxor		mm0,mm0

				subyuy32loop:
						mov         edx, myx
						xor         ecx, ecx

				    align 16
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

							//----- begin the fun (luma) stuff
							psubsw	mm6, mm7
							pmullw	mm6, mm1 	  	;mm6=scaled difference*255

							pand		mm5,mm3					;mask for chroma
							psrlw		mm5,8							;line'em up
							
							psrlw		mm6, 8		    ;scale result
							paddb		mm6, mm7		  ;add src1
							//----- end the fun stuff...

							//----- begin the fun (chroma) stuff
							psubsw	mm5, mm4
							pmullw	mm5, mm1 	  	;mm5=scaled difference*255

							mov		eax, ecx
							inc         ecx
							cmp         ecx, edx

							psrlw		mm5, 8		    ;scale result
							paddb		mm5, mm4		  ;add src1
							//----- end the fun stuff...

							psllw		mm5,8				;line up chroma
							por			mm6,mm5		;and merge'em back	

							movd        [edi + eax*4],mm6
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
						
				movq mm3, oxffooffooffooffoo     ; Chroma mask
				movq mm2, oxooffooffooffooff     ; Luma mask
				movd		mm1, mylevel			;alpha
				punpcklwd		mm1,mm1	  ;mm1= 0000|0000|00aa*|00aa*
				punpckldq		mm1, mm1	;mm1= 00aa*|00aa*|00aa*|00aa*
				movq					mylevel, mm1
				pxor		mm0,mm0

				suby032loop:
						mov         edx, myx
						xor         ecx, ecx

				    align 16
						suby032xloop:
							//---- fetch src1/dest
									
							movd		mm7, [edi + ecx*4] ;src1/dest;
							movd		mm6, [esi + ecx*4] ;src2
							movq		mm4,mm7					;temp mm4=mm7
							pand		mm7,mm2					;mask for luma
							pand		mm4,mm3					;mask for chroma
							pcmpeqb	mm5, mm5					;mm5 will be sacrificed 
							psubb		mm5, mm6					;mm5 = 255-mm6
							movq		mm6,mm5					;temp mm6=mm5
							pand		mm6,mm2					;mask for luma

							//----- begin the fun (luma) stuff
							psubsw	mm6, mm7
							pmullw	mm6, mm1 	  	;mm6=scaled difference*255
							
							psrlw		mm4,8							;line up chroma
							movq		mm5,oxoo80oo80oo80oo80	 					;get null chroma

							psrlw		mm6, 8		    ;scale result
							paddb		mm6, mm7		  ;add src1
							//----- end the fun stuff...

							//----- begin the fun (chroma) stuff
							psubsw	mm5, mm4
							pmullw	mm5, mm1 	  	;mm5=scaled difference*255

							mov		eax, ecx
							inc         ecx
							cmp         ecx, edx

							psrlw		mm5, 8		    ;scale result
							paddb		mm5, mm4		  ;add src1
							//----- end the fun stuff...

							psllw		mm5,8				;line up chroma
							por			mm6,mm5		;and merge'em back	

							movd        [edi + eax*4],mm6

						jnz         suby032xloop

						add			edi, src1_pitch
						add			esi, src2_pitch
				dec		ebx
				jnz		suby032loop
				emms
				}
			}
		}
		if (!lstrcmpi(Op, "Lighten"))
		{
			if (chroma)
			{
				__asm {
				mov			edi, src1p
				mov			esi, src2p
				mov			ebx, myy
				
				movq mm3, oxffooffooffooffoo    ; Chroma mask
				movq mm2, oxooffooffooffooff    ; Luma mask
				movd		mm0, mylevel				;alpha
				punpcklwd		mm0,mm0			;mm0= 0000|0000|00aa*|00aa*
				punpckldq		mm0, mm0			;mm0= 00aa*|00aa*|00aa*|00aa*

				lightenyuy32loop:
						mov         edx, myx
						xor         ecx, ecx

				    align 16
						lightenyuy32xloop:
							//---- fetch src1/dest
									
							movd		mm7, [edi + ecx*4] ;src1/dest;
							movd		mm6, [esi + ecx*4] ;src2
							movq		mm1, thresh				;we'll need this in a minute
							movq		mm4,mm7					;temp mm4=mm7
							pand		mm7,mm2					;mask for luma	src1__YY__YY__YY__YY
							movq		mm5,mm6					;temp mm5=mm6
							pand		mm6,mm2					;mask for luma	src2__YY__YY__YY__YY

							paddw		mm1, mm6	 			;add threshold + lum into temporary home
							pand		mm5,mm3					;mask for chroma	src2VV__UU__VV__UU__
							psrlw		mm5,8							;line'em up	src2__VV__UU__VV__UU

							pcmpgtw	mm1, mm7				;see which is greater
							pand			mm1, mm0				;mm1 now has alpha mask	

							//----- begin the fun (luma) stuff
							psubsw	mm6, mm7
							pmullw	mm6, mm1 	  	;mm6=scaled difference*255
							
							pand		mm4,mm3					;mask for chroma	src1VV__UU__VV__UU__
							psrlw		mm4,8							;line up chroma	src1__VV__UU__VV__UU

							psrlw		mm6, 8		    ;scale result
							paddb		mm6, mm7		  ;add src1
							//----- end the fun stuff...

							//----- begin the fun (chroma) stuff
							psubsw	mm5, mm4
							pmullw	mm5, mm1 	  	;mm5=scaled difference*255

							mov		eax, ecx
							inc         ecx
							cmp         ecx, edx

							psrlw		mm5, 8		    ;scale result
							paddb		mm5, mm4		  ;add src1
							//----- end the fun stuff...

							psllw		mm5,8				;line up chroma
							por			mm6,mm5		;and merge'em back	

							movd        [edi + eax*4],mm6

						jnz         lightenyuy32xloop

						add			edi, src1_pitch
						add			esi, src2_pitch
				dec		ebx
				jnz		lightenyuy32loop
				emms
				}
			} else {
			      env->ThrowError("Layer: monochrome lighten illegal op");
			}
		}
		if (!lstrcmpi(Op, "Darken"))
		{
			if (chroma)
			{
				__asm {
				mov			edi, src1p
				mov			esi, src2p
				mov			ebx, myy
				
				movq mm3, oxffooffooffooffoo     ; Chroma mask
				movq mm2, oxooffooffooffooff     ; Luma mask
				movd		mm0, mylevel				;alpha
				punpcklwd		mm0,mm0			;mm0= 0000|0000|00aa*|00aa*
				punpckldq		mm0, mm0			;mm0= 00aa*|00aa*|00aa*|00aa*

				darkenyuy32loop:
						mov         edx, myx
						xor         ecx, ecx

				    align 16
						darkenyuy32xloop:
							//---- fetch src1/dest
									
							movd		mm7, [edi + ecx*4] ;src1/dest;
							movd		mm6, [esi + ecx*4] ;src2
							movq		mm1, thresh				;we'll need this in a minute
							movq		mm4,mm7					;temp mm4=mm7
							pand		mm7,mm2					;mask for luma	src1__YY__YY__YY__YY
							pand		mm4,mm3					;mask for chroma	src1VV__UU__VV__UU__
							movq		mm5,mm6					;temp mm5=mm6
							pand		mm6,mm2					;mask for luma	src2__YY__YY__YY__YY
							psrlw		mm4,8							;line up chroma	src1__VV__UU__VV__UU

							paddw		mm1, mm7	 			;add threshold + lum into temporary home

							pcmpgtw	mm1, mm6				;see which is greater
							pand			mm1, mm0				;mm1 now has alpha mask	

							//----- begin the fun (luma) stuff
							psubsw	mm6, mm7
							pmullw	mm6, mm1 	  	;mm6=scaled difference*255
							
							pand		mm5,mm3					;mask for chroma	src2VV__UU__VV__UU__
							psrlw		mm5,8							;line'em up	src2__VV__UU__VV__UU

							psrlw		mm6, 8		    ;scale result
							paddb		mm6, mm7		  ;add src1
							//----- end the fun stuff...

							//----- begin the fun (chroma) stuff
							psubsw	mm5, mm4
							pmullw	mm5, mm1 	  	;mm5=scaled difference*255

							mov		eax, ecx
							inc         ecx
							cmp         ecx, edx

							psrlw		mm5, 8		    ;scale result
							paddb		mm5, mm4		  ;add src1
							//----- end the fun stuff...

							psllw		mm5,8				;line up chroma
							por			mm6,mm5		;and merge'em back	

							movd        [edi + eax*4],mm6

						jnz         darkenyuy32xloop

						add			edi, src1_pitch
						add			esi, src2_pitch
				dec		ebx
				jnz		darkenyuy32loop
				emms
				}
			} else {
			      env->ThrowError("Layer: monochrome darken illegal op");
			}
		}
	}
	else if (vi.IsRGB32())
	{
		const int cyb = int(0.114*32768+0.5);
		const int cyg = int(0.587*32768+0.5);
		const int cyr = int(0.299*32768+0.5);
		__declspec(align(8)) static const unsigned __int64 rgb2lum = ((__int64)cyb << 32) | (cyg << 16) | cyr;

		BYTE* src1p = src1->GetWritePtr();
		const BYTE* src2p = src2->GetReadPtr();
		const int myx = xcount;

		src1p += (src1_pitch * ydest) + (xdest * 4);
		src2p += (src2_pitch * ysrc) + (xsrc * 4);

		__int64	 thresh=0x0000000000000000 | (T & 0xFF);

		if (!lstrcmpi(Op, "Mul"))
		{
			if (chroma)
			{
				__asm {
				mov			edi, src1p
				mov			esi, src2p
				mov			ebx, myy
				movd		mm1, mylevel			;alpha
				pxor		mm0,mm0
				
				mul32loop:
						mov         edx, myx
						xor         ecx, ecx

				    align 16
						mul32xloop:
								movd		mm6, [esi + ecx*4] ;src2
								movd		mm7, [edi + ecx*4] ;src1/dest
								movq		mm2,mm6

						//----- extract alpha into four channels

								psrlq		mm2,24		    ;mm2= 0000|0000|0000|00aa
								pmullw	mm2,mm1		    ;mm2= pixel alpha * script alpha

								punpcklbw		mm6,mm0		;mm6= 00aa|00bb|00gg|00rr [src2]
								punpcklbw		mm7, mm0	;mm7= 00aa|00bb|00gg|00rr [src1]
								
								psrlw		mm2,8		      ;mm2= 0000|0000|0000|00aa*
								pmullw				mm6,mm7

								punpcklwd		mm2,mm2	  ;mm2= 0000|0000|00aa*|00aa*
								punpckldq		mm2, mm2	;mm2= 00aa*|00aa*|00aa*|00aa*

						//----- alpha mask now in all four channels of mm2

								psrlw		mm6, 8		    ;scale multiply result

						//----- begin the fun stuff
								
								psubsw	mm6, mm7
								pmullw	mm6, mm2 	  	;mm6=scaled difference*255

								mov		eax, ecx
								inc         ecx
								cmp         ecx, edx

								psrlw		mm6, 8		    ;scale result
								paddb		mm6, mm7		  ;add src1

						//----- end the fun stuff...

								packuswb			mm6,mm0
								movd        [edi + eax*4],mm6
						jnz         mul32xloop

						add			edi, src1_pitch
						add			esi, src2_pitch
				dec		ebx
				jnz		mul32loop
				emms
				}
			} else {
				__asm {
				mov			edi, src1p
				mov			esi, src2p
				mov			ebx, myy
				movd		mm1, mylevel
				pxor		mm0,mm0
						
				mul32yloop:
						mov         edx, myx
						xor         ecx, ecx

				    align 16
						mul32yxloop:
								movd		mm6, [esi + ecx*4] ;src2
								movd		mm7, [edi + ecx*4] ;src1/dest
								movq		mm3, rgb2lum

						//----- extract alpha into four channels

								movq		mm2,mm6
								psrlq		mm2,24		      ;mm2= 0000|0000|0000|00aa
								pmullw		mm2,mm1		    ;mm2= pixel alpha * script alpha

								punpcklbw		mm6,mm0		  ;mm6= 00aa|00bb|00gg|00rr [src2]
								punpcklbw		mm7,mm0		  ;mm7= 00aa|00bb|00gg|00rr [src1]

						//----- start rgb -> monochrome
								pmaddwd			mm6,mm3			;partial monochrome result

								psrlw		mm2,8		        ;mm2= 0000|0000|0000|00aa*
								
								punpckldq		mm3,mm6			;ready to add
								paddd			mm6, mm3		  ;32 bit result
								psrlq			mm6, 47				;8 bit result
								punpcklwd		mm6, mm6		;propagate words
								punpckldq		mm6, mm6
						//----- end rgb -> monochrome

								pmullw				mm6,mm7

								punpcklwd		mm2,mm2		  ;mm2= 0000|0000|00aa*|00aa*
								punpckldq		mm2, mm2		;mm2= 00aa*|00aa*|00aa*|00aa*

						//----- alpha mask now in all four channels of mm3

								psrlw		mm6, 8		    ;scale multiply result

						//----- begin the fun stuff

								psubsw		mm6, mm7
								pmullw		mm6,mm2		;mm6=scaled difference*255

								mov		eax, ecx
								inc         ecx
								cmp         ecx, edx

								psrlw		  mm6,8		  ;scale result
								paddb		  mm6,mm7		;add src1

						//----- end the fun stuff...

								packuswb		mm6,mm0
								movd        [edi + eax*4],mm6

						jnz         mul32yxloop

						add				edi, src1_pitch
						add				esi, src2_pitch
				dec		ebx
				jnz		mul32yloop
				emms
				}
			}
		}
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

				    align 16
						add32xloop:
								movd		mm6, [esi + ecx*4] ;src2
								movd		mm7, [edi + ecx*4] ;src1/dest
								movq		mm2,mm6

						//----- extract alpha into four channels

								psrlq		mm2,24		    ;mm2= 0000|0000|0000|00aa
								pmullw	mm2,mm1		    ;mm2= pixel alpha * script alpha

								punpcklbw		mm6,mm0		;mm6= 00aa|00bb|00gg|00rr [src2]
								punpcklbw		mm7, mm0	;mm7= 00aa|00bb|00gg|00rr [src1]
								
								psrlw		mm2,8		      ;mm2= 0000|0000|0000|00aa*
								punpcklwd		mm2,mm2	  ;mm2= 0000|0000|00aa*|00aa*
								punpckldq		mm2, mm2	;mm2= 00aa*|00aa*|00aa*|00aa*

						//----- alpha mask now in all four channels of mm2


						//----- begin the fun stuff
								
								psubsw	mm6, mm7
								pmullw	mm6, mm2 	  	;mm6=scaled difference*255

								mov		eax, ecx
								inc         ecx
								cmp         ecx, edx

								psrlw		mm6, 8		    ;scale result
								paddb		mm6, mm7		  ;add src1

						//----- end the fun stuff...

								packuswb			mm6,mm0
								movd        [edi + eax*4],mm6

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

				align 16
						add32yxloop:
								movd		mm6, [esi + ecx*4] ;src2
								movd		mm7, [edi + ecx*4] ;src1/dest
								movq		mm3, rgb2lum
								movq		mm2,mm6

						//----- extract alpha into four channels

								psrlq		mm2,24		      ;mm2= 0000|0000|0000|00aa
								pmullw		mm2,mm1		    ;mm2= pixel alpha * script alpha

								punpcklbw		mm6,mm0		  ;mm6= 00aa|00bb|00gg|00rr [src2]
								punpcklbw		mm7,mm0		  ;mm7= 00aa|00bb|00gg|00rr [src1]

								psrlw		mm2,8		        ;mm2= 0000|0000|0000|00aa*

						//----- start rgb -> monochrome
								pmaddwd			mm6,mm3			;partial monochrome result

								punpcklwd		mm2,mm2		  ;mm2= 0000|0000|00aa*|00aa*
								punpckldq		mm2, mm2		;mm2= 00aa*|00aa*|00aa*|00aa*

								punpckldq		mm3,mm6			;ready to add
								paddd			mm6, mm3		  ;32 bit result
								psrlq			mm6, 47				;8 bit result
								punpcklwd		mm6, mm6		;propagate words
								punpckldq		mm6, mm6
						//----- end rgb -> monochrome

								psubsw		mm6, mm7
								pmullw		mm6,mm2		;mm6=scaled difference*255

								mov		eax, ecx
								inc         ecx
								cmp         ecx, edx

								psrlw		  mm6,8		  ;scale result
								paddb		  mm6,mm7		;add src1

						//----- end the fun stuff...

								packuswb		mm6,mm0
								movd        [edi + eax*4],mm6

						jnz         add32yxloop

						add				edi, src1_pitch
						add				esi, src2_pitch
				dec		ebx
				jnz		add32yloop
				emms
				}
			}
		}
		if (!lstrcmpi(Op, "Lighten"))
		{
			if (chroma)
			{
				__asm {
				mov			edi, src1p
				mov			esi, src2p
				mov			ebx, myy
				movd		mm1, mylevel			;alpha
				pxor		mm0,mm0
				
				lighten32loop:
						mov         edx, myx
						xor         ecx, ecx

				align 16
						lighten32xloop:
								movd		mm6, [esi + ecx*4] ;src2
								movd		mm7, [edi + ecx*4] ;src1/dest		;what a mess...

						movq		mm3, rgb2lum

								movq		mm2,mm6
								psrlq		mm2,24							;mm2= 0000|0000|0000|00aa
								pmullw	mm2,mm1						;mm2= pixel alpha * script alpha
								punpcklbw		mm6,mm0			;mm6= 00aa|00bb|00gg|00rr [src2]
						movq		mm4, mm6								;make a copy of this for conversion
								punpcklbw		mm7, mm0			;mm7= 00aa|00bb|00gg|00rr [src1]
						movq		mm5, mm3								;avoid refetching rgb2lum from mem
								psrlw		mm2,8								;mm2= 0000|0000|0000|00aa*

				//----- start rgb -> monochrome - interleaved pixels: twice the fun!

						pmaddwd			mm4,mm3					;partial monochrome result src2

						//----- extract alpha into four channels.. let's do this while we wait
								punpckldq		mm2, mm2	;mm2= 00aa*|00aa*|00aa*|00aa*
								punpcklwd		mm2,mm2	  ;mm2= 0000|0000|00aa*|00aa*

						punpckldq		mm3,mm4					;ready to add partial products
						paddd			mm4, mm3							;32 bit monochrome result src
						movq		mm3, mm7								;now get src1
						pmaddwd			mm3,mm5					;partial monochrome result src1
						psrlq			mm4, 47								;8 bit result src2
						punpckldq		mm5,mm3					;ready to add partial products src2
						paddd			mm3, mm5							;32 bit result src2
						psrlq			mm3, 47								;8 bit result src2

				//----- end rgb -> monochrome

				//----- now monochrome src2 in mm4, monochrome src1 in mm3 can be used for pixel compare

						paddw		mm3, thresh						;add threshold to src2
						pcmpgtd		mm4,mm3						;and see if src1 still greater
						punpckldq		mm4, mm4					;extend compare result to entire quadword

						//----- alpha mask now in all four channels of mm2

						pand		mm2, mm4

						//----- begin the fun stuff
								
								psubsw	mm6, mm7

								mov		eax, ecx					;remember where we are

								pmullw	mm6, mm2 	  	;mm6=scaled difference*255

						//---- something else to do while we wait...

								inc         ecx							;point to where we are going
								cmp         ecx, edx				;and see if we are done

								psrlw		mm6, 8					;now scale result from multiplier
								paddb		mm6, mm7			;and add src1

						//----- end the fun stuff...

								packuswb			mm6,mm0
								movd        [edi + eax*4],mm6

						jnz         lighten32xloop

						add			edi, src1_pitch
						add			esi, src2_pitch
				dec		ebx
				jnz		lighten32loop
				emms
				}
			} else {
			      env->ThrowError("Layer: monochrome lighten illegal op");
			}
		}
		if (!lstrcmpi(Op, "Darken"))
		{
			if (chroma)
			{
				__asm {
				mov			edi, src1p
				mov			esi, src2p
				mov			ebx, myy
				movd		mm1, mylevel			;alpha
				pxor		mm0,mm0
				
				darken32loop:
						mov         edx, myx
						xor         ecx, ecx

				align 16
						darken32xloop:
								movd		mm6, [esi + ecx*4] ;src2
								movd		mm7, [edi + ecx*4] ;src1/dest		;what a mess...

						movq		mm3, rgb2lum

								movq		mm2,mm6
								psrlq		mm2,24							;mm2= 0000|0000|0000|00aa
								pmullw	mm2,mm1						;mm2= pixel alpha * script alpha
								punpcklbw		mm6,mm0			;mm6= 00aa|00bb|00gg|00rr [src2]
						movq		mm4, mm6								;make a copy of this for conversion
								punpcklbw		mm7, mm0			;mm7= 00aa|00bb|00gg|00rr [src1]
						movq		mm5, mm3								;avoid refetching rgb2lum from mem
								psrlw		mm2,8								;mm2= 0000|0000|0000|00aa*

				//----- start rgb -> monochrome - interleaved pixels: twice the fun!

						pmaddwd			mm4,mm3					;partial monochrome result src2

						//----- extract alpha into four channels.. let's do this while we wait
								punpckldq		mm2, mm2	;mm2= 00aa*|00aa*|00aa*|00aa*
								punpcklwd		mm2,mm2	  ;mm2= 0000|0000|00aa*|00aa*

						punpckldq		mm3,mm4					;ready to add partial products
						paddd			mm4, mm3							;32 bit monochrome result src
						movq		mm3, mm7								;now get src1
						pmaddwd			mm3,mm5					;partial monochrome result src1
						psrlq			mm4, 47								;8 bit result src2
						punpckldq		mm5,mm3					;ready to add partial products src2
						paddd			mm3, mm5							;32 bit result src2
						psrlq			mm3, 47								;8 bit result src2

				//----- end rgb -> monochrome

				//----- now monochrome src2 in mm4, monochrome src1 in mm3 can be used for pixel compare

						paddw		mm4, thresh						;add threshold to src2
						pcmpgtd		mm3,mm4						;and see if src1 less 
						punpckldq		mm3, mm3					;extend compare result to entire quadword

						//----- alpha mask now in all four channels of mm2

						pand		mm2, mm3

						//----- begin the fun stuff
								
								psubsw	mm6, mm7

								mov		eax, ecx					;remember where we are

								pmullw	mm6, mm2 	  	;mm6=scaled difference*255

						//---- something else to do while we wait...

								inc         ecx							;point to where we are going
								cmp         ecx, edx				;and see if we are done

								psrlw		mm6, 8					;now scale result from multiplier
								paddb		mm6, mm7			;and add src1

						//----- end the fun stuff...

								packuswb			mm6,mm0
								movd        [edi + eax*4],mm6

						jnz         darken32xloop

						add			edi, src1_pitch
						add			esi, src2_pitch
				dec		ebx
				jnz		darken32loop
				emms
				}
			} else {
			      env->ThrowError("Layer: monochrome darken illegal op");
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

				align 16
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

				align 16
						sub32xloop:
								movd	  mm6, [esi + ecx*4] ;src2	
								movd		mm7, [edi + ecx*4] ;src1/dest
								movq		mm2,mm6

						//----- extract alpha into four channels

								psrlq		mm2,24		  ;mm2= 0000|0000|0000|00aa
								pmullw		mm2,mm1		;mm2= pixel alpha * script alpha

								punpcklbw		mm6,mm0		;mm6= 00aa|00bb|00gg|00rr [src2]
								pandn				mm6, mm4	;mm6 = mm6*
								punpcklbw		mm7,mm0		;mm7= 00aa|00bb|00gg|00rr [src1]
								
								psrlw		mm2,8		        ;mm2= 0000|0000|0000|00aa*
								punpcklwd		mm2,mm2		  ;mm2= 0000|0000|00aa*|00aa*
								punpckldq		mm2, mm2		;mm2=00aa*|00aa*|00aa*|00aa*

						//----- begin the fun stuff
								
								psubsw	mm6, mm7
								pmullw	mm6,mm2		;mm6=scaled difference*255

								mov		eax, ecx
								inc         ecx
								cmp         ecx, edx

								psrlw		mm6,8		  ;scale result
								paddb		mm6,mm7		;add src1

						//----- end the fun stuff...

								packuswb			mm6,mm0
								movd        [edi + eax*4],mm6

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

				align 16
						sub32yxloop:
								movd		mm6, [esi + ecx*4] ;src2
								movd		mm7, [edi + ecx*4] ;src1/dest
								movq		mm3, rgb2lum
								
						//----- extract alpha into four channels

								movq		mm2,mm6
								psrlq		mm2,24		;mm2= 0000|0000|0000|00aa
								pmullw	mm2,mm1		;mm2= pixel alpha * script alpha

								punpcklbw		mm6,mm0		;mm6= 00aa|00bb|00gg|00rr [src2]
								pandn				mm6, mm4	;mm6 = mm6*
								punpcklbw		mm7,mm0		;mm7= 00aa|00bb|00gg|00rr [src1]

								psrlw		mm2,8		        ;mm2= 0000|0000|0000|00aa*

						//----- start rgb -> monochrome
								pmaddwd			mm6,mm3			;partial monochrome result
								
								punpcklwd		mm2,mm2		  ;mm2= 0000|0000|00aa*|00aa*
								punpckldq		mm2, mm2		;mm2=00aa*|00aa*|00aa*|00aa*

								punpckldq		mm3,mm6			;ready to add
								paddd			mm6, mm3		  ;32 bit result
								psrlq			mm6, 47				;8 bit result
								punpcklwd		mm6, mm6		;propagate words
								punpckldq		mm6, mm6
						//----- end rgb -> monochrome

								psubsw		mm6, mm7
								pmullw		mm6,mm2		;mm6=scaled difference*255

								mov		eax, ecx
								inc         ecx
								cmp         ecx, edx

								psrlw		  mm6,8		  ;scale result
								paddb		  mm6,mm7		;add src1

								packuswb		mm6,mm0
								movd        [edi + eax*4],mm6
						//----- end the fun stuff...

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

  env->MakeWritable(&src1);

  BYTE* src1p = src1->GetWritePtr();
  const BYTE* src2p = src2->GetReadPtr();
  int row_size = src1->GetRowSize();

  if (vi.IsYV12()) {
    for (int y=0; y<vi.height; y++) {
      for (int x=0; x<row_size; x++) {
        src1p[x] = (src1p[x] - src2p[x]) + 126;
      }
      src1p += src1->GetPitch();
      src2p += src2->GetPitch();
    }
    BYTE* src1p = src1->GetWritePtr(PLANAR_U);
    const BYTE* src2p = src2->GetReadPtr(PLANAR_U);
    BYTE* src1pV = src1->GetWritePtr(PLANAR_V);
    const BYTE* src2pV = src2->GetReadPtr(PLANAR_V);

    row_size=src1->GetRowSize(PLANAR_U);

    for (y=0; y<src1->GetHeight(PLANAR_U); y++) {
      for (int x=0; x<row_size; x++) {
        src1p[x] = (src1p[x] - src2p[x]) + 128;
        src1pV[x] = (src1pV[x] - src2pV[x]) + 128;
      }
      src1p += src1->GetPitch(PLANAR_U);
      src2p += src2->GetPitch(PLANAR_U);
      src1pV += src1->GetPitch(PLANAR_V);
      src2pV += src2->GetPitch(PLANAR_V);
    }
    return src1;
  } // End planar

  for (int y=0; y<vi.height; ++y) 
  {
    // For YUY2, 50% gray is about (126,128,128) instead of (128,128,128).  Grr...
    if (vi.IsYUY2()) {
      for (int x=0; x<row_size; x+=2) {
        src1p[x] = src1p[x] - src2p[x] + 126;
        src1p[x+1] = src1p[x+1] - src2p[x+1] + 128;
      }
    } else {
      for (int x=0; x<row_size; ++x)
        src1p[x] = src1p[x] - src2p[x] + 128;
    }
    src1p += src1->GetPitch();
    src2p += src2->GetPitch();
  }

  return src1;
}



AVSValue __cdecl Subtract::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  return new Subtract(args[0].AsClip(), args[1].AsClip(), env);
}
