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

#include "histogram.h"





/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

AVSFunction Histogram_filters[] = {
  { "Histogram", "c[mode]s", Histogram::Create },   // src clip
  { 0 }
};




/***********************************
 *******   Histogram Filter   ******
 **********************************/

Histogram::Histogram(PClip _child, int _mode, IScriptEnvironment* env) 
  : GenericVideoFilter(_child), mode(_mode)
{

  if (mode ==0) {
    if (!vi.IsYUV())
      env->ThrowError("Histogram: YUV data only");
    vi.width += 256;
  }
  if (mode ==1) {
    if (!vi.IsYUV())
      env->ThrowError("Histogram: YUV data only");
    if (!vi.IsPlanar())
      env->ThrowError("Histogram: Levels mode only available in YV12.");
    vi.width += 256;
    vi.height = max(256,vi.height);
  }

  if (mode ==2) {
    if (!vi.IsYUV())
      env->ThrowError("Histogram: YUV data only");
    if (!vi.IsPlanar())
      env->ThrowError("Histogram: Color mode only available in YV12.");
    vi.width += 256;
    vi.height = max(256,vi.height);
  }

  if (mode ==3) {
    if (!vi.IsYUV())
      env->ThrowError("Histogram: Luma mode only available in YUV.");
  }

  if ((mode == 4)||(mode==5)) {

    child->SetCacheHints(CACHE_AUDIO,4096*1024);    

    if (!vi.HasVideo()) {
      mode = 4; // force mode to 4.
      vi.fps_numerator = 25;
      vi.fps_denominator = 1;
      vi.num_frames = vi.FramesFromAudioSamples(vi.num_audio_samples);
    }
    if (mode == 5)  {
      vi.height = max(512, vi.height);
      vi.width = max(512, vi.width);
      if(!vi.IsYV12())
        env->ThrowError("Histogram: StereoOverlay must be YV12");
    } else {
      vi.pixel_type = VideoInfo::CS_YV12;
      vi.height = 512;
      vi.width = 512;
    }
    if (!vi.HasAudio())
      env->ThrowError("Histogram: Stereo mode requires samples!");
    if (vi.AudioChannels() != 2)
      env->ThrowError("Histogram: Stereo mode only works on two audio channels.");

     aud_clip = ConvertAudio::Create(child,SAMPLE_INT16,SAMPLE_INT16);
  }
}

PVideoFrame __stdcall Histogram::GetFrame(int n, IScriptEnvironment* env) 
{
  switch (mode) {
  case 0:
    return DrawMode0(n, env);
  case 1:
    return DrawMode1(n, env);
  case 2:
    return DrawMode2(n, env);
  case 3:
    return DrawMode3(n, env);
  case 4:
    return DrawMode4(n, env);
  case 5:
    return DrawMode5(n, env);
  }
  return DrawMode0(n, env);
}


