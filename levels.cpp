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


#include "levels.h"





/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

AVSFunction Levels_filters[] = {
  { "Levels", "cifiii", Levels::Create },        // src_low, gamma, src_high, dst_low, dst_high 
  { "HSIAdjust", "cffifi", HSIAdjust::Create },  // H, S, min, gamma, max
  { "RGBAdjust", "cffff", RGBAdjust::Create },   // R, G, B, A
  { "Tweak", "c[hue]f[sat]f[bright]f[cont]f", Tweak::Create },  // hue, sat, bright, contrast
  { "Limiter", "c[min_luma]i[max_luma]i[min_chroma]i[max_chroma]i", Limiter::Create },
  { 0 }
};





/********************************
 *******   Levels Filter   ******
 ********************************/

Levels::Levels( PClip _child, int in_min, double gamma, int in_max, int out_min, int out_max, 
                IScriptEnvironment* env )
  : GenericVideoFilter(_child)
{
  if (gamma <= 0.0)
    env->ThrowError("Levels: gamma must be positive");
  gamma = 1/gamma;
  int divisor = in_max - in_min + (in_max == in_min);
  if (vi.IsYUV()) 
  {
    for (int i=0; i<256; ++i) 
    {
      double p = ((i-16)*(255.0/219.0) - in_min) / divisor;
      p = pow(min(max(p, 0.0), 1.0), gamma);
      p = p * (out_max - out_min) + out_min;
      int pp = int(p*(219.0/255.0)+16.5);
      map[i] = min(max(pp,16),235);

      int q = ((i-128) * (out_max-out_min) + (divisor>>1)) / divisor + 128;
      mapchroma[i] = min(max(q,16),240);
    }
  } 
  else {
    for (int i=0; i<256; ++i) 
    {
      double p = double(i - in_min) / divisor;
      p = pow(min(max(p, 0.0), 1.0), gamma);
      p = p * (out_max - out_min) + out_min;
      map[i] = PixelClip(int(p+0.5));
    }
  }
}

PVideoFrame __stdcall Levels::GetFrame(int n, IScriptEnvironment* env) 
{
  PVideoFrame frame = child->GetFrame(n, env);
  env->MakeWritable(&frame);
  BYTE* p = frame->GetWritePtr();
  int pitch = frame->GetPitch();
  if (vi.IsYUY2()) {
    for (int y=0; y<vi.height; ++y) {
      for (int x=0; x<vi.width; ++x) {
        p[x*2] = map[p[x*2]];
        p[x*2+1] = mapchroma[p[x*2+1]];
      }
      p += pitch;
    }
  } 
  else if (vi.IsYV12()){
    for (int y=0; y<vi.height; ++y) {
      for (int x=0; x<vi.width; ++x) {
        p[x] = map[p[x]];
      }
      p += pitch;
    }
    pitch = frame->GetPitch(PLANAR_U);
    p = frame->GetWritePtr(PLANAR_U);
    int w=frame->GetRowSize(PLANAR_U);
    int h=frame->GetHeight(PLANAR_U);
    for (y=0; y<h; ++y) {
      for (int x=0; x<w; ++x) {
        p[x] = mapchroma[p[x]];
      }
      p += pitch;
    }
    p = frame->GetWritePtr(PLANAR_V);
    for (y=0; y<h; ++y) {
      for (int x=0; x<w; ++x) {
        p[x] = mapchroma[p[x]];
      }
      p += pitch;
    }

  } else if (vi.IsRGB()) {
    const int row_size = frame->GetRowSize();
    for (int y=0; y<vi.height; ++y) {
      for (int x=0; x<row_size; ++x) {
        p[x] = map[p[x]];
      }
      p += pitch;
    }
  }
  return frame;
}

AVSValue __cdecl Levels::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  return new Levels( args[0].AsClip(), args[1].AsInt(), args[2].AsFloat(), args[3].AsInt(), 
                     args[4].AsInt(), args[5].AsInt(), env );
}






/*******************************
 *******    HSI Filter    ******
 *******************************/

