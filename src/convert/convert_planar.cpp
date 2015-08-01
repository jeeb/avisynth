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

// ConvertPlanar (c) 2005 by Klaus Post


#include "stdafx.h"

#include "convert_planar.h"
#include "../filters/planeswap.h"
#include "../filters/field.h"


#define USE_DYNAMIC_COMPILER true


int getMatrix( const char* matrix, IScriptEnvironment* env) {
  if (matrix) {
    if (!lstrcmpi(matrix, "rec601"))
      return Rec601;
    if (!lstrcmpi(matrix, "rec709"))
      return Rec709;
    if (!lstrcmpi(matrix, "PC.601"))
      return PC_601;
    if (!lstrcmpi(matrix, "PC.709"))
      return PC_709;
    if (!lstrcmpi(matrix, "PC601"))
      return PC_601;
    if (!lstrcmpi(matrix, "PC709"))
      return PC_709;
    if (!lstrcmpi(matrix, "AVERAGE"))
      return AVERAGE;
    env->ThrowError("Convert: Unknown colormatrix");
  }
  return Rec601; // Default colorspace conversion for AviSynth
}

ConvertToY8::ConvertToY8(PClip src, int in_matrix, IScriptEnvironment* env) : GenericVideoFilter(src), matrix(0) {
  yuy2_input = blit_luma_only = rgb_input = false;

  if (vi.IsPlanar()) {
    blit_luma_only = true;
    vi.pixel_type = VideoInfo::CS_Y8;
    return;
  }

  if (vi.IsYUY2()) {
    yuy2_input = true;
    vi.pixel_type = VideoInfo::CS_Y8;
    return;
  }

  if (vi.IsRGB()) {
    rgb_input = true;
    pixel_step = vi.BytesFromPixels(1);
    vi.pixel_type = VideoInfo::CS_Y8;
    matrix = (signed short*)_aligned_malloc(sizeof(short)*4, 16);
    signed short* m = matrix;
    if (in_matrix == Rec601) {
      *m++ = (signed short)((219.0/255.0)*0.114*32768.0+0.5);  //B
      *m++ = (signed short)((219.0/255.0)*0.587*32768.0+0.5);  //G
      *m++ = (signed short)((219.0/255.0)*0.299*32768.0+0.5);  //R
      offset_y = 16;
    } else if (in_matrix == PC_601) {
      *m++ = (signed short)(0.114*32768.0+0.5);  //B
      *m++ = (signed short)(0.587*32768.0+0.5);  //G
      *m++ = (signed short)(0.299*32768.0+0.5);  //R
      offset_y = 0;
    } else if (in_matrix == Rec709) {
      *m++ = (signed short)((219.0/255.0)*0.0722*32768.0+0.5);  //B
      *m++ = (signed short)((219.0/255.0)*0.7152*32768.0+0.5);  //G
      *m++ = (signed short)((219.0/255.0)*0.2126*32768.0+0.5);  //R
      offset_y = 16;
    } else if (in_matrix == PC_709) {
      *m++ = (signed short)(0.0722*32768.0+0.5);  //B
      *m++ = (signed short)(0.7152*32768.0+0.5);  //G
      *m++ = (signed short)(0.2126*32768.0+0.5);  //R
      offset_y = 0;
    } else if (in_matrix == AVERAGE) {
      *m++ = (signed short)(32768.0/3 + 0.5);  //B
      *m++ = (signed short)(32768.0/3 + 0.5);  //G
      *m++ = (signed short)(32768.0/3 + 0.5);  //R
      offset_y = 0;
    } else {
      _aligned_free(matrix);
      matrix = 0;
      env->ThrowError("ConvertToY8: Unknown matrix.");
    }
    *m = 0;  // Alpha
 
    if (pixel_step == 4)
      genRGB32toY8(vi.width, vi.height, offset_y, matrix, env);
    else if (pixel_step == 3)
      genRGB24toY8(vi.width, vi.height, offset_y, matrix, env);

    return;
  }

  env->ThrowError("ConvertToY8: Unknown input format");
}

ConvertToY8::~ConvertToY8() {
  if (matrix)
    _aligned_free(matrix);
  matrix = 0;
}

PVideoFrame __stdcall ConvertToY8::GetFrame(int n, IScriptEnvironment* env) {
  PVideoFrame src = child->GetFrame(n, env);

  if (blit_luma_only) {
    // Abuse Subframe to snatch the Y plane
    return env->Subframe(src, 0, src->GetPitch(PLANAR_Y), src->GetRowSize(PLANAR_Y), src->GetHeight(PLANAR_Y));
  }

  PVideoFrame dst = env->NewVideoFrame(vi);

  if (yuy2_input) {

    const BYTE* srcP = src->GetReadPtr();
    const int srcPitch = src->GetPitch();
    const int awidth = min(srcPitch>>1, (vi.width+7) & -8);

    BYTE* dstY = dst->GetWritePtr(PLANAR_Y);
    const int dstPitch = dst->GetPitch(PLANAR_Y);

    if (!(awidth & 7)) {
      this->convYUV422toY8(srcP, dstY, srcPitch, dstPitch, awidth, vi.height);
      return dst;
    }

    const int w = dst->GetRowSize(PLANAR_Y);
    const int h = dst->GetHeight(PLANAR_Y);

    for (int y=0; y<h; y++) {
      for (int x=0; x<w; x++) {
        dstY[x] = srcP[x*2];
      }
      srcP+=srcPitch;
      dstY+=dstPitch;
    }
  }
  else if (rgb_input) {

    const int srcPitch = src->GetPitch();
    const BYTE* srcp = src->GetReadPtr() + srcPitch * (vi.height-1);  // We start at last line

    BYTE* dstY = dst->GetWritePtr(PLANAR_Y);
    const int dstPitch = dst->GetPitch(PLANAR_Y);

    if (pixel_step == 3 || pixel_step == 4) {
      assembly.Call(srcp, dstY, -srcPitch, dstPitch);
      return dst;
    }

    const int srcMod = srcPitch + (vi.width * pixel_step);
    const int m0 = matrix[0];
    const int m1 = matrix[1];
    const int m2 = matrix[2];
    for (int y=0; y<vi.height; y++) {
      for (int x=0; x<vi.width; x++) {
        const int Y = offset_y + ((m0 * srcp[0] + m1 * srcp[1] + m2 * srcp[2] + 16384) >> 15);
        dstY[x] = PixelClip(Y);  // All the safety we can wish for.
        srcp += pixel_step;
      }
      srcp -= srcMod;
      dstY += dstPitch;
    }
  }
  return dst;
}


