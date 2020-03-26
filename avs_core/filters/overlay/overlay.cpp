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
#include <avisynth.h>
#ifdef AVS_WINDOWS
    #include <avs/win.h>
#else
    #include <avs/posix.h>
#endif

#include <stdlib.h>
#include "overlay.h"
#include <string>
#include "../core/internal.h"

/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

extern const AVSFunction Overlay_filters[] = {
  { "Overlay", BUILTIN_FUNC_PREFIX, "cc[x]i[y]i[mask]c[opacity]f[mode]s[greymask]b[output]s[ignore_conditional]b[PC_Range]b[use444]b[condvarsuffix]s", Overlay::Create },
    // 0, src clip
    // 1, overlay clip
    // 2, x
    // 3, y
    // 4, mask clip
    // 5, overlay opacity.(0.0->1.0)
    // 6, mode string, "blend", "add"
    // 7, greymask bool - true = only use luma information for mask
    // 8, output type, string
    // 9, ignore conditional variabels
    // 10, full YUV range.
    // 11, ignore 4:4:4 conversion
    // 12, conditional variable suffix AVS+
  { 0 }
};

enum {
  ARG_SRC = 0,
  ARG_OVERLAY = 1,
  ARG_X = 2,
  ARG_Y = 3,
  ARG_MASK = 4,
  ARG_OPACITY = 5,
  ARG_MODE = 6,
  ARG_GREYMASK = 7,
  ARG_OUTPUT = 8,
  ARG_IGNORE_CONDITIONAL = 9,
  ARG_FULL_RANGE = 10,
  ARG_USE444 = 11, // 170103 possible conversionless option experimental
  ARG_CONDVARSUFFIX = 12 // 190408
};