HSIAdjust::HSIAdjust( PClip _child, double h, double s, int min, double gamma, int max, 
                      IScriptEnvironment* env )
  : GenericVideoFilter(_child)
{
  if (!vi.IsYUV())
    env->ThrowError("HSIAdjust requires YUV input");
	BYTE p;
	double x;
	int xx;
	int Hdivisor = int((1 / s) * 224);
	double Ydivisor = ((max - min) + (max == min));
	double rotU = (h < 0)?0-h:0;
	double rotV = (0 < h)?h:0;
	for (int i=0; i<256; ++i)
	{
    p = ((int(rotU + i) -128) * 224 + (Hdivisor>>1)) / Hdivisor + 128;
    mapU[i] = min(max(p,16),240);

    p = ((int(rotV + i) -128) * 224 + (Hdivisor>>1)) / Hdivisor + 128;
    mapV[i] = min(max(p,16),240);

    x = ((i-16)*(255.0/219.0) - min) / Ydivisor;
    x = pow(min(max(x, 0.0), 1.0), gamma);
    x = x * 219 + 16;
    xx = int(x*(219.0/255.0)+16.5);
    mapY[i] = min(max(xx,16),235);
  }	
}


PVideoFrame __stdcall HSIAdjust::GetFrame(int n, IScriptEnvironment* env) 
{
  PVideoFrame frame = child->GetFrame(n, env);
  env->MakeWritable(&frame);
  BYTE* p = frame->GetWritePtr();
  int pitch = frame->GetPitch();
  if (vi.IsYUY2()) {
	  for (int y=0; y<vi.height; ++y) 
    {
      for (int x=0; x<vi.width; x+=2) 
      {
        p[x*2] = mapY[p[x*2]];
        p[x*2+1] = mapU[p[x*2+1]];
        p[x*2+2] = mapY[p[x*2+2]];
        p[x*2+3] = mapV[p[x*2+3]];
      }
   	  p += pitch;
    }
  } else if (vi.IsYV12()) {
    for (int y=0; y<vi.height; ++y) {
      for (int x=0; x<vi.width; ++x) {
        p[x] = mapY[p[x]];
      }
      p += pitch;
    }
    pitch = frame->GetPitch(PLANAR_U);
    p = frame->GetWritePtr(PLANAR_U);
    int w = frame->GetRowSize(PLANAR_U);
    int h = frame->GetHeight(PLANAR_U);
    for (y=0; y<h; ++y) {
      for (int x=0; x<w; ++x) {
        p[x] = mapU[p[x]];
      }
      p += pitch;
    }
    p = frame->GetWritePtr(PLANAR_V);
    for (y=0; y<h; ++y) {
      for (int x=0; x<w; ++x) {
        p[x] = mapV[p[x]];
      }
      p += pitch;
    }

  }
  return frame;
}


AVSValue __cdecl HSIAdjust::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  return new HSIAdjust( args[0].AsClip(), args[1].AsFloat(), args[2].AsFloat(), args[3].AsInt(), 
                        args[4].AsFloat(), args[5].AsInt(), env );
}






/********************************
 *******    RGBA Filter    ******
 ********************************/

RGBAdjust::RGBAdjust(PClip _child, double r, double g, double b, double a, IScriptEnvironment* env)
  : GenericVideoFilter(_child)
{
  if (!vi.IsRGB())
    env->ThrowError("RGBAdjust requires RGB input");
  int p;
  for (int i=0; i<256; ++i) 
  {
    p = int (i * (r * 255) +0.5) >> 8;
    mapR[i] = min(max(p,0),255);
    p = int (i * (g * 255) +0.5) >> 8;
    mapG[i] = min(max(p,0),255);
    p = int (i * (b * 255) +0.5) >> 8;
    mapB[i] = min(max(p,0),255);
    p = int (i * (a * 255) +0.5) >> 8;
    mapA[i] = min(max(p,0),255);
  }
}