void ConvertToY8::convYUV422toY8(const unsigned char *src, unsigned char *py,
       int pitch1, int pitch2y, int width, int height)
{
	__asm
	{
		mov edi,[src]
		mov edx,[py]
		pcmpeqw mm5,mm5
		mov ecx,width
		psrlw mm5,8            ; 0x00FF00FF00FF00FFi64
        shr ecx,1
		align 16
	yloop:
		xor eax,eax
		align 16
	xloop:
		movq mm0,[edi+eax*4]   ; VYUYVYUY - 1
		movq mm1,[edi+eax*4+8] ; VYUYVYUY - 2
		pand mm0,mm5           ; 0Y0Y0Y0Y - 1
		pand mm1,mm5           ; 0Y0Y0Y0Y - 2
		add eax,4
		packuswb mm0,mm1       ; YYYYYYYY
		cmp eax,ecx
		movq [edx+eax*2-8],mm0 ; store y
		jl xloop
		add edi,pitch1
		add edx,pitch2y
		dec height
		jnz yloop
		emms
	}
}


AVSValue __cdecl ConvertToY8::Create(AVSValue args, void*, IScriptEnvironment* env) {
  PClip clip = args[0].AsClip();
  if (clip->GetVideoInfo().IsY8())
    return clip;
  return new ConvertToY8(clip, getMatrix(args[1].AsString(0), env), env);
}


/*****************************************************
 * ConvertRGBToYV24
 *
 * (c) Klaus Post, 2005
 ******************************************************/

// XMM_WORD ready
__declspec(align(16)) static const __int64 Post_Add00[2] = { 0x008000800000, 0x008000800000 };
__declspec(align(16)) static const __int64 Post_Add16[2] = { 0x008000800010, 0x008000800010 };


ConvertRGBToYV24::ConvertRGBToYV24(PClip src, int in_matrix, IScriptEnvironment* env)
  : GenericVideoFilter(src), matrix(0), unpckbuf(0)  {

  if (!vi.IsRGB())
    env->ThrowError("ConvertRGBToYV24: Only RGB data input accepted");

  pixel_step = vi.BytesFromPixels(1);
  vi.pixel_type = VideoInfo::CS_YV24;
  matrix = (signed short*)_aligned_malloc(sizeof(short)*16,64);

  const int shift = 15;

  if (in_matrix == Rec601) {
    /*
    Y'= 0.299*R' + 0.587*G' + 0.114*B'
    Cb=-0.169*R' - 0.331*G' + 0.500*B'
    Cr= 0.500*R' - 0.419*G' - 0.081*B'
    */
    BuildMatrix(0.299,  /* 0.587  */ 0.114,  219, 112, 16, shift);
  }
  else if (in_matrix == PC_601) {

    BuildMatrix(0.299,  /* 0.587  */ 0.114,  255, 127,  0, shift);
  }
  else if (in_matrix == Rec709) {
    /*
    Y'= 0.2126*R' + 0.7152*G' + 0.0722*B'
    Cb=-0.1145*R' - 0.3855*G' + 0.5000*B'
    Cr= 0.5000*R' - 0.4542*G' - 0.0458*B'
    */
    BuildMatrix(0.2126, /* 0.7152 */ 0.0722, 219, 112, 16, shift);
  }
  else if (in_matrix == PC_709) {

    BuildMatrix(0.2126, /* 0.7152 */ 0.0722, 255, 127,  0, shift);
  }
  else if (in_matrix == AVERAGE) {

    BuildMatrix(1.0/3, /* 1.0/3 */ 1.0/3, 255, 127,  0, shift);
  }
  else {
    _aligned_free(matrix);
    matrix = 0;
    env->ThrowError("ConvertRGBToYV24: Unknown matrix.");
  }

  // Thread unsafe!
  unpckbuf = (BYTE*)_aligned_malloc(vi.width * 4 + 32, 64);
  const __int64 *post_add = offset_y == 16 ? &Post_Add16[0] : &Post_Add00[0];
  this->GenerateAssembly(vi.width, shift, false, 0, post_add, pixel_step, 4, matrix, env);

  this->GenerateUnPacker(vi.width, env);
}

ConvertRGBToYV24::~ConvertRGBToYV24() {
  if (unpckbuf)
    _aligned_free(unpckbuf);
  unpckbuf = 0;

  if (matrix)
    _aligned_free(matrix);
  matrix = 0;

}


void ConvertRGBToYV24::BuildMatrix(double Kr, double Kb, int Sy, int Suv, int Oy, int shift)
{
/*
  Kr   = {0.299, 0.2126}
  Kb   = {0.114, 0.0722}
  Kg   = 1 - Kr - Kb // {0.587, 0.7152}
  Srgb = 255
  Sy   = {219, 255}
  Suv  = {112, 127}
  Oy   = {16, 0}
  Ouv  = 128

  R = r/Srgb                     // 0..1
  G = g/Srgb
  B = b*Srgb

  Y = Kr*R + Kg*G + Kb*B         // 0..1
  U = B - (Kr*R + Kg*G)/(1-Kb)   //-1..1
  V = R - (Kg*G + Kb*B)/(1-Kr)

  y = Y*Sy  + Oy                 // 16..235, 0..255
  u = U*Suv + Ouv                // 16..240, 1..255
  v = V*Suv + Ouv
*/
  const double mulfac = double(1<<shift);

  const double Kg = 1.- Kr - Kb;
  const int Srgb = 255;

  signed short* m = matrix;

  *m++ = (signed short)(Sy  * Kb        * mulfac / Srgb + 0.5); //B
  *m++ = (signed short)(Sy  * Kg        * mulfac / Srgb + 0.5); //G
  *m++ = (signed short)(Sy  * Kr        * mulfac / Srgb + 0.5); //R
  *m++ = (signed short)(           -0.5 * mulfac             ); //Rounder, assumes target is -1, 0xffff
  *m++ = (signed short)(Suv             * mulfac / Srgb + 0.5);
  *m++ = (signed short)(Suv * Kg/(Kb-1) * mulfac / Srgb + 0.5);
  *m++ = (signed short)(Suv * Kr/(Kb-1) * mulfac / Srgb + 0.5);
  *m++ = (signed short)(           -0.5 * mulfac             );
  *m++ = (signed short)(Suv * Kb/(Kr-1) * mulfac / Srgb + 0.5);
  *m++ = (signed short)(Suv * Kg/(Kr-1) * mulfac / Srgb + 0.5);
  *m++ = (signed short)(Suv             * mulfac / Srgb + 0.5);
  *m++ = (signed short)(           -0.5 * mulfac             );
  *m++ = (signed short)0x0000u;
  *m++ = (signed short)0xff00u;
  *m++ = (signed short)0x0000u;
  *m++ = (signed short)0xff00u;
  offset_y = Oy;
}