Overlay::Overlay(PClip _child, AVSValue args, IScriptEnvironment *env) :
GenericVideoFilter(_child) {

  full_range = args[ARG_FULL_RANGE].AsBool(false);  // Maintain CCIR601 range when converting to/from RGB.
  bool use444_defined = args[ARG_USE444].Defined();
  use444 = args[ARG_USE444].AsBool(true);  // avs+ option to use 444-conversionless mode
  name = args[ARG_MODE].AsString("Blend");
  condVarSuffix = args[ARG_CONDVARSUFFIX].AsString("");

  // Make copy of the VideoInfo
  inputVi = (VideoInfo*)malloc(sizeof(VideoInfo));
  memcpy(inputVi, &vi, sizeof(VideoInfo));
  outputVi = (VideoInfo*)malloc(sizeof(VideoInfo));
  memcpy(outputVi, &vi, sizeof(VideoInfo));
  viInternalWorkingFormat = (VideoInfo*)malloc(sizeof(VideoInfo));
  memcpy(viInternalWorkingFormat, &vi, sizeof(VideoInfo));

  viInternalOverlayWorkingFormat = (VideoInfo*)malloc(sizeof(VideoInfo));
  // fill later

  mask = 0;
  opacity_f = (float)args[ARG_OPACITY].AsDblDef(1.0); // for float support
  opacity = (int)(256.0*opacity_f + 0.5); // range is converted to 256 for all all bit_depth
  offset_x = args[ARG_X].AsInt(0);
  offset_y = args[ARG_Y].AsInt(0);

  if (!args[ARG_OVERLAY].IsClip())
    env->ThrowError("Overlay: Overlay parameter is not a clip");

  overlay = args[ARG_OVERLAY].AsClip();

  overlayVi = overlay->GetVideoInfo();
  *viInternalOverlayWorkingFormat = overlayVi;

  greymask = args[ARG_GREYMASK].AsBool(true);  // Grey mask, default true
  ignore_conditional = args[ARG_IGNORE_CONDITIONAL].AsBool(false);  // Don't ignore conditionals by default

  if (args[ARG_MASK].Defined()) {  // Mask defined
    mask = args[ARG_MASK].AsClip();
    maskVi = mask->GetVideoInfo();
    if (maskVi.width!=overlayVi.width) {
      env->ThrowError("Overlay: Mask and overlay must have the same image size! (Width is not the same)");
    }
    if (maskVi.height!=overlayVi.height) {
      env->ThrowError("Overlay: Mask and overlay must have the same image size! (Height is not the same)");
    }
    if (maskVi.BitsPerComponent() != overlayVi.BitsPerComponent()) {
      env->ThrowError("Overlay: Mask and overlay must have the same bit depths!");
    }
  }

  pixelsize = vi.ComponentSize();
  bits_per_pixel = vi.BitsPerComponent();

  if (bits_per_pixel != overlayVi.BitsPerComponent()) {
    env->ThrowError("Overlay: input and mask clip must have the same bit depths!");
  }

  // already filled vi = child->GetVideoInfo();
  // parse and check output format override vi
  const char *output_pixel_format_override = args[ARG_OUTPUT].AsString(0);
  if(output_pixel_format_override) {
    int output_pixel_type = GetPixelTypeFromName(output_pixel_format_override);
    if(output_pixel_type == VideoInfo::CS_UNKNOWN)
      env->ThrowError("Overlay: invalid pixel_type!");

    outputVi->pixel_type = output_pixel_type; // override output pixel format
    if(outputVi->BitsPerComponent() != inputVi->BitsPerComponent())
      env->ThrowError("Overlay: output bitdepth should be the same as input's!");
  }

  if (vi.Is444())
    use444 = true; // 444 is conversionless by default

  // let use444=false to go live for subfilters that are ready to use it
  // except: RGB must be converted to 444 for Luma and Chroma operation
  if (vi.IsRGB() && (_stricmp(name, "Luma") == 0 || _stricmp(name, "Chroma") == 0)) {
    if (use444_defined && !use444) {
      env->ThrowError("Overlay: for RGB you cannot specify use444=false for overlay mode: %s", name);
    }
    use444 = true;
  }
  else if (!use444_defined &&
    (vi.IsY() || vi.Is420() || vi.Is422() || vi.IsRGB()) &&
    (_stricmp(name, "Blend") == 0 || _stricmp(name, "Luma") == 0 || _stricmp(name, "Chroma") == 0))
  {
    use444 = false; // default false for modes capable handling of use444==false, and valid formats
  }

  if (!use444) {
    // check if we can work in conversionless mode
    // 1.) colorspace is greyscale, 4:2:0 or 4:2:2 or any RGB
    // 2.) mode is "blend-like" (at the moment)
    if (!vi.IsY() && !vi.Is420() && !vi.Is422() && !vi.IsRGB())
      env->ThrowError("Overlay: use444=false is allowed only for greyscale, 4:2:0, 4:2:2 or any RGB video formats");
    //if (output_pixel_format_override && outputVi->pixel_type != vi.pixel_type)
    //  env->ThrowError("Overlay: use444=false is allowed only when no output pixel format is specified");
    if (_stricmp(name, "Blend") != 0 && _stricmp(name, "Luma") != 0 && _stricmp(name, "Chroma") != 0)
      env->ThrowError("Overlay: cannot specify use444=false for this overlay mode: %s", name);
    //if (vi.pixel_type != overlayVi.pixel_type)
    //  env->ThrowError("Overlay: for use444=false input and overlay formats have to be the same");
  }

  bool hasAlpha = vi.IsYUVA() || vi.IsPlanarRGBA();

  // set internal working format
  if (use444) {
    // we convert everything to 4:4:4
    // fill yuv 444 template
    switch (bits_per_pixel) {
    case 8: viInternalWorkingFormat->pixel_type = hasAlpha ? VideoInfo::CS_YUVA444 : VideoInfo::CS_YV24; break;
    case 10: viInternalWorkingFormat->pixel_type = hasAlpha ? VideoInfo::CS_YUVA444P10 : VideoInfo::CS_YUV444P10; break;
    case 12: viInternalWorkingFormat->pixel_type = hasAlpha ? VideoInfo::CS_YUVA444P12 : VideoInfo::CS_YUV444P12; break;
    case 14: viInternalWorkingFormat->pixel_type = hasAlpha ? VideoInfo::CS_YUVA444P14 : VideoInfo::CS_YUV444P14; break;
    case 16: viInternalWorkingFormat->pixel_type = hasAlpha ? VideoInfo::CS_YUVA444P16 : VideoInfo::CS_YUV444P16; break;
    case 32: viInternalWorkingFormat->pixel_type = hasAlpha ? VideoInfo::CS_YUVA444PS : VideoInfo::CS_YUV444PS; break;
    }
  }
  else {
    if (vi.IsRGB() && !vi.IsPlanar()) {
      // use444=false -> packed RGB 8/16 bit silent conversion to planar RGB
      bool hasAlpha = vi.IsRGB32() || vi.IsRGB64();
      switch (bits_per_pixel) {
      case 8: viInternalWorkingFormat->pixel_type = hasAlpha ? VideoInfo::CS_RGBAP : VideoInfo::CS_RGBP; break;
      case 16: viInternalWorkingFormat->pixel_type = hasAlpha ? VideoInfo::CS_RGBAP16 : VideoInfo::CS_RGBP16; break;
      }
    }
    else {
    // keep input format for internal format. Always 4:2:0, 4:2:2 (or 4:4:4 / Planar RGB)
    // filters have to prepare to work for these formats
      memcpy(viInternalWorkingFormat, &vi, sizeof(VideoInfo));
    }
  }
  viInternalOverlayWorkingFormat->pixel_type = viInternalWorkingFormat->pixel_type;

  // Set GetFrame's real output format
  // We have fast conversions for YV12, YV16 and YUY2, and YUV420P10-16, YUV422P10-16
  if ((pixelsize == 1 || pixelsize == 2) && outputVi->Is420() && use444)
  {
    // on-the-fly fast conversion at the end of GetFrame
    // check the pixelsize == 1 or 2 condition, should be the same as there
    switch (bits_per_pixel) {
    case 8: vi.pixel_type = hasAlpha ? VideoInfo::CS_YUVA420 : VideoInfo::CS_YV12; break;
    case 10: vi.pixel_type = hasAlpha ? VideoInfo::CS_YUVA420P10 : VideoInfo::CS_YUV420P10; break;
    case 12: vi.pixel_type = hasAlpha ? VideoInfo::CS_YUVA420P12 : VideoInfo::CS_YUV420P12; break;
    case 14: vi.pixel_type = hasAlpha ? VideoInfo::CS_YUVA420P14 : VideoInfo::CS_YUV420P14; break;
    case 16: vi.pixel_type = hasAlpha ? VideoInfo::CS_YUVA420P16 : VideoInfo::CS_YUV420P16; break;
    case 32: vi.pixel_type = hasAlpha ? VideoInfo::CS_YUVA420PS : VideoInfo::CS_YUV420PS; break;
    }
  }
  else if ((pixelsize == 1 || pixelsize == 2) && outputVi->Is422() && use444)
  {
    // on-the-fly fast conversion at the end of GetFrame
    // check the pixelsize == 1 or 2 condition, should be the same as there
    switch (bits_per_pixel) {
    case 8: vi.pixel_type = hasAlpha ? VideoInfo::CS_YUVA422 : VideoInfo::CS_YV16; break;
    case 10: vi.pixel_type = hasAlpha ? VideoInfo::CS_YUVA422P10 : VideoInfo::CS_YUV422P10; break;
    case 12: vi.pixel_type = hasAlpha ? VideoInfo::CS_YUVA422P12 : VideoInfo::CS_YUV422P12; break;
    case 14: vi.pixel_type = hasAlpha ? VideoInfo::CS_YUVA422P14 : VideoInfo::CS_YUV422P14; break;
    case 16: vi.pixel_type = hasAlpha ? VideoInfo::CS_YUVA422P16 : VideoInfo::CS_YUV422P16; break;
    case 32: vi.pixel_type = hasAlpha ? VideoInfo::CS_YUVA422PS : VideoInfo::CS_YUV422PS; break;
    }
  }
  else if (outputVi->IsYUY2())
  {
    // on-the-fly fast conversion at the end of GetFrame
    vi.pixel_type = VideoInfo::CS_YUY2;
  } else {
    vi.pixel_type = viInternalWorkingFormat->pixel_type;
    // Y,420,422,444,PlanarRGB (and packed RGB converted to any intermediate)
  }

}