PVideoFrame __stdcall RGBAdjust::GetFrame(int n, IScriptEnvironment* env) 
{
  PVideoFrame frame = child->GetFrame(n, env);
  env->MakeWritable(&frame);
  BYTE* p = frame->GetWritePtr();
  const int pitch = frame->GetPitch();

  if (vi.IsRGB32())
  {
    const int row_size = frame->GetRowSize();
    for (int y=0; y<vi.height; ++y) 
    {
      for (int x=0; x < vi.width; ++x) 
      {
        p[x*4] = mapB[p[x*4]];
        p[x*4+1] = mapG[p[x*4+1]];
        p[x*4+2] = mapR[p[x*4+2]];
        p[x*4+3] = mapA[p[x*4+3]];
      }
      p += pitch;
    }
  }
  else if (vi.IsRGB24()) {
    const int row_size = frame->GetRowSize();
    for (int y=0; y<vi.height; ++y) 
    {
      for (int x=0; x<vi.width; ++x) 
      {
         p[x*3] = mapB[p[x*3]];
         p[x*3+1] = mapG[p[x*3+1]];
         p[x*3+2] = mapR[p[x*3+2]];
      }
      p += pitch;
    }
  }
  return frame;
}


AVSValue __cdecl RGBAdjust::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  return new RGBAdjust( args[0].AsClip(), args[1].AsFloat(), args[2].AsFloat(), args[3].AsFloat(), 
                        args[4].AsFloat(), env );
}








/**********************
******   Tweak    *****
**********************/

Tweak::Tweak( PClip _child, double _hue, double _sat, double _bright, double _cont, 
              IScriptEnvironment* env ) 
  : GenericVideoFilter(_child), hue(_hue), sat(_sat), bright(_bright), cont(_cont)
{
  if (vi.IsRGB())
		env->ThrowError("Tweak: YUV data only (no RGB)");
}


PVideoFrame __stdcall Tweak::GetFrame(int n, IScriptEnvironment* env)
{
	double Hue;
	int Sin, Cos;
	int y1, y2, u, v, ux;
	int Sat = (int) (sat * 512);
	int Cont = (int) (cont * 512);
	int Bright = (int) bright;

  PVideoFrame src = child->GetFrame(n, env);
  env->MakeWritable(&src);

  BYTE* srcp = src->GetWritePtr();

  int src_pitch = src->GetPitch();
  int height = src->GetHeight();
  int row_size = src->GetRowSize();
	
  if (row_size % 2 && vi.IsYUY2())
		env->ThrowError("Tweak: YUY2 width must be a multiple of 2; use Crop");
  if (row_size % 4 && vi.IsYV12())
		env->ThrowError("Tweak: YV12 width must be a multiple of 4; use Crop");
  

 	Hue = (hue * 3.1415926) / 180.0;
	Sin = (int) (sin(Hue) * 4096);
	Cos = (int) (cos(Hue) * 4096);

	if (env->GetCPUFlags() & CPUF_INTEGER_SSE) {
		__int64 hue64 = (in64 Cos<<48) + (in64 (-Sin)<<32) + (in64 Sin<<16) + in64 Cos;
		__int64 satcont64 = (in64 Sat<<48) + (in64 Cont<<32) + (in64 Sat<<16) + in64 Cont;
		__int64 bright64 = (in64 Bright<<32) + in64 Bright;

    if (vi.IsYUY2()) {
      asm_tweak_ISSE_YUY2(srcp, row_size>>2, height, src_pitch-row_size, hue64, satcont64, bright64);   
      return src;
    }
    else if (vi.IsYV12()) {
      //TODO: asm_tweak_ISSE_YV12
      //return src;
    }
	}

	if (vi.IsYUY2()) {
		for (int y = 0; y < height; y++)
		{
			for (int x = 0; x < row_size; x+=4)
			{
				/* brightness and contrast */
				y1 = srcp[x] - 16;
				y2 = srcp[x+2] - 16;
				y1 = (Cont * y1) >> 9;
				y2 = (Cont * y2) >> 9;
				y1 += (int) Bright;
				y2 += (int) Bright;
				y1 += 16;
				y2 += 16;
				y1 = min(max(y1,0),255);
        y2 = min(max(y2,0),255);
				srcp[x] = (int) y1;
				srcp[x+2] = (int) y2;

				/* hue and saturation */
				u = srcp[x+1] - 128;
				v = srcp[x+3] - 128;
				ux = (u * Cos + v * Sin) >> 12;
				v = (v * Cos - u * Sin) >> 12;
				u = ((ux * Sat) >> 9) + 128;
				v = ((v * Sat) >> 9) + 128;
				u = min(max(u,0),255);
        v = min(max(v,0),255);
				srcp[x+1] = u;
				srcp[x+3] = v;
			}
			srcp += src_pitch;
		}
  } else if (vi.IsYV12()) {
    int y;  // VC6 scoping sucks - Yes!
    for (y=0; y<height; ++y) {
      for (int x=0; x<row_size; ++x) {
        /* brightness and contrast */
				y1 = srcp[x] - 16;
				y1 = (Cont * y1) >> 9;				
				y1 += (int) Bright;
				y1 += 16;				
				y1 = min(max(y1,15),235);
				srcp[x] = (int) y1;
      }
      srcp += src_pitch;
    }
    src_pitch = src->GetPitch(PLANAR_U);
    BYTE * srcpu = src->GetWritePtr(PLANAR_U);
    BYTE * srcpv = src->GetWritePtr(PLANAR_V);
    row_size = src->GetRowSize(PLANAR_U);
    height = src->GetHeight(PLANAR_U);
    for (y=0; y<height; ++y) {
      for (int x=0; x<row_size; ++x) {
        /* hue and saturation */
				u = srcpu[x] - 128;
				v = srcpv[x] - 128;
				ux = (u * Cos + v * Sin) >> 12;
				v = (v * Cos - u * Sin) >> 12;
				u = ((ux * Sat) >> 9) + 128;
				v = ((v * Sat) >> 9) + 128;
				srcpu[x] = min(max(u,16),240);
        srcpv[x] = min(max(v,16),240);				
      }
      srcpu += src_pitch;
      srcpv += src_pitch;
    }
  }

  return src;
}