PVideoFrame __stdcall ConvertRGBToYV24::GetFrame(int n, IScriptEnvironment* env) {
  PVideoFrame src = child->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrame(vi);

  const BYTE* srcp = src->GetReadPtr();

  BYTE* dstY = dst->GetWritePtr(PLANAR_Y);
  BYTE* dstU = dst->GetWritePtr(PLANAR_U);
  BYTE* dstV = dst->GetWritePtr(PLANAR_V);

  const int Spitch = src->GetPitch();

  const int Ypitch = dst->GetPitch(PLANAR_Y);
  const int UVpitch = dst->GetPitch(PLANAR_U);

  const int awidth = dst->GetRowSize(PLANAR_Y_ALIGNED);

#ifdef USE_DYNAMIC_COMPILER
  {
    srcp += (vi.height-1)*Spitch;
//  BYTE* unpckbuf = (BYTE*)_aligned_malloc(vi.width * 4 + 32, 64);

    if (awidth & 7) {  // Should never happend, as all planar formats should have mod16 pitch
      int* iunpckbuf = (int*)unpckbuf;
      for (int y = 0; y < vi.height; y++) {

        assembly.Call(srcp, unpckbuf);

        for (int x = 0; x < vi.width; x++) {
          const int p = iunpckbuf[x];
          dstY[x] = byte(p&0xff);
          dstU[x] = byte((p>>8)&0xff);
          dstV[x] = byte((p>>16)&0xff);
        }

        srcp -= Spitch;

        dstY += Ypitch;
        dstU += UVpitch;
        dstV += UVpitch;
      }
    }
    else {
      for (int y = 0; y < vi.height; y++) {

        assembly.Call(srcp, unpckbuf);

        this->unpacker.Call(unpckbuf, dstY, dstU, dstV);

        srcp -= Spitch;

        dstY += Ypitch;
        dstU += UVpitch;
        dstV += UVpitch;
      }
    }
//  _aligned_free(unpckbuf);
    return dst;
  }
#else
  //Slow C-code.

  signed short* m = (signed short*)matrix;
  srcp += Spitch * (vi.height-1);  // We start at last line
  const int Sstep = Spitch + (vi.width * pixel_step);
  for (int y = 0; y < vi.height; y++) {
    for (int x = 0; x < vi.width; x++) {
      int b = srcp[0];
      int g = srcp[1];
      int r = srcp[2];
      int Y = offset_y + (((int)m[0] * b + (int)m[1] * g + (int)m[2] * r + 16384)>>15);
      int U = 128+(((int)m[4] * b + (int)m[5] * g + (int)m[6] * r + 16384)>>15);
      int V = 128+(((int)m[8] * b + (int)m[9] * g + (int)m[10] * r + 16384)>>15);
      *dstY++ = PixelClip(Y);  // All the safety we can wish for.
      *dstU++ = PixelClip(U);
      *dstV++ = PixelClip(V);
      srcp += pixel_step;
    }
    srcp -= Sstep;
    dstY += Ypitch - vi.width;
    dstU += UVpitch - vi.width;
    dstV += UVpitch - vi.width;
  }
  return dst;
#endif
}

AVSValue __cdecl ConvertRGBToYV24::Create(AVSValue args, void*, IScriptEnvironment* env) {
  PClip clip = args[0].AsClip();
  if (clip->GetVideoInfo().IsYV24())
    return clip;
  return new ConvertRGBToYV24(clip, getMatrix(args[1].AsString(0), env), env);
}


/*****************************************************
 * ConvertYV24ToRGB
 *
 * (c) Klaus Post, 2005
 ******************************************************/

// XMM_WORD ready
__declspec(align(16)) static const __int64 Pre_Add00[2]  = { 0xff80ff800000, 0xff80ff800000 };
__declspec(align(16)) static const __int64 Pre_Add16[2]  = { 0xff80ff80fff0, 0xff80ff80fff0 };


ConvertYV24ToRGB::ConvertYV24ToRGB(PClip src, int in_matrix, int _pixel_step, IScriptEnvironment* env)
 : GenericVideoFilter(src), pixel_step(_pixel_step), matrix(0), packbuf(0) {

  if (!vi.IsYV24())
    env->ThrowError("ConvertYV24ToRGB: Only YV24 data input accepted");

  vi.pixel_type = (pixel_step == 3) ? VideoInfo::CS_BGR24 : VideoInfo::CS_BGR32;
  matrix = (signed short*)_aligned_malloc(sizeof(short)*16,64);
  const int shift = 13;

  if (in_matrix == Rec601) {
/*
    B'= Y' + 1.772*U' + 0.000*V'
    G'= Y' - 0.344*U' - 0.714*V'
    R'= Y' + 0.000*U' + 1.402*V'
*/
    BuildMatrix(0.299,  /* 0.587  */ 0.114,  219, 112, 16, shift);

  }
  else if (in_matrix == PC_601) {

    BuildMatrix(0.299,  /* 0.587  */ 0.114,  255, 127,  0, shift);
  }
  else if (in_matrix == Rec709) {
/*
    B'= Y' + 1.8558*Cb + 0.0000*Cr
    G'= Y' - 0.1870*Cb - 0.4678*Cr
    R'= Y' + 0.0000*Cb + 1.5750*Cr
*/
    BuildMatrix(0.2126, /* 0.7152 */ 0.0722, 219, 112, 16, shift);
  }
  else if (in_matrix == PC_709) {

    BuildMatrix(0.2126, /* 0.7152 */ 0.0722, 255, 127,  0, shift);
  }
  else if (in_matrix == AVERAGE) {

    BuildMatrix(1.0/3, /* 1.0/3 */ 1.0/3, 255, 127,  0, shift);
  }
  else {
    _aligned_free(matrix);
    matrix = 0;
    env->ThrowError("ConvertYV24ToRGB: Unknown matrix.");
  }
  // Thread unsafe!
  packbuf = (BYTE*)_aligned_malloc(vi.width * 4 + 60, 64);
  const __int64 *pre_add = offset_y == -16 ? &Pre_Add16[0] : &Pre_Add00[0];
  this->GenerateAssembly(vi.width, shift, true, pre_add, 0, 4, pixel_step, matrix, env);

  this->GeneratePacker(vi.width, env);
}