PVideoFrame Histogram::DrawMode5(int n, IScriptEnvironment* env) {
  PVideoFrame dst = env->NewVideoFrame(vi);

  __int64 start = vi.AudioSamplesFromFrames(n); 
  __int64 end = vi.AudioSamplesFromFrames(n+1); 
  __int64 count = end-start;
  signed short* samples = new signed short[(int)count*vi.AudioChannels()];

  int w = dst->GetRowSize();
  int h = dst->GetHeight();
  int imgSize = h*dst->GetPitch();
  BYTE* dstp = dst->GetWritePtr();
  int p = dst->GetPitch(PLANAR_Y);

  PVideoFrame src = child->GetFrame(n, env);
  if ((src->GetHeight()<dst->GetHeight()) || (src->GetRowSize() < dst->GetRowSize())) {
    memset(dstp, 16, imgSize);
    int imgSizeU = dst->GetHeight(PLANAR_U) * dst->GetPitch(PLANAR_U);
    memset(dst->GetWritePtr(PLANAR_U) , 127, imgSizeU);
    memset(dst->GetWritePtr(PLANAR_V), 127, imgSizeU);
  }

  env->BitBlt(dstp, dst->GetPitch(), src->GetReadPtr(), src->GetPitch(), src->GetRowSize(), src->GetHeight());
  env->BitBlt(dst->GetWritePtr(PLANAR_U), dst->GetPitch(PLANAR_U), src->GetReadPtr(PLANAR_U), src->GetPitch(PLANAR_U), src->GetRowSize(PLANAR_U), src->GetHeight(PLANAR_U));
  env->BitBlt(dst->GetWritePtr(PLANAR_V), dst->GetPitch(PLANAR_V), src->GetReadPtr(PLANAR_V), src->GetPitch(PLANAR_V), src->GetRowSize(PLANAR_V), src->GetHeight(PLANAR_V));

  BYTE* _dstp = dstp;
  for (int iY = 0; iY<512; iY++) {
    for (int iX = 0; iX<512; iX++) {
      _dstp[iX] >>= 1;
    }
    _dstp+=p;
  }

  aud_clip->GetAudio(samples, max(0,start), count, env);
  
  int c = (int)count;
  for (int i=1; i < count;i++) {
    int l1 = (int)samples[i*2-2];
    int r1 = (int)samples[i*2-1];
    int l2 = (int)samples[i*2];
    int r2 = (int)samples[i*2+1];
    for (int s = 0 ; s < 8; s++) {  // 8 times supersampling (linear)
      int l = (l1*s) + (l2*(8-s));
      int r = (r1*s) + (r2*(8-s));
      int y = 256+((l+r)>>11);
      int x = 256+((l-r)>>11);
      int v = dstp[x+y*p]+48;
      dstp[x+y*p] = min(v,235);
    }
  }

  delete[] samples;
  return dst;
}


PVideoFrame Histogram::DrawMode4(int n, IScriptEnvironment* env) {
  PVideoFrame src = env->NewVideoFrame(vi);
  env->MakeWritable(&src);
  __int64 start = vi.AudioSamplesFromFrames(n); 
  __int64 end = vi.AudioSamplesFromFrames(n+1); 
  __int64 count = end-start;
  signed short* samples = new signed short[(int)count*vi.AudioChannels()];

  int w = src->GetRowSize();
  int h = src->GetHeight();
  int imgSize = h*src->GetPitch();
  BYTE* srcp = src->GetWritePtr();
  memset(srcp, 16, imgSize);

  aud_clip->GetAudio(samples, max(0,start), count, env);
  
  int c = (int)count;
  for (int i=1; i < count;i++) {
    int l1 = (int)samples[i*2-2];
    int r1 = (int)samples[i*2-1];
    int l2 = (int)samples[i*2];
    int r2 = (int)samples[i*2+1];
    for (int s = 0 ; s < 8; s++) {  // 8 times supersampling (linear)
      int l = (l1*s) + (l2*(8-s));
      int r = (r1*s) + (r2*(8-s));
      int y = 256+((l+r)>>11);
      int x = 256+((l-r)>>11);
      int v = srcp[x+y*512]+48;
      srcp[x+y*512] = min(v,235);
    }
  }

  if (vi.IsPlanar()) {
    srcp = src->GetWritePtr(PLANAR_U);
    imgSize = src->GetHeight(PLANAR_U) * src->GetPitch(PLANAR_U);
    memset(srcp, 127, imgSize);
    srcp = src->GetWritePtr(PLANAR_V);
    memset(srcp, 127, imgSize);
  }
  delete[] samples;
  return src;
}

