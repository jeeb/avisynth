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


// Avisynth filter: YUV merge / Swap planes
// by Klaus Post (kp@interact.dk)
// adapted by Richard Berg (avisynth-dev@richardberg.net)


#include "stdafx.h"

#include "merge.h"

/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

AVSFunction Merge_filters[] = {
  { "MergeChroma", "cc[chromaweight]f", MergeChroma::Create },  // src, chroma src, weight
  { "MergeLuma", "cc[lumaweight]f", MergeLuma::Create },      // src, luma src, weight
  {  "SwapUV","c", Swap::CreateUV },
  {  "UToY","c", Swap::CreateUToY },
  {  "VToY","c", Swap::CreateVToY },
  {  "YToUV","cc", Swap::CreateYToUV },
  {  "YToUV","ccc", Swap::CreateYToYUV },
  { 0 }
};


/**************************************
 *  Swap - swaps UV on planar maps
 **************************************/


AVSValue __cdecl Swap::CreateUV(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return new Swap(args[0].AsClip(), args[0].AsClip(), args[0].AsClip(), 1 , env);
}

AVSValue __cdecl Swap::CreateUToY(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return new Swap(args[0].AsClip(), args[0].AsClip(), args[0].AsClip(), 2 , env);
}

AVSValue __cdecl Swap::CreateVToY(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return new Swap(args[0].AsClip(), args[0].AsClip(), args[0].AsClip(), 3 , env);
}

AVSValue __cdecl Swap::CreateYToUV(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return new Swap(args[0].AsClip(), args[1].AsClip(), NULL , 4 , env);
}

AVSValue __cdecl Swap::CreateYToYUV(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return new Swap(args[0].AsClip(), args[1].AsClip(), args[2].AsClip(), 4 , env);
}

Swap::Swap(PClip _child, PClip _clip, PClip _clipY, int _mode, IScriptEnvironment* env) 
  : GenericVideoFilter(_child), clip(_clip), clipY(_clipY), mode(_mode) {
    VideoInfo vi2=clip->GetVideoInfo();
    VideoInfo vi3;

  if (clipY) 
    vi3=clipY->GetVideoInfo();

  if (!vi.IsYUV() || !vi2.IsYUV()) 
    env->ThrowError("Plane swapper: YUV data only!");  // Todo: report correct filter

  switch (mode) {
  case 1:
    break;
  case 2:
  case 3:
    if (vi.IsYV12()) vi.height >>=1;
    if (vi.IsYUY2() || vi.IsYV12()) vi.width >>=1;
    break;
  case 4:
    if (!(vi.IsYV12() == vi2.IsYV12() || vi.IsYUY2() == vi2.IsYUY2())) {
       env->ThrowError("YToUV: Clips must be the same colorspace!");
    }
    if (vi.height!=vi2.height)
      env->ThrowError("YToUV: Clips does not have the same height!");
    if (vi.width!=vi2.width)
      env->ThrowError("YToUV: Clips does not have the same width!");
    if (clipY) {
      if (!(vi3.IsYV12() == vi.IsYV12() || vi.IsYUY2() == vi3.IsYUY2())) 
        env->ThrowError("YToUV: Y clip must be be same colorspace as the UV clips!");

      if ((vi3.width/2)!=vi.width)
        env->ThrowError("YToUV: Y clip does not have the double width of the UV clips!");

      if (vi.IsYV12() && (vi3.height/2)!=vi.height)
        env->ThrowError("YToUV: Y clip does not have the double height of the UV clips! (YV12 mode)");

      if (vi.IsYUY2() && vi3.height!=vi.height)
        env->ThrowError("YToUV: Y clip does not have the same height of the UV clips! (YUY2 mode)");
    }

    if (!vi.IsYUY2()) vi.height <<=1;
    vi.width <<=1;
    break;
  }
}

