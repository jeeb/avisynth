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

// Overlay (c) 2003, 2004 by Klaus Post

#include "stdafx.h"
#include "overlayfunctions.h"

void OL_AddImage::BlendImageMask(Image444* base, Image444* overlay, Image444* mask) {
  BYTE* baseY = base->GetPtr(PLANAR_Y);
  BYTE* baseU = base->GetPtr(PLANAR_U);
  BYTE* baseV = base->GetPtr(PLANAR_V);

  BYTE* ovY = overlay->GetPtr(PLANAR_Y);
  BYTE* ovU = overlay->GetPtr(PLANAR_U);
  BYTE* ovV = overlay->GetPtr(PLANAR_V);

  BYTE* maskY = mask->GetPtr(PLANAR_Y);
  BYTE* maskU = mask->GetPtr(PLANAR_U);
  BYTE* maskV = mask->GetPtr(PLANAR_V);
  int w = base->w();
  int h = base->h();
  if (opacity == 256) {
    for (int y = 0; y < h; y++) {
      for (int x = 0; x < w; x++) {
        int Y = baseY[x] + ((ovY[x]*maskY[x])>>8);
        int U = baseU[x] + (((128*(256-maskU[x]))+(maskU[x]*(ovU[x])))>>8) - 128;
        int V = baseV[x] + (((128*(256-maskV[x]))+(maskV[x]*(ovV[x])))>>8) - 128;
        if (Y>255) {  // Apply overbrightness to UV
          int multiplier = max(0,288-Y);  // 0 to 32
          U = ((U*multiplier) + (128*(32-multiplier)))>>5;
          V = ((V*multiplier) + (128*(32-multiplier)))>>5;
          Y = 255;
        }
        baseU[x] = (BYTE)min(U,255);
        baseV[x] = (BYTE)min(V,255);
        baseY[x] = (BYTE)Y;
      }
      maskY += mask->pitch;
      maskU += mask->pitch;
      maskV += mask->pitch;

      baseY += base->pitch;
      baseU += base->pitch;
      baseV += base->pitch;

      ovY += overlay->pitch;
      ovU += overlay->pitch;
      ovV += overlay->pitch;

    }
  } else {
    for (int y = 0; y < h; y++) {
      for (int x = 0; x < w; x++) {
        int Y = baseY[x] + (((maskY[x]*opacity*ovY[x])>>16));
        int mU = (maskU[x] * opacity)>>8;
        int mV = (maskV[x] * opacity)>>8;
        int U = baseU[x] + (((128*(256-mU))+(mU*(ovU[x])))>>8) - 128;
        int V = baseV[x] + (((128*(256-mV))+(mV*(ovV[x])))>>8) - 128;

        if (Y>255) {  // Apply overbrightness to UV
          int multiplier = max(0,288-Y);  // 0 to 32
          U = ((U*multiplier) + (128*(32-multiplier)))>>5;
          V = ((V*multiplier) + (128*(32-multiplier)))>>5;
          Y = 255;
        }
        baseU[x] = (BYTE)min(U,255);
        baseV[x] = (BYTE)min(V,255);
        baseY[x] = (BYTE)Y;
      }
      baseY += base->pitch;
      baseU += base->pitch;
      baseV += base->pitch;

      ovY += overlay->pitch;
      ovU += overlay->pitch;
      ovV += overlay->pitch;

      maskY += mask->pitch;
      maskU += mask->pitch;
      maskV += mask->pitch;
    }
  }
  
}

void OL_AddImage::BlendImage(Image444* base, Image444* overlay) {
        
  BYTE* baseY = base->GetPtr(PLANAR_Y);
  BYTE* baseU = base->GetPtr(PLANAR_U);
  BYTE* baseV = base->GetPtr(PLANAR_V);

  BYTE* ovY = overlay->GetPtr(PLANAR_Y);
  BYTE* ovU = overlay->GetPtr(PLANAR_U);
  BYTE* ovV = overlay->GetPtr(PLANAR_V);
  
  int w = base->w();
  int h = base->h();
  if (opacity == 256) {
    for (int y = 0; y < h; y++) {
      for (int x = 0; x < w; x++) {
        int Y = baseY[x] + ovY[x];
        int U = baseU[x] + ovU[x] - 128;
        int V = baseV[x] + ovV[x] - 128;
        if (Y>255) {  // Apply overbrightness to UV
          int multiplier = max(0,288-Y);  // 0 to 32
          U = ((U*multiplier) + (128*(32-multiplier)))>>5;
          V = ((V*multiplier) + (128*(32-multiplier)))>>5;
          Y = 255;
        }
        baseU[x] = (BYTE)U;
        baseV[x] = (BYTE)V;
        baseY[x] = (BYTE)Y;
      }
      baseY += base->pitch;
      baseU += base->pitch;
      baseV += base->pitch;

      ovY += overlay->pitch;
      ovU += overlay->pitch;
      ovV += overlay->pitch;

    }
  } else {
    for (int y = 0; y < h; y++) {
      for (int x = 0; x < w; x++) {
        int Y = baseY[x] + ((opacity*ovY[x])>>8);
        int U = baseU[x] + (((128*inv_opacity)+(opacity*(ovU[x])))>>8) - 128;
        int V = baseV[x] + (((128*inv_opacity)+(opacity*(ovV[x])))>>8) - 128;
        if (Y>255) {  // Apply overbrightness to UV
          int multiplier = max(0,288-Y);  // 0 to 32
          U = ((U*multiplier) + (128*(32-multiplier)))>>5;
          V = ((V*multiplier) + (128*(32-multiplier)))>>5;
          Y = 255;
        }
        baseU[x] = (BYTE)min(U,255);
        baseV[x] = (BYTE)min(V,255);
        baseY[x] = (BYTE)Y;
      }
      baseY += base->pitch;
      baseU += base->pitch;
      baseV += base->pitch;

      ovY += overlay->pitch;
      ovU += overlay->pitch;
      ovV += overlay->pitch;
    }
  }
}