Overlay::~Overlay() {
  free(inputVi);
  free(outputVi);
  free(viInternalWorkingFormat);
  free(viInternalOverlayWorkingFormat);
}

PVideoFrame __stdcall Overlay::GetFrame(int n, IScriptEnvironment *env) {

  int op_offset;
  float op_offset_f;
  int con_x_offset;
  int con_y_offset;
  FetchConditionals(env, &op_offset, &op_offset_f, &con_x_offset, &con_y_offset, ignore_conditional, condVarSuffix);

  // always use avisynth converters
  AVSValue child2;
  PVideoFrame frame;

  // internal working formats:
  // subsampled planar: 420, 422
  // full info: 444, planarRGB(A)
  // full info 1 plane: greyscale
  bool isInternalRGB = viInternalWorkingFormat->IsRGB(); // must be planar rgb
  bool isInternalGrey = viInternalWorkingFormat->IsY();
  bool isInternal444 = viInternalWorkingFormat->Is444();
  bool isInternal422 = viInternalWorkingFormat->Is422();
  bool isInternal420 = viInternalWorkingFormat->Is420();

  if (inputVi->pixel_type == viInternalWorkingFormat->pixel_type)
  {
    // get frame as is.
    // includes 420, 422, 444, planarRGB(A)
    frame = child->GetFrame(n, env);
  }
  else if (isInternal444) {
    if ((pixelsize == 1 || pixelsize == 2) && inputVi->Is420()) {
      // use blazing fast YV12 -> YV24 converter
      PVideoFrame frameSrc420 = child->GetFrame(n, env);
      frame = env->NewVideoFrame(*viInternalWorkingFormat);
      // no fancy options for chroma resampler, etc.. simply fast
      Convert444FromYV12(frameSrc420, frame, pixelsize, bits_per_pixel, env);
    }
    else if ((pixelsize == 1 || pixelsize == 2) && inputVi->Is422()) {
   // use blazing fast YV16 -> YV24 converter
      PVideoFrame frameSrc422 = child->GetFrame(n, env);
      frame = env->NewVideoFrame(*viInternalWorkingFormat);
      Convert444FromYV16(frameSrc422, frame, pixelsize, bits_per_pixel, env);
    }
    else if (inputVi->IsYUY2()) {
   // use blazing fast YUY2 -> YV24 converter
      PVideoFrame frameSrcYUY2 = child->GetFrame(n, env);
      frame = env->NewVideoFrame(*viInternalWorkingFormat);
      Convert444FromYUY2(frameSrcYUY2, frame, pixelsize, bits_per_pixel, env);
    }
    else if (inputVi->IsRGB()) {
      // clip, interlaced, matrix
      AVSValue new_args[3] = { child, false, (full_range) ? "PC.601" : "rec601" };
      child2 = env->Invoke("ConvertToYUV444", AVSValue(new_args, 3)).AsClip();
      frame = child2.AsClip()->GetFrame(n, env);
    }
    else {
      // clip, interlaced
      AVSValue new_args[2] = { child, false };
      child2 = env->Invoke("ConvertToYUV444", AVSValue(new_args, 2)).AsClip();
      frame = child2.AsClip()->GetFrame(n, env);
    }
  }
  else if (isInternalRGB) {
    // when we reach here and working format is planar rgb, source is surely packed rgb
    // generic Avisynth conversion
    if (inputVi->IsRGB()) {
      if (viInternalWorkingFormat->IsPlanarRGB()) {
        AVSValue new_args[1] = { child };
        child2 = env->Invoke("ConvertToPlanarRGB", AVSValue(new_args, 1)).AsClip();
      }
      else if (viInternalWorkingFormat->IsPlanarRGBA()) {
        AVSValue new_args[1] = { child };
        child2 = env->Invoke("ConvertToPlanarRGBA", AVSValue(new_args, 1)).AsClip();
      }
    } // for completeness, really, internal is never RGB if source is YUV
    else {
      if (viInternalWorkingFormat->IsPlanarRGB()) {
        // clip, matrix, interlaced
        AVSValue new_args[3] = { overlay, (full_range) ? "PC.601" : "rec601", false };
        child2 = env->Invoke("ConvertToPlanarRGB", AVSValue(new_args, 3)).AsClip();
      }
      else if (viInternalWorkingFormat->IsPlanarRGBA()) {
        AVSValue new_args[3] = { overlay, (full_range) ? "PC.601" : "rec601", false };
        child2 = env->Invoke("ConvertToPlanarRGBA", AVSValue(new_args, 3)).AsClip();
      }
    }
    frame = child2.AsClip()->GetFrame(n, env);
  }

  // Fetch current frame and convert it to internal format
  env->MakeWritable(&frame);

  ImageOverlayInternal* img = new ImageOverlayInternal(frame, vi.width, vi.height, *viInternalWorkingFormat, child->GetVideoInfo().IsYUVA() || child->GetVideoInfo().IsPlanarRGBA(), false, *viInternalWorkingFormat, env);

  PVideoFrame Oframe;
  AVSValue overlay2;

  ImageOverlayInternal* maskImg = NULL;

  // overlay clip should be converted to internal format if different, except for internal Y,
  // for which original planar YUV is OK
  if(overlayVi.pixel_type == viInternalWorkingFormat->pixel_type)
  {
    // don't convert is input and overlay is the same formats
    // so we can work in YUV420 or YUV422 and Planar RGB directly besides YUV444 (use444==false)
    Oframe = overlay->GetFrame(n, env);
  }
  else if (isInternal444) {
    if ((pixelsize == 1 || pixelsize == 2) && overlayVi.Is420()) {
      // use blazing fast YV12 -> YV24 converter
      PVideoFrame frameSrc420 = overlay->GetFrame(n, env);
      Oframe = env->NewVideoFrame(*viInternalOverlayWorkingFormat);
      // no fancy options for chroma resampler, etc.. simply fast
      Convert444FromYV12(frameSrc420, Oframe, pixelsize, bits_per_pixel, env);
    }
    else if ((pixelsize == 1 || pixelsize == 2) && overlayVi.Is422()) {
      // use blazing fast YV16 -> YV24 converter
      PVideoFrame frameSrc422 = overlay->GetFrame(n, env);
      Oframe = env->NewVideoFrame(*viInternalOverlayWorkingFormat);
      Convert444FromYV16(frameSrc422, Oframe, pixelsize, bits_per_pixel, env);
    }
    else if (overlayVi.IsYUY2()) {
      // use blazing fast YUY2 -> YV24 converter
      PVideoFrame frameSrcYUY2 = overlay->GetFrame(n, env);
      Oframe = env->NewVideoFrame(*viInternalOverlayWorkingFormat);
      Convert444FromYUY2(frameSrcYUY2, Oframe, pixelsize, bits_per_pixel, env);
    }
    else if (overlayVi.IsRGB()) {
      // clip, interlaced, matrix
      AVSValue new_args[3] = { overlay, false, (full_range) ? "PC.601" : "rec601" };
      overlay2 = env->Invoke("ConvertToYUV444", AVSValue(new_args, 3)).AsClip();
      Oframe = overlay2.AsClip()->GetFrame(n, env);
    }
    else {
      // clip, interlaced
      AVSValue new_args[2] = { overlay, false };
      overlay2 = env->Invoke("ConvertToYUV444", AVSValue(new_args, 2)).AsClip();
      Oframe = overlay2.AsClip()->GetFrame(n, env);
    }
  }
  else if(isInternalGrey) {
    if (overlayVi.IsPlanar() && (overlayVi.IsYUV() || overlayVi.IsYUVA()))
    {
      // they are good as-is, we'll use only Y
      Oframe = overlay->GetFrame(n, env);
    } else if(overlayVi.IsRGB()) {
      // clip, matrix
      AVSValue new_args[2] = { overlay, (full_range) ? "PC.601" : "rec601" };
      overlay2 = env->Invoke("ConvertToY", AVSValue(new_args, 2)).AsClip();
      Oframe = overlay2.AsClip()->GetFrame(n, env);
    }
    else if (overlayVi.IsYUY2()) {
      // use blazing fast YUY2 -> YV24 converter
      // we'll use only Y anyway
      PVideoFrame frameSrcYUY2 = overlay->GetFrame(n, env);
      Oframe = env->NewVideoFrame(*viInternalOverlayWorkingFormat);
      Convert444FromYUY2(frameSrcYUY2, Oframe, pixelsize, bits_per_pixel, env);
    }
    else {
      AVSValue new_args[1] = { overlay };
      overlay2 = env->Invoke("ConvertToY", AVSValue(new_args, 1)).AsClip();
      Oframe = overlay2.AsClip()->GetFrame(n, env);
    }
  }
  else if (isInternalRGB) {
    if (overlayVi.IsPlanarRGB() || overlayVi.IsPlanarRGBA()) {
      Oframe = overlay->GetFrame(n, env);
    }
    else if (overlayVi.IsYUV() || overlayVi.IsYUVA()) {
      // clip, matrix, interlaced
      AVSValue new_args[3] = { overlay, (full_range) ? "PC.601" : "rec601", false };
      overlay2 = env->Invoke("ConvertToPlanarRGB", AVSValue(new_args, 3)).AsClip();
      Oframe = overlay2.AsClip()->GetFrame(n, env);
    } else {
      // packed RGB
      AVSValue new_args[1] = { overlay };
      overlay2 = env->Invoke("ConvertToPlanarRGB", AVSValue(new_args, 1)).AsClip();
      Oframe = overlay2.AsClip()->GetFrame(n, env);
    }
  }
  else if (isInternal420) {
    if (overlayVi.Is420())
    {
      Oframe = overlay->GetFrame(n, env);
    } else if(overlayVi.IsRGB()) {
      // clip, interlaced, matrix
      AVSValue new_args[3] = { overlay, false, (full_range) ? "PC.601" : "rec601" };
      overlay2 = env->Invoke("ConvertToYUV444", AVSValue(new_args, 3)).AsClip();
      // faster than invoking ConvertToYUV420
      PVideoFrame frameSrc420 = overlay2.AsClip()->GetFrame(n, env);
      Oframe = env->NewVideoFrame(*viInternalOverlayWorkingFormat);
      // no fancy options for chroma resampler, etc.. simply fast
      Convert444ToYV12(frameSrc420, Oframe, pixelsize, bits_per_pixel, env);
    }
    else {
      // clip, interlaced
      AVSValue new_args[2] = { overlay, false };
      overlay2 = env->Invoke("ConvertToYUV444", AVSValue(new_args, 2)).AsClip();
      PVideoFrame frameSrc420 = overlay2.AsClip()->GetFrame(n, env);
      Oframe = env->NewVideoFrame(*viInternalOverlayWorkingFormat);
      // no fancy options for chroma resampler, etc.. simply fast
      Convert444ToYV12(frameSrc420, Oframe, pixelsize, bits_per_pixel, env);
    }
  } else if(isInternal422) {
    if (overlayVi.Is422())
    {
      Oframe = overlay->GetFrame(n, env);
    } else if(overlayVi.IsRGB()) {
      // clip, interlaced, matrix
      AVSValue new_args[3] = { overlay, false, (full_range) ? "PC.601" : "rec601" };
      overlay2 = env->Invoke("ConvertToYUV444", AVSValue(new_args, 3)).AsClip();
      // faster than invoking ConvertToYUV422
      PVideoFrame frameSrc422 = overlay2.AsClip()->GetFrame(n, env);
      Oframe = env->NewVideoFrame(*viInternalOverlayWorkingFormat);
      // no fancy options for chroma resampler, etc.. simply fast
      Convert444ToYV16(frameSrc422, Oframe, pixelsize, bits_per_pixel, env);
    }
    else {
      AVSValue new_args[2] = { overlay, false };
      // clip, interlaced
      overlay2 = env->Invoke("ConvertToYUV444", AVSValue(new_args, 2)).AsClip();
      PVideoFrame frameSrc422 = overlay2.AsClip()->GetFrame(n, env);
      Oframe = env->NewVideoFrame(*viInternalOverlayWorkingFormat);
      // no fancy options for chroma resampler, etc.. simply fast
      Convert444ToYV16(frameSrc422, Oframe, pixelsize, bits_per_pixel, env);
    }
  }
  // Fetch current overlay and convert it to internal format
  ImageOverlayInternal* overlayImg = new ImageOverlayInternal(Oframe, overlayVi.width, overlayVi.height, *viInternalOverlayWorkingFormat, overlay->GetVideoInfo().IsYUVA() || overlay->GetVideoInfo().IsPlanarRGBA(), false, *viInternalWorkingFormat, env);

  // Clip overlay to original image
  ClipFrames(img, overlayImg, offset_x + con_x_offset, offset_y + con_y_offset);

  if (overlayImg->IsSizeZero()) { // Nothing to overlay
  }
  else {
    // from Avisynth's doc
    /*
    Inputting RGB for mask clip
    An RGB mask clip may behave a bit oddly if it contains color information.
    If you use a greyscale mask, or if you leave greymask=true, you will get the result you would expect.
    Note that mask values are never scaled, so it will automatically be in 0-255 range, directly copied from the RGB values.
    */
    // Note1: This last sentence is not true. Mask is converted from RGB the same way as input and overlay clips.
    // Note2: But yes, it's true. Classic AVS converts RGB to mask in "void Convert444FromRGB::ConvertImageLumaOnly"
    //        and it uses (comment copies as is sic!) dstY[x] = srcP[RGBx]; // Blue channel only ???

    // fetch current mask (if given)
    if (mask) {
      AVSValue mask2;
      PVideoFrame Mframe;

      if (maskVi.IsY() || isInternalGrey)
        greymask = true;

      if ((greymask && !maskVi.IsRGB() && !maskVi.IsYUY2()) || maskVi.pixel_type == viInternalWorkingFormat->pixel_type) {
        // 4:4:4,
        // 4:2:0, 4:2:2 -> greymask uses Y
        // internalworking format: 4:4:4, 4:2:2, 4:2:0
        Mframe = mask->GetFrame(n, env);
      }
      else if (isInternal444 || greymask) {
        if ((pixelsize == 1 || pixelsize == 2) && maskVi.Is420()) {
          // use blazing fast YV12 -> YV24 converter
          PVideoFrame frameSrc420 = mask->GetFrame(n, env);
          Mframe = env->NewVideoFrame(*viInternalOverlayWorkingFormat);
          // no fancy options for chroma resampler, etc.. simply fast
          Convert444FromYV12(frameSrc420, Mframe, pixelsize, bits_per_pixel, env);
          // convert frameSrc420 -> frame
        }
        else if ((pixelsize == 1 || pixelsize == 2) && maskVi.Is422()) {
          // use blazing fast YV16 -> YV24 converter
          PVideoFrame frameSrc422 = mask->GetFrame(n, env);
          Mframe = env->NewVideoFrame(*viInternalOverlayWorkingFormat);
          Convert444FromYV16(frameSrc422, Mframe, pixelsize, bits_per_pixel, env);
        }
        else if (maskVi.IsYUY2()) {
          // use blazing fast YUY2 -> YV24 converter
          PVideoFrame frameSrcYUY2 = mask->GetFrame(n, env);
          Mframe = env->NewVideoFrame(*viInternalOverlayWorkingFormat);
          Convert444FromYUY2(frameSrcYUY2, Mframe, pixelsize, bits_per_pixel, env);
        }
        else if (maskVi.IsRGB()) {
          if (greymask) {
            // Compatibility: ExtractB. See notes above.
            // This is good, because in the old times rgb32clip.ShowAlpha() was
            // used for feeding mask to overlay, that spreads alpha to all channels, so classic avisynth chose
            // B channel for source. (=R=G)
            // So we are not using greyscale conversion here at mask for compatibility reasons
            // Still: recommended usage for mask: rgbclip.ExtractA()
            /*
            AVSValue new_args[2] = { mask, (full_range) ? "PC.601" : "rec601" };
            mask2 = env->Invoke("ConvertToY", AVSValue(new_args, 2)).AsClip();
            */
            AVSValue new_args[1] = { mask };
            mask2 = env->Invoke("ExtractB", AVSValue(new_args, 1)).AsClip();
          }
          else {
            AVSValue new_args[3] = { mask, false, (full_range) ? "PC.601" : "rec601" };
            mask2 = env->Invoke("ConvertToYUV444", AVSValue(new_args, 3)).AsClip();
          }
          Mframe = mask2.AsClip()->GetFrame(n, env);
        }
      }
      else {
        // greymask == false
        if (isInternalRGB) {
          // clip, matrix, interlaced
          AVSValue new_args[3] = { mask, (full_range) ? "PC.601" : "rec601", false };
          mask2 = env->Invoke("ConvertToPlanarRGB", AVSValue(new_args, 3)).AsClip();
          Mframe = mask2.AsClip()->GetFrame(n, env);
        }
        else if (isInternal420) {
          AVSValue new_args[3] = { mask, false, (full_range) ? "PC.601" : "rec601" };
          mask2 = env->Invoke("ConvertToYUV444", AVSValue(new_args, 3)).AsClip();
          // faster than invoking ConvertToYUV420
          PVideoFrame frameSrc420 = mask2.AsClip()->GetFrame(n, env);
          Mframe = env->NewVideoFrame(*viInternalOverlayWorkingFormat);
          // no fancy options for chroma resampler, etc.. simply fast
          Convert444ToYV12(frameSrc420, Mframe, pixelsize, bits_per_pixel, env);
        }
        else if (isInternal422) {
          AVSValue new_args[3] = { mask, false, (full_range) ? "PC.601" : "rec601" };
          mask2 = env->Invoke("ConvertToYUV444", AVSValue(new_args, 3)).AsClip();
          // faster than invoking ConvertToYUV422
          PVideoFrame frameSrc422 = mask2.AsClip()->GetFrame(n, env);
          Mframe = env->NewVideoFrame(*viInternalOverlayWorkingFormat);
          // no fancy options for chroma resampler, etc.. simply fast
          Convert444ToYV16(frameSrc422, Mframe, pixelsize, bits_per_pixel, env);
        }
        else {
          AVSValue new_args[3] = { mask, false, (full_range) ? "PC.601" : "rec601" };
          mask2 = env->Invoke("ConvertToYUV444", AVSValue(new_args, 3)).AsClip();
          Mframe = mask2.AsClip()->GetFrame(n, env);
        }
      }
      // MFrame here is either internalWorkingFormat or Y or 4:4:4
      maskImg = new ImageOverlayInternal(Mframe, maskVi.width, maskVi.height, *viInternalOverlayWorkingFormat, mask->GetVideoInfo().IsYUVA() || mask->GetVideoInfo().IsPlanarRGBA(), greymask, maskVi, env);

      img->ReturnOriginal(true);
      ClipFrames(img, maskImg, offset_x + con_x_offset, offset_y + con_y_offset);


    }

    OverlayFunction* func = SelectFunction(name, of_mode, env);

    // Process the image
    func->setMode(of_mode);
    func->setBitsPerPixel(bits_per_pixel);
    func->setOpacity(opacity + op_offset, opacity_f + op_offset_f);
    func->setColorSpaceInfo(viInternalWorkingFormat->IsRGB(), viInternalWorkingFormat->IsY());
    func->setEnv(env);

    if (!mask) {
      func->DoBlendImage(img, overlayImg);
    } else {
      func->DoBlendImageMask(img, overlayImg, maskImg);
    }

    delete func;

    // Reset overlay & image offsets
    img->ReturnOriginal(true);
    overlayImg->ReturnOriginal(true);
    if (mask)
        maskImg->ReturnOriginal(true);
  }

  // Cleanup
  if (mask) {
    delete maskImg;
  }
  delete overlayImg;
  if (img) {
    delete img;
  }

  // here img->frame is 444
  // new from 170103: can be 420 or 422, + fast 10-16 bit
  // apply fast conversion
  if((pixelsize==1 || pixelsize == 2) && outputVi->Is420() && viInternalWorkingFormat->Is444())
  {
    PVideoFrame outputFrame = env->NewVideoFrame(*outputVi);
    Convert444ToYV12(frame, outputFrame, pixelsize, bits_per_pixel, env);
    return outputFrame;
  } else if((pixelsize==1 || pixelsize == 2) && outputVi->Is422() && viInternalWorkingFormat->Is444()) {
    PVideoFrame outputFrame = env->NewVideoFrame(*outputVi);
    Convert444ToYV16(frame, outputFrame, pixelsize, bits_per_pixel, env);
    return outputFrame;
  } else if(outputVi->IsYUY2()) {
    PVideoFrame outputFrame = env->NewVideoFrame(*outputVi);
    Convert444ToYUY2(frame, outputFrame, pixelsize, bits_per_pixel, env);
    return outputFrame;
  }
  // all other cases return 4:4:4
  // except when use444 is false
  return frame;
}


