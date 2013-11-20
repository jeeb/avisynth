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


// Avisynth filter:  Swap planes
// by Klaus Post
// adapted by Richard Berg (avisynth-dev@richardberg.net)
// iSSE code by Ian Brabham


#include "planeswap.h"
#include "../core/internal.h"
#include <tmmintrin.h>
#include <avs/alignment.h>


/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

extern const AVSFunction Swap_filters[] = {
  {  "SwapUV","c", SwapUV::CreateSwapUV },
  {  "UToY","c", SwapUVToY::CreateUToY },
  {  "VToY","c", SwapUVToY::CreateVToY },
  {  "UToY8","c", SwapUVToY::CreateUToY8 },
  {  "VToY8","c", SwapUVToY::CreateVToY8 },
  {  "YToUV","cc", SwapYToUV::CreateYToUV },
  {  "YToUV","ccc", SwapYToUV::CreateYToYUV },
  { 0 }
};


/**************************************
 *  Swap - swaps UV on planar maps
 **************************************/

typedef __m128i (SseYuy2Swap)(__m128i, __m128i);

__forceinline __m128i sse2_yuy2_swap_register(__m128i src, __m128i zero) {
  __m128i src_unpck_lo = _mm_unpacklo_epi8(src, zero); //0V0Y0U0Y x2
  __m128i src_unpck_hi = _mm_unpackhi_epi8(src, zero); 

  src_unpck_lo = _mm_shufflelo_epi16(src_unpck_lo, _MM_SHUFFLE(1, 2, 3, 0)); 
  src_unpck_lo = _mm_shufflehi_epi16(src_unpck_lo, _MM_SHUFFLE(1, 2, 3, 0));
  src_unpck_hi = _mm_shufflelo_epi16(src_unpck_hi, _MM_SHUFFLE(1, 2, 3, 0)); 
  src_unpck_hi = _mm_shufflehi_epi16(src_unpck_hi, _MM_SHUFFLE(1, 2, 3, 0));

  return _mm_packus_epi16(src_unpck_lo, src_unpck_hi);
}

__forceinline __m128i ssse3_yuy2_swap_register(__m128i src, __m128i) {
  return _mm_shuffle_epi8(src, _mm_set_epi8(13, 14, 15, 12, 9, 10, 11, 8, 5, 6, 7, 4, 1, 2, 3, 0));
}

template<SseYuy2Swap swap>
static void ssex_yuy2_swap(const BYTE* srcp, BYTE* dstp, int src_pitch, int dst_pitch, int width, int height)
{
  int mod16width = width / 16 * 16;

  __m128i zero = _mm_setzero_si128();

  for (int y = 0; y < height; ++y ) {
    for (int x = 0; x < mod16width; x+= 16) {
      __m128i src = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp+x)); 
      __m128i dst = swap(src, zero);
      _mm_store_si128(reinterpret_cast<__m128i*>(dstp+x), dst);
    }
    
    if (mod16width != width) {
      int x = width-16;
      __m128i src = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcp+x)); 
      __m128i dst = swap(src, zero);
      _mm_storeu_si128(reinterpret_cast<__m128i*>(dstp+x), dst);
    }

    dstp += dst_pitch;
    srcp += src_pitch;
  }
}


#ifdef X86_32

static __forceinline __m64 isse_yuy2_swap_register(__m64 src, __m64 zero) {
  __m64 src_unpck_lo = _mm_unpacklo_pi8(src, zero); //0V0Y0U0Y
  __m64 src_unpck_hi = _mm_unpackhi_pi8(src, zero); 

  src_unpck_lo = _mm_shuffle_pi16(src_unpck_lo, _MM_SHUFFLE(1, 2, 3, 0)); 
  src_unpck_hi = _mm_shuffle_pi16(src_unpck_hi, _MM_SHUFFLE(1, 2, 3, 0)); 

  return _mm_packs_pu16(src_unpck_lo, src_unpck_hi);
}