PVideoFrame Histogram::DrawMode3(int n, IScriptEnvironment* env) {
  PVideoFrame src = child->GetFrame(n, env);
  env->MakeWritable(&src);
  int w = src->GetRowSize();
  int h = src->GetHeight();
  int imgsize = h*src->GetPitch();
  BYTE* srcp = src->GetWritePtr();
  if (vi.IsYUY2()) {
    for (int i=0; i<imgsize; i+=2) {
      int p = srcp[i];
      p<<=4;
      srcp[i+1] = 127;
      srcp[i] = (p&256) ? (255-(p&0xff)) : p&0xff;
    }
  } else {
    for (int i=0; i<imgsize; i++) {
      int p = srcp[i];
      p<<=4;
      srcp[i] = (p&256) ? (255-(p&0xff)) : p&0xff;
    }
  }
  if (vi.IsPlanar()) {
    srcp = src->GetWritePtr(PLANAR_U);
    imgsize = src->GetHeight(PLANAR_U) * src->GetPitch(PLANAR_U);
    memset(srcp, 127, imgsize);
    srcp = src->GetWritePtr(PLANAR_V);
    memset(srcp, 127, imgsize);
  }
  return src;
}


PVideoFrame Histogram::DrawMode2(int n, IScriptEnvironment* env) {
  PVideoFrame dst = env->NewVideoFrame(vi);
  BYTE* p = dst->GetWritePtr();
  PVideoFrame src = child->GetFrame(n, env);

  int imgSize = dst->GetHeight()*dst->GetPitch();

  if (src->GetHeight()<dst->GetHeight()) {
    memset(p, 16, imgSize);
    int imgSizeU = dst->GetHeight(PLANAR_U) * dst->GetPitch(PLANAR_U);
    memset(dst->GetWritePtr(PLANAR_U) , 127, imgSizeU);
    memset(dst->GetWritePtr(PLANAR_V), 127, imgSizeU);
  }


  env->BitBlt(p, dst->GetPitch(), src->GetReadPtr(), src->GetPitch(), src->GetRowSize(), src->GetHeight());
  if (vi.IsPlanar()) {
    env->BitBlt(dst->GetWritePtr(PLANAR_U), dst->GetPitch(PLANAR_U), src->GetReadPtr(PLANAR_U), src->GetPitch(PLANAR_U), src->GetRowSize(PLANAR_U), src->GetHeight(PLANAR_U));
    env->BitBlt(dst->GetWritePtr(PLANAR_V), dst->GetPitch(PLANAR_V), src->GetReadPtr(PLANAR_V), src->GetPitch(PLANAR_V), src->GetRowSize(PLANAR_V), src->GetHeight(PLANAR_V));
    int histUV[256*256] = {0};
    memset(histUV, 0, 256*256);

    const BYTE* pU = src->GetReadPtr(PLANAR_U);
    const BYTE* pV = src->GetReadPtr(PLANAR_V);

    int w = src->GetRowSize(PLANAR_U);
    int p = src->GetPitch(PLANAR_U);

    for (int y = 0; y < src->GetHeight(PLANAR_U); y++) {
      for (int x = 0; x < w; x++) {
        int u = pU[y*p+x];
        int v = pV[y*p+x];
        histUV[v*256+u]++;
      } // end for x
    } // end for y


    // Plot Histogram on Y.
    int maxval = 1;

    // Should we adjust the divisor (maxval)??

    unsigned char* pdstb = dst->GetWritePtr(PLANAR_Y);
    pdstb += src->GetRowSize(PLANAR_Y);

    // Erase all
    for (y=256;y<dst->GetHeight();y++) {
      int p = dst->GetPitch(PLANAR_Y);
      for (int x=0;x<256;x++) {
        pdstb[x+y*p] = 16;
      }
    }

    for (y=0;y<256;y++) {
      for (int x=0;x<256;x++) {
        int disp_val = histUV[x+y*256]/maxval;
        if (y<16 || y>240 || x<16 || x>240)
          disp_val -= 16;

        pdstb[x] = min(255, 16 + disp_val);
        
      }
      pdstb += dst->GetPitch(PLANAR_Y);
    }


    // Draw colors.

    pdstb = dst->GetWritePtr(PLANAR_U);
    pdstb += src->GetRowSize(PLANAR_U);

    // Erase all
    for (y=128;y<dst->GetHeight(PLANAR_U);y++) {
      memset(&pdstb[y*dst->GetPitch(PLANAR_U)], 127, 128);
    }

    for (y=0;y<128;y++) {
      for (int x=0;x<128;x++) {
        pdstb[x] = x*2;
      }
      pdstb += dst->GetPitch(PLANAR_U);
    }

    pdstb = dst->GetWritePtr(PLANAR_V);
    pdstb += src->GetRowSize(PLANAR_V);

    // Erase all
    for (y=128;y<dst->GetHeight(PLANAR_U);y++) {
      memset(&pdstb[y*dst->GetPitch(PLANAR_V)], 127, 128);
    }

    for (y=0;y<128;y++) {
      for (int x=0;x<128;x++) {
        pdstb[x] = y*2;
      }
      pdstb += dst->GetPitch(PLANAR_V);
    }
    
  }
  return dst;
}