/*************************
 *   Helper functions    *
 *************************/


OverlayFunction* Overlay::SelectFunction(const char* name, int &of_mode, IScriptEnvironment* env) {

  if (!lstrcmpi(name, "Blend")) {
    of_mode = OF_Blend;
    return new OL_BlendImage();
  }

  if (!lstrcmpi(name, "Add")) {
    of_mode = OF_Add;
    return new OL_AddImage();
  }

  if (!lstrcmpi(name, "Subtract")) {
    of_mode = OF_Subtract;
    //return new OL_SubtractImage();
    return new OL_AddImage(); // common with Add
  }

  if (!lstrcmpi(name, "Multiply")) {
    of_mode = OF_Multiply;
    return new OL_MultiplyImage();
  }

  if (!lstrcmpi(name, "Chroma")) {
    of_mode = OF_Chroma;
    return new OL_BlendImage(); // Common with BlendImage. plane range differs of_mode checked inside
    //return new OL_BlendChromaImage();
  }

  if (!lstrcmpi(name, "Luma")) {
    of_mode = OF_Luma;
    //return new OL_BlendLumaImage();
    return new OL_BlendImage(); // Common with BlendImage. plane range differs of_mode checked inside
  }

  if (!lstrcmpi(name, "Lighten")) {
    of_mode = OF_Lighten;
    //return new OL_LightenImage();
    return new OL_DarkenImage(); // common with Darken
  }

  if (!lstrcmpi(name, "Darken")) {
    of_mode = OF_Darken;
    return new OL_DarkenImage();
  }

  if (!lstrcmpi(name, "SoftLight")) {
    of_mode = OF_SoftLight;
    return new OL_SoftLightImage();
  }

  if (!lstrcmpi(name, "HardLight")) {
    of_mode = OF_HardLight;
    //return new OL_HardLightImage();
    return new OL_SoftLightImage(); // Common with SoftLight
  }

  if (!lstrcmpi(name, "Difference")) {
    of_mode = OF_Difference;
    return new OL_DifferenceImage();
  }

  if (!lstrcmpi(name, "Exclusion")) {
    of_mode = OF_Exclusion;
    return new OL_ExclusionImage();
  }

  env->ThrowError("Overlay: Invalid 'Mode' specified.");
  return 0;
}