static void isse_yuy2_swap(const BYTE* srcp, BYTE* dstp, int src_pitch, int dst_pitch, int width, int height)
{
  int mod8width = width / 8 * 8;

  __m64 zero = _mm_setzero_si64();

  for (int y = 0; y < height; ++y ) {
    for (int x = 0; x < mod8width; x+= 8) {
      __m64 src = *reinterpret_cast<const __m64*>(srcp+x); 
      __m64 dst = isse_yuy2_swap_register(src, zero);
      *reinterpret_cast<__m64*>(dstp+x) = dst;
    }

    if (mod8width != width) {
      int x = width-8;
      __m64 src = *reinterpret_cast<const __m64*>(srcp+x); 
      __m64 dst = isse_yuy2_swap_register(src, zero);
      *reinterpret_cast<__m64*>(dstp+x) = dst;
    }

    dstp += dst_pitch;
    srcp += src_pitch;
  }
  _mm_empty();
}
#endif



AVSValue __cdecl SwapUV::CreateSwapUV(AVSValue args, void* user_data, IScriptEnvironment* env) {
  PClip p = args[0].AsClip();
  if (p->GetVideoInfo().IsY8())
    return p;
  return new SwapUV(p, env);
}


SwapUV::SwapUV(PClip _child, IScriptEnvironment* env)
  : GenericVideoFilter(_child) {

  if (!vi.IsYUV())
    env->ThrowError("SwapUV: YUV data only!");    
}

PVideoFrame __stdcall SwapUV::GetFrame(int n, IScriptEnvironment* env) {
  PVideoFrame src = child->GetFrame(n, env);
  
  if (vi.IsPlanar()) {
    // Abuse subframe to flip the UV plane pointers -- extremely fast but a bit naughty!
    const int uvoffset = src->GetOffset(PLANAR_V) - src->GetOffset(PLANAR_U); // very naughty - don't do this at home!!
        
    return env->SubframePlanar(src, 0, src->GetPitch(PLANAR_Y), src->GetRowSize(PLANAR_Y), src->GetHeight(PLANAR_Y),
                         uvoffset, -uvoffset, src->GetPitch(PLANAR_V));
  }
  else if (vi.IsYUY2()) { // YUY2
    BYTE* srcp = src->GetWritePtr(); // Returns 0 if not writable
    if (srcp) { // Do it in place!
      for (int y=0; y<vi.height; y++) {
        for (int x = 0; x < src->GetRowSize(); x+=4) {
          const BYTE t = srcp[x+3]; // This is surprisingly fast,
          srcp[x+3] = srcp[x+1];    // faster than any MMX/SSE code
          srcp[x+1] = t;            // I could write.
        }
        srcp += src->GetPitch();
      }
      return src;
    }
    else { // avoid the cost of a frame blit, we have to parse the frame anyway
      const BYTE* srcp = src->GetReadPtr();
      PVideoFrame dst = env->NewVideoFrame(vi);
      
      if ((env->GetCPUFlags() & CPUF_SSSE3) && IsPtrAligned(srcp, 16) && dst->GetRowSize() >= 16) {
        ssex_yuy2_swap<ssse3_yuy2_swap_register>(srcp, dst->GetWritePtr(), src->GetPitch(), dst->GetPitch(), dst->GetRowSize(), src->GetHeight());
      } else if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(srcp, 16) && dst->GetRowSize() >= 16) {
        ssex_yuy2_swap<sse2_yuy2_swap_register>(srcp, dst->GetWritePtr(), src->GetPitch(), dst->GetPitch(), dst->GetRowSize(), src->GetHeight());
      } else
#ifdef X86_32
      if ((env->GetCPUFlags() & CPUF_INTEGER_SSE) && dst->GetRowSize() >= 8)   // need pshufw
      {
        isse_yuy2_swap(srcp, dst->GetWritePtr(), src->GetPitch(), dst->GetPitch(), dst->GetRowSize(), src->GetHeight());
      }
      else
#endif
      {
        short* dstp = (short*)dst->GetWritePtr();
        const int srcpitch = src->GetPitch();
        const int dstpitch = dst->GetPitch()>>1;
        const int endx = dst->GetRowSize()>>1;
        for (int y=0; y<vi.height; y++) {
          for (int x = 0; x < endx; x+=2) {
            // The compiler generates very good code for this construct 
            // using ah, al & ax register variants to very good effect.
            dstp[x+0] = (srcp[x*2+3] << 8) | srcp[x*2+0];
            dstp[x+1] = (srcp[x*2+1] << 8) | srcp[x*2+2];
          }
          srcp += srcpitch;
          dstp += dstpitch;
        }
      }
      return dst;
    }
  }
  return src;
}


