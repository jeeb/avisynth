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

#include "resample.h"
#include <malloc.h>
#include <avs/config.h>
#include "../core/internal.h"

typedef __m128i (SSELoader)(const __m128i*);

__forceinline __m128i simd_load_aligned(const __m128i* adr) {
  return _mm_load_si128(adr);
}

__forceinline __m128i simd_load_unaligned(const __m128i* adr) {
  return _mm_loadu_si128(adr);
}

__forceinline __m128i simd_load_unaligned_sse3(const __m128i* adr) {
  return _mm_lddqu_si128(adr);
}

__forceinline __m128i simd_load_streaming(const __m128i* adr) {
  return _mm_stream_load_si128(const_cast<__m128i*>(adr));
}

__forceinline __m128i simd_load_movq(const __m128i* adr) {
  __m128i hi = _mm_loadl_epi64(adr+8);
  __m128i lo = _mm_loadl_epi64(adr);
  return _mm_or_si128(_mm_srli_si128(hi, 8), lo);
}

/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

extern const AVSFunction Resample_filters[] = {
  { "PointResize", "cii[src_left]f[src_top]f[src_width]f[src_height]f", FilteredResize::Create_PointResize },
  { "BilinearResize", "cii[src_left]f[src_top]f[src_width]f[src_height]f", FilteredResize::Create_BilinearResize },
  { "BicubicResize", "cii[b]f[c]f[src_left]f[src_top]f[src_width]f[src_height]f", FilteredResize::Create_BicubicResize },
  { "LanczosResize", "cii[src_left]f[src_top]f[src_width]f[src_height]f[taps]i", FilteredResize::Create_LanczosResize},
  { "Lanczos4Resize", "cii[src_left]f[src_top]f[src_width]f[src_height]f", FilteredResize::Create_Lanczos4Resize},
  { "BlackmanResize", "cii[src_left]f[src_top]f[src_width]f[src_height]f[taps]i", FilteredResize::Create_BlackmanResize},
  { "Spline16Resize", "cii[src_left]f[src_top]f[src_width]f[src_height]f", FilteredResize::Create_Spline16Resize},
  { "Spline36Resize", "cii[src_left]f[src_top]f[src_width]f[src_height]f", FilteredResize::Create_Spline36Resize},
  { "Spline64Resize", "cii[src_left]f[src_top]f[src_width]f[src_height]f", FilteredResize::Create_Spline64Resize},
  { "GaussResize", "cii[src_left]f[src_top]f[src_width]f[src_height]f[p]f", FilteredResize::Create_GaussianResize},
  { "SincResize", "cii[src_left]f[src_top]f[src_width]f[src_height]f[taps]i", FilteredResize::Create_SincResize},
  /**
    * Resize(PClip clip, dst_width, dst_height [src_left, src_top, src_width, int src_height,] )
    *
    * src_left et al.   =  when these optional arguments are given, the filter acts just like
    *                      a Crop was performed with those parameters before resizing, only faster
   **/

  { 0 }
};

/****************************************
 ***** Filtered Resize - Horizontal *****
 ***************************************/

// interleave 4 pixels together
void make_sse2_program(int* out, const int* in, int size) {
  int copy_size = *in+1;
  int sizeMod4 = (size/4)*4;

  *out++ = *in++; // copy filter size

  for (int i = 0; i < sizeMod4; i+=4) {
    for (int j = 0; j < copy_size; j++) {
      *out++ = *(in+copy_size*0+j);
      *out++ = *(in+copy_size*1+j);
      *out++ = *(in+copy_size*2+j);
      *out++ = *(in+copy_size*3+j);
    }
    in += copy_size*4;
  }

  // Leftover
  switch (sizeMod4 - size) {
  case 3:
    for (int j = 0; j < copy_size; j++) {
      *out++ = *(in+copy_size*0+j);
      *out++ = *(in+copy_size*1+j);
      *out++ = *(in+copy_size*2+j);
      *out++ = 0;
    }
    break;
  case 2:
    for (int j = 0; j < copy_size; j++) {
      *out++ = *(in+copy_size*0+j);
      *out++ = *(in+copy_size*1+j);
      *out++ = 0;
      *out++ = 0;
    }
    break;
  case 1:
    for (int j = 0; j < copy_size; j++) {
      *out++ = *(in+copy_size*0+j);
      *out++ = 0;
      *out++ = 0;
      *out++ = 0;
    }
    break;
  }

  //FILE* fp = fopen("debug-coeff.txt", "w");
  //for (int i = 0; i < size; i++) {
  //  for (int j = 0; j < 
}

FilteredResizeH::FilteredResizeH( PClip _child, double subrange_left, double subrange_width,
                                  int target_width, ResamplingFunction* func, IScriptEnvironment* env )
  : GenericVideoFilter(_child), tempY(0), tempUV(0),pattern_luma(0),pattern_chroma(0)
{
  original_width = vi.width;

  if (target_width<=0)
    env->ThrowError("Resize: Width must be greater than 0.");

  if (vi.IsYUV()) {
    if (vi.IsYUY2()) {
      if (target_width&1)
        env->ThrowError("Resize: YUY2 destination width must be even");

      tempUV = (BYTE*) _aligned_malloc(original_width*4+8+32, 64);  // aligned for cache line
    }
    else if (vi.IsPlanar() && !vi.IsY8()) {
      const int mask = (1 << vi.GetPlaneWidthSubsampling(PLANAR_U)) - 1;

      if (target_width & mask)
        env->ThrowError("Resize: Planar destination width must be a multiple of %d.", mask+1);
    }
    tempY = (BYTE*) _aligned_malloc(original_width*2+4+32, 64);   // aligned for cache line
#if 0
    if (vi.IsYUY2()) {
      pattern_chroma = func->GetResamplingPatternYUV(
                                          vi.width       >> 1,
                                          subrange_left  /  2,
                                          subrange_width /  2,
                                          target_width   >> 1,
                                          false, tempUV, env );
    }
    else if (vi.IsPlanar() && !vi.IsY8()) {
      const int shift = vi.GetPlaneWidthSubsampling(PLANAR_U);
      const int div   = 1 << shift;

      pattern_chroma = func->GetResamplingPatternYUV(
                                          vi.width       >> shift,
                                          subrange_left  /  div,
                                          subrange_width /  div,
                                          target_width   >> shift,
                                          true, tempY, env );
    }
    pattern_luma = func->GetResamplingPatternYUV(vi.width, subrange_left, subrange_width, target_width, true, tempY, env);
#endif
    if (!vi.IsY8()) {
      const int shift = vi.GetPlaneWidthSubsampling(PLANAR_U);
      const int div   = 1 << shift;
      pattern_chroma = func->GetResamplingPatternRGB(vi.width >> shift, subrange_left /  div, subrange_width /  div, target_width >> shift, env);
    }
    pattern_luma = func->GetResamplingPatternRGB(vi.width, subrange_left, subrange_width, target_width, env);
  }
  else
    pattern_luma = func->GetResamplingPatternRGB(vi.width, subrange_left, subrange_width, target_width, env);

  // Temporary stuff for planar resizing
  unpacked_tmp = (short*) _aligned_malloc(original_width*2+4+32, 64);

  // Preprocess SSE2 coefficient
  processed_pattern_luma = (int*) malloc(sizeof(int)*(target_width+4)*(*pattern_luma + 1) + sizeof(int));
  make_sse2_program(processed_pattern_luma, pattern_luma, target_width);

  vi.width = target_width;


  if (vi.IsPlanar())
  {
#ifdef X86_32
    try {
      assemblerY         = GenerateResizer(PLANAR_Y, false, env);
      assemblerY_aligned = GenerateResizer(PLANAR_Y, true,  env);
      if (!vi.IsY8()) {
        assemblerUV         = GenerateResizer(PLANAR_U, false, env);
        assemblerUV_aligned = GenerateResizer(PLANAR_U, true,  env);
      }
    }
    catch (const SoftWire::Error &err) {
       env->ThrowError("Resize: SoftWire exception : %s", err.getString());
    }
#else
  //TODO
  env->ThrowError("FilteredResizeH is not yet ported to 64-bit.");
#endif
  }
}

#ifdef X86_32
/***********************************
 * Dynamically Assembled Resampler
 *
 * (c) 2003, Klaus Post
 * (c) 2009, Ian Brabham
 *
 * Dynamic version of the Horizontal resizer
 *
 * The Algorithm is the same, except this
 *  one is able to process 6 pixels in parallel.
 * The inner loop filter is unrolled based on the
 *  exact filter size.
 * Too much code to workaround for the 6 pixels, and
 *  still not quite perfect. Though still faster than
 *  the original code.
 * New SSE2 unpacker does 64 bytes per cycle, many times
 *  faster, contributes approx +5% to overall speed.
 *
 * :TODO: SSE2 version of main code
 **********************************/


