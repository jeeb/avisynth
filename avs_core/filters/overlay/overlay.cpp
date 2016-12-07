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

#include <avs/win.h>
#include <stdlib.h>
#include "overlay.h"
#include "../core/internal.h"

/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

extern const AVSFunction Overlay_filters[] = {
  { "Overlay", BUILTIN_FUNC_PREFIX, "cc[x]i[y]i[mask]c[opacity]f[mode]s[greymask]b[output]s[ignore_conditional]b[PC_Range]b", Overlay::Create },
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
  ARG_FULL_RANGE = 10
};

Overlay::Overlay(PClip _child, AVSValue args, IScriptEnvironment *env) :
GenericVideoFilter(_child) {

  full_range = args[ARG_FULL_RANGE].AsBool(false);  // Maintain CCIR601 range when converting to/from RGB.

  // Make copy of the VideoInfo
  inputVi = (VideoInfo*)malloc(sizeof(VideoInfo));
  memcpy(inputVi, &vi, sizeof(VideoInfo));
  outputVi = (VideoInfo*)malloc(sizeof(VideoInfo));
  memcpy(outputVi, &vi, sizeof(VideoInfo));
  vi444 = (VideoInfo*)malloc(sizeof(VideoInfo));
  memcpy(vi444, &vi, sizeof(VideoInfo));

  mask = 0;
  opacity_f = (float)args[ARG_OPACITY].AsDblDef(1.0); // rfu, if once overlay gets float support
  opacity = (int)(256.0*opacity_f + 0.5); // range is converted to 256 for all all bit_depth
  offset_x = args[ARG_X].AsInt(0);
  offset_y = args[ARG_Y].AsInt(0);

  if (!args[ARG_OVERLAY].IsClip())
    env->ThrowError("Overlay: Overlay parameter is not a clip");

  overlay = args[ARG_OVERLAY].AsClip();

  overlayVi = overlay->GetVideoInfo();
#if 0
  // we omit this phase. use ConvertToYUV444 instead of Overlay's Convert444fromXXX functions
  // we only use Convert444fromYV24
  overlayConv = SelectInputCS(&overlayVi, env, full_range);
#endif

#if 0 // we are converting in GetFrame on-the-fly
  if (!overlayConv) {
    AVSValue new_args[3] = { overlay, false, (full_range) ? "PC.601" : "rec601" };
    try {
      overlay = env->Invoke("ConvertToYUV444", AVSValue(new_args, 3)).AsClip();
    } catch (...)  {}

    overlayVi = overlay->GetVideoInfo();
#if 1
    // overlay here is surely 444 format, SelectInputCS will return this:
    overlayConv = new Convert444FromYV24(pixelsize, bits_per_pixel);
    overlayConv->SetVideoInfo(&overlayVi);
#else
    overlayConv = SelectInputCS(&overlayVi, env, full_range);
#endif

    if (!overlayConv) {  // ok - now we've tried everything ;)
      env->ThrowError("Overlay: Overlay image colorspace not supported.");
    }
  }
#endif

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


#if 0 // we are converting in GetFrame on-the-fly

#if 0
    maskConv = SelectInputCS(&maskVi, env, full_range);
#else
    maskConv = nullptr; // we are using avisynth's format converters
#endif
    if (!maskConv) {
      AVSValue new_args[3] = { mask, false, (full_range) ? "PC.601" : "rec601" };

      try {
        mask = env->Invoke((greymask) ? "ConvertToY" : "ConvertToYUV444", AVSValue(new_args, 3)).AsClip();
      } catch (...)  {}
      maskVi = mask->GetVideoInfo();
#if 1
      // overlay here is surely 444 or Y format, SelectInputCS will return either of this:
      maskConv = new Convert444FromYV24();
      // no need for new Convert444FromY8() because
      // if greyMask, only FromLuma will be called in GetFrame
      // they are simple Blts indeed
      maskConv->SetVideoInfo(&maskVi);
#else
      maskConv = SelectInputCS(&maskVi, env, full_range);
#endif
      if (!maskConv) {
        env->ThrowError("Overlay: Mask image colorspace not supported.");
      }
    }
#endif
  }

  inputCS = vi.pixel_type;
  pixelsize = vi.ComponentSize();
  bits_per_pixel = vi.BitsPerComponent();
#if 0 // we are converting in GetFrame on-the-fly