ConvertYV24ToRGB::~ConvertYV24ToRGB() {
  if (packbuf)
    _aligned_free(packbuf);
  packbuf = 0;

  if (matrix)
    _aligned_free(matrix);
  matrix = 0;

}


void ConvertYV24ToRGB::BuildMatrix(double Kr, double Kb, int Sy, int Suv, int Oy, int shift)
{
/*
  Kr   = {0.299, 0.2126}
  Kb   = {0.114, 0.0722}
  Kg   = 1 - Kr - Kb // {0.587, 0.7152}
  Srgb = 255
  Sy   = {219, 255}
  Suv  = {112, 127}
  Oy   = {16, 0}
  Ouv  = 128

  Y =(y-Oy)  / Sy                         // 0..1
  U =(u-Ouv) / Suv                        //-1..1
  V =(v-Ouv) / Suv

  R = Y                  + V*(1-Kr)       // 0..1
  G = Y - U*(1-Kb)*Kb/Kg - V*(1-Kr)*Kr/Kg
  B = Y + U*(1-Kb)

  r = R*Srgb                              // 0..255
  g = G*Srgb
  b = B*Srgb
*/
  const double mulfac = double(1<<shift);

  const double Kg = 1.- Kr - Kb;
  const int Srgb = 255;

  signed short* m = matrix;

  *m++ = (signed short)(Srgb * 1.000        * mulfac / Sy  + 0.5); //Y
  *m++ = (signed short)(Srgb * (1-Kb)       * mulfac / Suv + 0.5); //U
  *m++ = (signed short)(Srgb * 0.000        * mulfac / Suv + 0.5); //V
  *m++ = (signed short)(                0.5 * mulfac            ); //Rounder assumes target is +1, 0x0001
  *m++ = (signed short)(Srgb * 1.000        * mulfac / Sy  + 0.5);
  *m++ = (signed short)(Srgb * (Kb-1)*Kb/Kg * mulfac / Suv + 0.5);
  *m++ = (signed short)(Srgb * (Kr-1)*Kr/Kg * mulfac / Suv + 0.5);
  *m++ = (signed short)(                0.5 * mulfac            );
  *m++ = (signed short)(Srgb * 1.000        * mulfac / Sy  + 0.5);
  *m++ = (signed short)(Srgb * 0.000        * mulfac / Suv + 0.5);
  *m++ = (signed short)(Srgb * (1-Kr)       * mulfac / Suv + 0.5);
  *m++ = (signed short)(                0.5 * mulfac            );
  *m++ = (signed short)0x0000u;
  *m++ = (signed short)0xff00u;
  *m++ = (signed short)0x0000u;
  *m++ = (signed short)0xff00u;
  offset_y = -Oy;
}

PVideoFrame __stdcall ConvertYV24ToRGB::GetFrame(int n, IScriptEnvironment* env) {
  PVideoFrame src = child->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrame(vi, 8);

  const BYTE* srcY = src->GetReadPtr(PLANAR_Y);
  const BYTE* srcU = src->GetReadPtr(PLANAR_U);
  const BYTE* srcV = src->GetReadPtr(PLANAR_V);

  BYTE* dstp = dst->GetWritePtr();

  int awidth = src->GetRowSize(PLANAR_Y_ALIGNED);

  const int Ypitch = src->GetPitch(PLANAR_Y);
  const int UVpitch = src->GetPitch(PLANAR_U);

  const int Dpitch = dst->GetPitch();

#ifdef USE_DYNAMIC_COMPILER
  {
    dstp += (vi.height-1)*Dpitch;
//  BYTE* packbuf = (BYTE*)_aligned_malloc(vi.width * 4 + 60, 64);

    if (awidth & 15) { // This should be very safe to assume to never happend
      int* ipackbuf = (int*)packbuf;

      for (int y = 0; y < vi.height; y++) {
        for (int x = 0; x < vi.width; x++) {
          ipackbuf[x] = srcY[x] | (srcU[x] << 8 ) | (srcV[x] << 16) | (1 << 24);
        }

        assembly.Call(packbuf, dstp);

        srcY += Ypitch;
        srcU += UVpitch;
        srcV += UVpitch;
        dstp -= Dpitch;
      }
    }
    else {
      for (int y = 0; y < vi.height; y++) {

        this->packer.Call(srcY, srcU, srcV, packbuf);

        assembly.Call(packbuf, dstp);

        srcY += Ypitch;
        srcU += UVpitch;
        srcV += UVpitch;
        dstp -= Dpitch;
      }
    }
//  _aligned_free(packbuf);
    return dst;
  }
#else
  //Slow C-code.

  signed short* m = (signed short*)matrix;
  dstp += Dpitch * (vi.height-1);  // We start at last line
  if (pixel_step == 4) {
    for (int y = 0; y < vi.height; y++) {
      for (int x = 0; x < vi.width; x++) {
        int Y = srcY[x] + offset_y;
        int U = srcU[x] - 128;
        int V = srcV[x] - 128;
        int b = (((int)m[0] * Y + (int)m[1] * U + (int)m[ 2] * V + 4096)>>13);
        int g = (((int)m[4] * Y + (int)m[5] * U + (int)m[ 6] * V + 4096)>>13);
        int r = (((int)m[8] * Y + (int)m[9] * U + (int)m[10] * V + 4096)>>13);
        dstp[x*4+0] = PixelClip(b);  // All the safety we can wish for.
        dstp[x*4+1] = PixelClip(g);  // Probably needed here.
        dstp[x*4+2] = PixelClip(r);
        dstp[x*4+3] = 255; // alpha
      }
      dstp -= Dpitch;
      srcY += Ypitch;
      srcU += UVpitch;
      srcV += UVpitch;
    }
  } else {
    const int Dstep = Dpitch + (vi.width * pixel_step);
    for (int y = 0; y < vi.height; y++) {
      for (int x = 0; x < vi.width; x++) {
        int Y = srcY[x] + offset_y;
        int U = srcU[x] - 128;
        int V = srcV[x] - 128;
        int b = (((int)m[0] * Y + (int)m[1] * U + (int)m[ 2] * V + 4096)>>13);
        int g = (((int)m[4] * Y + (int)m[5] * U + (int)m[ 6] * V + 4096)>>13);
        int r = (((int)m[8] * Y + (int)m[9] * U + (int)m[10] * V + 4096)>>13);
        dstp[0] = PixelClip(b);  // All the safety we can wish for.
        dstp[1] = PixelClip(g);  // Probably needed here.
        dstp[2] = PixelClip(r);
        dstp += pixel_step;
      }
      dstp -= Dstep;
      srcY += Ypitch;
      srcU += UVpitch;
      srcV += UVpitch;
    }
  }
  return dst;
#endif
}