PVideoFrame __stdcall Swap::GetFrame(int n, IScriptEnvironment* env) {
  PVideoFrame src = child->GetFrame(n, env);

  if (mode==1) {  // SwapUV
    if (vi.IsPlanar()) {
      PVideoFrame dst = env->NewVideoFrame(vi);
      const BYTE* srcpY=src->GetReadPtr(PLANAR_Y);  // Y Plane must be touched!
      const BYTE* srcpV=src->GetReadPtr(PLANAR_V);
      const BYTE* srcpU=src->GetReadPtr(PLANAR_U);
    
      env->BitBlt(dst->GetWritePtr(PLANAR_Y),dst->GetPitch(PLANAR_Y),srcpY,src->GetPitch(PLANAR_Y),src->GetRowSize(PLANAR_Y),src->GetHeight(PLANAR_Y));
      env->BitBlt(dst->GetWritePtr(PLANAR_U),dst->GetPitch(PLANAR_U),srcpV,src->GetPitch(PLANAR_V),src->GetRowSize(PLANAR_V),src->GetHeight(PLANAR_V));
      env->BitBlt(dst->GetWritePtr(PLANAR_V),dst->GetPitch(PLANAR_V),srcpU,src->GetPitch(PLANAR_V),src->GetRowSize(PLANAR_V),src->GetHeight(PLANAR_V));

      return dst;
    } else if (vi.IsYUY2()) { // YUY2
      env->MakeWritable(&src);
      BYTE* srcp = src->GetWritePtr();  
      for (int y=0; y<vi.height; y++) {
        for (int x = 0; x < src->GetRowSize(); x+=4) {
          BYTE t = srcp[x+3];
          srcp[x+3] = srcp[x+1];
          srcp[x+1] = t;
        }
        srcp += src->GetPitch();
      }
      return src;
    }
  }
  if (mode==2 || mode ==3) {  // U to Y or V to Y
    PVideoFrame dst = env->NewVideoFrame(vi);

    if (vi.IsYUY2()) {  // YUY2 interleaved
      const BYTE* srcp = src->GetReadPtr();  
      BYTE* dstp = dst->GetWritePtr();
      for (int y=0; y<vi.height; y++) {
        int endx = dst->GetRowSize();
        for (int x = 0; x < endx; x+=4) {
          dstp[x] = (mode==2) ? srcp[x*2+1] : srcp[x*2+3];
          dstp[x+1] = 0x80;
          dstp[x+2] = (mode==2) ? srcp[x*2+5] : srcp[x*2+7];
          dstp[x+3] = 0x80;
        }
        srcp += src->GetPitch();
        dstp += dst->GetPitch();
      }
      return dst;
    }

    // Planar 

    if (mode==2) {  // U To Y
      env->BitBlt(dst->GetWritePtr(PLANAR_Y),dst->GetPitch(PLANAR_Y),src->GetReadPtr(PLANAR_U),src->GetPitch(PLANAR_U),dst->GetRowSize(),dst->GetHeight());
    } else if (mode==3) {
      env->BitBlt(dst->GetWritePtr(PLANAR_Y),dst->GetPitch(PLANAR_Y),src->GetReadPtr(PLANAR_V),src->GetPitch(PLANAR_V),dst->GetRowSize(),dst->GetHeight());
    }

    // Clear chroma

    int pitch = dst->GetPitch(PLANAR_U)/4;
    int *srcpUV = (int*)dst->GetWritePtr(PLANAR_U);
    int myx = dst->GetRowSize(PLANAR_U_ALIGNED)/4;
    int myy = dst->GetHeight(PLANAR_U);
	  for (int y=0; y<myy; y++) {
      for (int x=0; x<myx; x++) {
		    srcpUV[x] = 0x80808080;  // mod 8
      }
		  srcpUV += pitch;
	  }

    srcpUV = (int*)dst->GetWritePtr(PLANAR_V);
	  for (y=0; y<myy; ++y) {
      for (int x=0; x<myx; x++) {
		    srcpUV[x] = 0x80808080;  // mod 8
      }
		  srcpUV += pitch;
	  }
    return dst;
  }

  if (mode==4) {  // Merge UV  U=child V=clip, Y = Yclip (optional)

    PVideoFrame dst = env->NewVideoFrame(vi);

    if (vi.IsYUY2()) {
      const BYTE* srcpU = src->GetReadPtr();

      PVideoFrame srcV = clip->GetFrame(n, env);
      const BYTE* srcpV = srcV->GetReadPtr();

      PVideoFrame srcY;
      if (clipY) srcY = clipY->GetFrame(n, env);
      const BYTE* srcpY = (clipY) ? srcY->GetReadPtr() : 0;

      BYTE* dstp = dst->GetWritePtr();

      for (int y=0; y<vi.height; y++) {
        int endx = dst->GetRowSize();
        for (int x = 0; x < endx; x+=4) {
          if (clipY) {
            dstp[x] = srcpY[x];
            dstp[x+2] = srcpY[x+2];
          } else {
            dstp[x] = 80;
            dstp[x+2] = 80;
          }
          dstp[x+1] = srcpU[x>>1];
          dstp[x+3] = srcpV[x>>1];
        }
        srcpU += src->GetPitch();
        srcpV += srcV->GetPitch();
        dstp += dst->GetPitch();
        if (clipY) srcpY += srcY->GetPitch();
      }
      return dst;
    }

    env->BitBlt(dst->GetWritePtr(PLANAR_U),dst->GetPitch(PLANAR_U),src->GetReadPtr(PLANAR_Y),src->GetPitch(PLANAR_Y),src->GetRowSize(PLANAR_Y),src->GetHeight(PLANAR_Y));
    src = clip->GetFrame(n, env);
    env->BitBlt(dst->GetWritePtr(PLANAR_V),dst->GetPitch(PLANAR_V),src->GetReadPtr(PLANAR_Y),src->GetPitch(PLANAR_Y),src->GetRowSize(PLANAR_Y),src->GetHeight(PLANAR_Y));

    if (!clipY) {
      // Luma = 128
      int pitch = dst->GetPitch(PLANAR_Y)/4;
      int *srcpY = (int*)dst->GetWritePtr(PLANAR_Y);
      int myx = dst->GetRowSize(PLANAR_Y_ALIGNED)/4;
      int myy = dst->GetHeight(PLANAR_Y);
	    for (int y=0; y<myy; y++) {
        for (int x=0; x<myx; x++) {
		      srcpY[x] = 0x80808080;  // mod 4
        }
		    srcpY += pitch;
	    }
    } else {
      src = clipY->GetFrame(n, env);
      env->BitBlt(dst->GetWritePtr(PLANAR_Y),dst->GetPitch(PLANAR_Y),src->GetReadPtr(PLANAR_Y),src->GetPitch(PLANAR_Y),src->GetRowSize(PLANAR_Y),src->GetHeight(PLANAR_Y));
    }
    return dst;
  }
  return src;
}


