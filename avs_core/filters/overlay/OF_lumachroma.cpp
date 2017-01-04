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
#include <emmintrin.h>

#include <stdint.h>

void OL_BlendLumaImage::DoBlendImageMask(ImageOverlayInternal* base, ImageOverlayInternal* overlay, ImageOverlayInternal* mask) {
  if (bits_per_pixel == 8)
    BlendImageMask<uint8_t>(base, overlay, mask);
  else if(bits_per_pixel <= 16)
    BlendImageMask<uint16_t>(base, overlay, mask);
  //else if(bits_per_pixel == 32)
  //  BlendImageMask<float>(base, overlay, mask);
}

void OL_BlendLumaImage::DoBlendImage(ImageOverlayInternal* base, ImageOverlayInternal* overlay) {
  if (bits_per_pixel == 8)
    BlendImage<uint8_t>(base, overlay);
  else if(bits_per_pixel <= 16)
    BlendImage<uint16_t>(base, overlay);
  //else if(bits_per_pixel == 32)
  //  BlendImage<float>(base, overlay);
}

void OL_BlendChromaImage::DoBlendImageMask(ImageOverlayInternal* base, ImageOverlayInternal* overlay, ImageOverlayInternal* mask) {
  if (bits_per_pixel == 8)
    BlendImageMask<uint8_t>(base, overlay, mask);
  else if(bits_per_pixel <= 16)
    BlendImageMask<uint16_t>(base, overlay, mask);
  //else if(bits_per_pixel == 32)
  //  BlendImageMask<float>(base, overlay, mask);
}

void OL_BlendChromaImage::DoBlendImage(ImageOverlayInternal* base, ImageOverlayInternal* overlay) {
  if (bits_per_pixel == 8)
    BlendImage<uint8_t>(base, overlay);
  else if(bits_per_pixel <= 16)
    BlendImage<uint16_t>(base, overlay);
  //else if(bits_per_pixel == 32)
  //  BlendImage<float>(base, overlay);
}

template<typename pixel_t>
void OL_BlendLumaImage::BlendImageMask(ImageOverlayInternal* base, ImageOverlayInternal* overlay, ImageOverlayInternal* mask) {
  BYTE* baseY = base->GetPtr(PLANAR_Y);

  BYTE* ovY = overlay->GetPtr(PLANAR_Y);
  
  BYTE* maskY = mask->GetPtr(PLANAR_Y);
  
  int w = base->w();
  int h = base->h();

  const int pixelsize = sizeof(pixel_t);

  if (opacity == 256) {
    if (pixelsize == 1 && (env->GetCPUFlags() & CPUF_SSE2)) {
      overlay_blend_sse2_plane_masked(baseY, ovY, maskY, base->pitch, overlay->pitch, mask->pitch, w, h);
    } else
#ifdef X86_32
    if (pixelsize == 1 && (env->GetCPUFlags() & CPUF_MMX)) {
      overlay_blend_mmx_plane_masked(baseY, ovY, maskY, base->pitch, overlay->pitch, mask->pitch, w, h);
      _mm_empty();
    } else
#endif
    {
      switch (bits_per_pixel) {
      case 8:
        overlay_blend_c_plane_masked<uint8_t, 8>(baseY, ovY, maskY, base->pitch, overlay->pitch, mask->pitch, w, h);
        break;
      case 10:
        overlay_blend_c_plane_masked<uint16_t,10>(baseY, ovY, maskY, base->pitch, overlay->pitch, mask->pitch, w, h);
        break;
      case 12:
        overlay_blend_c_plane_masked<uint16_t,12>(baseY, ovY, maskY, base->pitch, overlay->pitch, mask->pitch, w, h);
        break;
      case 14:
        overlay_blend_c_plane_masked<uint16_t,14>(baseY, ovY, maskY, base->pitch, overlay->pitch, mask->pitch, w, h);
        break;
      case 16:
        overlay_blend_c_plane_masked<uint16_t,16>(baseY, ovY, maskY, base->pitch, overlay->pitch, mask->pitch, w, h);
        break;
      }
    }
  } else {
    if (pixelsize == 1 && (env->GetCPUFlags() & CPUF_SSE2)) {
      overlay_blend_sse2_plane_masked_opacity(baseY, ovY, maskY, base->pitch, overlay->pitch, mask->pitch, w, h, opacity);
    } else
#ifdef X86_32
    if (pixelsize == 1 && (env->GetCPUFlags() & CPUF_MMX)) {
      overlay_blend_mmx_plane_masked_opacity(baseY, ovY, maskY, base->pitch, overlay->pitch, mask->pitch, w, h, opacity);
      _mm_empty();
    } else
#endif
    {
      switch (bits_per_pixel) {
      case 8:
        overlay_blend_c_plane_masked_opacity<uint8_t, 8>(baseY, ovY, maskY, base->pitch, overlay->pitch, mask->pitch, w, h, opacity);
        break;
      case 10:
        overlay_blend_c_plane_masked_opacity<uint16_t,10>(baseY, ovY, maskY, base->pitch, overlay->pitch, mask->pitch, w, h, opacity);
        break;
      case 12:
        overlay_blend_c_plane_masked_opacity<uint16_t,12>(baseY, ovY, maskY, base->pitch, overlay->pitch, mask->pitch, w, h, opacity);
        break;
      case 14:
        overlay_blend_c_plane_masked_opacity<uint16_t,14>(baseY, ovY, maskY, base->pitch, overlay->pitch, mask->pitch, w, h, opacity);
        break;
      case 16:
        overlay_blend_c_plane_masked_opacity<uint16_t,16>(baseY, ovY, maskY, base->pitch, overlay->pitch, mask->pitch, w, h, opacity);
        break;
      }
    }
  }
}


