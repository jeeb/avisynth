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

#include "levels.h"





/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

AVSFunction Levels_filters[] = {
  { "Levels", "cifiii[coring]b", Levels::Create },        // src_low, gamma, src_high, dst_low, dst_high 
  { "RGBAdjust", "cffff", RGBAdjust::Create },   // R, G, B, A
  { "Tweak", "c[hue]f[sat]f[bright]f[cont]f[coring]b", Tweak::Create },  // hue, sat, bright, contrast
  { "Limiter", "c[min_luma]i[max_luma]i[min_chroma]i[max_chroma]i", Limiter::Create },
  { 0 }
};





/********************************
 *******   Levels Filter   ******
 ********************************/

Levels::Levels( PClip _child, int in_min, double gamma, int in_max, int out_min, int out_max, bool coring, 
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
      double p;

      if (coring) 
        p = ((i-16)*(255.0/219.0) - in_min) / divisor;
      else 
        p = double(i - in_min) / divisor;
      
      p = pow(min(max(p, 0.0), 1.0), gamma);
      p = p * (out_max - out_min) + out_min;
      int pp;

      if (coring) 
        pp = int(p*(219.0/255.0)+16.5);
      else 
        pp = int(p+0.5);

      map[i] = min(max(pp, (coring) ? 16 : 0), (coring) ? 235 : 255);

      int q = ((i-128) * (out_max-out_min) + (divisor>>1)) / divisor + 128;
      mapchroma[i] = min(max(q, (coring) ? 16 : 0), (coring) ? 240 : 255);
    }
  } else if (vi.IsRGB()) {
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
  else if (vi.IsPlanar()){
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
                     args[4].AsInt(), args[5].AsInt(), args[6].AsBool(true), env );
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

Tweak::Tweak( PClip _child, double _hue, double _sat, double _bright, double _cont, bool _coring,
              IScriptEnvironment* env ) 
  : GenericVideoFilter(_child), hue(_hue), sat(_sat), bright(_bright), cont(_cont), coring(_coring)
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
  
	int maxY = coring ? 235 : 255;
	int maxUV = coring ? 240 : 255;
	int minY = coring ? 16 : 0;
	int minUV = coring ? 16 : 0;

 	Hue = (hue * 3.1415926) / 180.0;
	Sin = (int) (sin(Hue) * 4096);
	Cos = (int) (cos(Hue) * 4096);

	if (env->GetCPUFlags() & CPUF_INTEGER_SSE) {
		__int64 hue64 = (in64 Cos<<48) + (in64 (-Sin)<<32) + (in64 Sin<<16) + in64 Cos;
		__int64 satcont64 = (in64 Sat<<48) + (in64 Cont<<32) + (in64 Sat<<16) + in64 Cont;
		__int64 bright64 = (in64 Bright<<32) + in64 Bright;

    if (vi.IsYUY2() && (!coring)) {
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
				y1 = min(max(y1,minY),maxY);
        y2 = min(max(y2,minY),maxY);
				srcp[x] = (int) y1;
				srcp[x+2] = (int) y2;

				/* hue and saturation */
				u = srcp[x+1] - 128;
				v = srcp[x+3] - 128;
				ux = (u * Cos + v * Sin) >> 12;
				v = (v * Cos - u * Sin) >> 12;
				u = ((ux * Sat) >> 9) + 128;
				v = ((v * Sat) >> 9) + 128;
				u = min(max(u,minUV),maxUV);
        v = min(max(v,minUV),maxUV);
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
				y1 = min(max(y1,minY),maxY);
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
				srcpu[x] = min(max(u,minUV),maxUV);
        srcpv[x] = min(max(v,minUV),maxUV);				
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
					 args[5].AsBool(true),    // coring
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

  luma_emulator=false;
  chroma_emulator=false;

}

PVideoFrame __stdcall Limiter::GetFrame(int n, IScriptEnvironment* env) {
	PVideoFrame frame = child->GetFrame(n, env);
	env->MakeWritable(&frame);
	unsigned char* srcp = frame->GetWritePtr();
	int pitch = frame->GetPitch();
  int row_size = frame->GetRowSize();
  int height = frame->GetHeight();

  if (vi.IsYUY2()) {

    /** Run emulator if CPU supports it**/
    if (env->GetCPUFlags() & CPUF_INTEGER_SSE) {
      c_plane = srcp;
      if (!luma_emulator) {  
        assemblerY = create_emulator(row_size, height, env);
        luma_emulator=true;
      }
      emu_cmin =  min_luma|(min_chroma<<8);
      emu_cmax =  max_luma|(max_chroma<<8);
      modulo = pitch-row_size;
      assemblerY.Call();
      return frame;
    } else {  // If not ISSE
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
      return frame;
    }  
  } else if(vi.IsYV12()) {
    if (env->GetCPUFlags() & CPUF_INTEGER_SSE) {
      /** Run emulator if CPU supports it**/
      row_size= frame->GetRowSize(PLANAR_Y_ALIGNED);
      if (!luma_emulator) {
        assemblerY = create_emulator(row_size, height, env);
        luma_emulator=true;
      }
      emu_cmin = min_luma|(min_luma<<8);
      emu_cmax = max_luma|(max_luma<<8);
      modulo = pitch-row_size;
      
      c_plane = (BYTE*)srcp;
      assemblerY.Call();
      
      // Prepare for chroma
      row_size = frame->GetRowSize(PLANAR_U_ALIGNED);
      pitch = frame->GetPitch(PLANAR_U);
      
      if (!chroma_emulator) {
        height = frame->GetHeight(PLANAR_U);
        assemblerUV = create_emulator(row_size, height, env);
        chroma_emulator=true;
      }
      emu_cmin = min_chroma|(min_chroma<<8);
      emu_cmax = max_chroma|(max_chroma<<8);
      modulo = pitch-row_size;
      
      c_plane = frame->GetWritePtr(PLANAR_U);
      assemblerUV.Call();
      
      c_plane = frame->GetWritePtr(PLANAR_V);
      assemblerUV.Call();
      
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

Limiter::~Limiter() {
  if (luma_emulator) assemblerY.Free();
  if (chroma_emulator) assemblerUV.Free();
}

/***
 * Dynamicly assembled code - runs at approx 14000 fps at 480x480 on a 1.8Ghz Athlon. 5-6x faster than C code
 ***/

DynamicAssembledCode Limiter::create_emulator(int row_size, int height, IScriptEnvironment* env) {

  int mod32_w = row_size/32;
  int remain_4 = (row_size-(mod32_w*32))/4;

  int prefetchevery = 1;  // 32 byte cache line

  if ((env->GetCPUFlags() & CPUF_3DNOW_EXT)||((env->GetCPUFlags() & CPUF_SSE2))) {
    // We have either an Athlon or a P4
    prefetchevery = 2;  // 64 byte cacheline
  }

  bool use_movntq = true;  // We cannot enable write combining as we are only writing 32 bytes between reads. Softwire also crashes here!!!
  bool hard_prefetch = false;   // Do we prefetch ALL data before any processing takes place?

  if (env->GetCPUFlags() & CPUF_3DNOW_EXT) {
    hard_prefetch = true;   // We have AMD - so we enable hard prefetching.
  }


  Assembler x86;   // This is the class that assembles the code.
  
  if (env->GetCPUFlags() & CPUF_INTEGER_SSE) {
    x86.push(eax);
    x86.push(ebx);
    x86.push(ecx);
    x86.push(edx);
    x86.push(esi);
    x86.push(edi);


    x86.mov(eax, height);
    x86.mov(ebx, dword_ptr [&c_plane]);  // Pointer to the current plane
    x86.mov(ecx, dword_ptr [&modulo]);   // Modulo
    x86.movd(mm7,dword_ptr [&emu_cmax]);  
    x86.movd(mm6, dword_ptr [&emu_cmin]);
    x86.pshufw(mm7,mm7,0);  // Move thresholds into all 8 bytes
    x86.pshufw(mm6,mm6,0);

    x86.align(16);
    x86.label("yloop");
    if (hard_prefetch) {
      for (int i=0;i<=mod32_w;i+=2) {
        x86.mov(edx, dword_ptr [ebx + (i*32)]);
      }
    }
    for (int i=0;i<mod32_w;i++) {
      // This loop processes 32 bytes at the time.
      // All remaining pixels are handled by the next loop.
      if ((!(i%prefetchevery)) && (!hard_prefetch)) {
         //Prefetch only once per cache line
       x86.prefetchnta(dword_ptr [ebx+256]);
      }
      x86.movq(mm0, qword_ptr[ebx]);
      x86.movq(mm1, qword_ptr[ebx+8]);
      x86.movq(mm2, qword_ptr[ebx+16]);
      x86.movq(mm3, qword_ptr[ebx+24]);
      x86.pminub(mm0,mm7);
      x86.pminub(mm1,mm7);
      x86.pminub(mm2,mm7);
      x86.pminub(mm3,mm7);
      x86.pmaxub(mm0,mm6);
      x86.pmaxub(mm1,mm6);
      x86.pmaxub(mm2,mm6);
      x86.pmaxub(mm3,mm6);
      if (!use_movntq) {
        x86.movq(qword_ptr[ebx],mm0);
        x86.movq(qword_ptr[ebx+8],mm1);
        x86.movq(qword_ptr[ebx+16],mm2);
        x86.movq(qword_ptr[ebx+24],mm3);
      } else {
        x86.movntq(qword_ptr [ebx],mm0);
        x86.movntq(qword_ptr [ebx+8],mm1);
        x86.movntq(qword_ptr [ebx+16],mm2);
        x86.movntq(qword_ptr [ebx+24],mm3);
      }
      x86.add(ebx,32);
    }
    for (i=0;i<remain_4;i++) {
      // Here we process any pixels not being within mod32.
      x86.movd(mm0,dword_ptr [ebx]);
      x86.pminub(mm0,mm7);
      x86.pmaxub(mm0,mm6);      
      x86.movd(dword_ptr [ebx],mm0);
      x86.add(ebx,4);
    }
    x86.add(ebx,ecx);
    x86.dec(eax);
    x86.jnz("yloop");
    if (use_movntq) {
      x86.sfence();  // Flush write combiner.
    }
    x86.emms();

    x86.pop(edi);
    x86.pop(esi);
    x86.pop(edx);
    x86.pop(ecx);
    x86.pop(ebx);
    x86.pop(eax);

    x86.ret();
  }
  return DynamicAssembledCode(x86, env, "Limiter: ISSE code could not be compiled.");
}


AVSValue __cdecl Limiter::Create(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	return new Limiter(args[0].AsClip(), args[1].AsInt(16), args[2].AsInt(235), args[3].AsInt(16), args[4].AsInt(240), env);
}