/****************************
******   Merge Chroma   *****
****************************/

MergeChroma::MergeChroma(PClip _child, PClip _clip, float _weight, IScriptEnvironment* env) 
  : GenericVideoFilter(_child), clip(_clip), weight(_weight)
{
  const VideoInfo& vi2 = clip->GetVideoInfo();

  if (!vi.IsYUV() || !vi2.IsYUV())
    env->ThrowError("MergeLuma: YUV data only (no RGB); use ConvertToYUY2 or ConvertToYV12");

  if (!((vi.IsYV12()==vi2.IsYV12()) || (vi.IsYUY2()==vi2.IsYUY2())))
    env->ThrowError("MergeLuma: YUV data is not same type. Both must be YV12 or YUY2");

  if (vi.width!=vi2.width || vi.height!=vi2.height)
    env->ThrowError("MergeLuma: Images must have same width and height!");

  if (weight<0.0f) weight=0.0f;
  if (weight>1.0f) weight=1.0f;

  if (weight>=0.9999f && !(env->GetCPUFlags() & CPUF_MMX))
      env->ThrowError("MergeChroma: MMX required (K6, K7, P5 MMX, P-II, P-III or P4)");
}



PVideoFrame __stdcall MergeChroma::GetFrame(int n, IScriptEnvironment* env)
{
  
  PVideoFrame src = child->GetFrame(n, env);
  
  if (weight<0.0001f) return src;
  
  PVideoFrame chroma = clip->GetFrame(n, env);
  
  const int h = src->GetHeight();
  const int w = src->GetRowSize()>>1; // width in pixels
  
  if (weight<0.9999f) {
    if (vi.IsYUY2()) {
      env->MakeWritable(&src);
      unsigned int* srcp = (unsigned int*)src->GetWritePtr();
      unsigned int* chromap = (unsigned int*)chroma->GetReadPtr();
      
      const int isrc_pitch = (src->GetPitch())>>2;  // int pitch (one pitch=two pixels)
      const int ichroma_pitch = (chroma->GetPitch())>>2;  // Ints
      
      (env->GetCPUFlags() & CPUF_INTEGER_SSE ? isse_weigh_chroma : weigh_chroma)
        (srcp,chromap,isrc_pitch,ichroma_pitch,w,h,(int)(weight*32768.0f),32768-(int)(weight*32768.0f));
    } else {
      env->MakeWritable(&src);
      int* srcp = (int*)src->GetReadPtr(PLANAR_Y);

      int iweight=(int)(weight*65536.0f);
      int invweight = 65536-(int)(weight*65536.0f);

      int* srcpU = (int*)src->GetReadPtr(PLANAR_U);
      int* chromapU = (int*)chroma->GetWritePtr(PLANAR_U);
      int* srcpV = (int*)src->GetReadPtr(PLANAR_V);
      int* chromapV = (int*)chroma->GetWritePtr(PLANAR_V);
      if (env->GetCPUFlags() & CPUF_MMX) {
        mmx_weigh_yv12((BYTE*)srcpU,(BYTE*)chromapU,src->GetPitch(PLANAR_U),chroma->GetPitch(PLANAR_U),src->GetRowSize(PLANAR_U_ALIGNED),src->GetHeight(PLANAR_U),(int)(weight*32767.0f),32767-(int)(weight*32767.0f));
        mmx_weigh_yv12((BYTE*)srcpV,(BYTE*)chromapV,src->GetPitch(PLANAR_U),chroma->GetPitch(PLANAR_U),src->GetRowSize(PLANAR_V_ALIGNED),src->GetHeight(PLANAR_U),(int)(weight*32767.0f),32767-(int)(weight*32767.0f));
        return src;
      }
      const int isrc_pitch = (src->GetPitch(PLANAR_U))>>2;  // int pitch (one pitch=two pixels)
      const int ichroma_pitch = (chroma->GetPitch(PLANAR_U))>>2;  // Ints
      const int xpixels=src->GetRowSize(PLANAR_U_ALIGNED)>>2; // Ints
      const int yloops=src->GetHeight(PLANAR_U);
      
      for (int y=0;y<yloops;y++) {
        for (int x=0;x<xpixels;x++) {
          int srcU = srcpU[x];
          int chromU = chromapU[x];
          int srcV = srcpV[x];
          int chromV = chromapV[x];
          srcpU[x] = (((srcU&0xff)*invweight + ((chromU&0xff)*iweight))>>16) |
            ( ((((srcU&0xff00)>>8)*invweight + (((chromU&0xff00)>>8)*iweight))>>8)&0xff00) |
            ( ((((srcU&0xff0000)>>16)*invweight + (((chromU&0xff0000)>>16)*iweight)))&0xff0000) |
            ( ((((srcU&0xff000000)>>24)*invweight + (((chromU&0xff000000)>>24)*iweight))<<8)&0xff000000) ;
          
          srcpV[x] = (((srcV&0xff)*invweight + ((chromV&0xff)*iweight))>>16) |
            ( ((((srcV&0xff00)>>8)*invweight + (((chromV&0xff00)>>8)*iweight))>>8)&0xff00) |
            ( ((((srcV&0xff0000)>>16)*invweight + (((chromV&0xff0000)>>16)*iweight)))&0xff0000) |
            ( ((((srcV&0xff000000)>>24)*invweight + (((chromV&0xff000000)>>24)*iweight))<<8)&0xff000000) ;
        }
        chromapU+=ichroma_pitch;
        chromapV+=ichroma_pitch;
        srcpU+=isrc_pitch;
        srcpV+=isrc_pitch;
      }      
    }
  } else {
    if (vi.IsYUY2()) {
      env->MakeWritable(&src);
      unsigned int* srcp = (unsigned int*)src->GetWritePtr();
      unsigned int* chromap = (unsigned int*)chroma->GetReadPtr();
      
      const int isrc_pitch = (src->GetPitch())>>2;  // int pitch (one pitch=two pixels)
      const int ichroma_pitch = (chroma->GetPitch())>>2;  // Ints
      
      mmx_merge_luma(chromap,srcp,ichroma_pitch,isrc_pitch,w,h);  // Just swap luma/chroma
      return chroma;
    } else {
      env->MakeWritable(&src);
      unsigned int* srcp = (unsigned int*)src->GetWritePtr(); //Must be requested
      env->BitBlt(src->GetWritePtr(PLANAR_U),src->GetPitch(PLANAR_U),chroma->GetReadPtr(PLANAR_U),chroma->GetPitch(PLANAR_U),chroma->GetRowSize(PLANAR_U),chroma->GetHeight(PLANAR_U));
      env->BitBlt(src->GetWritePtr(PLANAR_V),src->GetPitch(PLANAR_V),chroma->GetReadPtr(PLANAR_V),chroma->GetPitch(PLANAR_V),chroma->GetRowSize(PLANAR_V),chroma->GetHeight(PLANAR_U));
    }
  }
  return src;
}


