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

#include "overlayfunctions.h"

#include <stdint.h>
#include <type_traits>

void OL_MultiplyImage::DoBlendImageMask(ImageOverlayInternal* base, ImageOverlayInternal* overlay, ImageOverlayInternal* mask) {
  if (bits_per_pixel == 8)
    BlendImageMask<uint8_t>(base, overlay, mask);
  else if(bits_per_pixel <= 16)
    BlendImageMask<uint16_t>(base, overlay, mask);
  //else if(bits_per_pixel == 32)
  //  BlendImageMask<float>(base, overlay, mask);

}

void OL_MultiplyImage::DoBlendImage(ImageOverlayInternal* base, ImageOverlayInternal* overlay) {
  if (bits_per_pixel == 8)
    BlendImage<uint8_t>(base, overlay);
  else if(bits_per_pixel <= 16)
    BlendImage<uint16_t>(base, overlay);
  //else if(bits_per_pixel == 32)
  //  BlendImage<float>(base, overlay);
}

template<typename pixel_t>
void OL_MultiplyImage::BlendImageMask(ImageOverlayInternal* base, ImageOverlayInternal* overlay, ImageOverlayInternal* mask) {
  pixel_t* baseY = reinterpret_cast<pixel_t *>(base->GetPtr(PLANAR_Y));
  pixel_t* baseU = reinterpret_cast<pixel_t *>(base->GetPtr(PLANAR_U));
  pixel_t* baseV = reinterpret_cast<pixel_t *>(base->GetPtr(PLANAR_V));

  pixel_t* ovY = reinterpret_cast<pixel_t *>(overlay->GetPtr(PLANAR_Y));
  pixel_t* ovU = reinterpret_cast<pixel_t *>(overlay->GetPtr(PLANAR_U));
  pixel_t* ovV = reinterpret_cast<pixel_t *>(overlay->GetPtr(PLANAR_V));

  pixel_t* maskY = reinterpret_cast<pixel_t *>(mask->GetPtr(PLANAR_Y));
  pixel_t* maskU = reinterpret_cast<pixel_t *>(mask->GetPtr(PLANAR_U));
  pixel_t* maskV = reinterpret_cast<pixel_t *>(mask->GetPtr(PLANAR_V));

  const int half_pixel_value_rounding = (sizeof(pixel_t) == 1) ? 128 : (1 << (bits_per_pixel - 1));
  const int max_pixel_value = (sizeof(pixel_t) == 1) ? 255 : (1 << bits_per_pixel) - 1;
  const int pixel_range = max_pixel_value + 1;
  const int MASK_CORR_SHIFT = (sizeof(pixel_t) == 1) ? 8 : bits_per_pixel;
  const int OPACITY_SHIFT  = 8; // opacity always max 0..256
  const int basepitch = (base->pitch) / sizeof(pixel_t);
  const int overlaypitch = (overlay->pitch) / sizeof(pixel_t);
  const int maskpitch = (mask->pitch) / sizeof(pixel_t);

  // avoid "uint16*uint16 can't get into int32" overflows
  typedef typename std::conditional < sizeof(pixel_t) == 1, int, typename std::conditional < sizeof(pixel_t) == 2, __int64, float>::type >::type result_t;

  int w = base->w();
  int h = base->h();
  if (opacity == 256) {
    for (int y = 0; y < h; y++) {
      for (int x = 0; x < w; x++) {
        int op = maskY[x];
        result_t invop = pixel_range - op;
        result_t ovYx = ovY[x];
        int Y = (int)((baseY[x] * (pixel_range*invop + (ovYx * op))) >> (MASK_CORR_SHIFT*2));

        op = maskU[x];
        invop = pixel_range - op;
        int U = (int)(((baseU[x] * invop * pixel_range)  + (op * (baseU[x] * ovYx + half_pixel_value_rounding * (pixel_range-ovYx)))) >> (MASK_CORR_SHIFT*2));

        op = maskV[x];
        invop = pixel_range-op;
        int V = (int)(((baseV[x] * invop * pixel_range)  + (op * (baseV[x] * ovYx + half_pixel_value_rounding * (pixel_range-ovYx)))) >> (MASK_CORR_SHIFT*2));

        baseU[x] = (pixel_t)U;
        baseV[x] = (pixel_t)V;
        baseY[x] = (pixel_t)Y;
      }
      maskY += maskpitch;
      maskU += maskpitch;
      maskV += maskpitch;

      baseY += basepitch;
      baseU += basepitch;
      baseV += basepitch;

      ovY += overlaypitch;
      ovU += overlaypitch;
      ovV += overlaypitch;

    }
  } else {
    for (int y = 0; y < h; y++) {
      for (int x = 0; x < w; x++) {
        int op = (maskY[x]*opacity)>>OPACITY_SHIFT;
        result_t invop = pixel_range - op;
        result_t ovYx = ovY[x];
        int Y = (int)((baseY[x] * (pixel_range*invop + (ovYx * op))) >> (MASK_CORR_SHIFT*2));

        op = (maskU[x]*opacity)>>OPACITY_SHIFT;
        invop = pixel_range - op;
        int U = (int)(((baseU[x] * invop * pixel_range)  + (op * (baseU[x] * ovYx + half_pixel_value_rounding * (pixel_range-ovYx)))) >> (MASK_CORR_SHIFT*2));

        op = (maskV[x]*opacity)>>OPACITY_SHIFT;
        invop = pixel_range-op;
        int V = (int)(((baseV[x] * invop * pixel_range)  + (op * (baseV[x] * ovYx + half_pixel_value_rounding * (pixel_range-ovYx)))) >> (MASK_CORR_SHIFT*2));

        baseU[x] = (pixel_t)U;
        baseV[x] = (pixel_t)V;
        baseY[x] = (pixel_t)Y;
      }
      baseY += basepitch;
      baseU += basepitch;
      baseV += basepitch;

      ovY += overlaypitch;
      ovU += overlaypitch;
      ovV += overlaypitch;

      maskY += maskpitch;
      maskU += maskpitch;
      maskV += maskpitch;
    }
  }
  
}