DynamicAssembledCode FilteredResizeH::GenerateResizer(int gen_plane, bool source_aligned, IScriptEnvironment* env) {
  __declspec(align(8)) static const __int64 FPround       = 0x0000200000002000; // 16384/2
  __declspec(align(8)) static const __int64 Mask2_pix     = 0x000000000000ffff;
  __declspec(align(8)) static const __int64 Mask1_pix_inv = 0xffffffffffffff00;
  __declspec(align(8)) static const __int64 Mask2_pix_inv = 0xffffffffffff0000;
  __declspec(align(8)) static const __int64 Mask3_pix_inv = 0xffffffffff000000;

  Assembler x86;   // This is the class that assembles the code.

  // Set up variables for this plane.
  int vi_height = vi.height >> vi.GetPlaneHeightSubsampling(gen_plane);
  int vi_dst_width = vi.width >> vi.GetPlaneWidthSubsampling(gen_plane);
  int vi_src_width = original_width >> vi.GetPlaneWidthSubsampling(gen_plane);

  int mod16_w = ((3+vi_src_width)/16);  // Src size!
  int mod16_remain = (3+vi_src_width-(mod16_w*16))/4;  //Src size!

  bool isse = !!(env->GetCPUFlags() & CPUF_INTEGER_SSE);
  bool sse2 = !!(env->GetCPUFlags() & CPUF_SSE2);
  bool sse3 = !!(env->GetCPUFlags() & CPUF_SSE3);
  bool sse4 = !!(env->GetCPUFlags() & CPUF_SSE4);

  if (source_aligned && !sse2) // No fast aligned version without SSE2+
    return DynamicAssembledCode();

  int prefetchevery = 2;
  if ((env->GetCPUFlags() & CPUF_3DNOW_EXT)||((env->GetCPUFlags() & CPUF_SSE2))) {
    // We have either an Athlon or a P4 with 64byte cacheline
    prefetchevery = 4;
  }

  bool unroll_fetch = false;
  // Unroll fetch loop on Athlon. P4 has a very small l1 cache, so unrolling will not give performance benefits here.
  if ((env->GetCPUFlags() & CPUF_3DNOW_EXT)) {
    unroll_fetch = true;
  }
  // We forcibly does not unroll fetch, if image width is more than 512
  if (vi_src_width > 512) {
    unroll_fetch = false;
  }

  if (!(vi_src_width && vi_dst_width && vi_height)) { // Skip
    x86.ret();
    return DynamicAssembledCode(x86, env, "ResizeH: ISSE code could not be compiled.");
  }

  int* array = (gen_plane == PLANAR_Y) ? pattern_luma : pattern_chroma;
  int fir_filter_size = array[0];
  int filter_offset=fir_filter_size*8+8;  // This is the length from one pixel pair to another
  int* cur_luma = array+2;

  int six_loops = (vi_dst_width-2)/6;  // How many loops can we do safely, with 6 pixels.

  // Store registers
  x86.push(eax);
  x86.push(ebx);
  x86.push(ecx);
  x86.push(edx);
  x86.push(esi);
  x86.push(edi);
  x86.push(ebp);

  // Initialize registers.
  x86.mov(eax,(int)&FPround);
  x86.pxor(mm6,mm6);  // Cleared mmx register - Not touched!
  if (sse2)
    x86.pxor(xmm6,xmm6);

  x86.movq(mm7, qword_ptr[eax]);  // Rounder for final division. Not touched!

  x86.mov(dword_ptr [&gen_h],vi_height);  // This is our y counter.

  x86.align(16);
  x86.label("yloop");

  x86.mov(esi, dword_ptr[&gen_srcp]);
  if (isse | sse2)
    x86.prefetchnta(dword_ptr [esi]);  //Prefetch current cache line

  x86.mov(eax,dword_ptr [&gen_dstp]);
  x86.mov(edi, dword_ptr[&tempY]);
  x86.mov(dword_ptr [&gen_temp_destp],eax);


  // Unpack source bytes to words in tempY buffer

  if (sse2) {
    int mod64_w = mod16_w / 4;
    int mod64_r = mod16_w % 4;
    if (!mod64_r && mod64_w) {
      mod64_w -= 1;
      mod64_r += 4;
    }

    if (mod64_w) {
      if (mod64_w > 1) {
        x86.mov(        ebx, mod64_w);
        x86.align(16);
        x86.label("fetch_loopback");
      }
      x86.prefetchnta(  dword_ptr [esi+64]);         //Prefetch one cache line ahead for single use
      if (source_aligned) {                          // Load source
        if (sse4) {
          x86.movntdqa( xmm0, xmmword_ptr[esi]);     // fetch aligned dq word for single use
          x86.movntdqa( xmm1, xmmword_ptr[esi+16]);
          x86.movntdqa( xmm2, xmmword_ptr[esi+32]);
          x86.movntdqa( xmm3, xmmword_ptr[esi+48]);
        } else {
          x86.movdqa(   xmm0, xmmword_ptr[esi]);     // fetch aligned dq word
          x86.movdqa(   xmm1, xmmword_ptr[esi+16]);
          x86.movdqa(   xmm2, xmmword_ptr[esi+32]);
          x86.movdqa(   xmm3, xmmword_ptr[esi+48]);
        }
      } else {
        if (sse3) {
          x86.lddqu(    xmm0, xmmword_ptr[esi]);     // fast fetch unaligned dq word
          x86.lddqu(    xmm1, xmmword_ptr[esi+16]);
          x86.lddqu(    xmm2, xmmword_ptr[esi+32]);
          x86.lddqu(    xmm3, xmmword_ptr[esi+48]);
        } else {
          x86.movdqu(   xmm0, xmmword_ptr[esi]);     // fetch unaligned dq word
          x86.movdqu(   xmm1, xmmword_ptr[esi+16]);
          x86.movdqu(   xmm2, xmmword_ptr[esi+32]);
          x86.movdqu(   xmm3, xmmword_ptr[esi+48]);
        }
      }                                              // Unpack bytes -> words
      x86.punpckhbw(    xmm4, xmm0);                 // xmm4 can contain junk, as this is cleared below.
      x86.punpckhbw(    xmm5, xmm1);
      x86.punpcklbw(    xmm0, xmm6);
      x86.punpcklbw(    xmm1, xmm6);
      x86.add(          esi, 64);
      x86.psrlw(        xmm4, 8);                    // This will also clear out the junk in
      x86.add(          edi, 128);                   // xmm4 that was there before the unpack
      x86.psrlw(        xmm5, 8);
      x86.movdqa(       xmmword_ptr[edi-128], xmm0);
      x86.movdqa(       xmmword_ptr[edi-112], xmm4);
      x86.movdqa(       xmmword_ptr[edi- 96], xmm1);
      x86.movdqa(       xmmword_ptr[edi- 80], xmm5);

      x86.punpckhbw(    xmm4, xmm2);
      x86.punpckhbw(    xmm5, xmm3);
      x86.punpcklbw(    xmm2, xmm6);
      x86.punpcklbw(    xmm3, xmm6);
      x86.psrlw(        xmm4, 8);
      if (mod64_w > 1)
        x86.dec(        ebx);

      x86.psrlw(        xmm5, 8);
      x86.movdqa(       xmmword_ptr[edi- 64], xmm2);
      x86.movdqa(       xmmword_ptr[edi- 48], xmm4);
      x86.movdqa(       xmmword_ptr[edi- 32], xmm3);
      x86.movdqa(       xmmword_ptr[edi- 16], xmm5);
      if (mod64_w > 1)
        x86.jnz(        "fetch_loopback");
    }

    if (source_aligned) {                            // Do the remainder
      if (sse4) {
        if (mod64_r > 0) x86.movntdqa( xmm0, xmmword_ptr[esi]);     // 16 pixels
        if (mod64_r > 1) x86.movntdqa( xmm1, xmmword_ptr[esi+16]);  // 32 pixels
        if (mod64_r > 2) x86.movntdqa( xmm2, xmmword_ptr[esi+32]);  // 48 pixels
        if (mod64_r > 3) x86.movntdqa( xmm3, xmmword_ptr[esi+48]);  // 64 pixels
      } else {
        if (mod64_r > 0) x86.movdqa(   xmm0, xmmword_ptr[esi]);
        if (mod64_r > 1) x86.movdqa(   xmm1, xmmword_ptr[esi+16]);
        if (mod64_r > 2) x86.movdqa(   xmm2, xmmword_ptr[esi+32]);
        if (mod64_r > 3) x86.movdqa(   xmm3, xmmword_ptr[esi+48]);
      }
    } else {
      if (sse3) {
        if (mod64_r > 0) x86.lddqu(    xmm0, xmmword_ptr[esi]);
        if (mod64_r > 1) x86.lddqu(    xmm1, xmmword_ptr[esi+16]);
        if (mod64_r > 2) x86.lddqu(    xmm2, xmmword_ptr[esi+32]);
        if (mod64_r > 3) x86.lddqu(    xmm3, xmmword_ptr[esi+48]);
      } else {
        if (mod64_r > 0) x86.movdqu(   xmm0, xmmword_ptr[esi]);
        if (mod64_r > 1) x86.movdqu(   xmm1, xmmword_ptr[esi+16]);
        if (mod64_r > 2) x86.movdqu(   xmm2, xmmword_ptr[esi+32]);
        if (mod64_r > 3) x86.movdqu(   xmm3, xmmword_ptr[esi+48]);
      }
    }
    if (mod64_r > 0) x86.punpckhbw(    xmm4, xmm0);
    if (mod64_r > 1) x86.punpckhbw(    xmm5, xmm1);
    if (mod64_r > 0) x86.punpcklbw(    xmm0, xmm6);
    if (mod64_r > 1) x86.punpcklbw(    xmm1, xmm6);
    if (mod64_r > 0) x86.psrlw(        xmm4, 8);
    if (mod64_r > 1) x86.psrlw(        xmm5, 8);
    if (mod64_r > 0) x86.movdqa(       xmmword_ptr[edi+  0], xmm0);
    if (mod64_r > 0) x86.movdqa(       xmmword_ptr[edi+ 16], xmm4);
    if (mod64_r > 1) x86.movdqa(       xmmword_ptr[edi+ 32], xmm1);
    if (mod64_r > 1) x86.movdqa(       xmmword_ptr[edi+ 48], xmm5);

    if (mod64_r > 2) x86.punpckhbw(    xmm4, xmm2);
    if (mod64_r > 3) x86.punpckhbw(    xmm5, xmm3);
    if (mod64_r > 2) x86.punpcklbw(    xmm2, xmm6);
    if (mod64_r > 3) x86.punpcklbw(    xmm3, xmm6);
    if (mod64_r > 2) x86.psrlw(        xmm4, 8);
    if (mod64_r > 3) x86.psrlw(        xmm5, 8);
    if (mod64_r > 2) x86.movdqa(       xmmword_ptr[edi+ 64], xmm2);
    if (mod64_r > 2) x86.movdqa(       xmmword_ptr[edi+ 80], xmm4);
    if (mod64_r > 3) x86.movdqa(       xmmword_ptr[edi+ 96], xmm3);
    if (mod64_r > 3) x86.movdqa(       xmmword_ptr[edi+112], xmm5);

    if (mod64_r > 0) x86.add(          esi, mod64_r*16);
    if (mod64_r > 0) x86.add(          edi, mod64_r*32);
  } else { // if (sse2)
    for (int i=0;i<mod16_w;i++) {
      if ((!(i%prefetchevery)) && ((prefetchevery+i)*16 < vi_src_width) && isse && unroll_fetch) {
         //Prefetch only once per cache line
        x86.prefetchnta(dword_ptr [esi+(prefetchevery*16)]);
      }
      if (!unroll_fetch) {  // Should we create a loop instead of unrolling?
        i = mod16_w;  // Jump out of loop
        x86.mov(ebx, mod16_w);
        x86.align(16);
        x86.label("fetch_loopback");
      }
      x86.movq(mm0, qword_ptr[esi]);        // Move pixels into mmx-registers
       x86.movq(mm1, qword_ptr[esi+8]);
      x86.movq(mm2,mm0);
       x86.punpcklbw(mm0,mm6);     // Unpack bytes -> words
      x86.movq(mm3,mm1);
       x86.punpcklbw(mm1,mm6);
      x86.add(esi,16);
       x86.punpckhbw(mm2,mm6);
      x86.add(edi,32);
       x86.punpckhbw(mm3,mm6);
      if (!unroll_fetch)   // Loop on if not unrolling
        x86.dec(ebx);

      x86.movq(qword_ptr[edi-32],mm0);        // Store unpacked pixels in temporary space.
      x86.movq(qword_ptr[edi+8-32],mm2);
      x86.movq(qword_ptr[edi+16-32],mm1);
      x86.movq(qword_ptr[edi+24-32],mm3);
    }
    if (!unroll_fetch)   // Loop on if not unrolling
      x86.jnz("fetch_loopback");
  }
  switch (mod16_remain) {
  case 3:
    x86.movq(mm0, qword_ptr[esi]);        // Move 12 pixels into mmx-registers
     x86.movd(mm1, dword_ptr[esi+8]);
    x86.movq(mm2,mm0);
     x86.punpcklbw(mm0,mm6);               // Unpack bytes -> words
    x86.punpckhbw(mm2,mm6);
     x86.punpcklbw(mm1,mm6);
    x86.movq(qword_ptr[edi],mm0);         // Store 12 unpacked pixels in temporary space.
     x86.movq(qword_ptr[edi+8],mm2);
    x86.movq(qword_ptr[edi+16],mm1);
    break;
  case 2:
    x86.movq(mm0, qword_ptr[esi]);        // Move 8 pixels into mmx-registers
    x86.movq(mm2,mm0);
     x86.punpcklbw(mm0,mm6);               // Unpack bytes -> words
    x86.punpckhbw(mm2,mm6);
     x86.movq(qword_ptr[edi],mm0);         // Store 8 unpacked pixels in temporary space.
    x86.movq(qword_ptr[edi+8],mm2);
    break;
  case 1:
    x86.movd(mm0,dword_ptr [esi]);        // Move 4 pixels into mmx-registers
    x86.punpcklbw(mm0,mm6);               // Unpack bytes -> words
    x86.movq(qword_ptr[edi],mm0);         // Store 4 unpacked pixels in temporary space.
    break;
  case 0:
    break;
  default:
    env->ThrowError("Resize: FilteredResizeH::GenerateResizer illegal state %d.", mod16_remain);  // Opps!
  }

  // Calculate destination pixels

  x86.mov(edi, (int)cur_luma);  // First there are offsets into the tempY planes, defining where the filter starts
                                // After that there is (filter_size) constants for multiplying.
                                // Next pixel pair is put after (filter_offset) bytes.

  if (six_loops) {       // Do we have at least 1 loops worth to do?
    if (six_loops > 1) { // Do we have more than 1 loop to do?
      x86.mov(dword_ptr [&gen_x],six_loops);
      x86.align(16);
      x86.label("xloop");
    }
    x86.mov(eax,dword_ptr [edi]);   // Move pointers of first pixel pair into registers
    x86.mov(ebx,dword_ptr [edi+4]);
    x86.mov(ecx,dword_ptr [edi+filter_offset]);     // Move pointers of next pixel pair into registers
    x86.mov(edx,dword_ptr [edi+filter_offset+4]);
    x86.movq(mm3,mm7);  // Start with rounder!
    x86.mov(esi,dword_ptr [edi+(filter_offset*2)]);   // Move pointers of next pixel pair into registers
    x86.movq(mm5,mm7);
    x86.mov(ebp,dword_ptr [edi+(filter_offset*2)+4]);
    x86.movq(mm4,mm7);
    x86.add(edi,8); // cur_luma++

    for (int i=0;i<fir_filter_size;i++) {       // Unroll filter inner loop based on the filter size.
        x86.movd(mm0, dword_ptr[eax+i*4]);
         x86.movd(mm1, dword_ptr[ecx+i*4]);
        x86.punpckldq(mm0, qword_ptr[ebx+i*4]);
         x86.punpckldq(mm1, qword_ptr[edx+i*4]);
        x86.pmaddwd(mm0, qword_ptr[edi+i*8]);
         x86.movd(mm2, dword_ptr[esi+i*4]);
        x86.pmaddwd(mm1,qword_ptr[edi+filter_offset+(i*8)]);
         x86.punpckldq(mm2, qword_ptr[ebp+i*4]);
        x86.paddd(mm3, mm0);
         x86.pmaddwd(mm2, qword_ptr[edi+(filter_offset*2)+(i*8)]);
        x86.paddd(mm4, mm1);
         x86.paddd(mm5, mm2);
    }
    x86.psrad(mm3,14);
     x86.mov(eax,dword_ptr[&gen_temp_destp]);
    x86.psrad(mm4,14);
     x86.add(dword_ptr [&gen_temp_destp],6);
    x86.psrad(mm5,14);
     x86.packssdw(mm3, mm4);       // [...3 ...2] [...1 ...0] => [.3 .2 .1 .0]
    x86.packssdw(mm5, mm6);        // [...z ...z] [...5 ...4] => [.z .z .5 .4]
     x86.add(edi,filter_offset*3-8);
    x86.packuswb(mm3, mm5);        // [.z .z .5 .4] [.3 .2 .1 .0] => [zz543210]
    if (six_loops > 1) {   // Do we have more than 1 loop to do?
       x86.dec(dword_ptr [&gen_x]);
      x86.movq(qword_ptr[eax],mm3);  // This was a potential 2 byte overwrite!
       x86.jnz("xloop");
    } else {
      x86.movq(qword_ptr[eax],mm3);  // This was a potential 2 byte overwrite!
    }
  }

  // Process any remaining pixels

//      vi_dst_width;                              1,2,3,4,5,6,7,8,9,A,B,C,D,E,F,10
//      vi_dst_width-2                            -1,0,1,2,3,4,5,6,7,8,9,A,B,C,D,E,F
//      six_loops = (vi_dst_width-2)/6;            0,0,0,0,0,0,0,1,1,1,1,1,1,2,2,2,2
  int remainx = vi_dst_width-(six_loops*6); //   1,2,3,4,5,6,7,2,3,4,5,6,7,2,3,4,5,6,7

  while (remainx>=4) {
    x86.mov(eax,dword_ptr [edi]);
    x86.mov(ebx,dword_ptr [edi+4]);
    x86.movq(mm3,mm7);  // Used for pix 1+2
    x86.mov(ecx,dword_ptr [edi+filter_offset]);
    x86.movq(mm4,mm7);  // Used for pix 3+4
    x86.mov(edx,dword_ptr [edi+filter_offset+4]);

    x86.add(edi,8); // cur_luma++
    for (int i=0;i<fir_filter_size;i++) {
      x86.movd(mm0, dword_ptr [eax+i*4]);
       x86.movd(mm1, dword_ptr [ecx+i*4]);
      x86.punpckldq(mm0, qword_ptr[ebx+i*4]);
       x86.punpckldq(mm1, qword_ptr[edx+i*4]);
      x86.pmaddwd(mm0, qword_ptr[edi+i*8]);
       x86.pmaddwd(mm1, qword_ptr[edi+filter_offset+(i*8)]);
      x86.paddd(mm3, mm0);
       x86.paddd(mm4, mm1);
    }
    x86.psrad(mm3,14);
    x86.psrad(mm4,14);
    x86.mov(eax,dword_ptr[&gen_temp_destp]);
    x86.packssdw(mm3, mm4);      // [...3 ...2] [...1 ...0] => [.3 .2 .1 .0]
    x86.packuswb(mm3, mm6);      // [.. .. .. ..] [.3 .2 .1 .0] => [....3210]

    x86.movd(dword_ptr[eax],mm3); 
    remainx -= 4;
    if (remainx) {
      x86.add(dword_ptr [&gen_temp_destp],4);
      x86.add(edi,filter_offset*2-8);
    }
  }
  if (remainx==3) {
    x86.mov(eax,dword_ptr [edi]);
    x86.movq(mm3,mm7);  // Used for pix 1+2
    x86.mov(ebx,dword_ptr [edi+4]);
    x86.movq(mm4,mm7);  // Used for pix 3
    x86.mov(ecx,dword_ptr [edi+filter_offset]);

    x86.add(edi,8); // cur_luma++
    for (int i=0;i<fir_filter_size;i++) {
      x86.movd(mm0, dword_ptr [eax+i*4]);
       x86.movd(mm1, dword_ptr [ecx+i*4]);
      x86.punpckldq(mm0, qword_ptr[ebx+i*4]);
       x86.pmaddwd(mm1, qword_ptr[edi+filter_offset+(i*8)]);
      x86.pmaddwd(mm0, qword_ptr[edi+i*8]);
       x86.paddd(mm4, mm1);
      x86.paddd(mm3, mm0);
    }
     x86.psrad(mm4,14);
    x86.psrad(mm3,14);
     x86.mov(eax,dword_ptr[&gen_temp_destp]);
    x86.packssdw(mm3, mm4);      // [...z ...2] [...1 ...0] => [.z .2 .1 .0]
     x86.movd(mm0,dword_ptr[eax]);
    x86.packuswb(mm3, mm6);      // [.. .. .. ..] [.z .2 .1 .0] => [....z210]
     x86.pand(mm0,qword_ptr[(int)&Mask3_pix_inv]);
    x86.por(mm3,mm0);
    
    x86.movd(dword_ptr[eax],mm3); 
    remainx = 0;
  }
  if (remainx==2) {
    x86.mov(eax,dword_ptr [edi]);
    x86.movq(mm3,mm7);  // Used for pix 1+2
    x86.mov(ebx,dword_ptr [edi+4]);

    x86.add(edi,8); // cur_luma++
    for (int i=0;i<fir_filter_size;i+=2) {
      const int j = i+1;
      if (j < fir_filter_size) {
        x86.movd(mm0, dword_ptr [eax+i*4]);
         x86.movd(mm1, dword_ptr [eax+j*4]);
        x86.punpckldq(mm0, qword_ptr[ebx+i*4]);
         x86.punpckldq(mm1, qword_ptr[ebx+j*4]);
        x86.pmaddwd(mm0, qword_ptr[edi+i*8]);
         x86.pmaddwd(mm1, qword_ptr[edi+j*8]);
        x86.paddd(mm3, mm0);
        x86.paddd(mm3, mm1);
      } else {
        x86.movd(mm0, dword_ptr [eax+i*4]);
        x86.punpckldq(mm0, qword_ptr[ebx+i*4]);
        x86.pmaddwd(mm0, qword_ptr[edi+i*8]);
        x86.paddd(mm3, mm0);
      }
    }
     x86.mov(eax,dword_ptr[&gen_temp_destp]);
    x86.psrad(mm3,14);
     x86.movd(mm0,dword_ptr[eax]);
    x86.packssdw(mm3, mm6);      // [...z ...z] [...1 ...0] => [.z .z .1 .0]
     x86.pand(mm0,qword_ptr[(int)&Mask2_pix_inv]);
    x86.packuswb(mm3, mm6);      // [.z .z .z .z] [.z .z .1 .0] => [zzzzzz10]
     x86.por(mm3,mm0);
     x86.movd(dword_ptr[eax],mm3); 
    remainx = 0;
  }
  if (remainx==1) {
    x86.mov(eax,dword_ptr [edi]);
    x86.movq(mm3,mm7);  // Used for pix 1

    x86.add(edi,8); // cur_luma++
    for (int i=0;i<fir_filter_size;i+=2) {
      const int j = i+1;
      if (j < fir_filter_size) {
        x86.movd(mm0, dword_ptr [eax+i*4]);
         x86.movd(mm1, dword_ptr [eax+j*4]);
        x86.pmaddwd(mm0, qword_ptr[edi+i*8]);
         x86.pmaddwd(mm1, qword_ptr[edi+j*8]);
        x86.paddd(mm3, mm0);
        x86.paddd(mm3, mm1);
      } else {
        x86.movd(mm0, dword_ptr [eax+i*4]);
        x86.pmaddwd(mm0, qword_ptr[edi+i*8]);
        x86.paddd(mm3, mm0);
      }
    }
     x86.mov(eax,dword_ptr[&gen_temp_destp]);
    x86.psrad(mm3,14);
     x86.movd(mm0,dword_ptr[eax]);
    x86.pand(mm3,qword_ptr[(int)&Mask2_pix]);
     x86.pand(mm0,qword_ptr[(int)&Mask1_pix_inv]);
    x86.packuswb(mm3, mm6);      // [.z .z .z .z] [.z .z .Z .0] => [zzzzzzZ0]
    x86.por(mm3,mm0);
    x86.movd(dword_ptr[eax],mm3); 
    remainx = 0;
  }

  // End remaining pixels

  x86.mov(eax,dword_ptr [&gen_src_pitch]);
  x86.mov(ebx,dword_ptr [&gen_dst_pitch]);
  x86.add(dword_ptr [&gen_srcp], eax);
  x86.add(dword_ptr [&gen_dstp], ebx);

  x86.dec(dword_ptr [&gen_h]);
  x86.jnz("yloop");
  // No more mmx for now
  x86.emms();
  // Restore registers
  x86.pop(ebp);
  x86.pop(edi);
  x86.pop(esi);
  x86.pop(edx);
  x86.pop(ecx);
  x86.pop(ebx);
  x86.pop(eax);
  x86.ret();

  return DynamicAssembledCode(x86, env, "ResizeH: ISSE code could not be compiled.");
}
#endif

