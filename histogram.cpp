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
  if (!vi.IsYUV())
    env->ThrowError("Histogram: YUV data only");
  vi.width += 256;

  if (mode ==1) {
    if (!vi.IsPlanar())
      env->ThrowError("Histogram: Mode 1 only available in YV12.");
    vi.height = max(256,vi.height);
  }

  if (mode ==2) {
    if (!vi.IsPlanar())
      env->ThrowError("Histogram: Mode 2 only available in YV12.");
    vi.height = max(256,vi.height);
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
  }
  return DrawMode0(n, env);
}

PVideoFrame Histogram::DrawMode2(int n, IScriptEnvironment* env) {
  PVideoFrame dst = env->NewVideoFrame(vi);
  BYTE* p = dst->GetWritePtr();
  PVideoFrame src = child->GetFrame(n, env);
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

  return new Histogram(args[0].AsClip(), mode, env);
}