template<typename pixel_t>
void OL_BlendLumaImage::BlendImage(ImageOverlayInternal* base, ImageOverlayInternal* overlay) {
  BYTE* baseY = base->GetPtr(PLANAR_Y);

  BYTE* ovY = overlay->GetPtr(PLANAR_Y);

  int w = base->w();
  int h = base->h();

  const int pixelsize = sizeof(pixel_t);

  if (opacity == 256) {
    env->BitBlt(baseY, base->pitch, ovY, overlay->pitch, w*pixelsize, h);
  } else {
    if (pixelsize == 1 && (env->GetCPUFlags() & CPUF_SSE2)) {
      overlay_blend_sse2_plane_opacity(baseY, ovY, base->pitch, overlay->pitch, w, h, opacity);
    } else
#ifdef X86_32
    if (pixelsize == 1 && (env->GetCPUFlags() & CPUF_MMX)) {
      overlay_blend_mmx_plane_opacity(baseY, ovY, base->pitch, overlay->pitch, w, h, opacity);
      _mm_empty();
    } else
#endif
    {
      switch (bits_per_pixel) {
      case 8:
        overlay_blend_c_plane_opacity<uint8_t, 8>(baseY, ovY, base->pitch, overlay->pitch, w, h, opacity);
        break;
      case 10:
        overlay_blend_c_plane_opacity<uint16_t,10>(baseY, ovY, base->pitch, overlay->pitch, w, h, opacity);
        break;
      case 12:
        overlay_blend_c_plane_opacity<uint16_t,12>(baseY, ovY, base->pitch, overlay->pitch, w, h, opacity);
        break;
      case 14:
        overlay_blend_c_plane_opacity<uint16_t,14>(baseY, ovY, base->pitch, overlay->pitch, w, h, opacity);
        break;
      case 16:
        overlay_blend_c_plane_opacity<uint16_t,16>(baseY, ovY, base->pitch, overlay->pitch, w, h, opacity);
        break;
      }
    }
  }
}