void resize_h_c_plannar(BYTE* dst, const BYTE* src, int dst_pitch, int src_pitch, int* program, int width, int target_height) {
  int filter_size = *program;
  int* current = program+1;
  
  for (int x = 0; x < width; x++) {
    int begin = *current;
    current++;
    for (int y = 0; y < target_height; y++) {
      int result = 0;
      for (int i = 0; i < filter_size; i++) {
        result += (src+y*src_pitch)[(begin+i)] * current[i];
      }
      result = ((result+8192)/16384);
      result = result > 255 ? 255 : result < 0 ? 0 : result;
      (dst+y*dst_pitch)[x] = (BYTE)result;
    }
    current += filter_size;
  }
}

void resize_h_c_yuy2(BYTE* dst, const BYTE* src, int dst_pitch, int src_pitch, int* program, int* programUV, int width, int target_height) {
  int filter_size = *program;
  int* current = program+1;

  int filter_sizeUV = *programUV;
  int* currentUV = programUV+1;
  
  for (int x = 0; x < width; x++) {
    int begin = *current;
    current++;

    int beginUV = *currentUV;
    currentUV++;

    int chroma = x%2 ? 3 : 1;

    for (int y = 0; y < target_height; y++) {
      // Y resizing
      int result = 0;
      for (int i = 0; i < filter_size; i++) {
        result += (src+y*src_pitch)[(begin+i)*2] * current[i];
      }
      result = ((result+8192)/16384);
      result = result > 255 ? 255 : result < 0 ? 0 : result;
      (dst+y*dst_pitch)[x*2] = (BYTE)result;

      // UV resizing
      result = 0;
      for (int i = 0; i < filter_sizeUV; i++) {
        result += (src+y*src_pitch)[(beginUV+i)*4+chroma] * currentUV[i];
      }
      result = ((result+8192)/16384);
      result = result > 255 ? 255 : result < 0 ? 0 : result;
      (dst+y*dst_pitch)[x*2+1] = (BYTE)result;
    }

    current += filter_size;

    if (x%2) // == 1
      currentUV += filter_sizeUV;
    else
      currentUV--;
  }
}