AVSValue __cdecl MergeChroma::Create(AVSValue args, void* user_data, IScriptEnvironment* env)
{
  return new MergeChroma(args[0].AsClip(), args[1].AsClip(), args[2].AsFloat(1.0f), env);
}







/**************************
******   Merge Luma   *****
**************************/


MergeLuma::MergeLuma(PClip _child, PClip _clip, float _weight, IScriptEnvironment* env) 
  :	GenericVideoFilter(_child), clip(_clip), weight(_weight)
{
  const VideoInfo& vi2 = clip->GetVideoInfo();

  if (!vi.IsYUV() || !vi2.IsYUV())
    env->ThrowError("MergeLuma: YUV data only (no RGB); use ConvertToYUY2 or ConvertToYV12");

  if ((vi.IsYV12()!=vi2.IsYV12()) || (vi.IsYUY2()!=vi2.IsYUY2()))
    env->ThrowError("MergeLuma: YUV data is not same type. Both must be YV12 or YUY2");

  if (vi.width!=vi2.width || vi.height!=vi2.height)
    env->ThrowError("MergeLuma: Images must have same width and height!");

  if (weight<0.0f) weight=0.0f;
  if (weight>1.0f) weight=1.0f;
  if (vi.IsYUY2()) {
    if (weight>=0.9999f && !(env->GetCPUFlags() & CPUF_MMX))
        env->ThrowError("MergeLuma: MMX required (K6, K7, P5 MMX, P-II, P-III or P4)");
  }

}
    

