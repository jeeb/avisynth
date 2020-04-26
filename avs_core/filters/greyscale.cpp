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


#include "greyscale.h"
#include "../core/internal.h"
#include <avs/alignment.h>
#include <avs/minmax.h>

#ifdef AVS_WINDOWS
    #include <avs/win.h>
#else
    #include <avs/posix.h>
#endif

#include <stdint.h>
#include "../convert/convert_planar.h"


/*************************************
 *******   Convert to Greyscale ******
 ************************************/

extern const AVSFunction Greyscale_filters[] = {
  { "Greyscale", BUILTIN_FUNC_PREFIX, "c[matrix]s", Greyscale::Create },       // matrix can be "rec601", "rec709" or "Average" or "rec2020"
  { "Grayscale", BUILTIN_FUNC_PREFIX, "c[matrix]s", Greyscale::Create },
  { 0 }
};

Greyscale::Greyscale(PClip _child, const char* matrix, IScriptEnvironment* env)
 : GenericVideoFilter(_child)
{
  matrix_ = Rec601;
  if (matrix) {
    if (!vi.IsRGB())
      env->ThrowError("GreyScale: invalid \"matrix\" parameter (RGB data only)");
    if (!lstrcmpi(matrix, "rec709"))
      matrix_ = Rec709;
    else if (!lstrcmpi(matrix, "Average"))
      matrix_ = Average;
    else if (!lstrcmpi(matrix, "rec601"))
      matrix_ = Rec601;
    else if (!lstrcmpi(matrix, "rec2020"))
      matrix_ = Rec2020;
    else
      env->ThrowError("GreyScale: invalid \"matrix\" parameter (must be matrix=\"Rec601\", \"Rec709\", \"Rec2020\" or \"Average\")");
  }
  BuildGreyMatrix();
  pixelsize = vi.ComponentSize();
  bits_per_pixel = vi.BitsPerComponent();
}

template<typename pixel_t, int pixel_step>
static void greyscale_packed_rgb_c(BYTE *srcp8, int src_pitch, int width, int height, int cyb, int cyg, int cyr) {
  pixel_t *srcp = reinterpret_cast<pixel_t *>(srcp8);
  src_pitch /= sizeof(pixel_t);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      srcp[x*pixel_step+0] = srcp[x*pixel_step+1] = srcp[x*pixel_step+2] =
        (cyb*srcp[x*pixel_step+0] + cyg*srcp[x*pixel_step+1] + cyr*srcp[x*pixel_step+2] + 16384) >> 15;
    }
    srcp += src_pitch;
  }
}

template<typename pixel_t>
static void greyscale_planar_rgb_c(BYTE *srcp_r8, BYTE *srcp_g8, BYTE *srcp_b8, int src_pitch, int width, int height, int cyb, int cyg, int cyr) {
  pixel_t *srcp_r = reinterpret_cast<pixel_t *>(srcp_r8);
  pixel_t *srcp_g = reinterpret_cast<pixel_t *>(srcp_g8);
  pixel_t *srcp_b = reinterpret_cast<pixel_t *>(srcp_b8);
  src_pitch /= sizeof(pixel_t);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      srcp_b[x] = srcp_g[x] = srcp_r[x] =
        ((cyb*srcp_b[x] + cyg*srcp_g[x] + cyr*srcp_r[x] + 16384) >> 15);
    }
    srcp_r += src_pitch;
    srcp_g += src_pitch;
    srcp_b += src_pitch;
  }
}

static void greyscale_planar_rgb_float_c(BYTE *srcp_r8, BYTE *srcp_g8, BYTE *srcp_b8, int src_pitch, int width, int height, float cyb, float cyg, float cyr) {
  float *srcp_r = reinterpret_cast<float *>(srcp_r8);
  float *srcp_g = reinterpret_cast<float *>(srcp_g8);
  float *srcp_b = reinterpret_cast<float *>(srcp_b8);
  src_pitch /= sizeof(float);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      srcp_b[x] = srcp_g[x] = srcp_r[x] = cyb*srcp_b[x] + cyg*srcp_g[x] + cyr*srcp_r[x];
    }
    srcp_r += src_pitch;
    srcp_g += src_pitch;
    srcp_b += src_pitch;
  }
}