template <int size>
void resize_h_c_rgb(BYTE* dst, const BYTE* src, int dst_pitch, int src_pitch, int* program, int width, int target_height) {
  int filter_size = *program;
  int* current = program+1;
  
  for (int x = 0; x < width; x++) {
    int begin = *current;
    current++;
    for (int k = 0; k < size; k++) {
      for (int y = 0; y < target_height; y++) {
        int result = 0;
        for (int i = 0; i < filter_size; i++) {
          result += (src+y*src_pitch)[(begin+i)*size+k] * current[i];
        }
        result = ((result+8192)/16384);
        result = result > 255 ? 255 : result < 0 ? 0 : result;
        (dst+y*dst_pitch)[x*size+k] = (BYTE)result;
      }
    }
    current += filter_size;
  }
}

void resize_h_sse2_planar(BYTE* dst, const BYTE* src, int dst_pitch, int src_pitch, int* program, int width, int target_height) {
  int filter_size = *program;
  int* current = program+1;
  
  int sizeMod2 = (filter_size/2)*2;
  //sizeMod2 = 0;

  __m128i zero = _mm_setzero_si128();

  for (int y = 0; y < target_height; y++) {
    // reset program pointer
    current = program+1;
    
    for (int x = 0; x < width; x+=4) {
      __m128i result = _mm_set1_epi32(8192);
      
      int* begin = current;
      current += 4;

      for (int i = 0; i < sizeMod2; i+=2) {
        // Load
        __m128i pixel1 = _mm_cvtsi32_si128(*reinterpret_cast<const int*>(src+*(begin+0)+i));   // 00 00 00 00 00 00 XX Aa
        __m128i pixel2 = _mm_cvtsi32_si128(*reinterpret_cast<const int*>(src+*(begin+1)+i));   // 00 00 00 00 00 00 XX Bb
        __m128i pixel3 = _mm_cvtsi32_si128(*reinterpret_cast<const int*>(src+*(begin+2)+i));   // 00 00 00 00 00 00 XX Cc
        __m128i pixel4 = _mm_cvtsi32_si128(*reinterpret_cast<const int*>(src+*(begin+3)+i));   // 00 00 00 00 00 00 XX Dd

        // Interleave
        __m128i t1   = _mm_unpacklo_epi16(pixel1, pixel2);                               // 00 00 00 00 XX XX Bb Aa
        __m128i t2   = _mm_unpacklo_epi16(pixel3, pixel4);                               // 00 00 00 00 XX XX Dd Cc
        __m128i data = _mm_unpacklo_epi32(t1, t2);                                       // XX XX XX XX Dd Cc Bb Aa

        // Unpack
        data = _mm_unpacklo_epi8(data, zero);                                            // 0D 0d 0C 0c 0B 0b 0A 0a

        // Load coefficient
        __m128i coeff1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(current));     // S0 c4 S0 c3 S0 c2 S0 c1 (epi32)
        __m128i coeff2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(current+4));   // S0 C4 S0 C3 S0 C2 S0 C1 (epi32)
                coeff1 = _mm_packs_epi32(coeff1, zero);                                  // 00 00 00 00 c4 c3 c2 c1
                coeff2 = _mm_packs_epi32(coeff2, zero);                                  // 00 00 00 00 C4 C3 C2 C1
        __m128i coeff  = _mm_unpacklo_epi16(coeff1, coeff2);                             // C4 c4 C3 c3 C2 c2 C1 c1

        // Multiply
        __m128i res = _mm_madd_epi16(data, coeff);

        // Add result
        result = _mm_add_epi32(result, res);

        // Move to next coefficient
        current += 8;
      }
      
      // Leftover
      if (sizeMod2 != filter_size) {
      //for (int i = sizeMod2; i < filter_size; i++) {
        __m128i pixel[4];

        // Load
        for (int k = 0; k < 4; k++) {
          pixel[k] = _mm_cvtsi32_si128(*reinterpret_cast<const int*>(src+*(begin+k)+sizeMod2));   // 00 00 00 00 00 00 XX Xa
        }

        // Interleave
        __m128i t1   = _mm_unpacklo_epi8(pixel[0], pixel[1]);                            // 00 00 00 00 XX XX XX ba
        __m128i t2   = _mm_unpacklo_epi8(pixel[2], pixel[3]);                            // 00 00 00 00 XX XX XX dc
        __m128i data = _mm_unpacklo_epi16(t1, t2);                                       // XX XX XX XX XX XX dc ba

        // Unpack
        data = _mm_unpacklo_epi8(data, zero);                                            // 00 00 00 00 0d 0c 0b 0a
        data = _mm_unpacklo_epi16(data, zero);                                           // 00 0d 00 0c 00 0b 00 0a

        // Load coefficient
        __m128i coeff1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(current));     // S0 c4 S0 c3 S0 c2 S0 c1 (epi32)
        __m128i coeff  = _mm_and_si128(coeff1, _mm_set1_epi32(0x0000FFFF));              // 00 c4 00 c3 00 c2 00 c1

        // Multiply
        __m128i res = _mm_madd_epi16(data, coeff);

        // Add result
        result = _mm_add_epi32(result, res);

        // Move to next coefficient (in this case start of another quadpixel)
        current += 4;
      }

      // Pack and store result
      result = _mm_srai_epi32(result, 14); // Devided by FPRound (16384)
      result = _mm_packs_epi32(result, zero);
      result = _mm_packus_epi16(result, zero);

      *(reinterpret_cast<int*>(dst+x)) = _mm_cvtsi128_si32(result);
    }

    src += src_pitch;
    dst += dst_pitch;
  }
}

__forceinline void resize_h_dispatcher() {
}

PVideoFrame __stdcall FilteredResizeH::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame src = child->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrame(vi);
#if 1
  const BYTE* srcp = src->GetReadPtr();
  BYTE* dstp = dst->GetWritePtr();
  int src_pitch = src->GetPitch();
  int dst_pitch = dst->GetPitch();

  if (vi.IsPlanar()) {
    // Plane Y resizing
    //resize_h_c_plannar(dstp, srcp, dst_pitch, src_pitch, pattern_luma, vi.width, vi.height);
    resize_h_sse2_planar(dstp, srcp, dst_pitch, src_pitch, processed_pattern_luma, vi.width, vi.height);
    if (!vi.IsY8()) {
      int width = vi.width >> vi.GetPlaneWidthSubsampling(PLANAR_U);
      int height = vi.height >> vi.GetPlaneHeightSubsampling(PLANAR_U);

      // Plane U resizing
      src_pitch = src->GetPitch(PLANAR_U);
      dst_pitch = dst->GetPitch(PLANAR_U);
      srcp = src->GetReadPtr(PLANAR_U);
      dstp = dst->GetWritePtr(PLANAR_U);
      resize_h_c_plannar(dstp, srcp, dst_pitch, src_pitch, pattern_chroma, width, height);

      // Plane V resizing
      src_pitch = src->GetPitch(PLANAR_V);
      dst_pitch = dst->GetPitch(PLANAR_V);
      srcp = src->GetReadPtr(PLANAR_V);
      dstp = dst->GetWritePtr(PLANAR_V);
      resize_h_c_plannar(dstp, srcp, dst_pitch, src_pitch, pattern_chroma, width, height);
    }
  } else if (vi.IsYUY2()) {
    resize_h_c_yuy2(dstp, srcp, dst_pitch, src_pitch, pattern_luma, pattern_chroma, vi.width, vi.height);
  } else if (vi.IsRGB24()) {
    resize_h_c_rgb<3>(dstp, srcp, dst_pitch, src_pitch, pattern_luma, vi.width, vi.height);
  } else if (vi.IsRGB32()) {
    resize_h_c_rgb<4>(dstp, srcp, dst_pitch, src_pitch, pattern_luma, vi.width, vi.height);
  } else {
    env->ThrowError("Resize: Unsupport pixel type.");
  }

#endif

