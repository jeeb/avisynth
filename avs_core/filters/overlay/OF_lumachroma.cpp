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

void OL_BlendLumaImage::DoBlendImageMask(Image444* base, Image444* overlay, Image444* mask) {
  if (bits_per_pixel == 8)
    BlendImageMask<uint8_t>(base, overlay, mask);
  //else if(bits_per_pixel == 32)
  //  BlendImageMask<float>(base, overlay, mask);
  else if(bits_per_pixel == 16)
    BlendImageMask<uint16_t>(base, overlay, mask);
}

void OL_BlendLumaImage::DoBlendImage(Image444* base, Image444* overlay) {
  if (bits_per_pixel == 8)
    BlendImage<uint8_t>(base, overlay);
  //else if(bits_per_pixel == 32)
  //  BlendImage<float>(base, overlay);
  else if(bits_per_pixel == 16)
    BlendImage<uint16_t>(base, overlay);
}

void OL_BlendChromaImage::DoBlendImageMask(Image444* base, Image444* overlay, Image444* mask) {
  if (bits_per_pixel == 8)
    BlendImageMask<uint8_t>(base, overlay, mask);
  //else if(bits_per_pixel == 32)
  //  BlendImageMask<float>(base, overlay, mask);
  else if(bits_per_pixel == 16)
    BlendImageMask<uint16_t>(base, overlay, mask);
}

void OL_BlendChromaImage::DoBlendImage(Image444* base, Image444* overlay) {
  if (bits_per_pixel == 8)
    BlendImage<uint8_t>(base, overlay);
  //else if(bits_per_pixel == 32)
  //  BlendImage<float>(base, overlay);
  else if(bits_per_pixel == 16)
    BlendImage<uint16_t>(base, overlay);
}

template<typename pixel_t>
void OL_BlendLumaImage::BlendImageMask(Image444* base, Image444* overlay, Image444* mask) {
  BYTE* baseY = base->GetPtr(PLANAR_Y);

  BYTE* ovY = overlay->GetPtr(PLANAR_Y);
  
  BYTE* maskY = mask->GetPtr(PLANAR_Y);
  
  int w = base->w();
  int h = base->h();

  if (opacity == 256) {
    if (env->GetCPUFlags() & CPUF_SSE2) {
      overlay_blend_sse2_plane_masked(baseY, ovY, maskY, base->pitch, overlay->pitch, mask->pitch, w, h);
    } else
#ifdef X86_32
    if (env->GetCPUFlags() & CPUF_MMX) {
      overlay_blend_mmx_plane_masked(baseY, ovY, maskY, base->pitch, overlay->pitch, mask->pitch, w, h);
      _mm_empty();
    } else
#endif
    {
      overlay_blend_c_plane_masked(baseY, ovY, maskY, base->pitch, overlay->pitch, mask->pitch, w, h);
    }
  } else {
    if (env->GetCPUFlags() & CPUF_SSE2) {
      overlay_blend_sse2_plane_masked_opacity(baseY, ovY, maskY, base->pitch, overlay->pitch, mask->pitch, w, h, opacity);
    } else
#ifdef X86_32
    if (env->GetCPUFlags() & CPUF_MMX) {
      overlay_blend_mmx_plane_masked_opacity(baseY, ovY, maskY, base->pitch, overlay->pitch, mask->pitch, w, h, opacity);
      _mm_empty();
    } else
#endif
    {
      overlay_blend_c_plane_masked_opacity(baseY, ovY, maskY, base->pitch, overlay->pitch, mask->pitch, w, h, opacity);
    }
  }
}


template<typename pixel_t>
void OL_BlendLumaImage::BlendImage(Image444* base, Image444* overlay) {
  BYTE* baseY = base->GetPtr(PLANAR_Y);

  BYTE* ovY = overlay->GetPtr(PLANAR_Y);

  int w = base->w();
  int h = base->h();

  if (opacity == 256) {
    env->BitBlt(baseY, base->pitch, ovY, overlay->pitch, w, h);
  } else {
    if (env->GetCPUFlags() & CPUF_SSE2) {
      overlay_blend_sse2_plane_opacity(baseY, ovY, base->pitch, overlay->pitch, w, h, opacity);
    } else
#ifdef X86_32
    if (env->GetCPUFlags() & CPUF_MMX) {
      overlay_blend_mmx_plane_opacity(baseY, ovY, base->pitch, overlay->pitch, w, h, opacity);
      _mm_empty();
    } else
#endif
    {
      overlay_blend_c_plane_opacity(baseY, ovY, base->pitch, overlay->pitch, w, h, opacity);
    }
  }
}