PVideoFrame __stdcall MergeLuma::GetFrame(int n, IScriptEnvironment* env)
{
  
  PVideoFrame src = child->GetFrame(n, env);

  if (weight<0.0001f) return src;

  PVideoFrame luma = clip->GetFrame(n, env);
  
  if (vi.IsYUY2()) {
    env->MakeWritable(&src);
    unsigned int* srcp = (unsigned int*)src->GetWritePtr();
    unsigned int* lumap = (unsigned int*)luma->GetReadPtr();

    const int isrc_pitch = (src->GetPitch())>>2;  // int pitch (one pitch=two pixels)
    const int iluma_pitch = (luma->GetPitch())>>2;  // Ints
  
    const int h = src->GetHeight();
    const int w = src->GetRowSize()>>1; // width in pixels
    
    if (weight<0.9999f)
      (env->GetCPUFlags() & CPUF_INTEGER_SSE ? isse_weigh_luma : weigh_luma)
							  (srcp,lumap,isrc_pitch,iluma_pitch,w,h,(int)(weight*32768.0f),32768-(int)(weight*32768.0f));
    else
		  mmx_merge_luma(srcp,lumap,isrc_pitch,iluma_pitch,w,h);    
    return src;
  }
  if (weight>0.9999f) {
    env->MakeWritable(&luma);
    env->BitBlt(luma->GetWritePtr(PLANAR_U),luma->GetPitch(PLANAR_U),src->GetReadPtr(PLANAR_U),src->GetPitch(PLANAR_U),src->GetRowSize(PLANAR_U),src->GetHeight(PLANAR_U));
    env->BitBlt(luma->GetWritePtr(PLANAR_V),luma->GetPitch(PLANAR_V),src->GetReadPtr(PLANAR_V),src->GetPitch(PLANAR_V),src->GetRowSize(PLANAR_V),src->GetHeight(PLANAR_V));
    return luma;
  } else {
    env->MakeWritable(&src);
    int iweight=(int)(weight*65535.0f);
    int invweight = 65535-(int)(weight*65535.0f);
    int* srcpY = (int*)src->GetWritePtr(PLANAR_Y);
    int* lumapY = (int*)luma->GetReadPtr(PLANAR_Y);
    if (env->GetCPUFlags() & CPUF_MMX) {
      mmx_weigh_yv12((BYTE*)srcpY,(BYTE*)lumapY,src->GetPitch(PLANAR_Y),luma->GetPitch(PLANAR_Y),src->GetRowSize(PLANAR_Y_ALIGNED),src->GetHeight(PLANAR_Y),(int)(weight*32767.0f),32767-(int)(weight*32767.0f));
      return src;
    }

    const int isrc_pitch = (src->GetPitch())>>2;  // int pitch (one pitch=two pixels)
    const int iluma_pitch = (luma->GetPitch())>>2;  // Ints
    const int xpixels=src->GetRowSize(PLANAR_Y_ALIGNED)>>2; // Ints
    const int yloops=src->GetHeight(PLANAR_Y);
    for (int y=0;y<yloops;y++) {
      for (int x=0;x<xpixels;x++) {
        int srcY = srcpY[x];
        int lumY = lumapY[x];
        srcpY[x] = (((srcY&0xff)*invweight + ((lumY&0xff)*iweight))>>16) | 
          ( ((((srcY&0xff00)>>8)*invweight + (((lumY&0xff00)>>8)*iweight))>>8)&0xff00) | 
          ( ((((srcY&0xff0000)>>16)*invweight + (((lumY&0xff0000)>>16)*iweight)))&0xff0000) |
          ( ((((srcY&0xff000000)>>24)*invweight + (((lumY&0xff000000)>>24)*iweight))<<8)&0xff000000) ;
      }
      lumapY+=iluma_pitch;
      srcpY+=isrc_pitch;
    }
  }
  
  return src;
}


AVSValue __cdecl MergeLuma::Create(AVSValue args, void* user_data, IScriptEnvironment* env)
{
  return new MergeLuma(args[0].AsClip(), args[1].AsClip(), args[2].AsFloat(1.0f), env);
}





/****************************
******    C routines    *****
****************************/