AVSValue __cdecl ConvertYV24ToRGB::Create32(AVSValue args, void*, IScriptEnvironment* env) {
  PClip clip = args[0].AsClip();
  if (clip->GetVideoInfo().IsRGB())
    return clip;
  return new ConvertYV24ToRGB(clip, getMatrix(args[1].AsString(0), env), 4, env);
}

AVSValue __cdecl ConvertYV24ToRGB::Create24(AVSValue args, void*, IScriptEnvironment* env) {
  PClip clip = args[0].AsClip();
  if (clip->GetVideoInfo().IsRGB())
    return clip;
  return new ConvertYV24ToRGB(clip, getMatrix(args[1].AsString(0), env), 3, env);
}

/************************************
 * YUY2 to YV16
 ************************************/

ConvertYUY2ToYV16::ConvertYUY2ToYV16(PClip src, IScriptEnvironment* env) : GenericVideoFilter(src) {

  if (!vi.IsYUY2())
    env->ThrowError("ConvertYUY2ToYV16: Only YUY2 is allowed as input");

  vi.pixel_type = VideoInfo::CS_YV16;

}

PVideoFrame __stdcall ConvertYUY2ToYV16::GetFrame(int n, IScriptEnvironment* env) {
  PVideoFrame src = child->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrame(vi);

  const BYTE* srcP = src->GetReadPtr();
  const int awidth = min(src->GetPitch()>>1, (vi.width+7) & -8);

  BYTE* dstY = dst->GetWritePtr(PLANAR_Y);
  BYTE* dstU = dst->GetWritePtr(PLANAR_U);
  BYTE* dstV = dst->GetWritePtr(PLANAR_V);

  if (!(awidth&7)) {  // Use MMX
    this->convYUV422to422(srcP, dstY, dstU, dstV, src->GetPitch(), dst->GetPitch(PLANAR_Y),
                          dst->GetPitch(PLANAR_U),  awidth, vi.height);
  }
  else {
    const int w = vi.width/2;

    for (int y=0; y<vi.height; y++) {
      for (int x=0; x<w; x++) {
        dstY[x*2]   = srcP[x*4+0];
        dstY[x*2+1] = srcP[x*4+2];
        dstU[x]     = srcP[x*4+1];
        dstV[x]     = srcP[x*4+3];
      }
      srcP += src->GetPitch();
      dstY += dst->GetPitch(PLANAR_Y);
      dstU += dst->GetPitch(PLANAR_U);
      dstV += dst->GetPitch(PLANAR_V);
    }
  }
  return dst;
}

void ConvertYUY2ToYV16::convYUV422to422(const unsigned char *src,
                                        unsigned char *py, unsigned char *pu, unsigned char *pv,
                                        int pitch1, int pitch2y, int pitch2uv, int width, int height)
{
	__asm
	{
        push ebx
		mov edi,[src]
		mov ebx,[py]
		mov edx,[pu]
		mov esi,[pv]
		pcmpeqw mm5,mm5
		mov ecx,width
		psrlw mm5,8            ; 0x00FF00FF00FF00FFi64
        shr ecx,1
		align 16
	yloop:
		xor eax,eax
		align 16
	xloop:
		movq mm0,[edi+eax*4]   ; VYUYVYUY - 1
		movq mm1,[edi+eax*4+8] ; VYUYVYUY - 2
		movq mm2,mm0           ; VYUYVYUY - 1
		movq mm3,mm1           ; VYUYVYUY - 2
		pand mm0,mm5           ; 0Y0Y0Y0Y - 1
		psrlw mm2,8 	       ; 0V0U0V0U - 1
		pand mm1,mm5           ; 0Y0Y0Y0Y - 2
		psrlw mm3,8            ; 0V0U0V0U - 2
		packuswb mm0,mm1       ; YYYYYYYY
		packuswb mm2,mm3       ; VUVUVUVU
		movq [ebx+eax*2],mm0   ; store y
		movq mm4,mm2           ; VUVUVUVU
		pand mm2,mm5           ; 0U0U0U0U
		psrlw mm4,8            ; 0V0V0V0V
		add eax,4
		packuswb mm2,mm2       ; xxxxUUUU
		cmp eax,ecx
		packuswb mm4,mm4       ; xxxxVVVV
		movd [edx+eax-4],mm2   ; store u
		movd [esi+eax-4],mm4   ; store v
		jl xloop
		add edi,pitch1
		add ebx,pitch2y
		add edx,pitch2uv
		add esi,pitch2uv
		dec height
		jnz yloop
		emms
        pop ebx
	}
}


AVSValue __cdecl ConvertYUY2ToYV16::Create(AVSValue args, void*, IScriptEnvironment* env) {
  PClip clip = args[0].AsClip();
  if (clip->GetVideoInfo().IsYV16())
    return clip;
  return new ConvertYUY2ToYV16(clip, env);
}

/************************************
 * YV16 to YUY2
 ************************************/

ConvertYV16ToYUY2::ConvertYV16ToYUY2(PClip src, IScriptEnvironment* env) : GenericVideoFilter(src) {

  if (!vi.IsYV16())
    env->ThrowError("ConvertYV16ToYUY2: Only YV16 is allowed as input");

  vi.pixel_type = VideoInfo::CS_YUY2;

}

