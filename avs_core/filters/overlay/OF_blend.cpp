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

void OL_BlendImage::DoBlendImageMask(ImageOverlayInternal* base, ImageOverlayInternal* overlay, ImageOverlayInternal* mask) {
  if (bits_per_pixel == 8)
    BlendImageMask<uint8_t>(base, overlay, mask);
  else if(bits_per_pixel <= 16)
    BlendImageMask<uint16_t>(base, overlay, mask);
  else if(bits_per_pixel == 32)
    BlendImageMask<float>(base, overlay, mask);
}

void OL_BlendImage::DoBlendImage(ImageOverlayInternal* base, ImageOverlayInternal* overlay) {
  if (bits_per_pixel == 8)
    BlendImage<uint8_t>(base, overlay);
  else if(bits_per_pixel <= 16)
    BlendImage<uint16_t>(base, overlay);
  else if(bits_per_pixel == 32)
    BlendImage<float>(base, overlay);
}


template<typename pixel_t>
void OL_BlendImage::BlendImageMask(ImageOverlayInternal* base, ImageOverlayInternal* overlay, ImageOverlayInternal* mask) {
  int w = base->w();
  int h = base->h();

  const int pixelsize = sizeof(pixel_t);

  int planeindex_from, planeindex_to;

  if(of_mode == OF_Blend) {
    planeindex_from = 0;
    planeindex_to = greyscale ? 0 : 2;
  }
  else if (of_mode == OF_Luma) {
    planeindex_from = 0;
    planeindex_to = 0;
  }
  else if (of_mode == OF_Chroma) {
    if (greyscale)
      return;
    planeindex_from = 1;
    planeindex_to = 2;
  }

  if ((opacity == 256 && pixelsize != 4) || (opacity_f == 1.0f && pixelsize == 4)) {
    if (pixelsize == 4 && (env->GetCPUFlags() & CPUF_SSE2)) {
      for (int p = planeindex_from; p <= planeindex_to; p++) {
        overlay_blend_sse2_plane_masked<float, 8, false>(base->GetPtrByIndex(p), overlay->GetPtrByIndex(p), mask->GetPtrByIndex(p),
          base->GetPitchByIndex(p), overlay->GetPitchByIndex(p), mask->GetPitchByIndex(p),
          (w >> base->xSubSamplingShifts[p]), h >> base->ySubSamplingShifts[p]);
      }
    }
    else if (pixelsize == 2 && (env->GetCPUFlags() & CPUF_SSE4_1)) {
      for (int p = planeindex_from; p <= planeindex_to; p++) {
        switch (bits_per_pixel) {
        case 10:
          overlay_blend_sse2_plane_masked<uint16_t, 10, true>(base->GetPtrByIndex(p), overlay->GetPtrByIndex(p), mask->GetPtrByIndex(p),
            base->GetPitchByIndex(p), overlay->GetPitchByIndex(p), mask->GetPitchByIndex(p),
            (w >> base->xSubSamplingShifts[p]), h >> base->ySubSamplingShifts[p]);
          break;
        case 12:
          overlay_blend_sse2_plane_masked<uint16_t, 12, true>(base->GetPtrByIndex(p), overlay->GetPtrByIndex(p), mask->GetPtrByIndex(p),
            base->GetPitchByIndex(p), overlay->GetPitchByIndex(p), mask->GetPitchByIndex(p),
            (w >> base->xSubSamplingShifts[p]), h >> base->ySubSamplingShifts[p]);
          break;
        case 14:
          overlay_blend_sse2_plane_masked<uint16_t, 14, true>(base->GetPtrByIndex(p), overlay->GetPtrByIndex(p), mask->GetPtrByIndex(p),
            base->GetPitchByIndex(p), overlay->GetPitchByIndex(p), mask->GetPitchByIndex(p),
            (w >> base->xSubSamplingShifts[p]), h >> base->ySubSamplingShifts[p]);
          break;
        case 16:
          overlay_blend_sse2_plane_masked<uint16_t, 16, true>(base->GetPtrByIndex(p), overlay->GetPtrByIndex(p), mask->GetPtrByIndex(p),
            base->GetPitchByIndex(p), overlay->GetPitchByIndex(p), mask->GetPitchByIndex(p),
            (w >> base->xSubSamplingShifts[p]), h >> base->ySubSamplingShifts[p]);
          break;
        }
      }
    }
    else if (pixelsize == 1 && (env->GetCPUFlags() & CPUF_SSE4_1)) {
      for (int p = planeindex_from; p <= planeindex_to; p++) {
        overlay_blend_sse2_plane_masked<uint8_t, 8, true>(base->GetPtrByIndex(p), overlay->GetPtrByIndex(p), mask->GetPtrByIndex(p),
          base->GetPitchByIndex(p), overlay->GetPitchByIndex(p), mask->GetPitchByIndex(p),
          (w >> base->xSubSamplingShifts[p]), h >> base->ySubSamplingShifts[p]);
      }
    }
    else if (pixelsize == 1 && (env->GetCPUFlags() & CPUF_SSE2)) {
      for (int p = planeindex_from; p <= planeindex_to; p++) {
        overlay_blend_sse2_plane_masked<uint8_t, 8, false>(base->GetPtrByIndex(p), overlay->GetPtrByIndex(p), mask->GetPtrByIndex(p),
          base->GetPitchByIndex(p), overlay->GetPitchByIndex(p), mask->GetPitchByIndex(p),
          (w >> base->xSubSamplingShifts[p]), h >> base->ySubSamplingShifts[p]);
      }
    }
    else
#ifdef X86_32
      if (pixelsize == 1 && (env->GetCPUFlags() & CPUF_MMX)) {
        for (int p = planeindex_from; p <= planeindex_to; p++) {
          overlay_blend_mmx_plane_masked(base->GetPtrByIndex(p), overlay->GetPtrByIndex(p), mask->GetPtrByIndex(p),
            base->GetPitchByIndex(p), overlay->GetPitchByIndex(p), mask->GetPitchByIndex(p),
            (w >> base->xSubSamplingShifts[p]), h >> base->ySubSamplingShifts[p]);
        }
        _mm_empty();
      }
      else
#endif
      {
        for (int p = planeindex_from; p <= planeindex_to; p++) {
          switch (bits_per_pixel) {
          case 8:
            overlay_blend_c_plane_masked<uint8_t, 8>(base->GetPtrByIndex(p), overlay->GetPtrByIndex(p), mask->GetPtrByIndex(p),
              base->GetPitchByIndex(p), overlay->GetPitchByIndex(p), mask->GetPitchByIndex(p),
              (w >> base->xSubSamplingShifts[p]), h >> base->ySubSamplingShifts[p]);
            break;
          case 10:
            overlay_blend_c_plane_masked<uint16_t, 10>(base->GetPtrByIndex(p), overlay->GetPtrByIndex(p), mask->GetPtrByIndex(p),
              base->GetPitchByIndex(p), overlay->GetPitchByIndex(p), mask->GetPitchByIndex(p),
              (w >> base->xSubSamplingShifts[p]), h >> base->ySubSamplingShifts[p]);
            break;
          case 12:
            overlay_blend_c_plane_masked<uint16_t, 12>(base->GetPtrByIndex(p), overlay->GetPtrByIndex(p), mask->GetPtrByIndex(p),
              base->GetPitchByIndex(p), overlay->GetPitchByIndex(p), mask->GetPitchByIndex(p),
              (w >> base->xSubSamplingShifts[p]), h >> base->ySubSamplingShifts[p]);
            break;
          case 14:
            overlay_blend_c_plane_masked<uint16_t, 14>(base->GetPtrByIndex(p), overlay->GetPtrByIndex(p), mask->GetPtrByIndex(p),
              base->GetPitchByIndex(p), overlay->GetPitchByIndex(p), mask->GetPitchByIndex(p),
              (w >> base->xSubSamplingShifts[p]), h >> base->ySubSamplingShifts[p]);
            break;
          case 16:
            overlay_blend_c_plane_masked<uint16_t, 16>(base->GetPtrByIndex(p), overlay->GetPtrByIndex(p), mask->GetPtrByIndex(p),
              base->GetPitchByIndex(p), overlay->GetPitchByIndex(p), mask->GetPitchByIndex(p),
              (w >> base->xSubSamplingShifts[p]), h >> base->ySubSamplingShifts[p]);
            break;
          case 32:
            overlay_blend_c_plane_masked_f(base->GetPtrByIndex(p), overlay->GetPtrByIndex(p), mask->GetPtrByIndex(p),
              base->GetPitchByIndex(p), overlay->GetPitchByIndex(p), mask->GetPitchByIndex(p),
              (w >> base->xSubSamplingShifts[p]), h >> base->ySubSamplingShifts[p]);
            break;
          }
        }
      }
  }
  else {
    if (pixelsize == 4 && (env->GetCPUFlags() & CPUF_SSE2)) {
      for (int p = planeindex_from; p <= planeindex_to; p++) {
        overlay_blend_sse2_plane_masked_opacity<float, 8, false>(base->GetPtrByIndex(p), overlay->GetPtrByIndex(p), mask->GetPtrByIndex(p),
          base->GetPitchByIndex(p), overlay->GetPitchByIndex(p), mask->GetPitchByIndex(p),
          (w >> base->xSubSamplingShifts[p]), h >> base->ySubSamplingShifts[p], opacity, opacity_f);
          break;
      }
    }
    else if (pixelsize == 2 && (env->GetCPUFlags() & CPUF_SSE4_1)) {
      for (int p = planeindex_from; p <= planeindex_to; p++) {
        switch (bits_per_pixel)
        {
        case 10: overlay_blend_sse2_plane_masked_opacity<uint16_t, 10, true>(base->GetPtrByIndex(p), overlay->GetPtrByIndex(p), mask->GetPtrByIndex(p),
          base->GetPitchByIndex(p), overlay->GetPitchByIndex(p), mask->GetPitchByIndex(p),
          (w >> base->xSubSamplingShifts[p]), h >> base->ySubSamplingShifts[p], opacity, opacity_f);
          break;
        case 12: overlay_blend_sse2_plane_masked_opacity<uint16_t, 12, true>(base->GetPtrByIndex(p), overlay->GetPtrByIndex(p), mask->GetPtrByIndex(p),
          base->GetPitchByIndex(p), overlay->GetPitchByIndex(p), mask->GetPitchByIndex(p),
          (w >> base->xSubSamplingShifts[p]), h >> base->ySubSamplingShifts[p], opacity, opacity_f);
          break;
        case 14: overlay_blend_sse2_plane_masked_opacity<uint16_t, 14, true>(base->GetPtrByIndex(p), overlay->GetPtrByIndex(p), mask->GetPtrByIndex(p),
          base->GetPitchByIndex(p), overlay->GetPitchByIndex(p), mask->GetPitchByIndex(p),
          (w >> base->xSubSamplingShifts[p]), h >> base->ySubSamplingShifts[p], opacity, opacity_f);
          break;
        case 16: overlay_blend_sse2_plane_masked_opacity<uint16_t, 16, true>(base->GetPtrByIndex(p), overlay->GetPtrByIndex(p), mask->GetPtrByIndex(p),
          base->GetPitchByIndex(p), overlay->GetPitchByIndex(p), mask->GetPitchByIndex(p),
          (w >> base->xSubSamplingShifts[p]), h >> base->ySubSamplingShifts[p], opacity, opacity_f);
          break;

        }
      }
    }
    else if (pixelsize == 1 && (env->GetCPUFlags() & CPUF_SSE4_1)) {
      for (int p = planeindex_from; p <= planeindex_to; p++) {
        overlay_blend_sse2_plane_masked_opacity<uint8_t, 8, true>(base->GetPtrByIndex(p), overlay->GetPtrByIndex(p), mask->GetPtrByIndex(p),
          base->GetPitchByIndex(p), overlay->GetPitchByIndex(p), mask->GetPitchByIndex(p),
          (w >> base->xSubSamplingShifts[p]), h >> base->ySubSamplingShifts[p], opacity, opacity_f);
      }
    }
    else if (pixelsize==1 && (env->GetCPUFlags() & CPUF_SSE2)) {
      for (int p = planeindex_from; p <= planeindex_to; p++) {
        overlay_blend_sse2_plane_masked_opacity<uint8_t,8, false>(base->GetPtrByIndex(p), overlay->GetPtrByIndex(p), mask->GetPtrByIndex(p),
          base->GetPitchByIndex(p), overlay->GetPitchByIndex(p), mask->GetPitchByIndex(p),
          (w >> base->xSubSamplingShifts[p]), h >> base->ySubSamplingShifts[p], opacity, opacity_f);
      }
    } else
#ifdef X86_32
    if (pixelsize==1 && (env->GetCPUFlags() & CPUF_MMX)) {
      for (int p = planeindex_from; p <= planeindex_to; p++) {
        overlay_blend_mmx_plane_masked_opacity(base->GetPtrByIndex(p), overlay->GetPtrByIndex(p), mask->GetPtrByIndex(p),
          base->GetPitchByIndex(p), overlay->GetPitchByIndex(p), mask->GetPitchByIndex(p),
          (w >> base->xSubSamplingShifts[p]), h >> base->ySubSamplingShifts[p], opacity);
      }
      _mm_empty();
    } else
#endif
    {
      for (int p = planeindex_from; p <= planeindex_to; p++) {
        switch (bits_per_pixel) {
        case 8:
          overlay_blend_c_plane_masked_opacity<uint8_t, 8>(base->GetPtrByIndex(p), overlay->GetPtrByIndex(p), mask->GetPtrByIndex(p),
            base->GetPitchByIndex(p), overlay->GetPitchByIndex(p), mask->GetPitchByIndex(p),
            (w >> base->xSubSamplingShifts[p]), h >> base->ySubSamplingShifts[p], opacity);
          break;
        case 10:
          overlay_blend_c_plane_masked_opacity<uint16_t,10>(base->GetPtrByIndex(p), overlay->GetPtrByIndex(p), mask->GetPtrByIndex(p),
            base->GetPitchByIndex(p), overlay->GetPitchByIndex(p), mask->GetPitchByIndex(p),
            (w >> base->xSubSamplingShifts[p]), h >> base->ySubSamplingShifts[p], opacity);
          break;
        case 12:
          overlay_blend_c_plane_masked_opacity<uint16_t,12>(base->GetPtrByIndex(p), overlay->GetPtrByIndex(p), mask->GetPtrByIndex(p),
            base->GetPitchByIndex(p), overlay->GetPitchByIndex(p), mask->GetPitchByIndex(p),
            (w >> base->xSubSamplingShifts[p]), h >> base->ySubSamplingShifts[p], opacity);
          break;
        case 14:
          overlay_blend_c_plane_masked_opacity<uint16_t,14>(base->GetPtrByIndex(p), overlay->GetPtrByIndex(p), mask->GetPtrByIndex(p),
            base->GetPitchByIndex(p), overlay->GetPitchByIndex(p), mask->GetPitchByIndex(p),
            (w >> base->xSubSamplingShifts[p]), h >> base->ySubSamplingShifts[p], opacity);
          break;
        case 16:
          overlay_blend_c_plane_masked_opacity<uint16_t,16>(base->GetPtrByIndex(p), overlay->GetPtrByIndex(p), mask->GetPtrByIndex(p),
            base->GetPitchByIndex(p), overlay->GetPitchByIndex(p), mask->GetPitchByIndex(p),
            (w >> base->xSubSamplingShifts[p]), h >> base->ySubSamplingShifts[p], opacity);
          break;
        case 32:
          overlay_blend_c_plane_masked_opacity_f(base->GetPtrByIndex(p), overlay->GetPtrByIndex(p), mask->GetPtrByIndex(p),
            base->GetPitchByIndex(p), overlay->GetPitchByIndex(p), mask->GetPitchByIndex(p),
            (w >> base->xSubSamplingShifts[p]), h >> base->ySubSamplingShifts[p], opacity_f);
          break;
        }
      }
    }
  }
}