AVSValue __cdecl SwapUVToY::CreateUToY(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return new SwapUVToY(args[0].AsClip(), UToY, env);
}

AVSValue __cdecl SwapUVToY::CreateUToY8(AVSValue args, void* user_data, IScriptEnvironment* env) {
  PClip clip = args[0].AsClip();
  return new SwapUVToY(clip, (clip->GetVideoInfo().IsYUY2()) ? YUY2UToY8 : UToY8, env);
}

AVSValue __cdecl SwapUVToY::CreateVToY(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return new SwapUVToY(args[0].AsClip(), VToY, env);
}

AVSValue __cdecl SwapUVToY::CreateVToY8(AVSValue args, void* user_data, IScriptEnvironment* env) {
  PClip clip = args[0].AsClip();
  return new SwapUVToY(clip, (clip->GetVideoInfo().IsYUY2()) ? YUY2VToY8 : VToY8, env);
}

SwapUVToY::SwapUVToY(PClip _child, int _mode, IScriptEnvironment* env)
  : GenericVideoFilter(_child), mode(_mode) {

  if (!vi.IsYUV())
    env->ThrowError("UVtoY: YUV data only!");

  if (vi.IsY8()) 
    env->ThrowError("UVtoY: There are no chroma channels in Y8!");

  vi.height >>= vi.GetPlaneHeightSubsampling(PLANAR_U);
  vi.width  >>= vi.GetPlaneWidthSubsampling(PLANAR_U);

  if (mode == UToY8 || mode == VToY8 || mode == YUY2UToY8 || mode == YUY2VToY8)
    vi.pixel_type = VideoInfo::CS_Y8;

}


PVideoFrame __stdcall SwapUVToY::GetFrame(int n, IScriptEnvironment* env) {
  PVideoFrame src = child->GetFrame(n, env);

  if (vi.IsYUY2()) {  // YUY2 interleaved
    PVideoFrame dst = env->NewVideoFrame(vi);
    const BYTE* srcp = src->GetReadPtr();
    short* dstp = (short*)dst->GetWritePtr();
    const int srcpitch = src->GetPitch();
    const int dstpitch = dst->GetPitch()>>1;
    const int endx = dst->GetRowSize()>>1;
    if (mode==UToY) {
      for (int y=0; y<vi.height; y++) {
        for (int x = 0; x < endx; x+=2) {
          dstp[x  ] = 0x8000 | srcp[x*4+1];
          dstp[x+1] = 0x8000 | srcp[x*4+5];
        }
        srcp += srcpitch;
        dstp += dstpitch;
      }
    }
    else if (mode==VToY) {
      for (int y=0; y<vi.height; y++) {
        for (int x = 0; x < endx; x+=2) {
          dstp[x  ] = 0x8000 | srcp[x*4+3];
          dstp[x+1] = 0x8000 | srcp[x*4+7];
        }
        srcp += srcpitch;
        dstp += dstpitch;
      }
    }
    return dst;
  }

  // Planar

  if (mode==UToY8) {
    const int offset = src->GetOffset(PLANAR_U) - src->GetOffset(PLANAR_Y); // very naughty - don't do this at home!!
    // Abuse Subframe to snatch the U plane
    return env->Subframe(src, offset, src->GetPitch(PLANAR_U), src->GetRowSize(PLANAR_U), src->GetHeight(PLANAR_U));
  }
  else if (mode == VToY8) {
    const int offset = src->GetOffset(PLANAR_V) - src->GetOffset(PLANAR_Y); // very naughty - don't do this at home!!
    // Abuse Subframe to snatch the V plane
    return env->Subframe(src, offset, src->GetPitch(PLANAR_V), src->GetRowSize(PLANAR_V), src->GetHeight(PLANAR_V));
  }

  PVideoFrame dst = env->NewVideoFrame(vi);

  if (mode==YUY2UToY8 || mode==YUY2VToY8) {  // YUY2 U To Y
    const BYTE* srcp = src->GetReadPtr();
    BYTE* dstp = (BYTE*)dst->GetWritePtr(PLANAR_Y);
    srcp += (mode==YUY2UToY8) ? 1 : 3;
    for (int y=0; y<vi.height; y++) {
      for (int x = 0; x < vi.width; x++) {
        dstp[x] = srcp[(x<<2)];
      }
      srcp += src->GetPitch();
      dstp += dst->GetPitch(PLANAR_Y);
    }      
    return dst;
  }

  if (mode==UToY) {
    env->BitBlt(dst->GetWritePtr(PLANAR_Y), dst->GetPitch(PLANAR_Y),
                src->GetReadPtr(PLANAR_U), src->GetPitch(PLANAR_U), dst->GetRowSize(), dst->GetHeight());
  }
  else if (mode==VToY) {
    env->BitBlt(dst->GetWritePtr(PLANAR_Y), dst->GetPitch(PLANAR_Y),
                src->GetReadPtr(PLANAR_V), src->GetPitch(PLANAR_V), dst->GetRowSize(), dst->GetHeight());
  }

  // Clear chroma
  const int pitch = dst->GetPitch(PLANAR_U)/4;
  const int myx = (dst->GetRowSize(PLANAR_U)+3)/4;
  const int myy = dst->GetHeight(PLANAR_U);

  int *srcpUV = (int*)dst->GetWritePtr(PLANAR_U);
  {for (int y=0; y<myy; y++) {
    for (int x=0; x<myx; x++) {
      srcpUV[x] = 0x80808080;  // mod 8
    }
    srcpUV += pitch;
  }}

  srcpUV = (int*)dst->GetWritePtr(PLANAR_V);
  {for (int y=0; y<myy; ++y) {
    for (int x=0; x<myx; x++) {
      srcpUV[x] = 0x80808080;  // mod 8
    }
    srcpUV += pitch;
  }}
  return dst;
}