void Greyscale::BuildGreyMatrix() {
#if 0
  // not used, kept for sample
  // 16 bit scaled
  const int cyavb_sc16 = 21845;   // const int cyav = int(0.333333*65536+0.5);
  const int cyavg_sc16 = 21845;
  const int cyavr_sc16 = 21845;
  //  21845 sum: 65535 <= 65536 OK

  //  const int cyb = int(0.114*65536+0.5); //  7471
  //  const int cyg = int(0.587*65536+0.5); // 38470
  //  const int cyr = int(0.299*65536+0.5); // 19595

  const int cyb601_sc16 = 7471;  // int(0.114*65536+0.5); //  7471
  const int cyg601_sc16 = 38470; // int(0.587*65536+0.5); // 38470
  const int cyr601_sc16 = 19595; // int(0.299*65536+0.5); // 19595
                                 // sum: 65536 OK


  const int cyb709_sc16 = 4732; // int(0.0722 * 65536 + 0.5); //  4732
  const int cyg709_sc16 = 46871; // int(0.7152 * 65536 + 0.5); // 46871
  const int cyr709_sc16 = 13933; // int(0.2126 * 65536 + 0.5); // 13933
                                 //  Sum: 65536 OK
                                 // This is the correct brigtness calculations (standardized in Rec. 709)
#endif
  // 15 bit scaled
                                 // PF check: int32 overflow in 16 bits
                                 // 32769 * 65535 + 16384 = 8000BFFF int32 overflow
                                 // 32768 * 65535 + 16384 = 7FFFC000 OK
                                 // Let's make correction
                                 // --- Average
  const int cybav_sc15 = 10923;   // int(0.33333 * 32768 + 0.5); // 10923
  const int cygav_sc15 = 10923-1; // int(0.33333 * 32768 + 0.5); // 10923
  const int cyrav_sc15 = 10923;   // int(0.33333 * 32768 + 0.5); // 10923
                                  // w/o correction 3*10923 = 32769!
  const float cybav_f = 0.333333f;
  const float cygav_f = 0.333333f;
  const float cyrav_f = 0.333333f;

  // --- Rec601
  const int cyb601_sc15 = 3736;  // int(0.114 * 32768 + 0.5); // 3736
  const int cyg601_sc15 = 19235-1; // int(0.587 * 32768 + 0.5); // 19235
  const int cyr601_sc15 = 9798;  // int(0.299 * 32768 + 0.5); // 9798
                                 // w/o correction: 32769

  const float cyb601_f = 0.114f;
  const float cyg601_f = 0.587f;
  const float cyr601_f = 0.299f;

  // --- Rec709
  const int cyb709_sc15 = 2366;  // int(0.0722 * 32768 + 0.5); // 2366
  const int cyg709_sc15 = 23436; // int(0.7152 * 32768 + 0.5); // 23436
  const int cyr709_sc15 = 6966;  // int(0.2126 * 32768 + 0.5); // 6966
                                 // sum: 32768 OK
  const float cyb709_f = 0.0722f;
  const float cyg709_f = 0.7152f;
  const float cyr709_f = 0.2126f;

  // --- Rec2020
  const int cyb2020_sc15 = 1943;  // int(0.0593 * 32768 + 0.5); // 1943
  const int cyg2020_sc15 = 22217; // int(0.6780 * 32768 + 0.5); // 22217
  const int cyr2020_sc15 = 8608;  // int(0.2627 * 32768 + 0.5); // 8608
                                 // sum: 32768 OK
  const float cyb2020_f = 0.0593f;
  const float cyg2020_f = 0.6780f;
  const float cyr2020_f = 0.2627f;


  if(matrix_ == Rec709) {
    greyMatrix.b   = cyb709_sc15; greyMatrix.g   = cyg709_sc15; greyMatrix.r   = cyr709_sc15;
    greyMatrix.b_f = cyb709_f;    greyMatrix.g_f = cyg709_f;    greyMatrix.r_f = cyr709_f;
  } else if(matrix_ == Rec2020) {
    greyMatrix.b   = cyb2020_sc15; greyMatrix.g   = cyg2020_sc15; greyMatrix.r   = cyr2020_sc15;
    greyMatrix.b_f = cyb2020_f;    greyMatrix.g_f = cyg2020_f;    greyMatrix.r_f = cyr2020_f;
  } else if (matrix_ == Average) {
    greyMatrix.b   = cybav_sc15;  greyMatrix.g   = cygav_sc15;  greyMatrix.r   = cyrav_sc15;
    greyMatrix.b_f = cybav_f;     greyMatrix.g_f = cygav_f;     greyMatrix.r_f = cyrav_f;
  } else if (matrix_ == Rec601) {
    greyMatrix.b   = cyb601_sc15; greyMatrix.g   = cyg601_sc15; greyMatrix.r   = cyr601_sc15;
    greyMatrix.b_f = cyb601_f;    greyMatrix.g_f = cyg601_f;    greyMatrix.r_f = cyr601_f;
  } else {
    // n/a not valid matrix, checked earlier
  }

}