void Overlay::ClipFrames(ImageOverlayInternal* input, ImageOverlayInternal* overlay, int x, int y) {

  input->ResetFake();
  overlay->ResetFake();

  input->ReturnOriginal(false);  // We now use cropped space
  overlay->ReturnOriginal(false);

  // Crop negative offset off overlay
  if (x<0) {
    overlay->SubFrame(-x,0,overlay->w()+x, overlay->h());
    x=0;
  }
  if (y<0) {
    overlay->SubFrame(0,-y, overlay->w(), overlay->h()+y);
    y=0;
  }
  // Clip input-frame to topleft overlay:
  input->SubFrame(x,y,input->w()-x, input->h()-y);

  // input and overlay are now topleft aligned

  // Clip overlay that is beyond the right side of the input

  if (overlay->w() > input->w()) {
    overlay->SubFrame(0,0,input->w(), overlay->h());
  }

  if (overlay->h() > input->h()) {
    overlay->SubFrame(0,0,overlay->w(), input->h());
  }

  // Clip right/ bottom of input

  if(input->w() > overlay->w()) {
    input->SubFrame(0,0, overlay->w(), input->h());
  }

  if(input->h() > overlay->h()) {
    input->SubFrame(0,0, input->w(), overlay->h());
  }

}

void Overlay::FetchConditionals(IScriptEnvironment* env, int* op_offset, float* op_offset_f, int* con_x_offset, int* con_y_offset, bool ignore_conditional, const char *condVarSuffix) {
  *op_offset = 0;
  *op_offset_f = 0.0f;
  *con_x_offset = 0;
  *con_y_offset = 0;

  if (!ignore_conditional) {
    IScriptEnvironment2 *env2 = static_cast<IScriptEnvironment2*>(env);
    {
      std::string s = std::string("OL_opacity_offset") + condVarSuffix;
      *op_offset = (int)(env2->GetVar(s.c_str(), 0.0) * 256);
      *op_offset_f = (float)(env2->GetVar(s.c_str(), 0.0));
    }
    {
      std::string s = std::string("OL_x_offset") + condVarSuffix;
      *con_x_offset = (int)(env2->GetVar(s.c_str(), 0.0));
    }
    {
      std::string s = std::string("OL_y_offset") + condVarSuffix;
      *con_y_offset = (int)(env2->GetVar(s.c_str(), 0.0));
    }
  }
}