#if 0
  inputConv = SelectInputCS(inputVi, env, full_range);
#else
  inputConv = nullptr;
#endif

  if (!inputConv) {
    AVSValue new_args[3] = { child, false, (full_range) ? "PC.601" : "rec601" };
    try {
      child = env->Invoke("ConvertToYUV444", AVSValue(new_args, 3)).AsClip();
    } catch (...)  {}

    vi = child->GetVideoInfo();
    memcpy(inputVi, &vi, sizeof(VideoInfo));
#if 1
    inputConv = new Convert444FromYV24();
#else
    inputConv = SelectInputCS(inputVi, env, full_range);
#endif
    if (!inputConv) {
      env->ThrowError("Overlay: Colorspace not supported.");
    }
  }
#endif

#if 0
  outputConv = SelectOutputCS(args[ARG_OUTPUT].AsString(0),env);
#else
  vi = child->GetVideoInfo();
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

  bool hasAlpha = vi.IsYUVA() || vi.IsPlanarRGBA();

  // fill yuv 444 template
  switch(bits_per_pixel) {
  case 8: vi444->pixel_type = hasAlpha ? VideoInfo::CS_YUVA444 : VideoInfo::CS_YV24; break;
  case 10: vi444->pixel_type = hasAlpha ? VideoInfo::CS_YUVA444P10 : VideoInfo::CS_YUV444P10; break;
  case 12: vi444->pixel_type = hasAlpha ? VideoInfo::CS_YUVA444P12 : VideoInfo::CS_YUV444P12; break;
  case 14: vi444->pixel_type = hasAlpha ? VideoInfo::CS_YUVA444P14 : VideoInfo::CS_YUV444P14; break;
  case 16: vi444->pixel_type = hasAlpha ? VideoInfo::CS_YUVA444P16 : VideoInfo::CS_YUV444P16; break;
  case 32: vi444->pixel_type = hasAlpha ? VideoInfo::CS_YUVA444PS : VideoInfo::CS_YUV444PS; break;
  }

  // Set GetFrame's real output format
  // We have fast conversions for YV12 and YUY2
  if(pixelsize==1 && outputVi->Is420())
  {
    // on-the-fly fast conversion at the end of GetFrame
    // check the pixelsize == 1 condition, should be the same as there
    switch(bits_per_pixel) {
    case 8: vi.pixel_type = hasAlpha ? VideoInfo::CS_YUVA420 : VideoInfo::CS_YV12; break;
    case 10: vi.pixel_type = hasAlpha ? VideoInfo::CS_YUVA420P10 : VideoInfo::CS_YUV420P10; break;
    case 12: vi.pixel_type = hasAlpha ? VideoInfo::CS_YUVA420P12 : VideoInfo::CS_YUV420P12; break;
    case 14: vi.pixel_type = hasAlpha ? VideoInfo::CS_YUVA420P14 : VideoInfo::CS_YUV420P14; break;
    case 16: vi.pixel_type = hasAlpha ? VideoInfo::CS_YUVA420P16 : VideoInfo::CS_YUV420P16; break;
    case 32: vi.pixel_type = hasAlpha ? VideoInfo::CS_YUVA420PS : VideoInfo::CS_YUV420PS; break;
    }
  } else if (outputVi->IsYUY2())
  {
    // on-the-fly fast conversion at the end of GetFrame
    vi.pixel_type = VideoInfo::CS_YUY2;
  } else {
    vi.pixel_type = vi444->pixel_type;
  }

#endif

  name = args[ARG_MODE].AsString("Blend");

}


Overlay::~Overlay() {
  free(inputVi);
  free(outputVi);
  free(vi444);
#if 0
  delete outputConv;
  delete inputConv;
  delete overlayConv;
#endif
}