PVideoFrame Histogram::DrawMode1(int n, IScriptEnvironment* env) {
  PVideoFrame dst = env->NewVideoFrame(vi);
  BYTE* p = dst->GetWritePtr();
  PVideoFrame src = child->GetFrame(n, env);

  int imgSize = dst->GetHeight()*dst->GetPitch();
  if (src->GetHeight()<dst->GetHeight()) {
    memset(p, 16, imgSize);
    int imgSizeU = dst->GetHeight(PLANAR_U) * dst->GetPitch(PLANAR_U);
    memset(dst->GetWritePtr(PLANAR_U) , 127, imgSizeU);
    memset(dst->GetWritePtr(PLANAR_V), 127, imgSizeU);
  }

  env->BitBlt(p, dst->GetPitch(), src->GetReadPtr(), src->GetPitch(), src->GetRowSize(), src->GetHeight());
  if (vi.IsPlanar()) {
    env->BitBlt(dst->GetWritePtr(PLANAR_U), dst->GetPitch(PLANAR_U), src->GetReadPtr(PLANAR_U), src->GetPitch(PLANAR_U), src->GetRowSize(PLANAR_U), src->GetHeight(PLANAR_U));
    env->BitBlt(dst->GetWritePtr(PLANAR_V), dst->GetPitch(PLANAR_V), src->GetReadPtr(PLANAR_V), src->GetPitch(PLANAR_V), src->GetRowSize(PLANAR_V), src->GetHeight(PLANAR_V));
    
    int histY[256] = {0};
    int histU[256] = {0};
    int histV[256] = {0};
    memset(histY, 0, 256);
    memset(histU, 0, 256);
    memset(histV, 0, 256);
    
    const BYTE* pY = src->GetReadPtr(PLANAR_Y);
    const BYTE* pU = src->GetReadPtr(PLANAR_U);
    const BYTE* pV = src->GetReadPtr(PLANAR_V);
    
    int w = src->GetRowSize(PLANAR_U);
    int p = src->GetPitch(PLANAR_U);
    int pitY = src->GetPitch(PLANAR_Y);
    
    for (int y = 0; y < src->GetHeight(PLANAR_U); y++) {
      for (int x = 0; x < w; x++) {
        histY[pY[y*2*pitY+x*2]]++;
        histY[pY[y*2*pitY+x*2+1]]++;
        histY[pY[y*2*pitY+x*2+pitY]]++;
        histY[pY[y*2*pitY+x*2+pitY+1]]++;
        
        histU[pU[y*p+x]]++;
        histV[pV[y*p+x]]++;
        
      } // end for x
    } // end for y

    unsigned char* pdstb = dst->GetWritePtr(PLANAR_Y);
    pdstb += src->GetRowSize(PLANAR_Y);

    // Clear Y
    for (y=0;y<dst->GetHeight();y++) {
      memset(&pdstb[y*dst->GetPitch(PLANAR_Y)], 16, 256);
    }

    int dstPitch = dst->GetPitch(PLANAR_Y);

    // Draw Unsafe zone (UV-graph)
    for (y=64+16; y<128+16+2; y++) {
      for (int x=0; x<16; x++) {
        pdstb[dstPitch*y+x] = 32;
        pdstb[dstPitch*y+x+240] = 32;
        pdstb[dstPitch*(y+80)+x] = 32;
        pdstb[dstPitch*(y+80)+x+240] = 32;
      }
    }

    // Draw dotted centerline
    for (y=0; y<=256-32; y++) {
      if ((y&3)>1)
        pdstb[dstPitch*y+128] = 128;
    }

    // Draw Y histograms

    int maxval = 0;
    for (int i=0;i<256;i++) {
      maxval = max(histY[i], maxval);
    }

    float scale = 64.0 / maxval;

    for (int x=0;x<256;x++) {
      float scaled_h = (float)histY[x] * scale;
      int h = 64 -  min((int)scaled_h, 64)+1;
      int left = (int)(220.0f*(scaled_h-(float)((int)scaled_h)));

      for (y=64+1 ; y > h ; y--) {
        pdstb[x+y*dstPitch] = 235;
      }
      if (left) pdstb[x+h*dstPitch] = pdstb[x+h*dstPitch]+left;
    }

     // Draw U
    maxval = 0;
    for (i=0; i<256 ;i++) {
      maxval = max(histU[i], maxval);
    }

    scale = 64.0 / maxval;

    for (x=0;x<256;x++) {
      float scaled_h = (float)histU[x] * scale;
      int h = 128+16 -  min((int)scaled_h, 64)+1;
      int left = (int)(220.0f*(scaled_h-(float)((int)scaled_h)));

      for (y=128+16+1 ; y > h ; y--) {
        pdstb[x+y*dstPitch] = 235;
      }
      if (left) pdstb[x+h*dstPitch] = pdstb[x+h*dstPitch]+left;
    }

    // Draw V

    maxval = 0;
    for (i=0; i<256 ;i++) {
      maxval = max(histV[i], maxval);
    }

    scale = 64.0 / maxval;

    for (x=0;x<256;x++) {
      float scaled_h = (float)histV[x] * scale;
      int h = 192+32 -  min((int)scaled_h, 64)+1;
      int left = (int)(220.0f*((int)scaled_h-scaled_h));
      for (y=192+32+1 ; y > h ; y--) {
        pdstb[x+y*dstPitch] = 235;
      }
      if (left) pdstb[x+h*dstPitch] = pdstb[x+h*dstPitch]+left;

    }

    // Draw chroma
    unsigned char* pdstbU = dst->GetWritePtr(PLANAR_U);
    unsigned char* pdstbV = dst->GetWritePtr(PLANAR_V);
    pdstbU += src->GetRowSize(PLANAR_U);
    pdstbV += src->GetRowSize(PLANAR_V);

    // Clear chroma
    int dstPitchUV = dst->GetPitch(PLANAR_U);

    for (y=0;y<dst->GetHeight(PLANAR_U);y++) {
      memset(&pdstbU[y*dstPitchUV], 127, 128);
      memset(&pdstbV[y*dstPitchUV], 127, 128);
    }

    // Draw Unsafe zone (Y-graph)
    for (y=0;y<=32;y++) {
      for (int x=0; x<8; x++) {
        pdstbV[dstPitchUV*y+x] = 160;
        pdstbU[dstPitchUV*y+x] = 16;

      }

      for (x=236/2; x<128; x++) {
        pdstbV[dstPitchUV*y+x] = 160;
        pdstbU[dstPitchUV*y+x] = 16;
      }

    }


    // Draw upper gradient
    for (y=32+8;y<=64+8;y++) {
      for (int x=0; x<128; x++) {
        pdstbU[dstPitchUV*y+x] = x*2;
      }
    }

    //  Draw lower gradient
    for (y=64+16;y<=64+32+16;y++) {
      for (int x=0; x<128; x++) {
        pdstbV[dstPitchUV*y+x] = x*2;
      }
    }
    
  }
  
  return dst;
}