void weigh_luma(unsigned int *src,unsigned int *luma, int pitch, int luma_pitch,int width, int height, int weight, int invweight) {

	int lwidth=width>>1;

	for (int y=0;y<height;y++) {
	  unsigned int *tlu=&luma[y*luma_pitch];  // offset
		unsigned int *src2=src;
		for (int x=0;x<lwidth;x++) {
			unsigned int lumapix=*tlu++;
			unsigned int destpix=*src2;
			unsigned int luma1=lumapix&0xff;
			unsigned int luma2=(lumapix&0xff0000)>>16;

			unsigned int orgluma1=destpix&0xff;
			unsigned int orgluma2=(destpix&0xff0000)>>16;
			
			*src2++ = ( destpix & 0xff00ff00 ) |
				( ( luma1 * weight + orgluma1 * invweight + 16384 ) >> 15 ) |
				( ( ( luma2 * weight + orgluma2 * invweight + 16384 ) << 1 ) &0xff0000 );  // Insert new luma values
		}
		src+=pitch;
	} // end for y
}




void weigh_chroma(unsigned int *src,unsigned int *chroma, int pitch, int chroma_pitch,int width, int height, int weight, int invweight) {

	int lwidth=width>>1;

	for (int y=0;y<height;y++) {
	  unsigned int *tch=&chroma[y*chroma_pitch];  // offset
		unsigned int *src2=src;
		for (int x=0;x<lwidth;x++) {
			unsigned int chromapix=*tch++;
			unsigned int destpix=*src2;
			unsigned int chroma1=(chromapix&0xff00)>>8;
			unsigned int chroma2=(chromapix&0xff000000)>>24;

			unsigned int orgchroma1=(destpix&0xff00)>>8;
			unsigned int orgchroma2=(destpix&0xff000000)>>24;
			
			*src2++ = ( destpix & 0x00ff00ff ) |
				( ( ( chroma1 * weight + orgchroma1 * invweight + 16384 ) >> 7 ) & 0xff00 ) |
				( ( ( chroma2 * weight + orgchroma2 * invweight + 16384 ) << 9 ) & 0xff000000 );  // Insert new chroma values
		}
		src+=pitch;
	} // end for y
}





/****************************
******   MMX routines   *****
****************************/

void mmx_merge_luma( unsigned int *src, unsigned int *luma, int pitch, 
                     int luma_pitch,int width, int height ) 
{
	__declspec(align(8)) static __int64 I1=0x00ff00ff00ff00ff;  // Luma mask
	__declspec(align(8)) static __int64 I2=0xff00ff00ff00ff00;  // Chroma mask
  // [V][Y2][U][Y1]

  int row_size = width * 2;
	int lwidth_bytes = row_size & -16;	// bytes handled by the MMX loop
  __asm {
		movq mm7,[I1]     ; Luma
		movq mm6,[I2]     ; Chroma
  }
	for (int y=0;y<height;y++) {

		// eax=src
		// ebx=luma
		// ecx=src/luma offset
		
	__asm {
		mov eax,src
		xor ecx,ecx
		mov ebx,luma
		jmp afterloop
		align 16
goloop:
		add ecx,16   // 16 bytes per pass = 8 pixels = 2 quadwords
afterloop:
		cmp       ecx,[lwidth_bytes]	; Is eax(i) greater than endx
		jge       outloop		; Jump out of loop if true


		; Processes 8 pixels at the time 
		movq mm0,[eax+ecx]		; chroma 4 pixels
    movq mm1,[eax+ecx+8]  ; chroma next 4 pixels
    pand mm0,mm6
    movq mm2,[ebx+ecx]  ; load luma 4 pixels
    pand mm1,mm6
    movq mm3,[ebx+ecx+8]  ; load luma next 4 pixels
    pand mm2,mm7
    pand mm3,mm7
    por mm0,mm2
    por mm1,mm3
    movq [eax+ecx],mm0
    movq [eax+ecx+8],mm1
		jmp goloop
outloop:
		; processes remaining pixels
		cmp ecx,[row_size]
		jge exitloop
		mov esi,[eax+ecx]
		mov edi,[ebx+ecx]
		and esi,0xff00ff00
		and	edi,0x00ff00ff
		or esi,edi
		mov [eax+ecx],esi
		add ecx,4
		jmp outloop
exitloop:
		} 
	
		src += pitch;
		luma += luma_pitch;
	} // end for y
  __asm {emms};
}




