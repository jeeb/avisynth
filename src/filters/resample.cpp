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

#include "resample.h"




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

FilteredResizeH::FilteredResizeH( PClip _child, double subrange_left, double subrange_width,
                                  int target_width, ResamplingFunction* func, IScriptEnvironment* env )
  : GenericVideoFilter(_child), tempY(0), tempUV(0),pattern_luma(0),pattern_chroma(0)
{
	try {	// HIDE DAMN SEH COMPILER BUG!!!

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
  }
  else
    pattern_luma = func->GetResamplingPatternRGB(vi.width, subrange_left, subrange_width, target_width, env);

  vi.width = target_width;

  if (vi.IsPlanar()) {
    try {
      assemblerY         = GenerateResizer(PLANAR_Y, false, env);
      assemblerY_aligned = GenerateResizer(PLANAR_Y, true,  env);
      if (!vi.IsY8()) {
        assemblerUV         = GenerateResizer(PLANAR_U, false, env);
        assemblerUV_aligned = GenerateResizer(PLANAR_U, true,  env);
      }
    }
    catch (SoftWire::Error err) {
       env->ThrowError("Resize: SoftWire exception : %s", err.getString());
    }
  }
	}	catch (...) { throw; }
}

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
        if (sse4) { // Should be using  pmovzxbw xmm1, qword_ptr[esi+8]
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
  } else {
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

//    vi_dst_width;                              1,2,3,4,5,6,7,8,9,A,B,C,D,E,F,10
//    vi_dst_width-2                            -1,0,1,2,3,4,5,6,7,8,9,A,B,C,D,E,F
//    six_loops = (vi_dst_width-2)/6;            0,0,0,0,0,0,0,1,1,1,1,1,1,2,2,2,2
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



PVideoFrame __stdcall FilteredResizeH::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame src = child->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrame(vi);
  const BYTE* srcp = src->GetReadPtr();
  BYTE* dstp = dst->GetWritePtr();
  int src_pitch = src->GetPitch();
  int dst_pitch = dst->GetPitch();
  if (vi.IsPlanar()) {
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
  assemblerY.Free();
  assemblerUV.Free();
  assemblerY_aligned.Free();
  assemblerUV_aligned.Free();
}



/***************************************
 ***** Filtered Resize - Vertical ******
 ***************************************/

FilteredResizeV::FilteredResizeV( PClip _child, double subrange_top, double subrange_height,
                                  int target_height, ResamplingFunction* func, IScriptEnvironment* env )
  : GenericVideoFilter(_child), resampling_pattern(0), resampling_patternUV(0),
    yOfs(0), yOfsUV(0), pitch_gY(-1), pitch_gUV(-1)
{
  if (target_height<=0)
    env->ThrowError("Resize: Height must be greater than 0.");

  if (vi.IsPlanar() && !vi.IsY8()) {
    const int mask = (1 << vi.GetPlaneHeightSubsampling(PLANAR_U)) - 1;

    if (target_height & mask)
      env->ThrowError("Resize: Planar destination height must be a multiple of %d.", mask+1);
  }

  if (vi.IsRGB())
    subrange_top = vi.height - subrange_top - subrange_height;

  resampling_pattern = func->GetResamplingPatternRGB(vi.height, subrange_top, subrange_height, target_height, env);

  if (vi.IsPlanar() && !vi.IsY8()) {
    const int shift = vi.GetPlaneHeightSubsampling(PLANAR_U);
    const int div   = 1 << shift;

    resampling_patternUV = func->GetResamplingPatternRGB(
                                          vi.height      >> shift,
                                          subrange_top    / div,
                                          subrange_height / div,
                                          target_height  >> shift,
                                          env);
  }

  vi.height = target_height;

  try {
    assemblerY         = GenerateResizer(PLANAR_Y, false, env);
    assemblerY_aligned = GenerateResizer(PLANAR_Y, true, env);
    if (vi.IsPlanar() && !vi.IsY8()) {
      assemblerUV         = GenerateResizer(PLANAR_U, false, env);
      assemblerUV_aligned = GenerateResizer(PLANAR_U, true, env);
    }
  }
  catch (SoftWire::Error err) {
     env->ThrowError("Resize: SoftWire exception : %s", err.getString());
  }
}


/*******************************
 * Note on multithreading (Klaus Post, 2007):
 * GetFrame is currently not re-entrant due to dynamic code variables.
 * I have not been able to find a good solution for this
 * (pushing a struct pointer to dynamic data on to the stack is not a solution IMO).
 * We could guard the function, to avoid re-entrance.
 ******************************/


PVideoFrame __stdcall FilteredResizeV::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame src = child->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrame(vi);
  src_pitch = src->GetPitch();
  dst_pitch = dst->GetPitch();
  srcp = src->GetReadPtr();
  dstp = dst->GetWritePtr();
  y = vi.height;
  int plane = vi.IsPlanar() && (!vi.IsY8()) ? 4:1;

  if (pitch_gUV != src->GetPitch(PLANAR_U)) {  // Pitch is not the same as last frame
    int shUV = src->GetHeight(PLANAR_U);
    pitch_gUV = src->GetPitch(PLANAR_U);

    if (!yOfsUV)
      yOfsUV = new int[shUV];

    for (int i = 0; i < shUV; i++)
      yOfsUV[i] = pitch_gUV * i;
  }

  if (pitch_gY != src->GetPitch(PLANAR_Y))  { // Pitch is not the same as last frame
    int sh = src->GetHeight();
    pitch_gY = src->GetPitch(PLANAR_Y);

    if (!yOfs)
      yOfs = new int[sh];

    for (int i = 0; i < sh; i++)
      yOfs[i] = pitch_gY * i;
  }

  yOfs2 = this->yOfs;

  while (plane-->0){
    switch (plane) {
      case 2:  // take V plane
        src_pitch = src->GetPitch(PLANAR_V);
        dst_pitch = dst->GetPitch(PLANAR_V);
        dstp = dst->GetWritePtr(PLANAR_V);
        srcp = src->GetReadPtr(PLANAR_V);
        y = dst->GetHeight(PLANAR_V);
        yOfs2 = this->yOfsUV;
        if (((int)srcp & 15) || (src_pitch & 15) || !assemblerUV_aligned)
          assemblerUV.Call();
        else
          assemblerUV_aligned.Call();
        break;
      case 1: // U Plane
        dstp = dst->GetWritePtr(PLANAR_U);
        srcp = src->GetReadPtr(PLANAR_U);
        y = dst->GetHeight(PLANAR_U);
        src_pitch = src->GetPitch(PLANAR_U);
        dst_pitch = dst->GetPitch(PLANAR_U);
        yOfs2 = this->yOfsUV;
        plane--; // skip case 0
        if (((int)srcp & 15) || (src_pitch & 15) || !assemblerUV_aligned)
          assemblerUV.Call();
        else
          assemblerUV_aligned.Call();
        break;
      case 3: // Y plane for planar
      case 0: // Default for interleaved
        if (((int)srcp & 15) || (src_pitch & 15) || !assemblerY_aligned)
          assemblerY.Call();
        else
          assemblerY_aligned.Call();
        break;
    }
  } // end while
  return dst;
}