template<typename pixel_t>
void OL_MultiplyImage::BlendImage(ImageOverlayInternal* base, ImageOverlayInternal* overlay) {
        
  pixel_t* baseY = reinterpret_cast<pixel_t *>(base->GetPtr(PLANAR_Y));
  pixel_t* baseU = reinterpret_cast<pixel_t *>(base->GetPtr(PLANAR_U));
  pixel_t* baseV = reinterpret_cast<pixel_t *>(base->GetPtr(PLANAR_V));

  pixel_t* ovY = reinterpret_cast<pixel_t *>(overlay->GetPtr(PLANAR_Y));
  pixel_t* ovU = reinterpret_cast<pixel_t *>(overlay->GetPtr(PLANAR_U));
  pixel_t* ovV = reinterpret_cast<pixel_t *>(overlay->GetPtr(PLANAR_V));

  const int half_pixel_value_rounding = (sizeof(pixel_t) == 1) ? 128 : (1 << (bits_per_pixel - 1));
  const int max_pixel_value = (sizeof(pixel_t) == 1) ? 255 : (1 << bits_per_pixel) - 1;
  const int pixel_range = max_pixel_value + 1;
  const int MASK_CORR_SHIFT = (sizeof(pixel_t) == 1) ? 8 : bits_per_pixel;
  const int OPACITY_SHIFT  = 8; // opacity always max 0..256
  const int basepitch = (base->pitch) / sizeof(pixel_t);
  const int overlaypitch = (overlay->pitch) / sizeof(pixel_t);

  // avoid "uint16*uint16 can't get into int32" overflows
  typedef typename std::conditional < sizeof(pixel_t) == 1, int, typename std::conditional < sizeof(pixel_t) == 2, __int64, float>::type >::type result_t;

  int w = base->w();
  int h = base->h();
  if (opacity == 256) {
    for (int y = 0; y < h; y++) {
      for (int x = 0; x < w; x++) {
        result_t ovYx = ovY[x];
        int Y = (int)((baseY[x] * ovYx) >> MASK_CORR_SHIFT);
        int U = (int)((baseU[x] * ovYx + half_pixel_value_rounding * (pixel_range - ovYx)) >> MASK_CORR_SHIFT);
        int V = (int)((baseV[x] * ovYx + half_pixel_value_rounding * (pixel_range - ovYx)) >> MASK_CORR_SHIFT);
        baseY[x] = (pixel_t)Y;
        baseU[x] = (pixel_t)U;
        baseV[x] = (pixel_t)V;
      }
      baseY += basepitch;
      baseU += basepitch;
      baseV += basepitch;

      ovY += overlaypitch;
      ovU += overlaypitch;
      ovV += overlaypitch;

    }
  } else {
    for (int y = 0; y < h; y++) {
      for (int x = 0; x < w; x++) {
        result_t ovYx = ovY[x];
        result_t baseYx = baseY[x];
        int Y = (int)((baseYx * (pixel_range*inv_opacity + (ovYx * opacity))) >> (MASK_CORR_SHIFT+OPACITY_SHIFT));
        result_t baseUx = baseU[x];
        int U = (int)(((baseUx * inv_opacity * pixel_range)  + (opacity * (baseUx * ovYx + half_pixel_value_rounding * (pixel_range-ovYx)))) >> (MASK_CORR_SHIFT+OPACITY_SHIFT));
        result_t baseVx = baseV[x];
        int V = (int)(((baseVx * inv_opacity * pixel_range)  + (opacity * (baseVx * ovYx + half_pixel_value_rounding * (pixel_range-ovYx)))) >> (MASK_CORR_SHIFT+OPACITY_SHIFT));

        baseU[x] = (pixel_t)U;
        baseV[x] = (pixel_t)V;
        baseY[x] = (pixel_t)Y;
      }
      baseY += basepitch;
      baseU += basepitch;
      baseV += basepitch;

      ovY += overlaypitch;
      ovU += overlaypitch;
      ovV += overlaypitch;
    }
  }
}

