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
#include "overlay.h"

/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

AVSFunction Overlay_filters[] = {
  { "Overlay", "cc[x]i[y]i[mask]c[opacity]f[mode]s[greymask]b[output]s[ignore_conditional]b[PC_Range]b", Overlay::Create },
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

  mask = 0;
  opacity = (int)(256.0*args[ARG_OPACITY].AsFloat(1.0)+0.5);
  offset_x = args[ARG_X].AsInt(0);
  offset_y = args[ARG_Y].AsInt(0);

  overlay = args[ARG_OVERLAY].AsClip();
  overlayVi = overlay->GetVideoInfo();
  overlayConv = SelectInputCS(&overlayVi, env);

  if (!overlayConv) {
    env->ThrowError("Overlay: Overlay image colorspace not supported.");
  }

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

    maskConv = SelectInputCS(&maskVi, env);
    if (!maskConv) {
       env->ThrowError("Overlay: Mask image colorspace not supported.");
    }

    maskImg = new Image444(maskVi.width, maskVi.height);

    if (greymask) {
      maskImg->free_chroma();
      maskImg->SetPtr(maskImg->GetPtr(PLANAR_Y), PLANAR_U);
      maskImg->SetPtr(maskImg->GetPtr(PLANAR_Y), PLANAR_V);
    }

  }

  inputConv = SelectInputCS(inputVi, env);

  if (!inputConv) {
    env->ThrowError("Overlay: Colorspace not supported.");
  }

  if (args[ARG_OUTPUT].Defined())
    outputConv = SelectOutputCS(args[ARG_OUTPUT].AsString(),env);
  else
    outputConv = SelectOutputCS(0, env);

  img = new Image444(vi.width, vi.height);
  overlayImg = new Image444(overlayVi.width, overlayVi.height);

  func = SelectFunction(args[ARG_MODE].AsString("Blend"), env);

}


Overlay::~Overlay() {
  if (mask) {
    if (!greymask) {
      maskImg->free_chroma();
    }
    maskImg->free_luma();
    delete maskImg;
    delete maskConv;
  }
  overlayImg->free();
  delete overlayImg;
  if (img) {
    img->free();
    delete img;
  }
  free(inputVi);
  delete func;
  delete outputConv;
  delete inputConv;
  delete overlayConv;
}


PVideoFrame __stdcall Overlay::GetFrame(int n, IScriptEnvironment *env) {

  FetchConditionals(env);

    // Fetch current frame and convert it.
  PVideoFrame frame = child->GetFrame(n, env);
  inputConv->ConvertImage(frame, img, env);

  // Fetch current overlay and convert it
  PVideoFrame Oframe = overlay->GetFrame(n, env);
  overlayConv->ConvertImage(Oframe, overlayImg, env);

  // Clip overlay to original image
  ClipFrames(img, overlayImg, offset_x + con_x_offset, offset_y + con_y_offset);


  if (overlayImg->IsSizeZero()) {
    // Convert output image back
    img->ReturnOriginal(true);
    overlayImg->ReturnOriginal(true);
    PVideoFrame f = env->NewVideoFrame(vi);
    return outputConv->ConvertImage(img, f, env);
  }


  // fetch current mask (if given)

  if (mask) {
    PVideoFrame Mframe = mask->GetFrame(n, env);
    if (greymask)
      maskConv->ConvertImageLumaOnly(Mframe, maskImg, env);
    else
      maskConv->ConvertImage(Mframe, maskImg, env);

    img->ReturnOriginal(true);
    ClipFrames(img, maskImg, offset_x + con_x_offset, offset_y + con_y_offset);
  }

  // Process the image
  func->setOpacity(opacity + op_offset);
  func->setEnv(env);

  if (!mask) {
    func->BlendImage(img, overlayImg);
  } else {
    func->BlendImageMask(img, overlayImg, maskImg);
  }

  // Reset overlay & image offsets
  img->ReturnOriginal(true);
  overlayImg->ReturnOriginal(true);
  if (mask)
    maskImg->ReturnOriginal(true);

  // Convert output image back

  PVideoFrame f = env->NewVideoFrame(vi);
  f = outputConv->ConvertImage(img, f, env);
  return f;
}


/*************************
 *   Helper functions    *
 *************************/