void isse_weigh_luma(unsigned int *src,unsigned int *luma, int pitch,
										int luma_pitch,int width, int height, int weight, int invweight)
{
	__declspec(align(8)) static __int64 I1=0x00ff00ff00ff00ff;  // Luma mask
	__declspec(align(8)) static __int64 I2=0xff00ff00ff00ff00;  // Chroma mask
  // [V][Y2][U][Y1]

  int row_size = width * 2;
	int lwidth_bytes = row_size & -8;	// bytes handled by the main loop
  __int64 weight64  = (__int64)invweight | (((__int64)weight)<<16)| (((__int64)invweight)<<32) |(((__int64)weight)<<48);
  // weight LLLL llll LLLL llll
	__int64 rounder = 0x0000400000004000;		// (0.5)<<15 in each dword

  __asm {
		movq mm7,[I1]     ; Luma
		movq mm6,[I2]     ; Chroma
    movq mm5,[weight64] ; Weight
		movq mm4,[rounder]

  }
	for (int y=0;y<height;y++) {

		// eax=src
		// ebx=luma
		// ecx=src/luma offset
		
	__asm {
		mov eax,src
		xor ecx,ecx
		mov ebx,luma
		jmp afterloop
		align 16
goloop:
		add ecx,8   // 8 bytes per pass = 4 pixels = 1 quadword
afterloop:
		cmp       ecx,[lwidth_bytes]	; Is eax(i) greater than endx
		jge       outloop		; Jump out of loop if true


		; Processes 4 pixels at the time
		movq mm0,[eax+ecx]		; original 4 pixels   (cc)
    movq mm2,[ebx+ecx]    ; load 4 pixels
    movq mm3,mm0          ; move original pixel into mm3
    punpckhwd mm3,mm2     ; Interleave upper pixels in mm3 | mm3= CCLL ccll CCLL ccll
		movq mm1,mm0
		pand mm3,mm7					; mm3= 00LL 00ll 00LL 00ll
    pmaddwd mm3,mm5				; Mult with weights and add. Latency 2 cycles - mult unit cannot be used
		punpcklwd mm1,mm2     ; Interleave lower pixels in mm1 | mm1= CCLL ccll CCLL ccll
		pand mm0,mm6					; mm0= cc00 cc00 cc00 cc00
		pand mm1,mm7
		paddd mm3,mm4					; round to nearest
		psrld mm3,15					; Divide with total weight (=15bits) mm3 = 0000 00LL 0000 00LL
		pmaddwd mm1,mm5
		paddd mm1,mm4					; round to nearest
		psrld mm1,15					; Divide with total weight (=15bits) mm1 = 0000 00LL 0000 00LL
		packssdw mm1, mm3			; mm1 = 00LL 00LL 00LL 00LL
		por mm0,mm1
    movq [eax+ecx],mm0
  
		jmp goloop
outloop:				
		// processes remaining pixels here
		cmp ecx,[row_size]
		jge	exitloop
		movd mm0,[eax+ecx]			; original 2 pixels
		movd mm2,[ebx+ecx]			; luma 2 pixels
		movq mm1,mm0
		punpcklwd mm1,mm2				; mm1= CCLL ccll CCLL ccll
		pand mm1,mm7						; mm1= 00LL 00ll 00LL 00ll
		pmaddwd mm1,mm5
		pand mm0,mm6						; mm0= 0000 0000 cc00 cc00
		paddd mm1,mm4						; round to nearest
		psrld mm1,15						; mm1= 0000 00LL 0000 00LL
		packssdw mm1,mm1				; mm1= 00LL 00LL 00LL 00LL
		por mm0,mm1							; mm0 finished
		movd [eax+ecx],mm0
		// no loop since there is at most 2 remaining pixels
exitloop:
		} 
		src += pitch;
		luma += luma_pitch;
	} // end for y
  __asm {emms};
}