template<typename pixel_t>
void OL_BlendImage::BlendImage(ImageOverlayInternal* base, ImageOverlayInternal* overlay) {
  int w = base->w();
  int h = base->h();

  const int pixelsize = sizeof(pixel_t);

  int planeindex_from, planeindex_to;

  if(of_mode == OF_Blend) {
    planeindex_from = 0;
    planeindex_to = greyscale ? 0 : 2;
  }
  else if (of_mode == OF_Luma) {
    planeindex_from = 0;
    planeindex_to = 0;
  }
  else if (of_mode == OF_Chroma) {
    if (greyscale)
      return;
    planeindex_from = 1;
    planeindex_to = 2;
  }

  if (opacity == 256) {
    for (int p = planeindex_from; p <= planeindex_to; p++) {
      env->BitBlt(base->GetPtrByIndex(p), base->GetPitchByIndex(p), overlay->GetPtrByIndex(p), overlay->GetPitchByIndex(p), (w >> base->xSubSamplingShifts[p])*pixelsize, h >> base->ySubSamplingShifts[p]);
    }
  } else {
    if (pixelsize == 4 && (env->GetCPUFlags() & CPUF_SSE2)) {
      for (int p = planeindex_from; p <= planeindex_to; p++) {
        overlay_blend_sse2_plane_opacity<float, 8>(base->GetPtrByIndex(p), overlay->GetPtrByIndex(p), base->GetPitchByIndex(p), overlay->GetPitchByIndex(p), (w >> base->xSubSamplingShifts[p]), h >> base->ySubSamplingShifts[p], opacity, opacity_f);
      }
    }
    else if (pixelsize == 2 && (env->GetCPUFlags() & CPUF_SSE4_1)) {
      for (int p = planeindex_from; p <= planeindex_to; p++) {
        switch (bits_per_pixel) {
        case 10:
          overlay_blend_sse2_plane_opacity<uint16_t, 10>(base->GetPtrByIndex(p), overlay->GetPtrByIndex(p), base->GetPitchByIndex(p), overlay->GetPitchByIndex(p), (w >> base->xSubSamplingShifts[p]), h >> base->ySubSamplingShifts[p], opacity, opacity_f);
          break;
        case 12:
          overlay_blend_sse2_plane_opacity<uint16_t, 12>(base->GetPtrByIndex(p), overlay->GetPtrByIndex(p), base->GetPitchByIndex(p), overlay->GetPitchByIndex(p), (w >> base->xSubSamplingShifts[p]), h >> base->ySubSamplingShifts[p], opacity, opacity_f);
          break;
        case 14:
          overlay_blend_sse2_plane_opacity<uint16_t, 14>(base->GetPtrByIndex(p), overlay->GetPtrByIndex(p), base->GetPitchByIndex(p), overlay->GetPitchByIndex(p), (w >> base->xSubSamplingShifts[p]), h >> base->ySubSamplingShifts[p], opacity, opacity_f);
          break;
        case 16:
          overlay_blend_sse2_plane_opacity<uint16_t, 16>(base->GetPtrByIndex(p), overlay->GetPtrByIndex(p), base->GetPitchByIndex(p), overlay->GetPitchByIndex(p), (w >> base->xSubSamplingShifts[p]), h >> base->ySubSamplingShifts[p], opacity, opacity_f);
          break;
        }
      }
    }
    else if (pixelsize == 1 && (env->GetCPUFlags() & CPUF_SSE2)) {
      for (int p = planeindex_from; p <= planeindex_to; p++) {
        overlay_blend_sse2_plane_opacity<uint8_t, 8>(base->GetPtrByIndex(p), overlay->GetPtrByIndex(p), base->GetPitchByIndex(p), overlay->GetPitchByIndex(p), (w >> base->xSubSamplingShifts[p]), h >> base->ySubSamplingShifts[p], opacity, opacity_f);
      }
    }
    else
#ifdef X86_32
    if (pixelsize == 1 && (env->GetCPUFlags() & CPUF_MMX)) {
      for (int p = planeindex_from; p <= planeindex_to; p++) {
        overlay_blend_mmx_plane_opacity(base->GetPtrByIndex(p), overlay->GetPtrByIndex(p), base->GetPitchByIndex(p), overlay->GetPitchByIndex(p), (w >> base->xSubSamplingShifts[p]), h >> base->ySubSamplingShifts[p], opacity);
      }
      _mm_empty();
    } else
#endif
    {
      for (int p = planeindex_from; p <= planeindex_to; p++) {
        switch (bits_per_pixel) {
        case 8:
          overlay_blend_c_plane_opacity<uint8_t, 8>(base->GetPtrByIndex(p), overlay->GetPtrByIndex(p), base->GetPitchByIndex(p), overlay->GetPitchByIndex(p), (w >> base->xSubSamplingShifts[p]), h >> base->ySubSamplingShifts[p], opacity);
          break;
        case 10:
          overlay_blend_c_plane_opacity<uint16_t, 10>(base->GetPtrByIndex(p), overlay->GetPtrByIndex(p), base->GetPitchByIndex(p), overlay->GetPitchByIndex(p), (w >> base->xSubSamplingShifts[p]), h >> base->ySubSamplingShifts[p], opacity);
          break;
        case 12:
          overlay_blend_c_plane_opacity<uint16_t, 12>(base->GetPtrByIndex(p), overlay->GetPtrByIndex(p), base->GetPitchByIndex(p), overlay->GetPitchByIndex(p), (w >> base->xSubSamplingShifts[p]), h >> base->ySubSamplingShifts[p], opacity);
          break;
        case 14:
          overlay_blend_c_plane_opacity<uint16_t, 14>(base->GetPtrByIndex(p), overlay->GetPtrByIndex(p), base->GetPitchByIndex(p), overlay->GetPitchByIndex(p), (w >> base->xSubSamplingShifts[p]), h >> base->ySubSamplingShifts[p], opacity);
          break;
        case 16:
          overlay_blend_c_plane_opacity<uint16_t, 16>(base->GetPtrByIndex(p), overlay->GetPtrByIndex(p), base->GetPitchByIndex(p), overlay->GetPitchByIndex(p), (w >> base->xSubSamplingShifts[p]), h >> base->ySubSamplingShifts[p], opacity);
          break;
        case 32:
          overlay_blend_c_plane_opacity_f(base->GetPtrByIndex(p), overlay->GetPtrByIndex(p), base->GetPitchByIndex(p), overlay->GetPitchByIndex(p), (w >> base->xSubSamplingShifts[p]), h >> base->ySubSamplingShifts[p], opacity_f);
          break;
        }
      }
    }
  }
}