#if 0
#ifdef X86_32
  const BYTE* srcp = src->GetReadPtr();
  BYTE* dstp = dst->GetWritePtr();
  int src_pitch = src->GetPitch();
  int dst_pitch = dst->GetPitch();
  if (vi.IsPlanar()) {
    int plane = 0;
    gen_src_pitch = src_pitch;
    gen_dst_pitch = dst_pitch;
    gen_srcp = (BYTE*)srcp;
    gen_dstp = dstp;
    if (((int)gen_srcp & 15) || (gen_src_pitch & 15) || !assemblerY_aligned)
      assemblerY.Call();
    else 
      assemblerY_aligned.Call();

    if (src->GetRowSize(PLANAR_U)) {  // Y8 is finished here
      gen_src_pitch = src->GetPitch(PLANAR_U);
      gen_dst_pitch = dst->GetPitch(PLANAR_U);

      gen_srcp = (BYTE*)src->GetReadPtr(PLANAR_U);
      gen_dstp = dst->GetWritePtr(PLANAR_U);
      if (((int)gen_srcp & 15) || (gen_src_pitch & 15) || !assemblerUV_aligned)
        assemblerUV.Call();
      else 
        assemblerUV_aligned.Call();

      gen_srcp = (BYTE*)src->GetReadPtr(PLANAR_V);
      gen_dstp = dst->GetWritePtr(PLANAR_V);
      if (((int)gen_srcp & 15) || (gen_src_pitch & 15) || !assemblerUV_aligned)
        assemblerUV.Call();
      else 
        assemblerUV_aligned.Call();
    }
    return dst;
  } else
  if (vi.IsYUY2())
  {
    int fir_filter_size_luma = pattern_luma[0];
    int fir_filter_size_chroma = pattern_chroma[0];
    static const __int64 x0000000000FF00FF = 0x0000000000FF00FF;
    static const __int64 xFFFF0000FFFF0000 = 0xFFFF0000FFFF0000;
    static const __int64 FPround =           0x0000200000002000;  // 16384/2

    __asm {
      pxor        mm0, mm0
      movq        mm7, x0000000000FF00FF
      movq        mm6, FPround
      movq        mm5, xFFFF0000FFFF0000
    }
    if (env->GetCPUFlags() & CPUF_INTEGER_SSE) {
     for (int y=0; y<vi.height; ++y)
      {
        int* cur_luma = pattern_luma+2;
        int* cur_chroma = pattern_chroma+2;
        int x = vi.width / 2;

        __asm {
	     	push ebx    // stupid compiler forgets to save ebx!!
        mov         edi, this
        mov         ecx, [edi].original_width
        mov         edx, [edi].tempY
        mov         ebx, [edi].tempUV
        mov         esi, srcp
        mov         eax, -1
      // deinterleave current line
        align 16
      i_deintloop:
        prefetchnta [esi+256]
        movd        mm1, [esi]          ;mm'1 = 00 00 VY UY
        inc         eax
        movq        mm2, mm1
        punpcklbw   mm2, mm0            ;mm2 = 0V 0Y 0U 0Y
        pand        mm1, mm7            ;mm1 = 00 00 0Y 0Y
        movd        [edx+eax*4], mm1
        psrld       mm2, 16             ;mm2 = 00 0V 00 0U
        add         esi, 4
        movq        [ebx+eax*8], mm2
        sub         ecx, 2
        jnz         i_deintloop
      // use this as source from now on
        mov         eax, cur_luma
        mov         ebx, cur_chroma
        mov         edx, dstp
        align 16
      i_xloopYUV:
        mov         esi, [eax]          ;esi=&tempY[ofs0]
        movq        mm1, mm0
        mov         edi, [eax+4]        ;edi=&tempY[ofs1]
        movq        mm3, mm0
        mov         ecx, fir_filter_size_luma
        add         eax, 8              ;cur_luma++
        align 16
      i_aloopY:
        // Identifiers:
        // Ya, Yb: Y values in srcp[ofs0]
        // Ym, Yn: Y values in srcp[ofs1]
        movd        mm2, [esi]          ;mm2 =  0| 0|Yb|Ya
        add         esi, 4
        punpckldq   mm2, [edi]          ;mm2 = Yn|Ym|Yb|Ya
                                        ;[eax] = COn|COm|COb|COa
        add         edi, 4
        pmaddwd     mm2, [eax]          ;mm2 = Y1|Y0 (DWORDs)
        add         eax, 8              ;cur_luma++
        dec         ecx
        paddd       mm1, mm2            ;accumulate
        jz         out_i_aloopY
//unroll1
        movd        mm2, [esi]          ;mm2 =  0| 0|Yb|Ya
        add         esi, 4
        punpckldq   mm2, [edi]          ;mm2 = Yn|Ym|Yb|Ya
                                        ;[eax] = COn|COm|COb|COa
        add         edi, 4
        pmaddwd     mm2, [eax]          ;mm2 = Y1|Y0 (DWORDs)
        add         eax, 8              ;cur_luma++
        dec         ecx
        paddd       mm1, mm2            ;accumulate
        jz         out_i_aloopY
//unroll2
        movd        mm2, [esi]          ;mm2 =  0| 0|Yb|Ya
        add         esi, 4
        punpckldq   mm2, [edi]          ;mm2 = Yn|Ym|Yb|Ya
                                        ;[eax] = COn|COm|COb|COa
        add         edi, 4
        pmaddwd     mm2, [eax]          ;mm2 = Y1|Y0 (DWORDs)
        add         eax, 8              ;cur_luma++
        dec         ecx
        paddd       mm1, mm2            ;accumulate
        jz         out_i_aloopY
//unroll3
        movd        mm2, [esi]          ;mm2 =  0| 0|Yb|Ya
        add         esi, 4
        punpckldq   mm2, [edi]          ;mm2 = Yn|Ym|Yb|Ya
                                        ;[eax] = COn|COm|COb|COa
        add         edi, 4
        pmaddwd     mm2, [eax]          ;mm2 = Y1|Y0 (DWORDs)
        add         eax, 8              ;cur_luma++
        dec         ecx
        paddd       mm1, mm2            ;accumulate
        jz         out_i_aloopY
//unroll4
        movd        mm2, [esi]          ;mm2 =  0| 0|Yb|Ya
        add         esi, 4
        punpckldq   mm2, [edi]          ;mm2 = Yn|Ym|Yb|Ya
                                        ;[eax] = COn|COm|COb|COa
        add         edi, 4
        pmaddwd     mm2, [eax]          ;mm2 = Y1|Y0 (DWORDs)
        add         eax, 8              ;cur_luma++
        dec         ecx
        paddd       mm1, mm2            ;accumulate
        jz         out_i_aloopY
//unroll5
        movd        mm2, [esi]          ;mm2 =  0| 0|Yb|Ya
        add         esi, 4
        punpckldq   mm2, [edi]          ;mm2 = Yn|Ym|Yb|Ya
                                        ;[eax] = COn|COm|COb|COa
        add         edi, 4
        pmaddwd     mm2, [eax]          ;mm2 = Y1|Y0 (DWORDs)
        add         eax, 8              ;cur_luma++
        dec         ecx
        paddd       mm1, mm2            ;accumulate
        jz         out_i_aloopY
//unroll6
        movd        mm2, [esi]          ;mm2 =  0| 0|Yb|Ya
        add         esi, 4
        punpckldq   mm2, [edi]          ;mm2 = Yn|Ym|Yb|Ya
                                        ;[eax] = COn|COm|COb|COa
        add         edi, 4
        pmaddwd     mm2, [eax]          ;mm2 = Y1|Y0 (DWORDs)
        add         eax, 8              ;cur_luma++
        dec         ecx
        paddd       mm1, mm2            ;accumulate
        jz         out_i_aloopY
//unroll7
        movd        mm2, [esi]          ;mm2 =  0| 0|Yb|Ya
        add         esi, 4
        punpckldq   mm2, [edi]          ;mm2 = Yn|Ym|Yb|Ya
                                        ;[eax] = COn|COm|COb|COa
        add         edi, 4
        pmaddwd     mm2, [eax]          ;mm2 = Y1|Y0 (DWORDs)
        add         eax, 8              ;cur_luma++
        dec         ecx
        paddd       mm1, mm2            ;accumulate
        jnz         i_aloopY
        align 16
out_i_aloopY:
        mov         esi, [ebx]          ;esi=&tempUV[ofs]
        add         ebx, 8              ;cur_chroma++
        mov         ecx, fir_filter_size_chroma
        align 16
      i_aloopUV:
        movq        mm2, [esi]          ;mm2 = 0|V|0|U
                                        ;[ebx] = 0|COv|0|COu
        add         esi, 8
        pmaddwd     mm2, [ebx]          ;mm2 = V|U (DWORDs)
        add         ebx, 8              ;cur_chroma++
        dec         ecx
        paddd       mm3, mm2            ;accumulate
        jz         out_i_aloopUV
//unroll1
        movq        mm2, [esi]          ;mm2 = 0|V|0|U
                                        ;[ebx] = 0|COv|0|COu
        add         esi, 8
        pmaddwd     mm2, [ebx]          ;mm2 = V|U (DWORDs)
        add         ebx, 8              ;cur_chroma++
        dec         ecx
        paddd       mm3, mm2            ;accumulate
        jz         out_i_aloopUV
//unroll2
        movq        mm2, [esi]          ;mm2 = 0|V|0|U
                                        ;[ebx] = 0|COv|0|COu
        add         esi, 8
        pmaddwd     mm2, [ebx]          ;mm2 = V|U (DWORDs)
        add         ebx, 8              ;cur_chroma++
        dec         ecx
        paddd       mm3, mm2            ;accumulate
        jz         out_i_aloopUV
//unroll3
        movq        mm2, [esi]          ;mm2 = 0|V|0|U
                                        ;[ebx] = 0|COv|0|COu
        add         esi, 8
        pmaddwd     mm2, [ebx]          ;mm2 = V|U (DWORDs)
        add         ebx, 8              ;cur_chroma++
        dec         ecx
        paddd       mm3, mm2            ;accumulate
        jz         out_i_aloopUV
//unroll4
        movq        mm2, [esi]          ;mm2 = 0|V|0|U
                                        ;[ebx] = 0|COv|0|COu
        add         esi, 8
        pmaddwd     mm2, [ebx]          ;mm2 = V|U (DWORDs)
        add         ebx, 8              ;cur_chroma++
        dec         ecx
        paddd       mm3, mm2            ;accumulate
        jz         out_i_aloopUV
//unroll5
        movq        mm2, [esi]          ;mm2 = 0|V|0|U
                                        ;[ebx] = 0|COv|0|COu
        add         esi, 8
        pmaddwd     mm2, [ebx]          ;mm2 = V|U (DWORDs)
        add         ebx, 8              ;cur_chroma++
        dec         ecx
        paddd       mm3, mm2            ;accumulate
        jz         out_i_aloopUV
//unroll6
        movq        mm2, [esi]          ;mm2 = 0|V|0|U
                                        ;[ebx] = 0|COv|0|COu
        add         esi, 8
        pmaddwd     mm2, [ebx]          ;mm2 = V|U (DWORDs)
        add         ebx, 8              ;cur_chroma++
        dec         ecx
        paddd       mm3, mm2            ;accumulate
        jz         out_i_aloopUV
//unroll7
        movq        mm2, [esi]          ;mm2 = 0|V|0|U
                                        ;[ebx] = 0|COv|0|COu
        add         esi, 8
        pmaddwd     mm2, [ebx]          ;mm2 = V|U (DWORDs)
        add         ebx, 8              ;cur_chroma++
        dec         ecx
        paddd       mm3, mm2            ;accumulate
        jnz         i_aloopUV
        align 16
out_i_aloopUV:
        paddd       mm3, mm6            ; V| V| U| U  (round)
         paddd       mm1, mm6            ;Y1|Y1|Y0|Y0  (round)
        pslld       mm3, 2              ; Shift up from 14 bits fraction to 16 bit fraction
         pxor        mm4,mm4             ;Clear mm4 - utilize shifter stall
        psrad       mm1, 14             ;mm1 = --y1|--y0
        pmaxsw      mm1,mm4             ;Clamp at 0
        pand        mm3, mm5            ;mm3 = v| 0|u| 0
        por         mm3,mm1
        packuswb    mm3, mm3            ;mm3 = ...|v|y1|u|y0
        movd        [edx], mm3
        add         edx, 4
        dec         x
        jnz         i_xloopYUV
		  pop ebx
        }
        srcp += src_pitch;
        dstp += dst_pitch;
      }
    } else {  // MMX
      for (int y=0; y<vi.height; ++y)
      {
        int* cur_luma = pattern_luma+2;
        int* cur_chroma = pattern_chroma+2;
        int x = vi.width / 2;

        __asm {
		push ebx    // stupid compiler forgets to save ebx!!
        mov         edi, this
        mov         ecx, [edi].original_width
        mov         edx, [edi].tempY
        mov         ebx, [edi].tempUV
        mov         esi, srcp
        mov         eax, -1
      // deinterleave current line
        align 16
      deintloop:
        inc         eax
        movd        mm1, [esi]          ;mm1 = 0000VYUY
        movq        mm2, mm1
        punpcklbw   mm2, mm0            ;mm2 = 0V0Y0U0Y
         pand        mm1, mm7            ;mm1 = 00000Y0Y
        movd        [edx+eax*4], mm1
         psrld       mm2, 16             ;mm2 = 000V000U
        add         esi, 4
        movq        [ebx+eax*8], mm2
        sub         ecx, 2
        jnz         deintloop
      // use this as source from now on
        mov         eax, cur_luma
        mov         ebx, cur_chroma
        mov         edx, dstp
        align 16
      xloopYUV:
        mov         esi, [eax]          ;esi=&tempY[ofs0]
        movq        mm1, mm0
        mov         edi, [eax+4]        ;edi=&tempY[ofs1]
        movq        mm3, mm0
        mov         ecx, fir_filter_size_luma
        add         eax, 8              ;cur_luma++
        align 16
      aloopY:
        // Identifiers:
        // Ya, Yb: Y values in srcp[ofs0]
        // Ym, Yn: Y values in srcp[ofs1]
        movd        mm2, [esi]          ;mm2 =  0| 0|Yb|Ya
        add         esi, 4
        punpckldq   mm2, [edi]          ;mm2 = Yn|Ym|Yb|Ya
                                        ;[eax] = COn|COm|COb|COa
        add         edi, 4
        pmaddwd     mm2, [eax]          ;mm2 = Y1|Y0 (DWORDs)
        add         eax, 8              ;cur_luma++
        dec         ecx
        paddd       mm1, mm2            ;accumulate
        jnz         aloopY

        mov         esi, [ebx]          ;esi=&tempUV[ofs]
        add         ebx, 8              ;cur_chroma++
        mov         ecx, fir_filter_size_chroma
        align 16
      aloopUV:
        movq        mm2, [esi]          ;mm2 = 0|V|0|U
                                        ;[ebx] = 0|COv|0|COu
        add         esi, 8
        pmaddwd     mm2, [ebx]          ;mm2 = V|U (DWORDs)
        add         ebx, 8              ;cur_chroma++
        dec         ecx
        paddd       mm3, mm2            ;accumulate
        jnz         aloopUV
         paddd       mm1, mm6           ; Y1|Y1|Y0|Y0  (round)
        paddd       mm3, mm6            ; V| V| U| U  (round)
         psrad       mm1, 14            ; mm1 = 0|y1|0|y0
         pslld       mm3, 2             ; Shift up from 14 bits fraction to 16 bit fraction
        movq        mm4, mm1
         psrad       mm1, 31            ; sign extend right
        pand        mm3, mm5            ; mm3 = v| 0|u| 0
         pandn       mm1, mm4           ; clip luma at 0
         por         mm3, mm1
        add         edx, 4
         packuswb    mm3, mm3            ; mm3 = ...|v|y1|u|y0
        dec         x
        movd        [edx-4], mm3
        jnz         xloopYUV
		    pop ebx
        }
        srcp += src_pitch;
        dstp += dst_pitch;
      }
    }
    __asm { emms }
  }
  else
  if (vi.IsRGB24())
  {
    // RGB24 is not recommended. 75% of all pixels are not aligned.
    int y = vi.height;
    int w = vi.width * 3;
    int fir_filter_size = pattern_luma[0];
    int* pattern_lumaP1 = pattern_luma+1 - fir_filter_size;
    static const __int64 xFF000000 = 0xFF000000;
    static const __int64 FPround   = 0x0000200000002000;  // 16384/2
    __asm {
	  push        ebx
      mov         esi, srcp
      mov         edi, dstp
      pxor        mm2, mm2
      movq        mm4, xFF000000
      align 16
    yloop24:
      xor         ecx, ecx
      mov         edx, pattern_lumaP1       ;cur - fir_filter_size
      align 16
    xloop24:
      mov         eax, fir_filter_size
      movq        mm0, FPround              ;btotal, gtotal
      lea         edx, [edx+eax*4]          ;cur += fir_filter_size
      movq        mm1, mm0                  ;rtotal
      mov         ebx, [edx]
      add         edx, 4                    ;cur++
      lea         ebx, [ebx+ebx*2]          ;ebx = ofs = *cur * 3
      lea         edx, [edx+eax*4]          ;cur += fir_filter_size
      add         ebx, esi                  ;ebx = srcp + ofs*3
      lea         eax, [eax+eax*2]          ;eax = a = fir_filter_size*3
      align 16
    aloop24:
      sub         edx, 4                    ;cur--
      sub         eax, 3
      movd        mm5, [edx]                ;mm5 =    00|co (co = coefficient)
      movd        mm7, [ebx+eax]            ;mm7 = srcp[ofs+a] = 0|0|0|0|x|r|g|b
      packssdw    mm5, mm2
      punpcklbw   mm7, mm2                  ;mm7 = 0x|0r|0g|0b
      punpckldq   mm5, mm5                  ;mm5 =    co|co
      movq        mm6, mm7
      punpcklwd   mm7, mm2                  ;mm7 = 00|0g|00|0b
      punpckhwd   mm6, mm2                  ;mm6 = 00|0x|00|0r
      pmaddwd     mm7, mm5                  ;mm7 =  g*co|b*co
      pmaddwd     mm6, mm5                  ;mm6 =  x*co|r*co
      paddd       mm0, mm7
      paddd       mm1, mm6
      jnz         aloop24

      pslld       mm0, 2
      pslld       mm1, 2                    ;compensate the fact that FPScale = 16384
      packuswb    mm0, mm1                  ;mm0 = x|_|r|_|g|_|b|_
      psrlw       mm0, 8                    ;mm0 = 0|x|0|r|0|g|0|b
      packuswb    mm0, mm2                  ;mm0 = 0|0|0|0|x|r|g|b
      movd        mm3, [edi+ecx]            ;mm3 = 0|0|0|0|x|r|g|b (dst)
      pslld       mm0, 8                    ;mm0 = 0|0|0|0|r|g|b|0
      pand        mm3, mm4                  ;mm3 = 0|0|0|0|x|0|0|0 (dst)
      psrld       mm0, 8                    ;mm0 = 0|0|0|0|0|r|g|b
      add         ecx, 3
      por         mm3, mm0
      cmp         ecx, w
      movd        [edi+ecx-3], mm3
      jnz         xloop24

      add         esi, src_pitch
      add         edi, dst_pitch
      dec         y
      jnz         yloop24
      emms
	  pop         ebx
    }
  }
  else
  {
    // RGB32
    int y = vi.height;
    int w = vi.width;
    int fir_filter_size = pattern_luma[0];
    int* pattern_lumaP1 = &pattern_luma[1] - fir_filter_size;
    static const int FPround = 0x00002000;  // 16384/2

    __asm {
	  push        ebx
      mov         esi, srcp
      movd        mm3, FPround
      mov         edi, dstp
	  punpckldq   mm3, mm3
      pxor        mm2, mm2
      align 16
    yloop32:
      xor         ecx, ecx
      mov         edx, pattern_lumaP1       ;cur - fir_filter_size
      align 16
    xloop32:
      mov         eax, fir_filter_size
      movq        mm0, mm3                  ;btotal, gtotal
      lea         edx, [edx+eax*4]          ;cur += fir_filter_size
      movq        mm1, mm3                  ;atotal, rtotal
      mov         ebx, [edx]
      add         edx, 4                    ;cur++
      shl         ebx, 2                    ;ebx = ofs = *cur * 4
      lea         edx, [edx+eax*4]          ;cur += fir_filter_size
      add         ebx, esi                  ;ebx = srcp + ofs*4
      align 16
    aloop32:
      sub         edx, 4                    ;cur--
      movd        mm7, [ebx+eax*4-4]        ;mm7 = srcp[ofs+a] = 0|0|0|0|a|r|g|b
      movd        mm5, [edx]                ;mm5 =    00|co (co = coefficient)
      punpcklbw   mm7, mm2                  ;mm7 = 0a|0r|0g|0b
      packssdw    mm5, mm2
      movq        mm6, mm7
      punpcklwd   mm7, mm2                  ;mm7 = 00|0g|00|0b
      punpckldq   mm5, mm5                  ;mm5 =    co|co
      punpckhwd   mm6, mm2                  ;mm6 = 00|0a|00|0r
      pmaddwd     mm7, mm5                  ;mm7 =  g*co|b*co
      dec         eax
      pmaddwd     mm6, mm5                  ;mm6 =  a*co|r*co
      paddd       mm0, mm7
      paddd       mm1, mm6
      jnz         aloop32

      pslld       mm0, 2
      pslld       mm1, 2                    ;compensate the fact that FPScale = 16384
      packuswb    mm0, mm1                  ;mm0 = a|_|r|_|g|_|b|_
      psrlw       mm0, 8                    ;mm0 = 0|a|0|r|0|g|0|b
      inc         ecx
      packuswb    mm0, mm2                  ;mm0 = 0|0|0|0|a|r|g|b
      cmp         ecx, w
      movd        [edi+ecx*4-4], mm0
      jnz         xloop32

      add         esi, src_pitch
      add         edi, dst_pitch
      dec         y
      jnz         yloop32
      emms
	  pop         ebx
    }
  }