PVideoFrame Greyscale::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame frame = child->GetFrame(n, env);
  if (vi.NumComponents() == 1)
    return frame;

  env->MakeWritable(&frame);
  BYTE* srcp = frame->GetWritePtr();
  int pitch = frame->GetPitch();
  int height = vi.height;
  int width = vi.width;

  if (vi.IsPlanar() && (vi.IsYUV() || vi.IsYUVA())) {
    // planar YUV, set UV plane to neutral
    BYTE* dstp_u = frame->GetWritePtr(PLANAR_U);
    BYTE* dstp_v = frame->GetWritePtr(PLANAR_V);
    const int height = frame->GetHeight(PLANAR_U);
    const int dst_pitch = frame->GetPitch(PLANAR_U);
    switch (vi.ComponentSize())
    {
    case 1:
      fill_chroma<BYTE>(dstp_u, dstp_v, height, dst_pitch, 0x80); // in convert_planar
      break;
    case 2:
      fill_chroma<uint16_t>(dstp_u, dstp_v, height, dst_pitch, 1 << (vi.BitsPerComponent() - 1));
      break;
    case 4:
#ifdef FLOAT_CHROMA_IS_HALF_CENTERED
      const float shift = 0.5f;
#else
      const float shift = 0.0f;
#endif
      fill_chroma<float>(dstp_u, dstp_v, height, dst_pitch, shift);
      break;
    }
    return frame;
  }

  if (vi.IsYUY2()) {
      {
        for (int y = 0; y<height; ++y)
        {
          for (int x = 0; x<width; x++)
            srcp[x*2+1] = 128;
          srcp += pitch;
        }
      }

      return frame;
  }

  if (vi.IsRGB())
  {  // RGB C.
    if (vi.IsPlanarRGB() || vi.IsPlanarRGBA())
    {
      BYTE* srcp_g = frame->GetWritePtr(PLANAR_G);
      BYTE* srcp_b = frame->GetWritePtr(PLANAR_B);
      BYTE* srcp_r = frame->GetWritePtr(PLANAR_R);

      const int src_pitch = frame->GetPitch(); // same for all planes

      if (pixelsize == 1)
        greyscale_planar_rgb_c<uint8_t>(srcp_r, srcp_g, srcp_b, src_pitch, vi.width, vi.height, greyMatrix.b, greyMatrix.g, greyMatrix.r);
      else if (pixelsize == 2)
        greyscale_planar_rgb_c<uint16_t>(srcp_r, srcp_g, srcp_b, src_pitch, vi.width, vi.height, greyMatrix.b, greyMatrix.g, greyMatrix.r);
      else
        greyscale_planar_rgb_float_c(srcp_r, srcp_g, srcp_b, src_pitch, vi.width, vi.height, greyMatrix.b_f, greyMatrix.g_f, greyMatrix.r_f);

      return frame;
    }
    // packed RGB

    const int rgb_inc = vi.IsRGB32() || vi.IsRGB64() ? 4 : 3;

    if (pixelsize == 1) { // rgb24/32
      if (rgb_inc == 3)
        greyscale_packed_rgb_c<uint8_t, 3>(srcp, pitch, vi.width, vi.height, greyMatrix.b, greyMatrix.g, greyMatrix.r);
      else
        greyscale_packed_rgb_c<uint8_t, 4>(srcp, pitch, vi.width, vi.height, greyMatrix.b, greyMatrix.g, greyMatrix.r);
    }
    else { // rgb48/64
      if (rgb_inc == 3)
        greyscale_packed_rgb_c<uint16_t, 3>(srcp, pitch, vi.width, vi.height, greyMatrix.b, greyMatrix.g, greyMatrix.r);
      else
        greyscale_packed_rgb_c<uint16_t, 4>(srcp, pitch, vi.width, vi.height, greyMatrix.b, greyMatrix.g, greyMatrix.r);
    }

#if 0
    BYTE* p_count = srcp;
    if (matrix_ == Rec709) {

      for (int y = 0; y<vi.height; ++y) {
        for (int x = 0; x<vi.width; x++) {
          int greyscale = ((srcp[0]*4732)+(srcp[1]*46871)+(srcp[2]*13933)+32768)>>16; // This is the correct brigtness calculations (standardized in Rec. 709)
          srcp[0] = srcp[1] = srcp[2] = greyscale;
          srcp += rgb_inc;
        }
        p_count += pitch;
        srcp = p_count;
      }
    } else if (matrix_ == Average) {
      //	  const int cyav = int(0.333333*65536+0.5); //  21845 sum: 65535 <= 65536 OK

      for (int y = 0; y<vi.height; ++y) {
        for (int x = 0; x<vi.width; x++) {
          int greyscale = ((srcp[0]+srcp[1]+srcp[2])*21845+32768)>>16; // This is the average of R, G & B
          srcp[0] = srcp[1] = srcp[2] = greyscale;
          srcp += rgb_inc;
        }
        p_count += pitch;
        srcp = p_count;
      }
    } else {
      //  const int cyb = int(0.114*65536+0.5); //  7471
      //  const int cyg = int(0.587*65536+0.5); // 38470
      //  const int cyr = int(0.299*65536+0.5); // 19595
      // Sum: 65536
      for (int y = 0; y<vi.height; ++y) {
        for (int x = 0; x<vi.width; x++) {
          int greyscale = ((srcp[0]*7471)+(srcp[1]*38470)+(srcp[2]*19595)+32768)>>16; // This produces similar results as YUY2 (luma calculation)
          srcp[0] = srcp[1] = srcp[2] = greyscale;
          srcp += rgb_inc;
        }
        p_count += pitch;
        srcp = p_count;
      }
    }
#endif
  }
  return frame;
}


AVSValue __cdecl Greyscale::Create(AVSValue args, void*, IScriptEnvironment* env)
{
  PClip clip = args[0].AsClip();
  const VideoInfo& vi = clip->GetVideoInfo();

  if (vi.NumComponents() == 1)
    return clip;

  return new Greyscale(clip, args[1].AsString(0), env);
}