AVSValue __cdecl SwapYToUV::CreateYToUV(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return new SwapYToUV(args[0].AsClip(), args[1].AsClip(), NULL , env);
}

AVSValue __cdecl SwapYToUV::CreateYToYUV(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return new SwapYToUV(args[0].AsClip(), args[1].AsClip(), args[2].AsClip(), env);
}


SwapYToUV::SwapYToUV(PClip _child, PClip _clip, PClip _clipY, IScriptEnvironment* env)
  : GenericVideoFilter(_child), clip(_clip), clipY(_clipY) {

  if (!vi.IsYUV())
    env->ThrowError("YToUV: Only YUV data accepted");

  VideoInfo vi2=clip->GetVideoInfo();
  if (vi.height!=vi2.height)
    env->ThrowError("YToUV: Clips do not have the same height (U & V mismatch) !");
  if (vi.width!=vi2.width)
    env->ThrowError("YToUV: Clips do not have the same width (U & V mismatch) !");
  if (vi.IsYUY2() != vi2.IsYUY2()) 
    env->ThrowError("YToUV: YUY2 Clips must have same colorspace (U & V mismatch) !");

  if (clipY) {
    VideoInfo vi3=clipY->GetVideoInfo();
    if (vi.IsYUY2() != vi3.IsYUY2()) 
      env->ThrowError("YToUV: YUY2 Clips must have same colorspace (UV & Y mismatch) !");

    if (vi.IsYUY2()) {
      if (vi3.height != vi.height)
        env->ThrowError("YToUV: Y clip does not have the same height of the UV clips! (YUY2 mode)");

      vi.width *= 2;
      if (vi3.width!=vi.width)
        env->ThrowError("YToUV: Y clip does not have the double width of the UV clips!");
    }
    else {  // Autogenerate destination colorformat
      vi.pixel_type = VideoInfo::CS_YV12; // CS_Sub_Width_2 and CS_Sub_Height_2 are 0

      if (vi3.width == vi.width) {
        vi.pixel_type |= VideoInfo::CS_Sub_Width_1;
      }
      else if (vi3.width == vi.width * 4) {
        vi.pixel_type |= VideoInfo::CS_Sub_Width_4;
        vi.width *= 4;
      }
      else if (vi3.width != vi.width * 2) {
        env->ThrowError("YToUV: Video width ratio does not match any internal colorspace.");
      }
      else {
        vi.width *= 2;
      }
       
      if (vi3.height == vi.height) {
        vi.pixel_type |= VideoInfo::CS_Sub_Height_1;
      }
      else if (vi3.height == vi.height * 4) {
        vi.pixel_type |= VideoInfo::CS_Sub_Height_4;
        vi.height *= 4;
      }
      else if (vi3.height != vi.height * 2) {
        env->ThrowError("YToUV: Video height ratio does not match any internal colorspace.");
      }
      else {
        vi.height *= 2;
      }
    }
  }
  else {
    if (vi.IsYUY2())
      vi.width <<= 1;
    else if (vi.IsY8())
      vi.pixel_type = VideoInfo::CS_YV24;
    else {
      vi.height <<= vi.GetPlaneHeightSubsampling(PLANAR_U);
      vi.width  <<= vi.GetPlaneWidthSubsampling(PLANAR_U);
    }
  }
}