#else
  //TODO
  env->ThrowError("FilteredResizeH::GetFrame is not yet ported to 64-bit.");
#endif
#endif
  return dst;
}

FilteredResizeH::~FilteredResizeH(void)
{
  if (pattern_luma) _aligned_free(pattern_luma);
  if (pattern_chroma) _aligned_free(pattern_chroma);
  if (tempY)
  {
    if (tempUV) _aligned_free(tempUV);
    if (tempY) _aligned_free(tempY);
  }
#ifdef X86_32
  assemblerY.Free();
  assemblerUV.Free();
  assemblerY_aligned.Free();
  assemblerUV_aligned.Free();
#endif
}

/***************************************
 ***** Vertical Resizer Assembly *******
 ***************************************/

static void resize_v_c_planar_pointresize(BYTE* dst, const BYTE* src, int dst_pitch, int src_pitch, ResamplingProgram* program, int width, int target_height, const int* pitch_table) {
  int filter_size = program->filter_size;

  for (int y = 0; y < target_height; y++) {
    int offset = program->pixel_offset[y];
    const BYTE* src_ptr = src + pitch_table[offset];

    for (int x = 0; x < width; x++) {
      dst[x] = src_ptr[x];
    }

    dst += dst_pitch;
  }
}

template<SSELoader load>
static void resize_v_sse2_planar_pointresize(BYTE* dst, const BYTE* src, int dst_pitch, int src_pitch, ResamplingProgram* program, int width, int target_height, const int* pitch_table) {
  int filter_size = program->filter_size;

  int wMod16 = (width / 16) * 16;

  for (int y = 0; y < target_height; y++) {
    int offset = program->pixel_offset[y];
    const BYTE* src_ptr = src + pitch_table[offset];

    // Copy 16-pixel/loop
    for (int x = 0; x < wMod16; x+=16) {
      __m128i current_pixel = load(reinterpret_cast<const __m128i*>(src_ptr+x));
      _mm_store_si128(reinterpret_cast<__m128i*>(dst+x), current_pixel); // dst should always be aligned
    }

    // Leftover
    for (int i = wMod16; i < width; i++) {
      dst[i] = src_ptr[i];
    }

    dst += dst_pitch;
  }
}

static void resize_v_c_planar(BYTE* dst, const BYTE* src, int dst_pitch, int src_pitch, ResamplingProgram* program, int width, int target_height, const int* pitch_table) {
  int filter_size = program->filter_size;
  short* current_coeff = program->pixel_coefficient;

  for (int y = 0; y < target_height; y++) {
    int offset = program->pixel_offset[y];
    const BYTE* src_ptr = src + pitch_table[offset];

    for (int x = 0; x < width; x++) {
      int result = 0;
      for (int i = 0; i < filter_size; i++) {
        result += (src_ptr+pitch_table[i])[x] * current_coeff[i];
      }
      result = ((result+8192)/16384);
      result = result > 255 ? 255 : result < 0 ? 0 : result;
      dst[x] = (BYTE) result;
    }

    dst += dst_pitch;
    current_coeff += filter_size;
  }
}