template<typename pixel_t>
void OL_BlendChromaImage::BlendImageMask(ImageOverlayInternal* base, ImageOverlayInternal* overlay, ImageOverlayInternal* mask) {
  BYTE* baseU = base->GetPtr(PLANAR_U);
  BYTE* baseV = base->GetPtr(PLANAR_V);

  BYTE* ovU = overlay->GetPtr(PLANAR_U);
  BYTE* ovV = overlay->GetPtr(PLANAR_V);
  
  BYTE* maskU = mask->GetPtr(PLANAR_U);
  BYTE* maskV = mask->GetPtr(PLANAR_V);

  int w = base->w();
  int h = base->h();

  const int pixelsize = sizeof(pixel_t);

  if (opacity == 256) {
    if (pixelsize == 1 && (env->GetCPUFlags() & CPUF_SSE2)) {
      overlay_blend_sse2_plane_masked(baseU, ovU, maskU, base->pitch, overlay->pitch, mask->pitch, w, h);
      overlay_blend_sse2_plane_masked(baseV, ovV, maskV, base->pitch, overlay->pitch, mask->pitch, w, h);
    } else
#ifdef X86_32
    if (pixelsize == 1 && (env->GetCPUFlags() & CPUF_MMX)) {
      overlay_blend_mmx_plane_masked(baseU, ovU, maskU, base->pitch, overlay->pitch, mask->pitch, w, h);
      overlay_blend_mmx_plane_masked(baseV, ovV, maskV, base->pitch, overlay->pitch, mask->pitch, w, h);
      _mm_empty();
    } else
#endif
    {
      switch (bits_per_pixel) {
      case 8:
        overlay_blend_c_plane_masked<uint8_t, 8>(baseU, ovU, maskU, base->pitch, overlay->pitch, mask->pitch, w, h);
        overlay_blend_c_plane_masked<uint8_t, 8>(baseV, ovV, maskV, base->pitch, overlay->pitch, mask->pitch, w, h);
        break;
      case 10:
        overlay_blend_c_plane_masked<uint16_t,10>(baseU, ovU, maskU, base->pitch, overlay->pitch, mask->pitch, w, h);
        overlay_blend_c_plane_masked<uint16_t,10>(baseV, ovV, maskV, base->pitch, overlay->pitch, mask->pitch, w, h);
        break;
      case 120:
        overlay_blend_c_plane_masked<uint16_t,12>(baseU, ovU, maskU, base->pitch, overlay->pitch, mask->pitch, w, h);
        overlay_blend_c_plane_masked<uint16_t,12>(baseV, ovV, maskV, base->pitch, overlay->pitch, mask->pitch, w, h);
        break;
      case 14:
        overlay_blend_c_plane_masked<uint16_t,14>(baseU, ovU, maskU, base->pitch, overlay->pitch, mask->pitch, w, h);
        overlay_blend_c_plane_masked<uint16_t,14>(baseV, ovV, maskV, base->pitch, overlay->pitch, mask->pitch, w, h);
        break;
      case 16:
        overlay_blend_c_plane_masked<uint16_t,16>(baseU, ovU, maskU, base->pitch, overlay->pitch, mask->pitch, w, h);
        overlay_blend_c_plane_masked<uint16_t,16>(baseV, ovV, maskV, base->pitch, overlay->pitch, mask->pitch, w, h);
        break;
      }
    }
  } else {
    if (pixelsize == 1 && (env->GetCPUFlags() & CPUF_SSE2)) {
      overlay_blend_sse2_plane_masked_opacity(baseU, ovU, maskU, base->pitch, overlay->pitch, mask->pitch, w, h, opacity);
      overlay_blend_sse2_plane_masked_opacity(baseV, ovV, maskV, base->pitch, overlay->pitch, mask->pitch, w, h, opacity);
    } else
#ifdef X86_32
    if (pixelsize == 1 && (env->GetCPUFlags() & CPUF_MMX)) {
      overlay_blend_mmx_plane_masked_opacity(baseU, ovU, maskU, base->pitch, overlay->pitch, mask->pitch, w, h, opacity);
      overlay_blend_mmx_plane_masked_opacity(baseV, ovV, maskV, base->pitch, overlay->pitch, mask->pitch, w, h, opacity);
      _mm_empty();
    } else
#endif
    {
      switch (bits_per_pixel) {
      case 8:
        overlay_blend_c_plane_masked_opacity<uint8_t, 8>(baseU, ovU, maskU, base->pitch, overlay->pitch, mask->pitch, w, h, opacity);
        overlay_blend_c_plane_masked_opacity<uint8_t, 8>(baseV, ovV, maskV, base->pitch, overlay->pitch, mask->pitch, w, h, opacity);
        break;
      case 10:
        overlay_blend_c_plane_masked_opacity<uint16_t,10>(baseU, ovU, maskU, base->pitch, overlay->pitch, mask->pitch, w, h, opacity);
        overlay_blend_c_plane_masked_opacity<uint16_t,10>(baseV, ovV, maskV, base->pitch, overlay->pitch, mask->pitch, w, h, opacity);
        break;
      case 12:
        overlay_blend_c_plane_masked_opacity<uint16_t,12>(baseU, ovU, maskU, base->pitch, overlay->pitch, mask->pitch, w, h, opacity);
        overlay_blend_c_plane_masked_opacity<uint16_t,12>(baseV, ovV, maskV, base->pitch, overlay->pitch, mask->pitch, w, h, opacity);
        break;
      case 14:
        overlay_blend_c_plane_masked_opacity<uint16_t,14>(baseU, ovU, maskU, base->pitch, overlay->pitch, mask->pitch, w, h, opacity);
        overlay_blend_c_plane_masked_opacity<uint16_t,14>(baseV, ovV, maskV, base->pitch, overlay->pitch, mask->pitch, w, h, opacity);
        break;
      case 16:
        overlay_blend_c_plane_masked_opacity<uint16_t,16>(baseU, ovU, maskU, base->pitch, overlay->pitch, mask->pitch, w, h, opacity);
        overlay_blend_c_plane_masked_opacity<uint16_t,16>(baseV, ovV, maskV, base->pitch, overlay->pitch, mask->pitch, w, h, opacity);
        break;
      }
    }
  }
}