FilteredResizeV::~FilteredResizeV(void)
{
  if (resampling_pattern) { _aligned_free(resampling_pattern); resampling_pattern = 0; }
  if (resampling_patternUV) { _aligned_free(resampling_patternUV); resampling_patternUV = 0; }
  if (yOfs) { delete[] yOfs; yOfs = 0; }
  if (yOfsUV) { delete[] yOfsUV; yOfsUV = 0; }
  assemblerY.Free();
  assemblerUV.Free();
  assemblerY_aligned.Free();
  assemblerUV_aligned.Free();
}



/***********************************
 * Dynamically Assembled Resampler
 *
 * (c) 2007, Klaus Post
 * (c) 2009, Ian Brabham
 *
 * Dynamic version of the Vertical resizer
 *
 * The Algorithm is the same, except this
 *  one is able to process 16 pixels in parallel in SSE2+, 8 pixels in MMX.
 * The inner loop filter is unrolled based on the
 *  exact filter size.
 * SSSE3 version is approximately twice as fast as original MMX, 
 * SSE2 version is approximately 60% faster than new MMX, 
 * New mmx version is approximately 55% faster than original MMX, 
 * SSSE3 PSNR is more than 67dB to MMX version using 4 taps. i.e <1 bit
 * SSE2 is bit identical with MMX.
 * align parameter indicates if source plane and pitch is 16 byte aligned for sse2+.
 * dest should always be 16 byte aligned.
 **********************************/