PVideoFrame __stdcall SwapYToUV::GetFrame(int n, IScriptEnvironment* env) {
  PVideoFrame src = child->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrame(vi);
  
  if (vi.IsYUY2()) {
    const BYTE* srcpU = src->GetReadPtr();
    const int srcUpitch = src->GetPitch();
    
    PVideoFrame srcV = clip->GetFrame(n, env);
    const BYTE* srcpV = srcV->GetReadPtr();
    const int srcVpitch = srcV->GetPitch();
    
    short* dstp = (short*)dst->GetWritePtr();
    const int endx = dst->GetRowSize()>>1;
    const int dstpitch = dst->GetPitch()>>1;
    
    if (clipY) {
      PVideoFrame srcY = clipY->GetFrame(n, env);
      const BYTE* srcpY = srcY->GetReadPtr();
      const int srcYpitch = srcY->GetPitch();
      
      for (int y=0; y<vi.height; y++) {
        for (int x = 0; x < endx; x+=2) {
          dstp[x+0] = (srcpU[x] << 8) | srcpY[x*2+0];
          dstp[x+1] = (srcpV[x] << 8) | srcpY[x*2+2];
        }
        srcpY += srcYpitch;
        srcpU += srcUpitch;
        srcpV += srcVpitch;
        dstp += dstpitch;
      }
    }
    else {
      for (int y=0; y<vi.height; y++) {
        for (int x = 0; x < endx; x+=2) {
          dstp[x+0] = (srcpU[x] << 8) | 0x7e;  // Luma = 126
          dstp[x+1] = (srcpV[x] << 8) | 0x7e;
        }
        srcpU += srcUpitch;
        srcpV += srcVpitch;
        dstp += dstpitch;
      }
    }
    return dst;
  }
  // Planar:
  env->BitBlt(dst->GetWritePtr(PLANAR_U), dst->GetPitch(PLANAR_U),
              src->GetReadPtr(PLANAR_Y), src->GetPitch(PLANAR_Y), src->GetRowSize(PLANAR_Y), src->GetHeight(PLANAR_Y));
  
  src = clip->GetFrame(n, env);
  env->BitBlt(dst->GetWritePtr(PLANAR_V), dst->GetPitch(PLANAR_V),
              src->GetReadPtr(PLANAR_Y), src->GetPitch(PLANAR_Y), src->GetRowSize(PLANAR_Y), src->GetHeight(PLANAR_Y));
  
  if (!clipY) {
    // Luma = 126 (0x7e)
    const int pitch = dst->GetPitch(PLANAR_Y)/4;
    int *dstpY = (int*)dst->GetWritePtr(PLANAR_Y);
    const int myx = (dst->GetRowSize(PLANAR_Y)+3)/4;
    const int myy = dst->GetHeight(PLANAR_Y);
    for (int y=0; y<myy; y++) {
      for (int x=0; x<myx; x++) {
        dstpY[x] = 0x7e7e7e7e;  // mod 4
      }
      dstpY += pitch;
    }
  } else {
    src = clipY->GetFrame(n, env);
    env->BitBlt(dst->GetWritePtr(PLANAR_Y), dst->GetPitch(PLANAR_Y),
                src->GetReadPtr(PLANAR_Y), src->GetPitch(PLANAR_Y), src->GetRowSize(PLANAR_Y), src->GetHeight(PLANAR_Y));
  }
  return dst;
}
