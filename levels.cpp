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


#include "levels.h"





/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

AVSFunction Levels_filters[] = {
  { "Levels", "cifiii", Levels::Create },        // src_low, gamma, src_high, dst_low, dst_high 
  { "HSIAdjust", "cffifi", HSIAdjust::Create },  // H, S, min, gamma, max
  { "RGBAdjust", "cffff", RGBAdjust::Create },   // R, G, B, A
  { "Tweak", "c[hue]f[sat]f[bright]f[cont]f", Tweak::Create },  // hue, sat, bright, contrast
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
  if (vi.IsYUY2()) 
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
  const int pitch = frame->GetPitch();
  if (vi.IsYUY2()) {
    for (int y=0; y<vi.height; ++y) {
      for (int x=0; x<vi.width; ++x) {
        p[x*2] = map[p[x*2]];
        p[x*2+1] = mapchroma[p[x*2+1]];
      }
      p += pitch;
    }
  } 
  else {
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
  if (!vi.IsYUY2())
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
  const int pitch = frame->GetPitch();
	const int row_size = frame->GetRowSize();

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
  if (vi.IsYUY2())
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
  if (!vi.IsYUY2())
		env->ThrowError("Tweak: YUY2 data only (no RGB); use ConvertToYUY2");
}


PVideoFrame __stdcall Tweak::GetFrame(int n, IScriptEnvironment* env)
{
	double Hue;
	int Sin, Cos;
	int y1, y2, u, v, u2, v2;
	int Sat = (int) (sat * 512);
	int Cont = (int) (cont * 512);
	int Bright = (int) bright;

    PVideoFrame src = child->GetFrame(n, env);
    env->MakeWritable(&src);

    BYTE* srcp = src->GetWritePtr();

    const int src_pitch = src->GetPitch();
    const int row_size = src->GetRowSize();
	if (row_size % 4)
		env->ThrowError("Tweak: width must be a multiple of 2; use Crop");
    const int height = src->GetHeight();

 	Hue = (hue * 3.1415926) / 180.0;
	Sin = (int) (sin(Hue) * 4096);
	Cos = (int) (cos(Hue) * 4096);

	if (env->GetCPUFlags() & CPUF_INTEGER_SSE) {
		__int64 hue64 = (in64 Cos<<48) + (in64 (-Sin)<<32) + (in64 Sin<<16) + in64 Cos;
		__int64 satcont64 = (in64 Sat<<48) + (in64 Cont<<32) + (in64 Sat<<16) + in64 Cont;
		__int64 bright64 = (in64 Bright<<32) + in64 Bright;

		asm_tweak_ISSE(srcp, row_size>>2, height, src_pitch-row_size, hue64, satcont64, bright64);
	}
	else {

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
				if (y1 < 0) y1 = 0;
				else if (y1 > 255) y1 = 255;
				if (y2 < 0) y2 = 0;
				else if (y2 > 255) y2 = 255;
				srcp[x] = (int) y1;
				srcp[x+2] = (int) y2;

				/* hue and saturation */
				u = srcp[x+1] - 128;
				v = srcp[x+3] - 128;
				u2 = (u * Cos + v * Sin) >> 12;
				v2 = (v * Cos - u * Sin) >> 12;
				u = ((u2 * Sat) >> 9) + 128;
				v = ((v2 * Sat) >> 9) + 128;
				if (u < 0) u = 0;
				if (u > 255) u = 255;
				if (v < 0) v = 0;
				if (v > 255) v = 255;
				srcp[x+1] = u;
				srcp[x+3] = v;
			}
			srcp += src_pitch;
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
void __declspec(naked) asm_tweak_ISSE( BYTE *srcp, int w, int h, int modulo, __int64 hue, 
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