AVSValue __cdecl Tweak::Create(AVSValue args, void* user_data, IScriptEnvironment* env)
{
    return new Tweak(args[0].AsClip(),
					 args[1].AsFloat(0.0),		// hue
					 args[2].AsFloat(1.0),		// sat
					 args[3].AsFloat(0.0),		// bright
					 args[4].AsFloat(1.0),		// cont
					 env);
}



// Integer SSE optimization by "Dividee".
void __declspec(naked) asm_tweak_ISSE_YUY2( BYTE *srcp, int w, int h, int modulo, __int64 hue, 
                                       __int64 satcont, __int64 bright ) 
{
	static const __int64 norm = 0x0080001000800010i64;

	__asm {
		push		ebp
		push		edi
		push		esi
		push		ebx

		pxor		mm0, mm0
		movq		mm1, norm				// 128 16 128 16
		movq		mm2, [esp+16+20]		// Cos -Sin Sin Cos (fix12)
		movq		mm3, [esp+16+28]		// Sat Cont Sat Cont (fix9)
		movq		mm4, mm1
		paddw		mm4, [esp+16+36]		// 128 16+Bright 128 16+Bright

		mov			esi, [esp+16+4]			// srcp
		mov			edx, [esp+16+12]		// height
y_loop:
		mov			ecx, [esp+16+8]			// width
x_loop:
		movd		mm7, [esi]   			// 0000VYUY
		punpcklbw	mm7, mm0
		psubw		mm7, mm1				//  V Y U Y
		pshufw		mm6, mm7, 0xDD			//  V U V U
		pmaddwd		mm6, mm2				// V*Cos-U*Sin V*Sin+U*Cos (fix12)
		psrad		mm6, 12					// ? V' ? U'
		movq		mm5, mm7
		punpcklwd	mm7, mm6				// ? ? U' Y
		punpckhwd	mm5, mm6				// ? ? V' Y
		punpckldq	mm7, mm5				// V' Y U' Y
		psllw		mm7, 7					// (fix7)
		pmulhw		mm7, mm3	            // V'*Sat Y*Cont U'*Sat Y*Cont
		paddw		mm7, mm4				// V" Y" U" Y"
		packuswb	mm7, mm0				// 0000V"Y"U"Y"
		movd		[esi], mm7

		add			esi, 4
		dec			ecx
		jnz			x_loop

		add			esi, [esp+16+16]		// skip to next scanline
		dec			edx
		jnz			y_loop

		pop			ebx
		pop			esi
		pop			edi
		pop			ebp
		emms
		ret
	};
}