template<typename pixel_t>
void OL_BlendChromaImage::BlendImage(ImageOverlayInternal* base, ImageOverlayInternal* overlay) {
  BYTE* baseU = base->GetPtr(PLANAR_U);
  BYTE* baseV = base->GetPtr(PLANAR_V);

  BYTE* ovU = overlay->GetPtr(PLANAR_U);
  BYTE* ovV = overlay->GetPtr(PLANAR_V);
  
  int w = base->w();
  int h = base->h();

  const int pixelsize = sizeof(pixel_t);

  if (opacity == 256) {
    env->BitBlt(baseU, base->pitch, ovU, overlay->pitch, w*pixelsize, h);
    env->BitBlt(baseV, base->pitch, ovV, overlay->pitch, w*pixelsize, h);
  } else {
    if (pixelsize == 1 && (env->GetCPUFlags() & CPUF_SSE2)) {
      overlay_blend_sse2_plane_opacity(baseU, ovU, base->pitch, overlay->pitch, w, h, opacity);
      overlay_blend_sse2_plane_opacity(baseV, ovV, base->pitch, overlay->pitch, w, h, opacity);
    } else
#ifdef X86_32
    if (pixelsize == 1 && (env->GetCPUFlags() & CPUF_MMX)) {
      overlay_blend_mmx_plane_opacity(baseU, ovU, base->pitch, overlay->pitch, w, h, opacity);
      overlay_blend_mmx_plane_opacity(baseV, ovV, base->pitch, overlay->pitch, w, h, opacity);
      _mm_empty();
    } else
#endif
    {
      switch (bits_per_pixel) {
      case 8:
        overlay_blend_c_plane_opacity<uint8_t, 8>(baseU, ovU, base->pitch, overlay->pitch, w, h, opacity);
        overlay_blend_c_plane_opacity<uint8_t, 8>(baseV, ovV, base->pitch, overlay->pitch, w, h, opacity);
        break;
      case 10:
        overlay_blend_c_plane_opacity<uint16_t,10>(baseU, ovU, base->pitch, overlay->pitch, w, h, opacity);
        overlay_blend_c_plane_opacity<uint16_t,10>(baseV, ovV, base->pitch, overlay->pitch, w, h, opacity);
        break;
      case 12:
        overlay_blend_c_plane_opacity<uint16_t,12>(baseU, ovU, base->pitch, overlay->pitch, w, h, opacity);
        overlay_blend_c_plane_opacity<uint16_t,12>(baseV, ovV, base->pitch, overlay->pitch, w, h, opacity);
        break;
      case 14:
        overlay_blend_c_plane_opacity<uint16_t,14>(baseU, ovU, base->pitch, overlay->pitch, w, h, opacity);
        overlay_blend_c_plane_opacity<uint16_t,14>(baseV, ovV, base->pitch, overlay->pitch, w, h, opacity);
        break;
      case 16:
        overlay_blend_c_plane_opacity<uint16_t,16>(baseU, ovU, base->pitch, overlay->pitch, w, h, opacity);
        overlay_blend_c_plane_opacity<uint16_t,16>(baseV, ovV, base->pitch, overlay->pitch, w, h, opacity);
        break;
      }
    }
  }
}