template<typename pixel_t>
void OL_BlendChromaImage::BlendImageMask(Image444* base, Image444* overlay, Image444* mask) {
  BYTE* baseU = base->GetPtr(PLANAR_U);
  BYTE* baseV = base->GetPtr(PLANAR_V);

  BYTE* ovU = overlay->GetPtr(PLANAR_U);
  BYTE* ovV = overlay->GetPtr(PLANAR_V);
  
  BYTE* maskU = mask->GetPtr(PLANAR_U);
  BYTE* maskV = mask->GetPtr(PLANAR_V);

  int w = base->w();
  int h = base->h();

  if (opacity == 256) {
    if (env->GetCPUFlags() & CPUF_SSE2) {
      overlay_blend_sse2_plane_masked(baseU, ovU, maskU, base->pitch, overlay->pitch, mask->pitch, w, h);
      overlay_blend_sse2_plane_masked(baseV, ovV, maskV, base->pitch, overlay->pitch, mask->pitch, w, h);
    } else
#ifdef X86_32
    if (env->GetCPUFlags() & CPUF_MMX) {
      overlay_blend_mmx_plane_masked(baseU, ovU, maskU, base->pitch, overlay->pitch, mask->pitch, w, h);
      overlay_blend_mmx_plane_masked(baseV, ovV, maskV, base->pitch, overlay->pitch, mask->pitch, w, h);
      _mm_empty();
    } else
#endif
    {
      overlay_blend_c_plane_masked(baseU, ovU, maskU, base->pitch, overlay->pitch, mask->pitch, w, h);
      overlay_blend_c_plane_masked(baseV, ovV, maskV, base->pitch, overlay->pitch, mask->pitch, w, h);
    }
  } else {
    if (env->GetCPUFlags() & CPUF_SSE2) {
      overlay_blend_sse2_plane_masked_opacity(baseU, ovU, maskU, base->pitch, overlay->pitch, mask->pitch, w, h, opacity);
      overlay_blend_sse2_plane_masked_opacity(baseV, ovV, maskV, base->pitch, overlay->pitch, mask->pitch, w, h, opacity);
    } else
#ifdef X86_32
    if (env->GetCPUFlags() & CPUF_MMX) {
      overlay_blend_mmx_plane_masked_opacity(baseU, ovU, maskU, base->pitch, overlay->pitch, mask->pitch, w, h, opacity);
      overlay_blend_mmx_plane_masked_opacity(baseV, ovV, maskV, base->pitch, overlay->pitch, mask->pitch, w, h, opacity);
      _mm_empty();
    } else
#endif
    {
      overlay_blend_c_plane_masked_opacity(baseU, ovU, maskU, base->pitch, overlay->pitch, mask->pitch, w, h, opacity);
      overlay_blend_c_plane_masked_opacity(baseV, ovV, maskV, base->pitch, overlay->pitch, mask->pitch, w, h, opacity);
    }
  }
}

template<typename pixel_t>
void OL_BlendChromaImage::BlendImage(Image444* base, Image444* overlay) {
  BYTE* baseU = base->GetPtr(PLANAR_U);
  BYTE* baseV = base->GetPtr(PLANAR_V);

  BYTE* ovU = overlay->GetPtr(PLANAR_U);
  BYTE* ovV = overlay->GetPtr(PLANAR_V);
  
  int w = base->w();
  int h = base->h();

  if (opacity == 256) {
    env->BitBlt(baseU, base->pitch, ovU, overlay->pitch, w, h);
    env->BitBlt(baseV, base->pitch, ovV, overlay->pitch, w, h);
  } else {
    if (env->GetCPUFlags() & CPUF_SSE2) {
      overlay_blend_sse2_plane_opacity(baseU, ovU, base->pitch, overlay->pitch, w, h, opacity);
      overlay_blend_sse2_plane_opacity(baseV, ovV, base->pitch, overlay->pitch, w, h, opacity);
    } else
#ifdef X86_32
    if (env->GetCPUFlags() & CPUF_MMX) {
      overlay_blend_mmx_plane_opacity(baseU, ovU, base->pitch, overlay->pitch, w, h, opacity);
      overlay_blend_mmx_plane_opacity(baseV, ovV, base->pitch, overlay->pitch, w, h, opacity);
      _mm_empty();
    } else
#endif
    {
      overlay_blend_c_plane_opacity(baseU, ovU, base->pitch, overlay->pitch, w, h, opacity);
      overlay_blend_c_plane_opacity(baseV, ovV, base->pitch, overlay->pitch, w, h, opacity);
    }
  }
}