OverlayFunction* Overlay::SelectFunction(const char* name, IScriptEnvironment* env) {

  if (!lstrcmpi(name, "Blend"))
    return new OL_BlendImage();

  if (!lstrcmpi(name, "Add"))
    return new OL_AddImage();

  if (!lstrcmpi(name, "Subtract"))
    return new OL_SubtractImage();

  if (!lstrcmpi(name, "Multiply"))
    return new OL_MultiplyImage();

  if (!lstrcmpi(name, "Chroma"))
    return new OL_BlendChromaImage();

  if (!lstrcmpi(name, "Luma"))
    return new OL_BlendLumaImage();

  if (!lstrcmpi(name, "Lighten"))
    return new OL_LightenImage();

  if (!lstrcmpi(name, "Darken"))
    return new OL_DarkenImage();

  if (!lstrcmpi(name, "SoftLight"))
    return new OL_SoftLightImage();

  if (!lstrcmpi(name, "HardLight"))
    return new OL_HardLightImage();

  if (!lstrcmpi(name, "Difference"))
    return new OL_DifferenceImage();

  if (!lstrcmpi(name, "Exclusion"))
    return new OL_ExclusionImage();

  env->ThrowError("Overlay: Invalid 'Mode' specified.");
  return 0;
}

////////////////////////////////
// This function will return an output colorspace converter.
// First priority is the string.
// If if there isn't one supplied it will return a converter
// matching the current VideoInfo (vi)
////////////////////////////////

ConvertFrom444* Overlay::SelectOutputCS(const char* name, IScriptEnvironment* env) {
  if (!name) {
    if (vi.IsYV12()) {
      return new Convert444ToYV12();
    } else if (vi.IsYUY2()) {
      return new Convert444ToYUY2();
    } else if (vi.IsRGB()) {
      if (full_range) {
        return new Convert444NonCCIRToRGB();
      }
      return new Convert444ToRGB();
    }
  }

  if (!lstrcmpi(name, "YUY2")) {
    vi.pixel_type = VideoInfo::CS_YUY2;
    return new Convert444ToYUY2();
  }

  if (!lstrcmpi(name, "YV12")) {
    vi.pixel_type = VideoInfo::CS_YV12;
    return new Convert444ToYV12();
  }

  if (!lstrcmpi(name, "RGB")) {
    vi.pixel_type = VideoInfo::CS_BGR32;
    if (full_range)
      return new Convert444NonCCIRToRGB();
    return new Convert444ToRGB();
  }

  if (!lstrcmpi(name, "RGB32")) {
    vi.pixel_type = VideoInfo::CS_BGR32;
    if (full_range)
      return new Convert444NonCCIRToRGB();
    return new Convert444ToRGB();
  }

  if (!lstrcmpi(name, "RGB24")) {
    vi.pixel_type = VideoInfo::CS_BGR24;
    if (full_range)
      return new Convert444NonCCIRToRGB();
    return new Convert444ToRGB();
  }

  env->ThrowError("Overlay: Invalid 'output' colorspace specified.");
  return 0;
}

///////////////////////////////////
// Note: Instead of throwing an
// error, this function returns 0
// if it cannot fint the colorspace,
// for more accurate error reporting
///////////////////////////////////

ConvertTo444* Overlay::SelectInputCS(VideoInfo* VidI, IScriptEnvironment* env) {
  if (VidI->IsYV12()) {
    ConvertTo444* c = new Convert444FromYV12();
    c->SetVideoInfo(VidI);
    return c;
  } else if (VidI->IsYUY2()) {
    ConvertTo444* c = new Convert444FromYUY2();
    c->SetVideoInfo(VidI);
    return c;
  } else if (VidI->IsRGB()) {
    ConvertTo444* c;
    if (full_range)
      c = new Convert444NonCCIRFromRGB();
    else
      c = new Convert444FromRGB();
    c->SetVideoInfo(VidI);
    return c;
  }
  return 0;
}


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

void Overlay::FetchConditionals(IScriptEnvironment* env) {
  op_offset = 0;
  con_x_offset = 0;
  con_y_offset = 0;

  if (!ignore_conditional) {
    try {
		  AVSValue cv = env->GetVar("OL_opacity_offset");
		  if (cv.IsFloat())
        op_offset = (int)(cv.AsFloat()*256.0);
    } catch (IScriptEnvironment::NotFound) {}

    try {
  		AVSValue cv = env->GetVar("OL_x_offset");
	  	if (cv.IsFloat())
        con_x_offset = (int)(cv.AsFloat());
    } catch (IScriptEnvironment::NotFound) {}

    try {
  		AVSValue cv = env->GetVar("OL_y_offset");
	  	if (cv.IsFloat())
        con_y_offset = (int)(cv.AsFloat());
    } catch (IScriptEnvironment::NotFound) {}
  }
}


AVSValue __cdecl Overlay::Create(AVSValue args, void*, IScriptEnvironment* env) {
   return new Overlay(args[0].AsClip(), args, env);
}