PVideoFrame __stdcall ConvertYV16ToYUY2::GetFrame(int n, IScriptEnvironment* env) {
  PVideoFrame src = child->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrame(vi, 16);

  const BYTE* srcY = src->GetReadPtr(PLANAR_Y);
  const BYTE* srcU = src->GetReadPtr(PLANAR_U);
  const BYTE* srcV = src->GetReadPtr(PLANAR_V);
  const int awidth = min(src->GetPitch(PLANAR_Y), (vi.width+7) & -8);

  BYTE* dstp = dst->GetWritePtr();

  if (!(awidth&7)) {  // Use MMX
    this->conv422toYUV422(srcY, srcU, srcV, dstp,
      src->GetPitch(PLANAR_Y), src->GetPitch(PLANAR_U),
      dst->GetPitch(), awidth, vi.height);
  }
  else {
    const int w = vi.width/2;

    for (int y=0; y<vi.height; y++) {
      for (int x=0; x<w; x++) {
        dstp[x*4+0] = srcY[x*2];
        dstp[x*4+1] = srcU[x];
        dstp[x*4+2] = srcY[x*2+1];
        dstp[x*4+3] = srcV[x];
      }
      srcY += src->GetPitch(PLANAR_Y);
      srcU += src->GetPitch(PLANAR_U);
      srcV += src->GetPitch(PLANAR_V);
      dstp += dst->GetPitch();
    }
  }
  return dst;
}

void ConvertYV16ToYUY2::conv422toYUV422(const unsigned char *py, const unsigned char *pu, const unsigned char *pv,
                                        unsigned char *dst,
                                        int pitch1Y, int pitch1UV, int pitch2, int width, int height)
{
	__asm
	{
        push ebx
		mov ebx,[py]
		mov edx,[pu]
		mov esi,[pv]
		mov ecx,width
		mov edi,[dst]
        shr ecx,1
yloop:
		xor eax,eax
		align 16
xloop:
		movd mm1,[edx+eax]     ;0000UUUU
		movd mm2,[esi+eax]     ;0000VVVV
		movq mm0,[ebx+eax*2]   ;YYYYYYYY
		punpcklbw mm1,mm2      ;VUVUVUVU
		movq mm3,mm0           ;YYYYYYYY
		punpcklbw mm0,mm1      ;VYUYVYUY
		add eax,4
		punpckhbw mm3,mm1      ;VYUYVYUY
		movq [edi+eax*4-16],mm0 ;store
		cmp eax,ecx
		movq [edi+eax*4-8],mm3   ;store
		jl xloop
		add ebx,pitch1Y
		add edx,pitch1UV
		add esi,pitch1UV
		add edi,pitch2
		dec height
		jnz yloop
		emms
        pop ebx
	}
}

AVSValue __cdecl ConvertYV16ToYUY2::Create(AVSValue args, void*, IScriptEnvironment* env) {
  PClip clip = args[0].AsClip();
  if (clip->GetVideoInfo().IsYUY2())
    return clip;
  return new ConvertYV16ToYUY2(clip, env);
}

/**********************************************
 * Converter between arbitrary planar formats
 *
 * This uses plane copy for luma, and the
 * bicubic resizer for chroma (could be
 * customizable later)
 *
 * (c) Klaus Post, 2005
 * (c) Ian Brabham, 2011
 **********************************************/

inline float ChrOffset(bool point, int sIn, float dIn, int sOut, float dOut) {
      //     (1 - sOut/sIn)/2 + (dOut-dIn)/sIn; // Gavino Jan 2011
      return point ? (dOut-dIn)/sIn : 0.5f + (dOut-dIn - 0.5f*sOut)/sIn;
}