void isse_weigh_chroma( unsigned int *src,unsigned int *chroma, int pitch, 
                     int chroma_pitch,int width, int height, int weight, int invweight ) 
{

  __declspec(align(8)) static __int64 I1=0x00ff00ff00ff00ff;  // Luma mask
  __declspec(align(8)) static __int64 weight64  = (__int64)invweight | (((__int64)weight)<<16) | (((__int64)invweight)<<32) |(((__int64)weight)<<48);
	__declspec(align(8)) static __int64 rounder = 0x0000400000004000;		// (0.5)<<15 in each dword
  // [V][Y2][U][Y1]

  int row_size = width * 2;
	int lwidth_bytes = row_size & -8;	// bytes handled by the main loop

  __asm {
		movq mm7,[I1]     ; Luma
    movq mm5,[weight64] ; Weight
		movq mm4,[rounder]

  }
	for (int y=0;y<height;y++) {

		// eax=src
		// ebx=luma
		// ecx=src/luma offset
		
	__asm {
		mov eax,src
		xor ecx,ecx
		mov ebx,chroma
		jmp afterloop
		align 16
goloop:
		add ecx,8   // 8 bytes per pass = 4 pixels = 1 quadword
afterloop:
		cmp       ecx,[lwidth_bytes]	; Is eax(i) greater than endx
		jge       outloop		; Jump out of loop if true


		; Processes 4 pixels at the time
		movq mm0,[eax+ecx]		; original 4 pixels   (cc)
    movq mm2,[ebx+ecx]    ; load 4 pixels
    movq mm3,mm0          ; move original pixel into mm3
    punpckhwd mm3,mm2     ; Interleave upper pixels in mm3 | mm3= CCLL ccll CCLL ccll
		movq mm1,mm0
		psrlw mm3,8						; mm3= 00CC 00cc 00CC 00cc
    pmaddwd mm3,mm5				; Mult with weights and add. Latency 2 cycles - mult unit cannot be used
		punpcklwd mm1,mm2     ; Interleave lower pixels in mm1 | mm1= CCLL ccll CCLL ccll
		pand mm0,mm7					; mm0= 00ll 00ll 00ll 00ll
		psrlw mm1,8
		paddd mm3,mm4					; round to nearest
		psrld mm3,15					; Divide with total weight (=15bits) mm3 = 0000 00CC 0000 00CC
		pmaddwd mm1,mm5
		paddd mm1,mm4					; round to nearest
		psrld mm1,15					; Divide with total weight (=15bits) mm1 = 0000 00CC 0000 00CC
		packssdw mm1, mm3			; mm1 = 00CC 00CC 00CC 00CC
		pslld mm1,8
		por mm0,mm1
    movq [eax+ecx],mm0

		jmp goloop
outloop:				
		// processes remaining pixels here
		cmp ecx,[row_size]
		jge	exitloop
		movd mm0,[eax+ecx]			; original 2 pixels
		movd mm2,[ebx+ecx]			; luma 2 pixels
		movq mm1,mm0
		punpcklwd mm1,mm2				; mm1= CCLL ccll CCLL ccll
		psrlw mm1,8							; mm1= 00CC 00cc 00CC 00cc
		pmaddwd mm1,mm5
		pand mm0,mm7						; mm0= 0000 0000 00ll 00ll
		paddd mm1,mm4						; round to nearest
		psrld mm1,15						; mm1= 0000 00CC 0000 00CC
		packssdw mm1,mm1
		pslld mm1,8							; mm1= CC00 CC00 CC00 CC00
		por mm0,mm1							; mm0 finished
		movd [eax+ecx],mm0
		// no loop since there is at most 2 remaining pixels
exitloop:
		}
		src += pitch;
		chroma += chroma_pitch;
	} // end for y
  __asm {emms};
}

/*******************
 * Blends two planes.
 * A weight between the two planes are given.
 * Has rather ok pairing, 
 * and has very little memory usage.
 * Processes four pixels per loop, so rowsize must be mod 4.
 * Thanks to ARDA for squeezing out a bit more performance.
 * (c) 2002 by sh0dan.
 ********/

void mmx_weigh_yv12(BYTE *p1,BYTE *p2, int p1_pitch, int p2_pitch,int rowsize, int height, int weight, int invweight) {
  __int64 weight64  = (__int64)weight | (((__int64)invweight)<<16) | (((__int64)weight)<<32) |(((__int64)invweight)<<48);
	__int64 rounder = 0x0000400000004000;		// (0.5)<<15 in each dword

  __asm {
      movq mm5,[rounder]
      pxor mm6,mm6
      movq mm7,[weight64]
      mov ebx,[rowsize]
      mov esi,[p1]
      mov edi,[p2]
      xor ecx, ecx  // Height
      mov edx,[height]
      align 16
yloopback:
      cmp ecx, edx
      jge outy
      xor eax, eax
      align 16 
testloop:
      cmp ebx, eax
      jle outloop
      punpcklbw mm0,[esi+eax]  // 4 pixels
       pxor mm3,mm3
      punpcklbw mm1,[edi+eax]  // y300 y200 y100 y000
       psrlw mm0,8              // 00y3 00y2 00y1 00y0
      psrlw mm1,8              // 00y3 00y2 00y1 00y0  
       pxor mm2,mm2
      movq mm4,mm1
       punpcklwd mm2,mm0
      punpckhwd mm3,mm0  
       punpcklwd mm4,mm6
      punpckhwd mm1,mm6
       por mm2,mm4
      por mm3,mm1
       pmaddwd mm2,mm7     // Multiply pixels by weights.  pixel = img1*weight + img2*invweight (twice)
      pmaddwd mm3,mm7      // Stalls 1 cycle (multiply unit stall)
       paddd mm2,mm5       // Add rounder
      paddd mm3,mm5
       psrld mm2,15        // Shift down, so there is no fraction.
      psrld mm3,15        
      packssdw mm2,mm3
      packuswb mm2,mm6 
      movd [esi+eax],mm2
      add eax,4
      jmp testloop
      align 16
outloop:
      inc ecx
      add esi, [p1_pitch];
      add edi, [p2_pitch];
      jmp yloopback
outy:
      emms
  } // end asm
}