AVSValue __cdecl Overlay::Create(AVSValue args, void*, IScriptEnvironment* env) {
   //return new Overlay(args[0].AsClip(), args, env);

  Overlay* Result = new Overlay(args[0].AsClip(), args, env);
  if (Result->GetVideoInfo().pixel_type == Result->outputVi->pixel_type)
     return Result;
   // c[interlaced]b[matrix]s[ChromaInPlacement]s[chromaresample]s
   // chromaresample = 'bicubic' default
   // chromaresample = 'point' is faster
   if(Result->outputVi->Is444()) {
     // if workingFormat is not 444 but output was specified
     AVSValue new_args[3] = { Result, false, (Result->full_range) ? "PC.601" : "rec601" };
     return env->Invoke("ConvertToYUV444", AVSValue(new_args, 3)).AsClip();
   }
   if(Result->outputVi->Is422()) {
     AVSValue new_args[3] = { Result, false, (Result->full_range) ? "PC.601" : "rec601" };
     return env->Invoke("ConvertToYUV422", AVSValue(new_args, 3)).AsClip();
   }
   if(Result->outputVi->Is420()) {
     AVSValue new_args[3] = { Result, false, (Result->full_range) ? "PC.601" : "rec601" };
     return env->Invoke("ConvertToYUV420", AVSValue(new_args, 3)).AsClip();
   }
   if(Result->outputVi->IsYUY2()) {
     AVSValue new_args[3] = { Result, false, (Result->full_range) ? "PC.601" : "rec601" };
     return env->Invoke("ConvertToYUY2", AVSValue(new_args, 3)).AsClip();
   }
   if(Result->outputVi->IsY()) {
     AVSValue new_args[2] = { Result, (Result->full_range) ? "PC.601" : "rec601" };
     return env->Invoke("ConvertToY", AVSValue(new_args, 2)).AsClip();
   }
   if(Result->outputVi->IsRGB()) {
     // c[matrix]s[interlaced]b[ChromaInPlacement]s[chromaresample]s
     AVSValue new_args[3] = { Result, (Result->full_range) ? "PC.601" : "rec601", false};
     if(Result->outputVi->IsPlanarRGB()) {
       return env->Invoke("ConvertToPlanarRGB", AVSValue(new_args, 3)).AsClip();
     }
     if(Result->outputVi->IsPlanarRGBA()) {
       return env->Invoke("ConvertToPlanarRGBA", AVSValue(new_args, 3)).AsClip();
     }
     if(Result->outputVi->IsRGB24()) {
       return env->Invoke("ConvertToRGB24", AVSValue(new_args, 3)).AsClip();
     }
     if(Result->outputVi->IsRGB32()) {
       return env->Invoke("ConvertToRGB32", AVSValue(new_args, 3)).AsClip();
     }
     if(Result->outputVi->IsRGB48()) {
       return env->Invoke("ConvertToRGB48", AVSValue(new_args, 3)).AsClip();
     }
     if(Result->outputVi->IsRGB64()) {
       return env->Invoke("ConvertToRGB64", AVSValue(new_args, 3)).AsClip();
     }
   }
   env->ThrowError("Overlay: Invalid output format.");
   return Result;
}