template<SSELoader load>
static void resize_v_sse2_planar(BYTE* dst, const BYTE* src, int dst_pitch, int src_pitch, ResamplingProgram* program, int width, int target_height, const int* pitch_table) {
  int filter_size = program->filter_size;
  short* current_coeff = program->pixel_coefficient;
  
  int wMod16 = (width / 16) * 16;
  int sizeMod2 = (filter_size/2) * 2;

  __m128i zero = _mm_setzero_si128();

  for (int y = 0; y < target_height; y++) {
    int offset = program->pixel_offset[y];
    const BYTE* src_ptr = src + pitch_table[offset];

    for (int x = 0; x < wMod16; x += 16) {
      __m128i result_1 = _mm_set1_epi32(8192); // Init. with rounder (16384/2 = 8192)
      __m128i result_2 = result_1;
      __m128i result_3 = result_1;
      __m128i result_4 = result_1;
      
      for (int i = 0; i < sizeMod2; i += 2) {
        __m128i src_p1 = load(reinterpret_cast<const __m128i*>(src_ptr+pitch_table[i]+x));   // p|o|n|m|l|k|j|i|h|g|f|e|d|c|b|a
        __m128i src_p2 = load(reinterpret_cast<const __m128i*>(src_ptr+pitch_table[i+1]+x)); // P|O|N|M|L|K|J|I|H|G|F|E|D|C|B|A
         
        __m128i src_l = _mm_unpacklo_epi8(src_p1, src_p2);                                   // Hh|Gg|Ff|Ee|Dd|Cc|Bb|Aa
        __m128i src_h = _mm_unpackhi_epi8(src_p1, src_p2);                                   // Pp|Oo|Nn|Mm|Ll|Kk|Jj|Ii

        __m128i src_1 = _mm_unpacklo_epi8(src_l, zero);                                      // .D|.d|.C|.c|.B|.b|.A|.a
        __m128i src_2 = _mm_unpackhi_epi8(src_l, zero);                                      // .H|.h|.G|.g|.F|.f|.E|.e
        __m128i src_3 = _mm_unpacklo_epi8(src_h, zero);                                      // etc.
        __m128i src_4 = _mm_unpackhi_epi8(src_h, zero);                                      // etc.

        __m128i coeff = _mm_cvtsi32_si128(*reinterpret_cast<const int*>(current_coeff+i));   // XX|XX|XX|XX|XX|XX|CO|co
        coeff = _mm_shuffle_epi32(coeff, 0);                                                 // CO|co|CO|co|CO|co|CO|co
        
        __m128i dst_1 = _mm_madd_epi16(src_1, coeff);                                         // CO*D+co*d | CO*C+co*c | CO*B+co*b | CO*A+co*a
        __m128i dst_2 = _mm_madd_epi16(src_2, coeff);                                         // etc.
        __m128i dst_3 = _mm_madd_epi16(src_3, coeff);
        __m128i dst_4 = _mm_madd_epi16(src_4, coeff);

        result_1 = _mm_add_epi32(result_1, dst_1);
        result_2 = _mm_add_epi32(result_2, dst_2);
        result_3 = _mm_add_epi32(result_3, dst_3);
        result_4 = _mm_add_epi32(result_4, dst_4);
      }
      
      if (sizeMod2 < filter_size) { // do last odd row
        __m128i src_p = load(reinterpret_cast<const __m128i*>(src_ptr+pitch_table[sizeMod2]+x));

        __m128i src_l = _mm_unpacklo_epi8(src_p, zero);
        __m128i src_h = _mm_unpackhi_epi8(src_p, zero);

        __m128i coeff = _mm_set1_epi16(current_coeff[sizeMod2]);

        __m128i dst_ll = _mm_mullo_epi16(src_l, coeff);   // Multiply by coefficient
        __m128i dst_lh = _mm_mulhi_epi16(src_l, coeff);
        __m128i dst_hl = _mm_mullo_epi16(src_h, coeff);
        __m128i dst_hh = _mm_mulhi_epi16(src_h, coeff);

        __m128i dst_1 = _mm_unpacklo_epi16(dst_ll, dst_lh); // Unpack to 32-bit integer
        __m128i dst_2 = _mm_unpackhi_epi16(dst_ll, dst_lh);
        __m128i dst_3 = _mm_unpacklo_epi16(dst_hl, dst_hh);
        __m128i dst_4 = _mm_unpackhi_epi16(dst_hl, dst_hh);

        result_1 = _mm_add_epi32(result_1, dst_1);
        result_2 = _mm_add_epi32(result_2, dst_2);
        result_3 = _mm_add_epi32(result_3, dst_3);
        result_4 = _mm_add_epi32(result_4, dst_4);
      }
      
      // Divide by 16348 (FPRound)
      result_1  = _mm_srai_epi32(result_1, 14);
      result_2  = _mm_srai_epi32(result_2, 14);
      result_3  = _mm_srai_epi32(result_3, 14);
      result_4  = _mm_srai_epi32(result_4, 14);

      // Pack and store
      __m128i result_l = _mm_packs_epi32(result_1, result_2);
      __m128i result_h = _mm_packs_epi32(result_3, result_4);
      __m128i result   = _mm_packus_epi16(result_l, result_h);

      _mm_store_si128(reinterpret_cast<__m128i*>(dst+x), result);
    }

    // Leftover
    for (int x = wMod16; x < width; x++) {
      int result = 0;
      for (int i = 0; i < filter_size; i++) {
        result += (src_ptr+pitch_table[i])[x] * current_coeff[i];
      }
      result = ((result+8192)/16384);
      result = result > 255 ? 255 : result < 0 ? 0 : result;
      dst[x] = (BYTE) result;
    }

    dst += dst_pitch;
    current_coeff += filter_size;
  }
}

template<SSELoader load>
static void resize_v_ssse3_planar(BYTE* dst, const BYTE* src, int dst_pitch, int src_pitch, ResamplingProgram* program, int width, int target_height, const int* pitch_table) {
  int filter_size = program->filter_size;
  short* current_coeff = program->pixel_coefficient;
  
  int wMod16 = (width / 16) * 16;

  __m128i zero = _mm_setzero_si128();

  for (int y = 0; y < target_height; y++) {
    int offset = program->pixel_offset[y];
    const BYTE* src_ptr = src + pitch_table[offset];

    for (int x = 0; x < wMod16; x+=16) {
      __m128i result_l = _mm_set1_epi16(32); // Init. with rounder ((1 << 6)/2 = 32)
      __m128i result_h = result_l;

      for (int i = 0; i < filter_size; i++) {
        __m128i src_p = load(reinterpret_cast<const __m128i*>(src_ptr+pitch_table[i]+x));

        __m128i src_l = _mm_unpacklo_epi8(src_p, zero);
        __m128i src_h = _mm_unpackhi_epi8(src_p, zero);

        src_l = _mm_slli_epi16(src_l, 7);
        src_h = _mm_slli_epi16(src_h, 7);

        __m128i coeff = _mm_set1_epi16(static_cast<short>(current_coeff[i]));

        __m128i dst_l = _mm_mulhrs_epi16(src_l, coeff);   // Multiply by coefficient (SSSE3)
        __m128i dst_h = _mm_mulhrs_epi16(src_h, coeff);

        result_l = _mm_add_epi16(result_l, dst_l);
        result_h = _mm_add_epi16(result_h, dst_h);
      }

      // Divide by 64
      result_l  = _mm_srai_epi16(result_l, 6);
      result_h  = _mm_srai_epi16(result_h, 6);

      // Pack and store
      __m128i result   = _mm_packus_epi16(result_l, result_h);

      _mm_store_si128(reinterpret_cast<__m128i*>(dst+x), result);
    }

    // Leftover
    for (int x = wMod16; x < width; x++) {
      int result = 0;
      for (int i = 0; i < filter_size; i++) {
        result += (src_ptr+pitch_table[i])[x] * current_coeff[i];
      }
      result = ((result+8192)/16384);
      result = result > 255 ? 255 : result < 0 ? 0 : result;
      dst[x] = (BYTE) result;
    }

    dst += dst_pitch;
    current_coeff += filter_size;
  }
}

__forceinline static void resize_v_create_pitch_table(int* table, int pitch, int height) {
  table[0] = 0;
  for (int i = 1; i < height; i++) {
    table[i] = table[i-1]+pitch;
  }
}

/***************************************
 ***** Filtered Resize - Vertical ******
 ***************************************/

FilteredResizeV::FilteredResizeV( PClip _child, double subrange_top, double subrange_height,
                                  int target_height, ResamplingFunction* func, IScriptEnvironment* env )
  : GenericVideoFilter(_child),
    resampling_program_luma(0), resampling_program_chroma(0),
    src_pitch_table_luma(0), src_pitch_table_chromaU(0), src_pitch_table_chromaV(0),
    src_pitch_luma(-1), src_pitch_chromaU(-1), src_pitch_chromaV(-1)
{
  if (target_height <= 0)
    env->ThrowError("Resize: Height must be greater than 0.");

  if (vi.IsPlanar() && !vi.IsY8()) {
    const int mask = (1 << vi.GetPlaneHeightSubsampling(PLANAR_U)) - 1;

    if (target_height & mask)
      env->ThrowError("Resize: Planar destination height must be a multiple of %d.", mask+1);
  }

  if (vi.IsRGB())
    subrange_top = vi.height - subrange_top - subrange_height; // why?

  // Create resampling program and pitch table
  resampling_program_luma  = func->GetResamplingProgram(vi.height, subrange_top, subrange_height, target_height, env);
  src_pitch_table_luma     = new int[vi.height];
  resampler_luma_aligned   = GetResampler(env->GetCPUFlags(), true , resampling_program_luma);
  resampler_luma_unaligned = GetResampler(env->GetCPUFlags(), false, resampling_program_luma);

  if (vi.IsPlanar() && !vi.IsY8()) {
    const int shift = vi.GetPlaneHeightSubsampling(PLANAR_U);
    const int div   = 1 << shift;

    resampling_program_chroma = func->GetResamplingProgram(
                                  vi.height      >> shift,
                                  subrange_top    / div,
                                  subrange_height / div,
                                  target_height  >> shift,
                                  env);
    src_pitch_table_chromaU    = new int[vi.height >> shift];
    src_pitch_table_chromaV    = new int[vi.height >> shift];
    resampler_chroma_aligned   = GetResampler(env->GetCPUFlags(), true , resampling_program_chroma);
    resampler_chroma_unaligned = GetResampler(env->GetCPUFlags(), false, resampling_program_chroma);
  }

  // Create resampler

  // Change target video info size
  vi.height = target_height;
}