AVSValue __cdecl Tweak::FilterInfo(int request) {
  switch (request) {
    case FILTER_INPUT_COLORSPACE:
      return AVSValue(VideoInfo::CS_YUY2|VideoInfo::CS_YV12);

    case FILTER_NAME:
      return AVSValue("Tweak Filter");

    case FILTER_AUTHOR:
      return AVSValue("Donald Graft");

    case FILTER_VERSION:
      return AVSValue("1.1");

    case FILTER_ARGS:
      return AVSValue("c[hue]f[sat]f[bright]f[cont]f");

    case FILTER_ARGS_INFO:
      return AVSValue(";0.0,1.0,0.0;0.0,2.0,1.0;0.0,1.0,0.0;0.0,2.0,1.0");  // For floats and integers, use "Min, max, default" ';'delimits parameters

    case FILTER_ARGS_DESCRIPTION:
      return AVSValue("Input clip;Hue offset;Saturation;Brightness;Contrast");  // Description. Use ';' to delimit.

    case FILTER_DESCRIPTION:
      return AVSValue("Adjusts color and light levels of your picture depending on your parameters."); 
  }
  return AVSValue(-1);
}



Limiter::Limiter(PClip _child, int _min_luma, int _max_luma, int _min_chroma, int _max_chroma, IScriptEnvironment* env)
	: GenericVideoFilter(_child),
  min_luma(_min_luma),
  max_luma(_max_luma),
  min_chroma(_min_chroma),
  max_chroma(_max_chroma) {
	if(!vi.IsYUV())
		env->ThrowError("Limiter: Source must be YUV");

  if ((min_luma<0)||(min_luma>255))
      env->ThrowError("Limiter: Invalid minimum luma");
  if ((max_luma<0)||(max_luma>255))
      env->ThrowError("Limiter: Invalid maximum luma");
  if ((min_chroma<0)||(min_chroma>255))
      env->ThrowError("Limiter: Invalid minimum chroma");
  if ((max_chroma<0)||(max_chroma>255))
      env->ThrowError("Limiter: Invalid maximum chroma");
}