PVideoFrame __stdcall Overlay::GetFrame(int n, IScriptEnvironment *env) {

  int op_offset;
  int con_x_offset;
  int con_y_offset;
  FetchConditionals(env, &op_offset, &con_x_offset, &con_y_offset, ignore_conditional);


#if 0
  // Fetch current frame and convert it.
  PVideoFrame frame = child->GetFrame(n, env);
#endif

#ifndef USE_ORIG_FRAME
  // Image444 initialization
  Image444* maskImg = NULL;
  if (mask)
  {
      maskImg = new Image444(maskVi.width, maskVi.height, bits_per_pixel, mask->GetVideoInfo().IsYUVA() || mask->GetVideoInfo().IsPlanarRGBA(), env);
      if (greymask) {
          maskImg->free_chroma();
          maskImg->SetPtr(maskImg->GetPtr(PLANAR_Y), PLANAR_U);
          maskImg->SetPtr(maskImg->GetPtr(PLANAR_Y), PLANAR_V);
      }
  }
#endif

#if 1
  // always use avisynth converters
  AVSValue child2;
  PVideoFrame frame;

  if (inputVi->Is444())
  {
    frame = child->GetFrame(n, env);
  } else if(pixelsize == 1 && inputVi->Is420()) {
    // use blazing fast YV12 -> YV24 converter
    PVideoFrame frameSrc420 = child->GetFrame(n, env);
    frame = env->NewVideoFrame(*vi444);
    // no fancy options for chroma resampler, etc.. simply fast
    Convert444FromYV12(frameSrc420, frame, pixelsize, bits_per_pixel, env);
    // convert frameSrc420 -> frame
  } else if(pixelsize == 1 && inputVi->IsYUY2()) {
    // use blazing fast YUY2 -> YV24 converter
    PVideoFrame frameSrcYUY2 = child->GetFrame(n, env);
    frame = env->NewVideoFrame(*vi444);
    Convert444FromYUY2(frameSrcYUY2, frame, pixelsize, bits_per_pixel, env);
  } else if(inputVi->IsRGB()) {
    // generic Avisynth conversion
    AVSValue new_args[3] = { child, false, (full_range) ? "PC.601" : "rec601" };
    child2 = env->Invoke("ConvertToYUV444", AVSValue(new_args, 3)).AsClip();
    frame = child2.AsClip()->GetFrame(n, env);
    //internal_working_format = child2.AsClip()->GetVideoInfo().pixel_type;
  } else {
    AVSValue new_args[2] = { child, false};
    child2 = env->Invoke("ConvertToYUV444", AVSValue(new_args, 2)).AsClip();
    frame = child2.AsClip()->GetFrame(n, env);
    //internal_working_format = child2.AsClip()->GetVideoInfo().pixel_type;
  }
  // Fetch current frame and convert it to internal format
#ifdef USE_ORIG_FRAME
  env->MakeWritable(&frame); // == PVideoFrame &img->frame
#endif

  Image444* img = new Image444(frame, vi.width, vi.height, bits_per_pixel, child->GetVideoInfo().IsYUVA() || child->GetVideoInfo().IsPlanarRGBA(), env);
#ifndef USE_ORIG_FRAME
  CopyToImage444(frame, img, env);
#endif
#else
  PVideoFrame frame = child.AsClip()->GetFrame(n, env);
  inputConv->ConvertImage(frame, img, env); // always ConvertFrom444
#endif

#if 1
  // always use avisynth converters
  PVideoFrame Oframe;
  AVSValue overlay2;

  Image444* maskImg = NULL;

  if(overlayVi.Is444()
//    || (overlayVi.pixel_type == internal_working_format)
    )
    // don't convert is input and overlay is the same formats
    // so we can work in YUV420 or YUV422 directly besides YUV444
  {
    Oframe = overlay->GetFrame(n, env);
  } else if(pixelsize == 1 && overlayVi.Is420()) {
    // use blazing fast YV12 -> YV24 converter
    PVideoFrame frameSrc420 = overlay->GetFrame(n, env);
    Oframe = env->NewVideoFrame(*vi444);
    // no fancy options for chroma resampler, etc.. simply fast
    Convert444FromYV12(frameSrc420, Oframe, pixelsize, bits_per_pixel, env);
    // convert frameSrc420 -> frame
  } else if(pixelsize == 1 && overlayVi.IsYUY2()) {
    // use blazing fast YUY2 -> YV24 converter
    PVideoFrame frameSrcYUY2 = overlay->GetFrame(n, env);
    Oframe = env->NewVideoFrame(*vi444);
    Convert444FromYUY2(frameSrcYUY2, Oframe, pixelsize, bits_per_pixel, env);
  } else if(overlayVi.IsRGB()) {
    AVSValue new_args[3] = { overlay, false, (full_range) ? "PC.601" : "rec601" };
    overlay2 = env->Invoke("ConvertToYUV444", AVSValue(new_args, 3)).AsClip();
    Oframe = overlay2.AsClip()->GetFrame(n, env);
  } else {
    AVSValue new_args[2] = { overlay, false };
    overlay2 = env->Invoke("ConvertToYUV444", AVSValue(new_args, 2)).AsClip();
    Oframe = overlay2.AsClip()->GetFrame(n, env);
  }
  // Fetch current overlay and convert it to internal format
  Image444* overlayImg = new Image444(Oframe, overlayVi.width, overlayVi.height, bits_per_pixel, overlay->GetVideoInfo().IsYUVA() || overlay->GetVideoInfo().IsPlanarRGBA(), env);
  #ifndef USE_ORIG_FRAME
    CopyToImage444(Oframe, overlayImg, env);
  #endif
#else
  // Fetch current overlay and convert it
  PVideoFrame Oframe = overlay->GetFrame(n, env);
  overlayConv->ConvertImage(Oframe, overlayImg, env);
#endif

  // Clip overlay to original image
  ClipFrames(img, overlayImg, offset_x + con_x_offset, offset_y + con_y_offset);

  if (overlayImg->IsSizeZero()) { // Nothing to overlay
#ifndef USE_ORIG_FRAME
    // Convert output image back
    img->ReturnOriginal(true);
    overlayImg->ReturnOriginal(true);

    // Convert output image back
#if 1
    frameOutput = Convert444ToOriginal(&vi, // original or overridden format
      img, // Image444 * (source)
      frameOutput, // PVideoFrame target,
      full_range, // for RGB
      env
    );
#else
    frameOutput = outputConv->ConvertImage(img, frameOutput, pixelsize, bits_per_pixel, env);
#endif
#else
    // frame remains as-is
#endif
  } else {
    // from Avisynth's doc
    /*
    Inputting RGB for mask clip
    An RGB mask clip may behave a bit oddly if it contains color information.
    If you use a greyscale mask, or if you leave greymask=true, you will get the result you would expect.
    Note that mask values are never scaled, so it will automatically be in 0-255 range, directly copied from the RGB values.
    */
    // This last sentence is not true. Mask is converted from RGB the same way as input and overlay clips.

    // fetch current mask (if given)
    if (mask) {
#if 1
      AVSValue mask2;
      PVideoFrame Mframe;

      if(maskVi.Is444() || (greymask && maskVi.IsY())) {
        Mframe = mask->GetFrame(n, env);
      } else if(pixelsize == 1 && maskVi.Is420()) {
        // use blazing fast YV12 -> YV24 converter
        PVideoFrame frameSrc420 = mask->GetFrame(n, env);
        Mframe = env->NewVideoFrame(*vi444);
        // no fancy options for chroma resampler, etc.. simply fast
        Convert444FromYV12(frameSrc420, Mframe, pixelsize, bits_per_pixel, env);
        // convert frameSrc420 -> frame
      } else if(pixelsize == 1 && maskVi.IsYUY2()) {
        // use blazing fast YUY2 -> YV24 converter
        PVideoFrame frameSrcYUY2 = mask->GetFrame(n, env);
        Mframe = env->NewVideoFrame(*vi444);
        Convert444FromYUY2(frameSrcYUY2, Mframe, pixelsize, bits_per_pixel, env);
      } else if(maskVi.IsRGB()) {
        if(greymask) {
          AVSValue new_args[2] = { mask, (full_range) ? "PC.601" : "rec601" };
          mask2 = env->Invoke("ConvertToY", AVSValue(new_args, 2)).AsClip();
        } else {
          AVSValue new_args[3] = { mask, false, (full_range) ? "PC.601" : "rec601" };
          mask2 = env->Invoke("ConvertToYUV444", AVSValue(new_args, 3)).AsClip();
        }
        Mframe = mask2.AsClip()->GetFrame(n, env);
      } else {
        if(greymask) {
          AVSValue new_args[2] = { mask };
          mask2 = env->Invoke("ConvertToY", AVSValue(new_args, 1)).AsClip();
        } else {
          AVSValue new_args[2] = { mask, false };
          mask2 = env->Invoke("ConvertToYUV444", AVSValue(new_args, 2)).AsClip();
        }
        Mframe = mask2.AsClip()->GetFrame(n, env);
      }
      maskImg = new Image444(Mframe, maskVi.width, maskVi.height, bits_per_pixel, mask->GetVideoInfo().IsYUVA() || mask->GetVideoInfo().IsPlanarRGBA(), env);
      if (greymask) {
#ifndef USE_ORIG_FRAME
        maskImg->free_chroma();
#endif
        maskImg->SetPtr(maskImg->GetPtr(PLANAR_Y), PLANAR_U);
        maskImg->SetPtr(maskImg->GetPtr(PLANAR_Y), PLANAR_V);
      }
  #ifndef USE_ORIG_FRAME
      if (greymask)
        CopyToImage444LumaOnly(Mframe, maskImg, env);
      else
        CopyToImage444(Mframe, maskImg, env);
  #endif
#else
      PVideoFrame Mframe = mask->GetFrame(n, env);
      if (greymask)
        maskConv->ConvertImageLumaOnly(Mframe, maskImg, env);  // uses Convert444FromYV24's method, for luma it is the same as Convert444FromY8
      else
        maskConv->ConvertImage(Mframe, maskImg, env);
#endif

        img->ReturnOriginal(true);
        ClipFrames(img, maskImg, offset_x + con_x_offset, offset_y + con_y_offset);
    }

    OverlayFunction* func = SelectFunction(name, of_mode, env);

    // Process the image
    func->setMode(of_mode);
    func->setBitsPerPixel(bits_per_pixel);
    func->setOpacity(opacity + op_offset);
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

#ifndef USE_ORIG_FRAME
#if 1
    frameOutput = Convert444ToOriginal(&vi, // original or overridden format
      img, // Image444 * (source)
      frameOutput, // PVideoFrame target,
      full_range, // for RGB
      env
    );
#else
    f = outputConv->ConvertImage(img, f, pixelsize, bits_per_pixel, env);
#endif
#endif
  }

  // Cleanup
  if (mask) {
    if (!greymask) {
      maskImg->free_chroma();
    }
    maskImg->free_luma();
    delete maskImg;
  }
  overlayImg->free_all();
  delete overlayImg;
  if (img) {
    img->free_all();
    delete img;
  }

  // here img->frame is 444
  // apply fast conversion
  if((pixelsize==1) && outputVi->Is420())
  {
    PVideoFrame outputFrame = env->NewVideoFrame(*outputVi);
    Convert444ToYV12(frame, outputFrame, pixelsize, bits_per_pixel, env);
    return outputFrame;
  } else if(outputVi->IsYUY2()) {
    PVideoFrame outputFrame = env->NewVideoFrame(*outputVi);
    Convert444ToYUY2(frame, outputFrame, pixelsize, bits_per_pixel, env);
    return outputFrame;
  }
  // all other cases return 4:4:4
#ifndef USE_ORIG_FRAME
  return frameOutput;
#else
  return frame;
#endif
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
    return new OL_BlendChromaImage();
  }

  if (!lstrcmpi(name, "Luma")) {
    of_mode = OF_Luma;
    return new OL_BlendLumaImage();
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

////////////////////////////////
// This function will return an output colorspace converter.
// First priority is the string.
// If if there isn't one supplied it will return a converter
// matching the current VideoInfo (vi)
////////////////////////////////

#if 0
ConvertFrom444* Overlay::SelectOutputCS(const char* name, IScriptEnvironment* env) {
  if (!name) {
    if (vi.Is420()) {
      return new Convert444ToYV12();
    } else if (vi.Is444()) {
      return new Convert444ToYV24();
    } else if (vi.IsY()) {
      return new Convert444ToY8();
    } else if (vi.IsYUY2()) {
      return new Convert444ToYUY2();
    } else if (vi.IsRGB() && !vi.IsPlanarRGB() && !vi.IsPlanarRGBA()) {
      if (full_range) {
        return new Convert444NonCCIRToRGB();
      }
      return new Convert444ToRGB();
    }
    else
    {
      /* This branch is to prevent continuing execution for unsupported colorspaces,
         as we rely on "name" being non-NULL further on.
      */
	  env->ThrowError("Overlay: Unsupported colorspace.");
    }
  }

  if (!lstrcmpi(name, "YUY2")) {
    if(pixelsize != 1)
      env->ThrowError("Overlay: Source must be 8 bits.");
    vi.pixel_type = VideoInfo::CS_YUY2;
    return new Convert444ToYUY2();
  }

  if (!lstrcmpi(name, "YV12")) {
    if(pixelsize != 1)
      env->ThrowError("Overlay: Source must be 8 bits.");
    vi.pixel_type = VideoInfo::CS_YV12;
    return new Convert444ToYV12();
  }

  if (!lstrcmpi(name, "YV24")) {
    if(pixelsize != 1)
      env->ThrowError("Overlay: Source must be 8 bits.");
    vi.pixel_type = VideoInfo::CS_YV24;
    return new Convert444ToYV24();
  }

  if (!lstrcmpi(name, "Y8")) {
    if(pixelsize != 1)
      env->ThrowError("Overlay: Source must be 8 bits.");
    vi.pixel_type = VideoInfo::CS_Y8;
    return new Convert444ToY8();
  }

  if (!lstrcmpi(name, "YUV420")) {
    switch(bits_per_pixel) {
    case 8: vi.pixel_type = VideoInfo::CS_YV12; break;
    case 10: vi.pixel_type = VideoInfo::CS_YUV420P10; break;
    case 12: vi.pixel_type = VideoInfo::CS_YUV420P12; break;
    case 14: vi.pixel_type = VideoInfo::CS_YUV420P14; break;
    case 16: vi.pixel_type = VideoInfo::CS_YUV420P16; break;
    case 32: vi.pixel_type = VideoInfo::CS_YUV420PS; break;
    }
    return new Convert444ToYV12(pixelsize, bits_per_pixel);
  }

  if (!lstrcmpi(name, "YUV444")) {
    switch(bits_per_pixel) {
    case 8: vi.pixel_type = VideoInfo::CS_YV24; break;
    case 10: vi.pixel_type = VideoInfo::CS_YUV444P10; break;
    case 12: vi.pixel_type = VideoInfo::CS_YUV444P12; break;
    case 14: vi.pixel_type = VideoInfo::CS_YUV444P14; break;
    case 16: vi.pixel_type = VideoInfo::CS_YUV444P16; break;
    case 32: vi.pixel_type = VideoInfo::CS_YUV444PS; break;
    }
    return new Convert444ToYV24(pixelsize, bits_per_pixel);
  }

  if (!lstrcmpi(name, "Y")) {
    switch(bits_per_pixel) {
    case 8: vi.pixel_type = VideoInfo::CS_Y8; break;
    case 10: vi.pixel_type = VideoInfo::CS_Y10; break;
    case 12: vi.pixel_type = VideoInfo::CS_Y12; break;
    case 14: vi.pixel_type = VideoInfo::CS_Y14; break;
    case 16: vi.pixel_type = VideoInfo::CS_Y16; break;
    case 32: vi.pixel_type = VideoInfo::CS_Y32; break;
    }
    return new Convert444ToY8(pixelsize, bits_per_pixel);
  }

  if (!lstrcmpi(name, "RGB")) {
    if(pixelsize != 1)
      env->ThrowError("Overlay: Source must be 8 bits.");
    vi.pixel_type = VideoInfo::CS_BGR32;
    if (full_range)
      return new Convert444NonCCIRToRGB(pixelsize, bits_per_pixel);
    return new Convert444ToRGB(pixelsize, bits_per_pixel);
  }

  if (!lstrcmpi(name, "RGB32")) {
    if(pixelsize != 1)
      env->ThrowError("Overlay: Source must be 8 bits.");
    vi.pixel_type = VideoInfo::CS_BGR32;
    if (full_range)
      return new Convert444NonCCIRToRGB(pixelsize, bits_per_pixel);
    return new Convert444ToRGB(pixelsize, bits_per_pixel);
  }

  if (!lstrcmpi(name, "RGB64")) {
    if(pixelsize != 2)
      env->ThrowError("Overlay: Source must be 16 bits.");
    vi.pixel_type = VideoInfo::CS_BGR64;
    if (full_range)
      return new Convert444NonCCIRToRGB(pixelsize, bits_per_pixel);
    return new Convert444ToRGB(pixelsize, bits_per_pixel);
  }

  if (!lstrcmpi(name, "RGB24")) {
    if(pixelsize != 1)
      env->ThrowError("Overlay: Source must be 8 bits.");
    vi.pixel_type = VideoInfo::CS_BGR24;
    if (full_range)
      return new Convert444NonCCIRToRGB(pixelsize, bits_per_pixel);
    return new Convert444ToRGB(pixelsize, bits_per_pixel);
  }

  if (!lstrcmpi(name, "RGB48")) {
    if(pixelsize != 2)
      env->ThrowError("Overlay: Source must be 16 bits.");
    vi.pixel_type = VideoInfo::CS_BGR48;
    if (full_range)
      return new Convert444NonCCIRToRGB(pixelsize, bits_per_pixel);
    return new Convert444ToRGB(pixelsize, bits_per_pixel);
  }

  env->ThrowError("Overlay: Invalid 'output' colorspace specified.");
  return 0;
}
#endif

///////////////////////////////////
// Note: Instead of throwing an
// error, this function returns 0
// if it cannot fint the colorspace,
// for more accurate error reporting
///////////////////////////////////

#if 0
// let's use Avisynth's converters

ConvertTo444* Overlay::SelectInputCS(VideoInfo* VidI, IScriptEnvironment* env, bool full_range) {
  if (VidI->Is420()) {
    ConvertTo444* c = new Convert444FromYV12(pixelsize, bits_per_pixel);
    c->SetVideoInfo(VidI);
    return c;
  } else if (VidI->Is444()) {
    ConvertTo444* c = new Convert444FromYV24(pixelsize, bits_per_pixel);
    c->SetVideoInfo(VidI);
    return c;
  } else if (VidI->IsY()) {
    ConvertTo444* c = new Convert444FromY8(pixelsize, bits_per_pixel);
    c->SetVideoInfo(VidI);
    return c;
  } else if (VidI->IsYUY2()) {
    ConvertTo444* c = new Convert444FromYUY2(pixelsize, bits_per_pixel);
    c->SetVideoInfo(VidI);
    return c;
  } else if (VidI->IsRGB() && !VidI->IsPlanarRGB() && !VidI->IsPlanarRGBA()) {
    ConvertTo444* c;
    if (full_range)
      c = new Convert444NonCCIRFromRGB(pixelsize, bits_per_pixel);
    else
      c = new Convert444FromRGB(pixelsize, bits_per_pixel);
    c->SetVideoInfo(VidI);
    return c;
  }
  return 0;
}
#endif

void Overlay::ClipFrames(Image444* input, Image444* overlay, int x, int y) {

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

void Overlay::FetchConditionals(IScriptEnvironment* env, int* op_offset, int* con_x_offset, int* con_y_offset, bool ignore_conditional) {
  *op_offset = 0;
  *con_x_offset = 0;
  *con_y_offset = 0;

  if (!ignore_conditional) {
    IScriptEnvironment2 *env2 = static_cast<IScriptEnvironment2*>(env);
    *op_offset    = (int)(env2->GetVar("OL_opacity_offset", 0.0)*256);
    *con_x_offset = (int)(env2->GetVar("OL_x_offset"      , 0.0));
    *con_y_offset = (int)(env2->GetVar("OL_y_offset"      , 0.0));
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
     // never can be
     AVSValue new_args[2] = { Result, false};
     return env->Invoke("ConvertToYUV444", AVSValue(new_args, 2)).AsClip();
   }
   if(Result->outputVi->Is422()) {
     AVSValue new_args[2] = { Result, false};
     return env->Invoke("ConvertToYUV422", AVSValue(new_args, 2)).AsClip();
   }
   if(Result->outputVi->Is420()) {
     AVSValue new_args[2] = { Result, false};
     return env->Invoke("ConvertToYUV420", AVSValue(new_args, 2)).AsClip();
     // old overlay 444->YV12 direct converter is much faster than avisynth's internal converter
     // because it simply averages chroma and put it back directly
     // Avisynth's version is generic, uses generic resamplers for chroma
     /*
     AVSValue new_args[5] = { Result, "bicubic"};
     static const char* const arg_names[2] = { 0, "chromaresample" };
     return env->Invoke("ConvertToYUV420", AVSValue(new_args, 2), arg_names).AsClip();
     */
   }
   if(Result->outputVi->IsYUY2()) {
     AVSValue new_args[2] = { Result, false};
     return env->Invoke("ConvertToYUY2", AVSValue(new_args, 2)).AsClip();
   }
   if(Result->outputVi->IsY()) {
     AVSValue new_args[1] = { Result};
     return env->Invoke("ConvertToY", AVSValue(new_args, 1)).AsClip();
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