PVideoFrame Histogram::DrawMode0(int n, IScriptEnvironment* env) 
{
  PVideoFrame dst = env->NewVideoFrame(vi);
  BYTE* p = dst->GetWritePtr();
  PVideoFrame src = child->GetFrame(n, env);
  env->BitBlt(p, dst->GetPitch(), src->GetReadPtr(), src->GetPitch(), src->GetRowSize(), src->GetHeight());
  if (vi.IsPlanar()) {
    env->BitBlt(dst->GetWritePtr(PLANAR_U), dst->GetPitch(PLANAR_U), src->GetReadPtr(PLANAR_U), src->GetPitch(PLANAR_U), src->GetRowSize(PLANAR_U), src->GetHeight(PLANAR_U));
    env->BitBlt(dst->GetWritePtr(PLANAR_V), dst->GetPitch(PLANAR_V), src->GetReadPtr(PLANAR_V), src->GetPitch(PLANAR_V), src->GetRowSize(PLANAR_V), src->GetHeight(PLANAR_V));
    BYTE* p2 = dst->GetWritePtr(PLANAR_U);
    BYTE* p3 = dst->GetWritePtr(PLANAR_V);
    for (int y=0; y<src->GetHeight(PLANAR_Y); ++y) {
      int hist[256] = {0};
      int x;
      for (x=0; x<vi.width-256; ++x) {
        hist[p[x]]++;
      }
      if (y&1) {
        for (x=0; x<128; ++x) {
          p[x*2+vi.width-256] = min(255, hist[x*2]*4);
          p[x*2+vi.width-256+1] = min(255, hist[x*2+1]*4);
          if (x<8) {
            p2[x+(vi.width>>1)-128]  = 0;
            p3[x+(vi.width>>1)-128]  = 160; 
          } else if (x>118) {
            p2[x+(vi.width>>1)-128]  = 0;
            p3[x+(vi.width>>1)-128]  = 160;
          } else if (x==62) {
            p2[x+(vi.width>>1)-128]  = 160;
            p3[x+(vi.width>>1)-128]  = 0;
          } else {
            p2[x+(vi.width>>1)-128]  = 128;
            p3[x+(vi.width>>1)-128]  = 128;
          }
        }
        p2+= dst->GetPitch(PLANAR_U);
        p3+= dst->GetPitch(PLANAR_U);
      } else {
        for (x=0; x<256; ++x) {
          p[x+vi.width-256] = min(255, hist[x]*4);
        }
      }
      p += dst->GetPitch();
    }
    return dst;
  }
  for (int y=0; y<src->GetHeight(); ++y) {
    int hist[256] = {0};
    int x;
    for (x=0; x<vi.width-256; ++x) {
      hist[p[x*2]]++;
    }
    for (x=0; x<256; ++x) {
      p[x*2+vi.width*2-512] = min(255, hist[x]*3);
      p[x*2+vi.width*2-511] = 128;
    }
    p += dst->GetPitch();
  }
  return dst;
}


AVSValue __cdecl Histogram::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  const char* st_m = args[1].AsString("classic");

  int mode = 0; 

  if (!lstrcmpi(st_m, "classic"))
    mode = 0;

  if (!lstrcmpi(st_m, "levels"))
    mode = 1;

  if (!lstrcmpi(st_m, "color"))
    mode = 2;

  if (!lstrcmpi(st_m, "luma"))
    mode = 3;

  if (!lstrcmpi(st_m, "stereo"))
    mode = 4;

  if (!lstrcmpi(st_m, "stereooverlay"))
    mode = 5;

  return new Histogram(args[0].AsClip(), mode, env);
}