PVideoFrame __stdcall FilteredResizeV::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame src = child->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrame(vi);
  int src_pitch = src->GetPitch();
  int dst_pitch = dst->GetPitch();
  const BYTE* srcp = src->GetReadPtr();
        BYTE* dstp = dst->GetWritePtr();

  // Create pitch table
  if (src_pitch_luma != src->GetPitch()) {
    src_pitch_luma = src->GetPitch();
    resize_v_create_pitch_table(src_pitch_table_luma, src_pitch_luma, src->GetHeight());
  }

  if ((!vi.IsY8() && vi.IsPlanar()) && src_pitch_chromaU != src->GetPitch(PLANAR_U)) {
    src_pitch_chromaU = src->GetPitch(PLANAR_U);
    resize_v_create_pitch_table(src_pitch_table_chromaU, src_pitch_chromaU, src->GetHeight(PLANAR_U));
  }

  if ((!vi.IsY8() && vi.IsPlanar()) && src_pitch_chromaV != src->GetPitch(PLANAR_V)) {
    src_pitch_chromaV = src->GetPitch(PLANAR_V);
    resize_v_create_pitch_table(src_pitch_table_chromaV, src_pitch_chromaV, src->GetHeight(PLANAR_V));
  }

  // Do resizing
  if (IsPtrAligned(srcp, 16) && IsPtrAligned(src_pitch, 16))
    resampler_luma_aligned(dstp, srcp, dst_pitch, src_pitch, resampling_program_luma, vi.BytesFromPixels(vi.width), vi.height, src_pitch_table_luma);
  else
    resampler_luma_unaligned(dstp, srcp, dst_pitch, src_pitch, resampling_program_luma, vi.BytesFromPixels(vi.width), vi.height, src_pitch_table_luma);
    
  if (!vi.IsY8() && vi.IsPlanar()) {
    int width = vi.width >> vi.GetPlaneWidthSubsampling(PLANAR_U);
    int height = vi.height >> vi.GetPlaneHeightSubsampling(PLANAR_U);

    // Plane U resizing
    src_pitch = src->GetPitch(PLANAR_U);
    dst_pitch = dst->GetPitch(PLANAR_U);
    srcp = src->GetReadPtr(PLANAR_U);
    dstp = dst->GetWritePtr(PLANAR_U);
      
    if (IsPtrAligned(srcp, 16) && IsPtrAligned(src_pitch, 16))
      resampler_chroma_aligned(dstp, srcp, dst_pitch, src_pitch, resampling_program_chroma, width, height, src_pitch_table_chromaU);
    else
      resampler_chroma_unaligned(dstp, srcp, dst_pitch, src_pitch, resampling_program_chroma, width, height, src_pitch_table_chromaU);

    // Plane V resizing
    src_pitch = src->GetPitch(PLANAR_V);
    dst_pitch = dst->GetPitch(PLANAR_V);
    srcp = src->GetReadPtr(PLANAR_V);
    dstp = dst->GetWritePtr(PLANAR_V);
  
    if (IsPtrAligned(srcp, 16) && IsPtrAligned(src_pitch, 16))
      resampler_chroma_aligned(dstp, srcp, dst_pitch, src_pitch, resampling_program_chroma, width, height, src_pitch_table_chromaV);
    else
      resampler_chroma_unaligned(dstp, srcp, dst_pitch, src_pitch, resampling_program_chroma, width, height, src_pitch_table_chromaV);
  }

  return dst;
}

// IsPtrAligned(srcp, 16) && IsPtrAligned(src_pitch, 16)
ResamplerV FilteredResizeV::GetResampler(int CPU, bool aligned, ResamplingProgram* program) {
  if (program->filter_size == 1) {
    // Fast pointresize
    if (aligned) {
      if (CPU & CPUF_SSE4_1) { // SSE4.1 movntdqa
        return resize_v_sse2_planar_pointresize<simd_load_streaming>;
      } else if (CPU & CPUF_SSE2) { // SSE2 aligned
        return resize_v_sse2_planar_pointresize<simd_load_aligned>;
      } else { // C version
        return resize_v_c_planar_pointresize;
      }
    } else { // Not aligned
      if (CPU & CPUF_SSE3) { // SSE3 lddqu
        return resize_v_sse2_planar_pointresize<simd_load_unaligned_sse3>;
      } else if (CPU & CPUF_SSE2) { // SSE2 unaligned
        return resize_v_sse2_planar_pointresize<simd_load_unaligned>;
      } else { // C version
        return resize_v_c_planar_pointresize;
      }
    }
  } else {
    // Other resizers
    if (CPU & CPUF_SSSE3 && false) { // FIXME: this hack since current SSSE3 is slightly slower
      if (aligned) { // SSSE3 aligned
        return resize_v_ssse3_planar<simd_load_aligned>;
      } else if (CPU & CPUF_SSE3) { // SSE3 lddqu
        return resize_v_ssse3_planar<simd_load_unaligned_sse3>;
      } else { // SSSE3 unaligned
        return resize_v_ssse3_planar<simd_load_unaligned_sse3>;
      }
    } else if (CPU & CPUF_SSE2) {
      if (aligned) { // SSE2 aligned
        return resize_v_sse2_planar<simd_load_aligned>;
      } else if (CPU & CPUF_SSE3) { // SSE2 lddqu
        return resize_v_sse2_planar<simd_load_unaligned_sse3>;
      } else { // SSE2 unaligned
        return resize_v_sse2_planar<simd_load_unaligned_sse3>;
      }
    } else { // C version
      return resize_v_c_planar; //(dstp, srcp, dst_pitch, src_pitch, resampling_pattern, width, height);
    }
  }
}

FilteredResizeV::~FilteredResizeV(void)
{
  if (resampling_program_luma)   { delete resampling_program_luma; }
  if (resampling_program_chroma) { delete resampling_program_chroma; }
  if (src_pitch_table_luma)    { delete[] src_pitch_table_luma; }
  if (src_pitch_table_chromaU) { delete[] src_pitch_table_chromaU; }
  if (src_pitch_table_chromaV) { delete[] src_pitch_table_chromaV; }
}


/**********************************************
 *******   Resampling Factory Methods   *******
 **********************************************/

PClip FilteredResize::CreateResizeH(PClip clip, double subrange_left, double subrange_width, int target_width,
                    ResamplingFunction* func, IScriptEnvironment* env)
{
  const VideoInfo& vi = clip->GetVideoInfo();
  if (subrange_left == 0 && subrange_width == target_width && subrange_width == vi.width) {
    return clip;
  }

  if (subrange_left == int(subrange_left) && subrange_width == target_width
   && subrange_left >= 0 && subrange_left + subrange_width <= vi.width) {
    const int mask = (vi.IsYUV() && !vi.IsY8()) ? (1 << vi.GetPlaneWidthSubsampling(PLANAR_U)) - 1 : 0;

    if (((int(subrange_left) | int(subrange_width)) & mask) == 0)
      return new Crop(int(subrange_left), 0, int(subrange_width), vi.height, 0, clip, env);
  }
  return new FilteredResizeH(clip, subrange_left, subrange_width, target_width, func, env);
}


PClip FilteredResize::CreateResizeV(PClip clip, double subrange_top, double subrange_height, int target_height,
                    ResamplingFunction* func, IScriptEnvironment* env)
{
  const VideoInfo& vi = clip->GetVideoInfo();
  if (subrange_top == 0 && subrange_height == target_height && subrange_height == vi.height) {
    return clip;
  }

  if (subrange_top == int(subrange_top) && subrange_height == target_height
   && subrange_top >= 0 && subrange_top + subrange_height <= vi.height) {
    const int mask = (vi.IsYUV() && !vi.IsY8()) ? (1 << vi.GetPlaneHeightSubsampling(PLANAR_U)) - 1 : 0;

    if (((int(subrange_top) | int(subrange_height)) & mask) == 0)
      return new Crop(0, int(subrange_top), vi.width, int(subrange_height), 0, clip, env);
  }
  return new FilteredResizeV(clip, subrange_top, subrange_height, target_height, func, env);
}


PClip FilteredResize::CreateResize(PClip clip, int target_width, int target_height, const AVSValue* args,
                   ResamplingFunction* f, IScriptEnvironment* env)
{
  const VideoInfo& vi = clip->GetVideoInfo();
  const double subrange_left = args[0].AsFloat(0), subrange_top = args[1].AsFloat(0);

  double subrange_width = args[2].AsDblDef(vi.width), subrange_height = args[3].AsDblDef(vi.height);
  // Crop style syntax
  if (subrange_width  <= 0.0) subrange_width  = vi.width  - subrange_left + subrange_width;
  if (subrange_height <= 0.0) subrange_height = vi.height - subrange_top  + subrange_height;

  PClip result;
  // ensure that the intermediate area is maximal
  const double area_FirstH = subrange_height * target_width;
  const double area_FirstV = subrange_width * target_height;
  if (area_FirstH < area_FirstV)
  {
      result = CreateResizeV(clip, subrange_top, subrange_height, target_height, f, env);
      result = CreateResizeH(result, subrange_left, subrange_width, target_width, f, env);
  }
  else
  {
      result = CreateResizeH(clip, subrange_left, subrange_width, target_width, f, env);
      result = CreateResizeV(result, subrange_top, subrange_height, target_height, f, env);
  }
  return result;
}

AVSValue __cdecl FilteredResize::Create_PointResize(AVSValue args, void*, IScriptEnvironment* env)
{
  return CreateResize( args[0].AsClip(), args[1].AsInt(), args[2].AsInt(), &args[3],
                       &PointFilter(), env );
}


AVSValue __cdecl FilteredResize::Create_BilinearResize(AVSValue args, void*, IScriptEnvironment* env)
{
  return CreateResize( args[0].AsClip(), args[1].AsInt(), args[2].AsInt(), &args[3],
                       &TriangleFilter(), env );
}


AVSValue __cdecl FilteredResize::Create_BicubicResize(AVSValue args, void*, IScriptEnvironment* env)
{
  return CreateResize( args[0].AsClip(), args[1].AsInt(), args[2].AsInt(), &args[5],
                       &MitchellNetravaliFilter(args[3].AsDblDef(1./3.), args[4].AsDblDef(1./3.)), env );
}

AVSValue __cdecl FilteredResize::Create_LanczosResize(AVSValue args, void*, IScriptEnvironment* env)
{
  return CreateResize( args[0].AsClip(), args[1].AsInt(), args[2].AsInt(), &args[3],
                       &LanczosFilter(args[7].AsInt(3)), env );
}

AVSValue __cdecl FilteredResize::Create_Lanczos4Resize(AVSValue args, void*, IScriptEnvironment* env)
{
  return CreateResize( args[0].AsClip(), args[1].AsInt(), args[2].AsInt(), &args[3],
                       &LanczosFilter(4), env );
}

AVSValue __cdecl FilteredResize::Create_BlackmanResize(AVSValue args, void*, IScriptEnvironment* env)
{
  return CreateResize( args[0].AsClip(), args[1].AsInt(), args[2].AsInt(), &args[3],
                       &BlackmanFilter(args[7].AsInt(4)), env );
}

AVSValue __cdecl FilteredResize::Create_Spline16Resize(AVSValue args, void*, IScriptEnvironment* env)
{
  return CreateResize( args[0].AsClip(), args[1].AsInt(), args[2].AsInt(), &args[3],
                       &Spline16Filter(), env );
}

AVSValue __cdecl FilteredResize::Create_Spline36Resize(AVSValue args, void*, IScriptEnvironment* env)
{
  return CreateResize( args[0].AsClip(), args[1].AsInt(), args[2].AsInt(), &args[3],
                       &Spline36Filter(), env );
}

AVSValue __cdecl FilteredResize::Create_Spline64Resize(AVSValue args, void*, IScriptEnvironment* env)
{
  return CreateResize( args[0].AsClip(), args[1].AsInt(), args[2].AsInt(), &args[3],
                       &Spline64Filter(), env );
}

AVSValue __cdecl FilteredResize::Create_GaussianResize(AVSValue args, void*, IScriptEnvironment* env)
{
  return CreateResize( args[0].AsClip(), args[1].AsInt(), args[2].AsInt(), &args[3],
                       &GaussianFilter(args[7].AsFloat(30.0f)), env );
}

AVSValue __cdecl FilteredResize::Create_SincResize(AVSValue args, void*, IScriptEnvironment* env)
{
  return CreateResize( args[0].AsClip(), args[1].AsInt(), args[2].AsInt(), &args[3],
                       &SincFilter(args[7].AsInt(4)), env );
}