PVideoFrame __stdcall Limiter::GetFrame(int n, IScriptEnvironment* env) {
	PVideoFrame frame = child->GetFrame(n, env);
	env->MakeWritable(&frame);
	unsigned char* srcp = frame->GetWritePtr();
	int pitch = frame->GetPitch();
  int row_size = frame->GetRowSize();
  int height = frame->GetHeight();
  if (vi.IsYUY2()) {
    if (env->GetCPUFlags() & CPUF_INTEGER_SSE) {
      if (frame->GetRowSize()&7) {
        isse_limiter((BYTE*)srcp, row_size, height, pitch-row_size, min_luma|(min_chroma<<8), max_luma|(max_chroma<<8));
      } else {
        isse_limiter_mod8((BYTE*)srcp, row_size, height, pitch-row_size, min_luma|(min_chroma<<8), max_luma|(max_chroma<<8));
      }
    }
	  for(int y = 0; y < height; y++) {
      for(int x = 0; x < row_size; x++) {
        if(srcp[x] < min_luma )
          srcp[x++] = min_luma;
        else if(srcp[x] > max_luma)
          srcp[x++] = max_luma;
        else
          x++;
        if(srcp[x] < min_chroma)
          srcp[x] = min_chroma;
        else if(srcp[x] > max_chroma)
          srcp[x] = max_chroma;
      }
      srcp += pitch;
    }  
  } else if(vi.IsYV12()) {
  if (env->GetCPUFlags() & CPUF_INTEGER_SSE) {
    isse_limiter_mod8((BYTE*)srcp, frame->GetRowSize(PLANAR_Y_ALIGNED), height, pitch-frame->GetRowSize(PLANAR_Y_ALIGNED), min_luma|(min_luma<<8), max_luma|(max_luma<<8));

    srcp = frame->GetWritePtr(PLANAR_U);
    row_size = frame->GetRowSize(PLANAR_U_ALIGNED);
    pitch = frame->GetPitch(PLANAR_U);
    height = frame->GetHeight(PLANAR_U);

    isse_limiter_mod8((BYTE*)srcp, row_size, height, pitch-row_size, min_chroma|(min_chroma<<8), max_chroma|(max_chroma<<8));

    srcp = frame->GetWritePtr(PLANAR_V);
    isse_limiter_mod8((BYTE*)srcp, row_size, height, pitch-row_size, min_chroma|(min_chroma<<8), max_chroma|(max_chroma<<8));
    return frame;
  }

  for(int y = 0; y < height; y++) {
      for(int x = 0; x < row_size; x++) {
        if(srcp[x] < min_luma )
          srcp[x] = min_luma;
        else if(srcp[x] > max_luma)
          srcp[x] = max_luma;        
      }
      srcp += pitch;
    }
    srcp = frame->GetWritePtr(PLANAR_U);
	  unsigned char* srcpV = frame->GetWritePtr(PLANAR_V);
    row_size = frame->GetRowSize(PLANAR_U);
    height = frame->GetHeight(PLANAR_U);
    pitch = frame->GetPitch(PLANAR_U);
	  for(y = 0; y < height; y++) {
      for(int x = 0; x < row_size; x++) {
        if(srcp[x] < min_chroma)
          srcp[x] = min_chroma;
        else if(srcp[x] > max_chroma)
          srcp[x] = max_chroma;
        if(srcpV[x] < min_chroma)
          srcpV[x] = min_chroma;
        else if(srcpV[x] > max_chroma)
          srcpV[x] = max_chroma;
      }
      srcp += pitch;
      srcpV += pitch;
    }
  }
    return frame;
}

void Limiter::isse_limiter(BYTE* p, int row_size, int height, int modulo, int cmin, int cmax) {
  __asm {
    mov eax, [height]
    mov ebx, p
    mov ecx, modulo
    movd mm7,[cmax]
    movd mm6,[cmin]
    pshufw mm7,mm7,0
    pshufw mm6,mm6,0
yloop:
    mov edx,[row_size]
    align 16
xloop:
    prefetchnta [ebx+256]
    movd mm0,[ebx]
    pminub mm0,mm7
    pmaxub mm0,mm6
    movd [ebx],mm0
    add ebx,4
    sub edx,4
    jnz xloop
    add ebx,ecx;
    dec height
    jnz yloop
    emms
  }
}

void Limiter::isse_limiter_mod8(BYTE* p, int row_size, int height, int modulo, int cmin, int cmax) {
  __asm {
    mov eax, [height]
    mov ebx, p
    mov ecx, modulo
    movd mm7,[cmax]
    movd mm6,[cmin]
    pshufw mm7,mm7,0
    pshufw mm6,mm6,0
yloop:
    mov edx,[row_size]
    align 16
xloop:
    prefetchnta [ebx+256]
    movq mm0,[ebx]
    pminub mm0,mm7
    pmaxub mm0,mm6
    movq [ebx],mm0
    add ebx,8
    sub edx,8
    jnz xloop
    add ebx,ecx;
    dec height
    jnz yloop
    emms
  }
}

AVSValue __cdecl Limiter::Create(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	return new Limiter(args[0].AsClip(), args[1].AsInt(16), args[2].AsInt(236), args[3].AsInt(16), args[4].AsInt(240), env);
}