ConvertToPlanarGeneric::ConvertToPlanarGeneric(PClip src, int dst_space, bool interlaced,
                                               const AVSValue& InPlacement, const AVSValue& chromaResampler,
                                               const AVSValue& OutPlacement, IScriptEnvironment* env) : GenericVideoFilter(src) {
  Y8input = vi.IsY8();

  if (!Y8input) {

    if (! (vi.IsYV12() || dst_space == VideoInfo::CS_YV12))
      interlaced = false;  // Ignore, if YV12 is not involved.

    // Describe input pixel positioning
    float xdInU = 0.0f, txdInU = 0.0f, bxdInU = 0.0f;
    float ydInU = 0.0f, tydInU = 0.0f, bydInU = 0.0f;
    float xdInV = 0.0f, txdInV = 0.0f, bxdInV = 0.0f;
    float ydInV = 0.0f, tydInV = 0.0f, bydInV = 0.0f;

    if (vi.IsYV12()) {
      switch (getPlacement(InPlacement, env)) {
        case PLACEMENT_DV:
          ydInU = 0.0f, tydInU = 0.0f, bydInU = 0.5f;
          ydInV = 1.0f, tydInV = 0.5f, bydInV = 1.0f;
          break;
        case PLACEMENT_MPEG1:
          xdInU = 0.5f, txdInU = 0.5f, bxdInU = 0.5f;
          xdInV = 0.5f, txdInV = 0.5f, bxdInV = 0.5f;
          // fall thru
        case PLACEMENT_MPEG2:
          ydInU = 0.5f, tydInU = 0.25f, bydInU = 0.75f;
          ydInV = 0.5f, tydInV = 0.25f, bydInV = 0.75f;
          break;
      }
    }
    else if (InPlacement.Defined())
      env->ThrowError("Convert: Input ChromaPlacement only available with YV12 source.");

    const int xsIn = 1 << vi.GetPlaneWidthSubsampling(PLANAR_U);
    const int ysIn = 1 << vi.GetPlaneHeightSubsampling(PLANAR_U);

    vi.pixel_type = dst_space;

    // Describe output pixel positioning
    float xdOutU = 0.0f, txdOutU = 0.0f, bxdOutU = 0.0f;
    float ydOutU = 0.0f, tydOutU = 0.0f, bydOutU = 0.0f;
    float xdOutV = 0.0f, txdOutV = 0.0f, bxdOutV = 0.0f;
    float ydOutV = 0.0f, tydOutV = 0.0f, bydOutV = 0.0f;

    if (vi.IsYV12()) {
      switch (getPlacement(OutPlacement, env)) {
        case PLACEMENT_DV:
          ydOutU = 0.0f, tydOutU = 0.0f, bydOutU = 0.5f;
          ydOutV = 1.0f, tydOutV = 0.5f, bydOutV = 1.0f;
          break;
        case PLACEMENT_MPEG1:
          xdOutU = 0.5f, txdOutU = 0.5f, bxdOutU = 0.5f;
          xdOutV = 0.5f, txdOutV = 0.5f, bxdOutV = 0.5f;
          // fall thru
        case PLACEMENT_MPEG2:
          ydOutU = 0.5f, tydOutU = 0.25f, bydOutU = 0.75f;
          ydOutV = 0.5f, tydOutV = 0.25f, bydOutV = 0.75f;
          break;
      }
    }
    else if (OutPlacement.Defined())
      env->ThrowError("Convert: Output ChromaPlacement only available with YV12 output.");

    const int xsOut = 1 << vi.GetPlaneWidthSubsampling(PLANAR_U);
    const int xmask = xsOut - 1;
    if (vi.width & xmask)
      env->ThrowError("Convert: Cannot convert if width isn't mod%d!", xsOut);

    const int ysOut = 1 << vi.GetPlaneHeightSubsampling(PLANAR_U);
    const int ymask = ysOut - 1;
    if (vi.height & ymask)
      env->ThrowError("Convert: Cannot convert if height isn't mod%d!", ysOut);

    int uv_width  = vi.width  >> vi.GetPlaneWidthSubsampling(PLANAR_U);
    int uv_height = vi.height >> vi.GetPlaneHeightSubsampling(PLANAR_U);

    ResamplingFunction *filter = getResampler(chromaResampler.AsString("bicubic"), env);

    bool P = !lstrcmpi(chromaResampler.AsString(""), "point");

    if (interlaced) {
      uv_height /=  2;

      AVSValue tUsubSampling[4] = { ChrOffset(P, xsIn, txdInU, xsOut, txdOutU), ChrOffset(P, ysIn, tydInU, ysOut, tydOutU), AVSValue(), AVSValue() };
      AVSValue bUsubSampling[4] = { ChrOffset(P, xsIn, bxdInU, xsOut, bxdOutU), ChrOffset(P, ysIn, bydInU, ysOut, bydOutU), AVSValue(), AVSValue() };
      AVSValue tVsubSampling[4] = { ChrOffset(P, xsIn, txdInV, xsOut, txdOutV), ChrOffset(P, ysIn, tydInV, ysOut, tydOutV), AVSValue(), AVSValue() };
      AVSValue bVsubSampling[4] = { ChrOffset(P, xsIn, bxdInV, xsOut, bxdOutV), ChrOffset(P, ysIn, bydInV, ysOut, bydOutV), AVSValue(), AVSValue() };

      Usource = new SeparateFields(new AssumeParity(new SwapUVToY(child, SwapUVToY::UToY8, env), true), env);
      Vsource = new SeparateFields(new AssumeParity(new SwapUVToY(child, SwapUVToY::VToY8, env), true), env);

      PClip *tbUsource = new PClip[2]; // Interleave()::~Interleave() will delete these
      PClip *tbVsource = new PClip[2];

      tbUsource[0] = FilteredResize::CreateResize(new SelectEvery(Usource, 2, 0), uv_width, uv_height, tUsubSampling, filter, env);
      tbUsource[1] = FilteredResize::CreateResize(new SelectEvery(Usource, 2, 1), uv_width, uv_height, bUsubSampling, filter, env);
      tbVsource[0] = FilteredResize::CreateResize(new SelectEvery(Vsource, 2, 0), uv_width, uv_height, tVsubSampling, filter, env);
      tbVsource[1] = FilteredResize::CreateResize(new SelectEvery(Vsource, 2, 1), uv_width, uv_height, bVsubSampling, filter, env);

      Usource = new SelectEvery(new DoubleWeaveFields(new Interleave(2, tbUsource, env)), 2, 0);
      Vsource = new SelectEvery(new DoubleWeaveFields(new Interleave(2, tbVsource, env)), 2, 0);
    }
    else {
      AVSValue UsubSampling[4] = { ChrOffset(P, xsIn, xdInU, xsOut, xdOutU), ChrOffset(P, ysIn, ydInU, ysOut, ydOutU), AVSValue(), AVSValue() };
      AVSValue VsubSampling[4] = { ChrOffset(P, xsIn, xdInV, xsOut, xdOutV), ChrOffset(P, ysIn, ydInV, ysOut, ydOutV), AVSValue(), AVSValue() };

      Usource = FilteredResize::CreateResize(new SwapUVToY(child, SwapUVToY::UToY8, env), uv_width, uv_height, UsubSampling, filter, env);
      Vsource = FilteredResize::CreateResize(new SwapUVToY(child, SwapUVToY::VToY8, env), uv_width, uv_height, VsubSampling, filter, env);
    }
    delete filter;
  }
  else {
    vi.pixel_type = dst_space;
  }
}

PVideoFrame __stdcall ConvertToPlanarGeneric::GetFrame(int n, IScriptEnvironment* env) {
  PVideoFrame src = child->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrame(vi);

  env->BitBlt(dst->GetWritePtr(PLANAR_Y), dst->GetPitch(PLANAR_Y), src->GetReadPtr(PLANAR_Y), src->GetPitch(PLANAR_Y),
              src->GetRowSize(PLANAR_Y_ALIGNED), src->GetHeight(PLANAR_Y));
  if (Y8input) {
    memset(dst->GetWritePtr(PLANAR_U), 0x80, dst->GetHeight(PLANAR_U)*dst->GetPitch(PLANAR_U));
    memset(dst->GetWritePtr(PLANAR_V), 0x80, dst->GetHeight(PLANAR_V)*dst->GetPitch(PLANAR_V));
  } else {
    src = Usource->GetFrame(n, env);
    env->BitBlt(dst->GetWritePtr(PLANAR_U), dst->GetPitch(PLANAR_U), src->GetReadPtr(PLANAR_Y), src->GetPitch(PLANAR_Y),
                src->GetRowSize(PLANAR_Y_ALIGNED), dst->GetHeight(PLANAR_U));
    src = Vsource->GetFrame(n, env);
    env->BitBlt(dst->GetWritePtr(PLANAR_V), dst->GetPitch(PLANAR_V), src->GetReadPtr(PLANAR_Y), src->GetPitch(PLANAR_Y),
                src->GetRowSize(PLANAR_Y_ALIGNED), dst->GetHeight(PLANAR_V));
  }
  return dst;
}