DynamicAssembledCode FilteredResizeV::GenerateResizer(int gen_plane, bool aligned, IScriptEnvironment* env) {
  __declspec(align(8)) static const __int64 FPround           = 0x0000200000002000; // 16384/2
  __declspec(align(8)) static const __int64 FProundSSSE3      = 0x0020002000200020; // 128/2
  __declspec(align(8)) static const __int64 UnpackByteShuffle = 0x0100010001000100; 

  Assembler x86;   // This is the class that assembles the code.
  bool ssse3 = !!(env->GetCPUFlags() & CPUF_SSSE3);  // We have one version for SSSE3 and one for plain MMX.
  bool sse3  = !!(env->GetCPUFlags() & CPUF_SSE3);   // We have specialized load routine for SSE3.
  bool sse2  = !!(env->GetCPUFlags() & CPUF_SSE2);
  bool isse  = !!(env->GetCPUFlags() & CPUF_INTEGER_SSE);

  if (aligned && !sse2) // No fast aligned version without SSE2+
    return DynamicAssembledCode();

  int xloops = 0;
  int y = vi.height;

  int* array = (gen_plane == PLANAR_U || gen_plane == PLANAR_V) ? resampling_patternUV : resampling_pattern ;
  int fir_filter_size = array[0];
  int* cur = &array[1];

  if (vi.IsPlanar()) {
    xloops = vi.width >> vi.GetPlaneWidthSubsampling(gen_plane);
    y = y >> (vi.GetPlaneHeightSubsampling(gen_plane));
  } else {
    xloops = vi.BytesFromPixels(vi.width);
  }

  if (sse2)
    xloops = ((xloops+15) / 16) * 16;
  else
    xloops = (xloops+7) / 8;


  // Store registers
  x86.push(           eax);
  x86.push(           ebx);
  x86.push(           ecx);
  x86.push(           edx);
  x86.push(           esi);
  x86.push(           edi);
  x86.push(           ebp);

  if (fir_filter_size == 1) {                 // Fast PointResize
// eax ebx ecx edx esi edi ebp
// mm0 mm1 mm2 mm3 mm4 mm5 mm6 mm7
// xmm0 XMM1 xmm2 XMM3 xmm4 XMM5 xmm6 XMM7
    x86.mov(          edx, (int)cur);              // edx = &array[1] -> start_pos
    x86.mov(          ebx, dword_ptr[(int)&dst_pitch]);
    x86.mov(          ebp, dword_ptr[(int)&dstp]);
    x86.mov(          edi, y);                     // edi = vi.height

    x86.align(16);
x86.label("yloop");
    x86.mov(          esi, dword_ptr[(int)&yOfs2]);// int pitch_table[height] = {0, src_pitch, src_pitch*2, ...}
    x86.mov(          eax, dword_ptr[edx]);        // eax = *cur = start_pos
    x86.lea(          edx, dword_ptr[edx+(fir_filter_size*4)+4]); // cur += fir_filter_size+1
    x86.mov(          esi, dword_ptr[esi+eax*4]);  // esi = yOfs[*cur] = start_pos * src_pitch
    x86.mov(          eax, ebp);                   // eax = dstp
    x86.add(          esi, dword_ptr[(int)&srcp]); // esi = srcp + yOfs[*cur] = srcp + start_pos * src_pitch
    x86.add(          ebp, ebx);                   // dstp += dst_pitch
                 
    if (sse2) {
      int ploops = xloops / 64;
      int plrem  = xloops % 64;
      if (!plrem && ploops) {
        ploops -=  1;
        plrem  += 64;
      }
      if (ploops>1) {
        x86.mov(      ecx, ploops);
        x86.align(16);
x86.label("xloop");
      }
      if (ploops) {
        if (aligned) {
          x86.movdqa( xmm0, xmmword_ptr[esi+ 0]);  // xmm0 = *(srcp2= srcp + x) = p|o|n|m|l|k|j|i|h|g|f|e|d|c|b|a
          x86.movdqa( xmm2, xmmword_ptr[esi+16]);
          x86.movdqa( xmm4, xmmword_ptr[esi+32]);
          x86.movdqa( xmm6, xmmword_ptr[esi+48]);
        } else if (sse3) {
          x86.lddqu(  xmm0, xmmword_ptr[esi+ 0]); // SSE3
          x86.lddqu(  xmm2, xmmword_ptr[esi+16]);
          x86.lddqu(  xmm4, xmmword_ptr[esi+32]);
          x86.lddqu(  xmm6, xmmword_ptr[esi+48]);
        } else {
          x86.movdqu( xmm0, xmmword_ptr[esi+ 0]);
          x86.movdqu( xmm2, xmmword_ptr[esi+16]);
          x86.movdqu( xmm4, xmmword_ptr[esi+32]);
          x86.movdqu( xmm6, xmmword_ptr[esi+48]);
        }
        x86.add(      esi, 64);

        x86.movdqa(   xmmword_ptr[eax+ 0], xmm0);   // dstp[x] = p|o|n|m|l|k|j|i|h|g|f|e|d|c|b|a
        x86.movdqa(   xmmword_ptr[eax+16], xmm2);
        x86.movdqa(   xmmword_ptr[eax+32], xmm4);
        x86.movdqa(   xmmword_ptr[eax+48], xmm6);
        x86.add(      eax, 64);
      }
      if (ploops>1)
        x86.loop(     "xloop");
      if (aligned) {
        if (plrem> 0) x86.movdqa( xmm0, xmmword_ptr[esi+ 0]);
        if (plrem>16) x86.movdqa( xmm2, xmmword_ptr[esi+16]);
        if (plrem>32) x86.movdqa( xmm4, xmmword_ptr[esi+32]);
        if (plrem>48) x86.movdqa( xmm6, xmmword_ptr[esi+48]);
      } else if (sse3) {
        if (plrem> 0) x86.lddqu(  xmm0, xmmword_ptr[esi+ 0]);
        if (plrem>16) x86.lddqu(  xmm2, xmmword_ptr[esi+16]);
        if (plrem>32) x86.lddqu(  xmm4, xmmword_ptr[esi+32]);
        if (plrem>48) x86.lddqu(  xmm6, xmmword_ptr[esi+48]);
      } else {
        if (plrem> 0) x86.movdqu( xmm0, xmmword_ptr[esi+ 0]);
        if (plrem>16) x86.movdqu( xmm2, xmmword_ptr[esi+16]);
        if (plrem>32) x86.movdqu( xmm4, xmmword_ptr[esi+32]);
        if (plrem>48) x86.movdqu( xmm6, xmmword_ptr[esi+48]);
      }
      if (plrem> 0) x86.movdqa(   xmmword_ptr[eax+ 0], xmm0);
      if (plrem>16) x86.movdqa(   xmmword_ptr[eax+16], xmm2);
      if (plrem>32) x86.movdqa(   xmmword_ptr[eax+32], xmm4);
      if (plrem>48) x86.movdqa(   xmmword_ptr[eax+48], xmm6);
    }
    else { // MMX
      int ploops = xloops / 8;
      int plrem  = xloops % 8;
      if (!plrem && ploops) {
        ploops -= 1;
        plrem  += 8;
      }
      if (ploops>1) {
        x86.mov(      ecx, ploops);
        x86.align(16);
x86.label("xloop");
      }
      if (ploops) {
        x86.movq(     mm0, qword_ptr[esi+ 0]);  // mm0 = *(srcp2= srcp + x) = h|g|f|e|d|c|b|a
        x86.movq(     mm1, qword_ptr[esi+ 8]);
        x86.movq(     mm2, qword_ptr[esi+16]);
        x86.movq(     mm3, qword_ptr[esi+24]);
        x86.movq(     mm4, qword_ptr[esi+32]);
        x86.movq(     mm5, qword_ptr[esi+40]);
        x86.movq(     mm6, qword_ptr[esi+48]);
        x86.movq(     mm7, qword_ptr[esi+56]);
        x86.add(      esi, 64);

        x86.movq(     qword_ptr[eax+ 0], mm0); // dstp[x] = h|g|f|e|d|c|b|a
        x86.movq(     qword_ptr[eax+ 8], mm1);
        x86.movq(     qword_ptr[eax+16], mm2);
        x86.movq(     qword_ptr[eax+24], mm3);
        x86.movq(     qword_ptr[eax+32], mm4);
        x86.movq(     qword_ptr[eax+40], mm5);
        x86.movq(     qword_ptr[eax+48], mm6);
        x86.movq(     qword_ptr[eax+56], mm7);
        x86.add(      eax, 64);
      }
      if (ploops>1)
        x86.loop(     "xloop");
      if (plrem>0) x86.movq( mm0, qword_ptr[esi+ 0]);
      if (plrem>1) x86.movq( mm1, qword_ptr[esi+ 8]);
      if (plrem>2) x86.movq( mm2, qword_ptr[esi+16]);
      if (plrem>3) x86.movq( mm3, qword_ptr[esi+24]);
      if (plrem>4) x86.movq( mm4, qword_ptr[esi+32]);
      if (plrem>5) x86.movq( mm5, qword_ptr[esi+40]);
      if (plrem>6) x86.movq( mm6, qword_ptr[esi+48]);
      if (plrem>7) x86.movq( mm7, qword_ptr[esi+56]);

      if (plrem>0) x86.movq( qword_ptr[eax+ 0], mm0);
      if (plrem>1) x86.movq( qword_ptr[eax+ 8], mm1);
      if (plrem>2) x86.movq( qword_ptr[eax+16], mm2);
      if (plrem>3) x86.movq( qword_ptr[eax+24], mm3);
      if (plrem>4) x86.movq( qword_ptr[eax+32], mm4);
      if (plrem>5) x86.movq( qword_ptr[eax+40], mm5);
      if (plrem>6) x86.movq( qword_ptr[eax+48], mm6);
      if (plrem>7) x86.movq( qword_ptr[eax+56], mm7);
    }
    x86.dec(        edi);                       // y -= 1
    x86.jnz(        "yloop");
  }
  else if (ssse3 && fir_filter_size <= 8) { // We will get too many rounding errors. Probably only lanczos etc
                                            // if taps parameter is large and very high downscale ratios. 
    x86.mov(          edx, (int)cur);
    x86.mov(          ebp, dword_ptr[(int)&src_pitch]);
    x86.mov(          ebx, dword_ptr[(int)&dst_pitch]);
    x86.mov(          edi, y);
    x86.movq(         xmm6, qword_ptr[(int)&FProundSSSE3]);        // Rounder for final division. Not touched!
    x86.movq(         xmm0, qword_ptr[(int)&UnpackByteShuffle]);   // Rounder for final division. Not touched!
    x86.pxor(         xmm5, xmm5);                                 // zeroes
    x86.punpcklqdq(   xmm6, xmm6);
    x86.punpcklqdq(   xmm0, xmm0);
                      
    x86.align(16);    
x86.label("yloop");  
    x86.mov(          esi, dword_ptr[(int)&yOfs2]);
    x86.mov(          eax, dword_ptr[edx]);               // eax = *cur
    x86.add(          edx, 4);                            // cur++
    x86.mov(          esi, dword_ptr[esi+eax*4]);
    x86.xor(          ecx, ecx);                          // ecx = x = 0
    x86.add(          esi, dword_ptr[(int)&srcp]);        // esi = srcp + yOfs[*cur]
    x86.movdqa(       xmm1, xmm6);                        // Init with rounder
    x86.movdqa(       xmm7, xmm6);
                      
    x86.align(16);    
x86.label("xloop");  
    x86.lea(          eax, dword_ptr[esi+ecx]);           // eax = srcp2 = srcp + x
    if (aligned) {    
      x86.movdqa(     xmm4, xmmword_ptr[eax]);            // xmm4 = p|o|n|m|l|k|j|i|h|g|f|e|d|c|b|a
    } else if (sse3) {
      x86.lddqu(      xmm4, xmmword_ptr[eax]); // SSE3
    } else {          
      x86.movdqu(     xmm4, xmmword_ptr[eax]);
    }

    for (int i = 0; i< fir_filter_size; i++) {
      x86.movd(       xmm3, dword_ptr[edx+i*4]);          // mm3 = cur[b] = 0|co
      x86.movdqa(     xmm2, xmm4);
      x86.punpckhbw(  xmm4, xmm5);                        // mm4 = *srcp2 = 0p|0o|0n|0m|0l|0k|0j|0i
      x86.punpcklbw(  xmm2, xmm5);                        // mm2 = *srcp2 = 0h|0g|0f|0e|0d|0c|0b|0a
      x86.pshufb(     xmm3, xmm0);                        // Unpack coefficient to all words
      x86.psllw(      xmm2, 7);                           // Extend to signed word
      x86.psllw(      xmm4, 7);                           // Extend to signed word
      x86.add(        eax, ebp);                          // srcp2 += src_pitch
      x86.pmulhrsw(   xmm2, xmm3); // SSSE3               // Multiply [h|g|f|e|d|c|b|a] 14bit(coeff) x 16bit(signed) ->
      x86.pmulhrsw(   xmm3, xmm4); // SSSE3               // Multiply [p|o|n|m|l|k|j|i] 16bit signed and rounded result.
      if (i<fir_filter_size-1) {                          // Load early for next loop
        if (aligned)
          x86.movdqa( xmm4, xmmword_ptr[eax]);            // xmm4 = p|o|n|m|l|k|j|i|h|g|f|e|d|c|b|a
        else if (sse3)
          x86.lddqu(  xmm4, xmmword_ptr[eax]); // SSE3
        else
          x86.movdqu( xmm4, xmmword_ptr[eax]);
      }
      x86.paddsw(     xmm1, xmm2);                        // Accumulate: h|g|f|e|d|c|b|a (signed words)
      x86.paddsw(     xmm7, xmm3);                        // Accumulate: p|o|n|m|l|k|j|i
    }
    x86.psraw(        xmm1, 6);                           // Compensate fraction
    x86.psraw(        xmm7, 6);                           // Compensate fraction
    x86.mov(          eax, dword_ptr[(int)&dstp]);
    x86.packuswb(     xmm1, xmm7);                        // mm1 = p|o|n|m|l|k|j|i|h|g|f|e|d|c|b|a
    x86.add(          ecx, 16);
    x86.movdqa(       xmm7, xmm6);
    x86.movdqa(       xmmword_ptr[eax+ecx-16], xmm1);     // Dest should always be aligned.
    x86.cmp(          ecx, xloops);
    x86.movdqa(       xmm1, xmm6);                        // Init with rounder
    x86.jl(           "xloop");

    x86.add(          eax, ebx);
    x86.lea(          edx, dword_ptr[edx+(fir_filter_size*4)]); //cur += fir_filter_size
    x86.dec(          edi);
    x86.mov(          dword_ptr[(int)&dstp], eax);
    x86.jnz(          "yloop");
  }
  else if (sse2) {
  // eax ebx ecx edx esi edi ebp
  // xmm0 xmm1 xmm2 xmm3 xmm4 xmm5 xmm6 xmm7
    x86.mov(     edx, (int)cur);                  // edx = &array[1] -> start_pos
    x86.mov(     ebp, dword_ptr[(int)&src_pitch]);
    x86.mov(     ebx, dword_ptr[(int)&dst_pitch]);
    x86.mov(     edi, y);                         // edi = vi.height

    x86.align(16);
x86.label("yloop");
    x86.mov(     esi, dword_ptr[(int)&yOfs2]);    // int pitch_table[height] = {0, src_pitch, src_pitch*2, ...}
    x86.mov(     eax, dword_ptr[edx]);            // eax = *cur = start_pos
    x86.mov(     esi, dword_ptr[esi+eax*4]);      // esi = yOfs[*cur] = start_pos * src_pitch
    x86.add(     edx, 4);                         // cur++  (*cur = coeff[0])
    x86.add(     esi, dword_ptr[(int)&srcp]);     // esi = srcp + yOfs[*cur] = srcp + start_pos * src_pitch
    x86.xor(     ecx, ecx);                       // ecx = x = 0

    x86.align(16);
x86.label("xloop");
    x86.lea(          eax,  dword_ptr[esi+ecx]);  // eax = srcp2 = srcp + x
    x86.movd(         xmm7, dword_ptr[(int)&FPround]);// Rounder for final division.
    if (aligned)
      x86.movdqa(     xmm0, xmmword_ptr[eax]);    // xmm0 = *srcp2 = p|o|n|m|l|k|j|i|h|g|f|e|d|c|b|a
    else if (sse3)
      x86.lddqu(      xmm0, xmmword_ptr[eax]); // SSE3
    else
      x86.movdqu(     xmm0, xmmword_ptr[eax]);
    x86.pshufd(       xmm7, xmm7, 0x00);          // totalPONM
    x86.movdqa(       xmm6, xmm7);                // totalLKJI
    x86.movdqa(       xmm5, xmm7);                // totalHGFE
    x86.movdqa(       xmm4, xmm7);                // totalDCBA
    int i=0;
    for ( ; i < fir_filter_size/2; i++) { // Doing row pairs
      if (aligned)
        x86.movdqa(     xmm2, xmmword_ptr[eax+ebp]);// xmm2 = *(srcp2+src_pitch) = P|O|N|M|L|K|J|I|H|G|F|E|D|C|B|A
      else if (sse3)
        x86.lddqu(      xmm2, xmmword_ptr[eax+ebp]); // SSE3
      else
        x86.movdqu(     xmm2, xmmword_ptr[eax+ebp]);

      x86.movq(       xmm3, qword_ptr[edx+i*8]);  // xmm3 = cur[b] = 00|00|--coeff[i+1]|--coeff[i]
      x86.lea(        eax,  dword_ptr[eax+ebp*2]);// srcp2 += src_pitch*2
      x86.movdqa(     xmm1, xmm0);                // xmm1                      = p|o|n|m|l|k|j|i|h|g|f|e|d|c|b|a
      x86.packssdw(   xmm3, xmm3);                // xmm3 = 00|00|00|00|CO|co|CO|co

      x86.punpcklbw(  xmm0, xmm2);                // xmm0 = Hh|Gg|Ff|Ee|Dd|Cc|Bb|Aa
      x86.punpckhbw(  xmm1, xmm2);                // xmm1 = Pp|Oo|Nn|Mm|Ll|Kk|Jj|Ii
      x86.pshufd(     xmm3, xmm3, 0x00);          // xmm3 = CO|co|CO|co|CO|co|CO|co

      x86.punpckhbw(  xmm2, xmm0);                // xmm2 = HP|hO|GN|gM|FL|fK|EJ|eI
      x86.punpcklbw(  xmm0, xmm0);                // xmm0 = DD|dd|CC|cc|BB|bb|AA|aa
      x86.psrlw(      xmm2, 8);                   // xmm2 = 0H|0h|0G|0g|0F|0f|0E|0e
      x86.psrlw(      xmm0, 8);                   // xmm0 = 0D|0d|0C|0c|0B|0b|0A|0a

      x86.pmaddwd(    xmm2, xmm3);                // xmm2 =  H*CO+h*co|G*CO+g*co|F*CO+f*co|E*CO+e*co
      x86.pmaddwd(    xmm0, xmm3);                // xmm0 =  D*CO+d*co|C*CO+c*co|B*CO+b*co|A*CO+a*co
      x86.paddd(      xmm5, xmm2);                // accumulateHGFE
      x86.paddd(      xmm4, xmm0);                // accumulateDCBA

      x86.pxor(       xmm0, xmm0);
      x86.movdqa(     xmm2, xmm1);                // xmm2 = Pp|Oo|Nn|Mm|Ll|Kk|Jj|Ii
      x86.punpcklbw(  xmm1, xmm0);                // xmm1 = 0L|0l|0K|0k|0J|0j|0I|0i
      x86.punpckhbw(  xmm2, xmm0);                // xmm2 = 0P|0p|0O|0o|0N|0n|0M|0m

      if (i*2 < fir_filter_size-2) {              // Load early for next loop
        if (aligned)
          x86.movdqa( xmm0, xmmword_ptr[eax]);    // xmm0 = *srcp2 = p|o|n|m|l|k|j|i|h|g|f|e|d|c|b|a
        else if (sse3)
          x86.lddqu(  xmm0, xmmword_ptr[eax]); // SSE3
        else
          x86.movdqu( xmm0, xmmword_ptr[eax]);
      }
      x86.pmaddwd(    xmm1, xmm3);                // xmm1 =  L*CO+l*co|K*CO+k*co|J*CO+j*co|I*CO+i*co
      x86.pmaddwd(    xmm2, xmm3);                // xmm4 =  P*CO+p*co|O*CO+o*co|N*CO+n*co|M*CO+m*co
      x86.paddd(      xmm6, xmm1);                // accumulateLKJI
      x86.paddd(      xmm7, xmm2);                // accumulatePONM
    }
    if (i*2 < fir_filter_size) { // Do last odd row
      x86.movd(       xmm3, dword_ptr[edx+i*8]);  // xmm3 = cur[b] = 00|00|00|--coeff[i]
      x86.pxor(       xmm2, xmm2);
      x86.punpckhbw(  xmm1, xmm0);                // xmm1 = p.|o.|n.|m.|l.|k.|j.|i.
      x86.pshufd(     xmm3, xmm3, 0x00);          // xmm3 = --co|--co|--co|--co
      x86.punpcklbw(  xmm0, xmm2);                // xmm0 = 0h|0g|0f|0e|0d|0c|0b|0a 
      x86.pslld(      xmm3, 16);                  // xmm3 = co|00|co|00|co|00|co|00
      x86.psrlw(      xmm1, 8);                   // xmm1 = 0p|0o|0n|0m|0l|0k|0j|0i
      x86.punpckhwd(  xmm2, xmm0);                // xmm2 = 0h|..|0g|..|0f|..|0e|..
      x86.punpcklwd(  xmm0, xmm0);                // xmm0 = 0d|0d|0c|0c|0b|0b|0a|0a
      x86.pmaddwd(    xmm2, xmm3);                // xmm2 =  h*co|g*co|f*co|e*co
      x86.pmaddwd(    xmm0, xmm3);                // xmm0 =  d*co|c*co|b*co|a*co
      x86.paddd(      xmm5, xmm2);                // accumulateHGFE
      x86.paddd(      xmm4, xmm0);                // accumulateDCBA
      x86.punpckhwd(  xmm2, xmm1);                // xmm2 = 0p|..|0o|..|0n|..|0m|..
      x86.punpcklwd(  xmm1, xmm1);                // xmm1 = 0l|0l|0k|0k|0j|0j|0i|0i
      x86.pmaddwd(    xmm2, xmm3);                // xmm4 =  p*co|o*co|n*co|m*co
      x86.pmaddwd(    xmm1, xmm3);                // xmm1 =  l*co|k*co|j*co|i*co
      x86.paddd(      xmm7, xmm2);                // accumulatePONM
      x86.paddd(      xmm6, xmm1);                // accumulateLKJI
    }
    x86.psrad(        xmm4, 14);                  // 14 bits -> 16bit fraction [--FF....|--FF....]
    x86.psrad(        xmm5, 14);                  // compensate the fact that FPScale = 16384
    x86.psrad(        xmm6, 14);
    x86.psrad(        xmm7, 14);
    x86.packssdw(     xmm4, xmm5);                // xmm4 = 0h|0g|0f|0e|0d|0c|0b|0a
    x86.mov(          eax, dword_ptr[(int)&dstp]);
    x86.packssdw(     xmm6, xmm7);                // xmm6 = 0p|0o|0n|0m|0l|0k|0j|0i
    x86.add(          ecx, 16);                   // x += 16
    x86.packuswb(     xmm4, xmm6);                // xmm4 = p|o|n|m|l|k|j|i|h|g|f|e|d|c|b|a
    x86.cmp(          ecx, xloops);
    x86.movdqa(       xmmword_ptr[eax+ecx-16], xmm4); // dstp[x] = p|o|n|m|l|k|j|i|h|g|f|e|d|c|b|a
    x86.jnz(          "xloop");
                     
    x86.lea(          edx, dword_ptr[edx+(fir_filter_size*4)]); // cur += fir_filter_size
    x86.add(          eax, ebx);                  // dstp += dst_pitch
    x86.dec(          edi);                       // y -= 1
    x86.mov(          dword_ptr[(int)&dstp], eax);
    x86.jnz(          "yloop");
  }
  else {
  // eax ebx ecx edx esi edi ebp
  // mm0 mm1 mm2 mm3 mm4 mm5 mm6 mm7
    x86.mov(        edx, (int)cur);              // edx = &array[1] -> start_pos
    x86.mov(        ebp, dword_ptr[(int)&src_pitch]);
    x86.mov(        ebx, dword_ptr[(int)&dst_pitch]);
    x86.mov(        edi, y);                     // edi = vi.height

    x86.align(16);
x86.label("yloop");
    x86.mov(        esi, dword_ptr[(int)&yOfs2]);// int pitch_table[height] = {0, src_pitch, src_pitch*2, ...}
    x86.mov(        eax, dword_ptr[edx]);        // eax = *cur = start_pos
    x86.mov(        esi, dword_ptr[esi+eax*4]);  // esi = yOfs[*cur] = start_pos * src_pitch
    x86.add(        edx, 4);                     // cur++  (*cur = coeff[0])
    x86.add(        esi, dword_ptr[(int)&srcp]); // esi = srcp + yOfs[*cur] = srcp + start_pos * src_pitch
    x86.xor(        ecx, ecx);                   // ecx = x = 0

    x86.align(16);
x86.label("xloop");
    x86.lea(        eax, qword_ptr[esi+ecx*8]);  // eax = srcp2 = srcp + x
    x86.movq(       mm7, qword_ptr[(int)&FPround]);// totalHG = Rounder for final division.
    x86.movq(       mm0, qword_ptr[eax]);        // mm0 = *srcp2             = h|g|f|e|d|c|b|a
    x86.movq(       mm6, mm7);                   // totalFE
    x86.movq(       mm5, mm7);                   // totalDC
    x86.movq(       mm4, mm7);                   // totalBA
    int i=0;
    for ( ; i < fir_filter_size/2; i++) { // Doing row pairs
      x86.movq(       mm2, qword_ptr[eax+ebp]);  // mm2 = *(srcp2+src_pitch) = H|G|F|E|D|C|B|A
      x86.movq(       mm3, qword_ptr[edx+i*8]);  // mm3 = cur[b] = --coeff[i+1]|--coeff[i]
      x86.lea(        eax, qword_ptr[eax+ebp*2]);// srcp2 += src_pitch*2
      x86.movq(       mm1, mm0);                 // mm1                      = h|g|f|e|d|c|b|a
      x86.packssdw(   mm3, mm3);                 // mm3 = CO|co|CO|co

      x86.punpcklbw(  mm0, mm2);                 // mm0 = Dd|Cc|Bb|Aa
      x86.punpckhbw(  mm1, mm2);                 // mm1 = Hh|Gg|Ff|Ee

      x86.punpckhbw(  mm2, mm0);                 // mm2 = DH|dG|CF|cE
      x86.punpcklbw(  mm0, mm0);                 // mm0 = BB|bb|AA|aa
      x86.psrlw(      mm2, 8);                   // mm2 = 0D|0d|0C|0c
      x86.psrlw(      mm0, 8);                   // mm0 = 0B|0b|0A|0a

      x86.pmaddwd(    mm2, mm3);                 // mm2 = D*CO+d*co|C*CO+c*co
      x86.pmaddwd(    mm0, mm3);                 // mm0 = B*CO+b*co|A*CO+a*co
      x86.paddd(      mm5, mm2);                 // accumulateDC
      x86.paddd(      mm4, mm0);                 // accumulateBA

      x86.pxor(       mm0, mm0);
      x86.movq(       mm2, mm1);                 // mm2 = Hh|Gg|Ff|Ee
      x86.punpcklbw(  mm1, mm0);                 // mm1 = 0F|0f|0E|0e
      x86.punpckhbw(  mm2, mm0);                 // mm2 = 0H|0h|0G|0g

      if (i*2 < fir_filter_size-2)               // Load early for next loop
        x86.movq(     mm0, qword_ptr[eax]);      // mm0 = *srcp2             = h|g|f|e|d|c|b|a

      x86.pmaddwd(    mm1, mm3);                 // mm1 = F*CO+f*co|E*CO+e*co
      x86.pmaddwd(    mm2, mm3);                 // mm2 = H*CO+h*co|G*CO+g*co
      x86.paddd(      mm6, mm1);                 // accumulateFE
      x86.paddd(      mm7, mm2);                 // accumulateHG
    }
    if (i*2 < fir_filter_size) { // Do last odd row
      x86.movd(       mm3, dword_ptr[edx+i*8]);  // mm3 = cur[b] = 0|--coeff[i]
      x86.pxor(       mm2, mm2);
      x86.punpckhbw(  mm1, mm0);                 // mm1 = h.|g.|f.|e.
      if (isse) {
        x86.punpcklbw(mm0, mm2);                 // mm0 = 0d|0c|0b|0a
        x86.pshufw(   mm3, mm3, 0x33);           // mm3 = co|00|co|00
      } else {
        x86.punpckldq(mm3, mm3);                 // mm3 = --co|--co
        x86.punpcklbw(mm0, mm2);                 // mm0 = 0d|0c|0b|0a
        x86.pslld(    mm3, 16);                  // mm3 = co|00|co|00
      }
      x86.psrlw(      mm1, 8);                   // mm1 = 0h|0g|0f|0e
      x86.punpckhwd(  mm2, mm0);                 // mm2 = 0d|..|0c|..
      x86.punpcklwd(  mm0, mm0);                 // mm0 = 0b|0b|0a|0a
      x86.pmaddwd(    mm2, mm3);                 // mm2 =  d*co|c*co
      x86.pmaddwd(    mm0, mm3);                 // mm0 =  b*co|a*co
      x86.paddd(      mm5, mm2);                 // accumulateDC
      x86.paddd(      mm4, mm0);                 // accumulateBA
      x86.punpckhwd(  mm2, mm1);                 // mm2 = 0h|..|0g|..
      x86.punpcklwd(  mm1, mm1);                 // mm1 = 0f|0f|0e|0e
      x86.pmaddwd(    mm2, mm3);                 // mm2 =  h*co|g*co
      x86.pmaddwd(    mm1, mm3);                 // mm1 =  f*co|e*co
      x86.paddd(      mm7, mm2);                 // accumulateHG
      x86.paddd(      mm6, mm1);                 // accumulateFE
    }
    x86.psrad(      mm4, 14);                    // 14 bits -> 16bit fraction [--FF....|--FF....]
    x86.psrad(      mm5, 14);                    // compensate the fact that FPScale = 16384
    x86.psrad(      mm6, 14);
    x86.psrad(      mm7, 14);
    x86.packssdw(   mm4, mm5);                   // mm4 = 0d|0c|0b|0a
    x86.mov(        eax, dword_ptr[(int)&dstp]);
    x86.packssdw(   mm6, mm7);                   // mm6 = 0h|0g|0f|0e
    x86.inc(        ecx);                        // x += 1
    x86.packuswb(   mm4, mm6);                   // mm4 = h|g|f|e|d|c|b|a
    x86.cmp(        ecx, xloops);
    x86.movq(       qword_ptr[eax+ecx*8-8], mm4); // dstp[x] = h|g|f|e|d|c|b|a
    x86.jnz(        "xloop");

    x86.lea(        edx, dword_ptr[edx+(fir_filter_size*4)]);        // cur += fir_filter_size
    x86.add(        eax, ebx);                   // dstp += dst_pitch
    x86.dec(        edi);                        // y -= 1
    x86.mov(        dword_ptr[(int)&dstp], eax);
    x86.jnz(        "yloop");
  }
  // No more mmx for now
  x86.emms();
  // Restore registers
  x86.pop(          ebp);
  x86.pop(          edi);
  x86.pop(          esi);
  x86.pop(          edx);
  x86.pop(          ecx);
  x86.pop(          ebx);
  x86.pop(          eax);
  x86.ret();

  return DynamicAssembledCode(x86, env, "ResizeV: Dynamic code could not be compiled.");
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
	try {	// HIDE DAMN SEH COMPILER BUG!!!
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
	catch (...) { throw; }
}

AVSValue __cdecl FilteredResize::Create_PointResize(AVSValue args, void*, IScriptEnvironment* env)
{
  PointFilter _PointFilter;

  return CreateResize( args[0].AsClip(), args[1].AsInt(), args[2].AsInt(), &args[3],
                       &_PointFilter, env );
}


AVSValue __cdecl FilteredResize::Create_BilinearResize(AVSValue args, void*, IScriptEnvironment* env)
{
  TriangleFilter _TriangleFilter;

  return CreateResize( args[0].AsClip(), args[1].AsInt(), args[2].AsInt(), &args[3],
                       &_TriangleFilter, env );
}


AVSValue __cdecl FilteredResize::Create_BicubicResize(AVSValue args, void*, IScriptEnvironment* env)
{
  MitchellNetravaliFilter _MitchellNetravaliFilter(args[3].AsDblDef(1./3.), args[4].AsDblDef(1./3.));

  return CreateResize( args[0].AsClip(), args[1].AsInt(), args[2].AsInt(), &args[5],
                       &_MitchellNetravaliFilter, env );
}

AVSValue __cdecl FilteredResize::Create_LanczosResize(AVSValue args, void*, IScriptEnvironment* env)
{
	try {	// HIDE DAMN SEH COMPILER BUG!!!
  LanczosFilter _LanczosFilter(args[7].AsInt(3));

  return CreateResize( args[0].AsClip(), args[1].AsInt(), args[2].AsInt(), &args[3],
                       &_LanczosFilter, env );
	}
	catch (...) { throw; }
}

AVSValue __cdecl FilteredResize::Create_Lanczos4Resize(AVSValue args, void*, IScriptEnvironment* env)
{
	try {	// HIDE DAMN SEH COMPILER BUG!!!
  LanczosFilter _LanczosFilter(4);

  return CreateResize( args[0].AsClip(), args[1].AsInt(), args[2].AsInt(), &args[3],
                       &_LanczosFilter, env );
	}
	catch (...) { throw; }
}

AVSValue __cdecl FilteredResize::Create_BlackmanResize(AVSValue args, void*, IScriptEnvironment* env)
{
	try {	// HIDE DAMN SEH COMPILER BUG!!!
  BlackmanFilter _BlackmanFilter(args[7].AsInt(4));

  return CreateResize( args[0].AsClip(), args[1].AsInt(), args[2].AsInt(), &args[3],
                       &_BlackmanFilter, env );
	}
	catch (...) { throw; }
}

AVSValue __cdecl FilteredResize::Create_Spline16Resize(AVSValue args, void*, IScriptEnvironment* env)
{
  Spline16Filter _Spline16Filter;

  return CreateResize( args[0].AsClip(), args[1].AsInt(), args[2].AsInt(), &args[3],
                       &_Spline16Filter, env );
}

AVSValue __cdecl FilteredResize::Create_Spline36Resize(AVSValue args, void*, IScriptEnvironment* env)
{
  Spline36Filter _Spline36Filter;

  return CreateResize( args[0].AsClip(), args[1].AsInt(), args[2].AsInt(), &args[3],
                       &_Spline36Filter, env );
}

AVSValue __cdecl FilteredResize::Create_Spline64Resize(AVSValue args, void*, IScriptEnvironment* env)
{
  Spline64Filter _Spline64Filter;

  return CreateResize( args[0].AsClip(), args[1].AsInt(), args[2].AsInt(), &args[3],
                       &_Spline64Filter, env );
}

AVSValue __cdecl FilteredResize::Create_GaussianResize(AVSValue args, void*, IScriptEnvironment* env)
{
	try {	// HIDE DAMN SEH COMPILER BUG!!!
  GaussianFilter _GaussianFilter(args[7].AsFloat(30.0f));

  return CreateResize( args[0].AsClip(), args[1].AsInt(), args[2].AsInt(), &args[3],
                       &_GaussianFilter, env );
	}
	catch (...) { throw; }
}

AVSValue __cdecl FilteredResize::Create_SincResize(AVSValue args, void*, IScriptEnvironment* env)
{
	try {	// HIDE DAMN SEH COMPILER BUG!!!
  SincFilter _SincFilter(args[7].AsInt(4));

  return CreateResize( args[0].AsClip(), args[1].AsInt(), args[2].AsInt(), &args[3],
                       &_SincFilter, env );
	}
	catch (...) { throw; }
}