AVSValue __cdecl ConvertToPlanarGeneric::CreateYV12(AVSValue args, void*, IScriptEnvironment* env) {
  PClip clip = args[0].AsClip();

  if (clip->GetVideoInfo().IsYV12()) {
    if (getPlacement(args[3], env) == getPlacement(args[5], env))
      return clip;
  }
  else if (clip->GetVideoInfo().IsRGB())
    clip = new ConvertRGBToYV24(clip, getMatrix(args[2].AsString(0), env), env);
  else if (clip->GetVideoInfo().IsYUY2())
    clip = new ConvertYUY2ToYV16(clip,  env);

  if (!clip->GetVideoInfo().IsPlanar())
    env->ThrowError("ConvertToYV12: Can only convert from Planar YUV.");

  // ConvertToPlanarGeneric will invoke 3 chains upon clip, cache it!
  clip = env->Invoke("Cache", AVSValue(clip)).AsClip();
  return new ConvertToPlanarGeneric(clip, VideoInfo::CS_YV12, args[1].AsBool(false), args[3], args[4], args[5], env);
}

AVSValue __cdecl ConvertToPlanarGeneric::CreateYV16(AVSValue args, void*, IScriptEnvironment* env) {
  PClip clip = args[0].AsClip();

  if (clip->GetVideoInfo().IsYV16())
    return clip;

  if (clip->GetVideoInfo().IsYUY2())
    return new ConvertYUY2ToYV16(clip,  env);

  if (clip->GetVideoInfo().IsRGB())
    clip = new ConvertRGBToYV24(clip, getMatrix(args[2].AsString(0), env), env);

  if (!clip->GetVideoInfo().IsPlanar())
    env->ThrowError("ConvertToYV16: Can only convert from Planar YUV.");

  // ConvertToPlanarGeneric will invoke 3 chains upon clip, cache it!
  clip = env->Invoke("Cache", AVSValue(clip)).AsClip();
  return new ConvertToPlanarGeneric(clip, VideoInfo::CS_YV16, args[1].AsBool(false), args[3], args[4], AVSValue(), env);
}

AVSValue __cdecl ConvertToPlanarGeneric::CreateYV24(AVSValue args, void*, IScriptEnvironment* env) {
  PClip clip = args[0].AsClip();

  if (clip->GetVideoInfo().IsYV24() )
    return clip;

  if (clip->GetVideoInfo().IsRGB())
    return new ConvertRGBToYV24(clip, getMatrix(args[2].AsString(0), env), env);

  if (clip->GetVideoInfo().IsYUY2())
    clip = new ConvertYUY2ToYV16(clip,  env);

  if (!clip->GetVideoInfo().IsPlanar())
    env->ThrowError("ConvertToYV24: Can only convert from Planar YUV.");

  // ConvertToPlanarGeneric will invoke 3 chains upon clip, cache it!
  clip = env->Invoke("Cache", AVSValue(clip)).AsClip();
  return new ConvertToPlanarGeneric(clip, VideoInfo::CS_YV24, args[1].AsBool(false), args[3], args[4], AVSValue(), env);
}

AVSValue __cdecl ConvertToPlanarGeneric::CreateYV411(AVSValue args, void*, IScriptEnvironment* env) {
  PClip clip = args[0].AsClip();

  if (clip->GetVideoInfo().IsYV411() )
    return clip;

  if (clip->GetVideoInfo().IsRGB())
    clip = new ConvertRGBToYV24(clip, getMatrix(args[2].AsString(0), env), env);
  else if (clip->GetVideoInfo().IsYUY2())
    clip = new ConvertYUY2ToYV16(clip,  env);

  if (!clip->GetVideoInfo().IsPlanar())
    env->ThrowError("ConvertToYV411: Can only convert from Planar YUV.");

  // ConvertToPlanarGeneric will invoke 3 chains upon clip, cache it!
  clip = env->Invoke("Cache", AVSValue(clip)).AsClip();
  return new ConvertToPlanarGeneric(clip, VideoInfo::CS_YV411, args[1].AsBool(false), args[3], args[4], AVSValue(), env);
}


static int getPlacement(const AVSValue& _placement, IScriptEnvironment* env) {
  const char* placement = _placement.AsString(0);

  if (placement) {
    if (!lstrcmpi(placement, "mpeg2"))
      return PLACEMENT_MPEG2;

    if (!lstrcmpi(placement, "mpeg1"))
      return PLACEMENT_MPEG1;

    if (!lstrcmpi(placement, "dv"))
      return PLACEMENT_DV;

    env->ThrowError("Convert: Unknown chromaplacement");
  }
  return PLACEMENT_MPEG2;
}


static ResamplingFunction* getResampler( const char* resampler, IScriptEnvironment* env) {
  if (resampler) {
    if      (!lstrcmpi(resampler, "point"))
      return new PointFilter();
    else if (!lstrcmpi(resampler, "bilinear"))
      return new TriangleFilter();
    else if (!lstrcmpi(resampler, "bicubic"))
      return new MitchellNetravaliFilter(1./3,1./3); // Parse out optional B= and C= from string
    else if (!lstrcmpi(resampler, "lanczos"))
      return new LanczosFilter(3); // Parse out optional Taps= from string
    else if (!lstrcmpi(resampler, "lanczos4"))
      return new LanczosFilter(4);
    else if (!lstrcmpi(resampler, "blackman"))
      return new BlackmanFilter(4);
    else if (!lstrcmpi(resampler, "spline16"))
      return new Spline16Filter();
    else if (!lstrcmpi(resampler, "spline36"))
      return new Spline36Filter();
    else if (!lstrcmpi(resampler, "spline64"))
      return new Spline64Filter();
    else if (!lstrcmpi(resampler, "gauss"))
      return new GaussianFilter(30.0); // Parse out optional P= from string
    else if (!lstrcmpi(resampler, "sinc"))
      return new SincFilter(4); // Parse out optional Taps= from string
    else
      env->ThrowError("Convert: Unknown chroma resampler, '%s'", resampler);
  }
  return new MitchellNetravaliFilter(1./3,1./3); // Default colorspace conversion for AviSynth
}
