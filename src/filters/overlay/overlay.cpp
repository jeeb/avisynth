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
  { "Overlay", "cc[mask]c[opacity]f", Overlay::Create },   
    // 0, src clip 
    // 1, overlay clip
    // 2, mask clip
    // 3, overlay opacity.(0.0->1.0)
  { 0 }
};
 

Overlay::Overlay(PClip _child, AVSValue args, IScriptEnvironment *env) :
 GenericVideoFilter(_child) {
   mask = 0;
   opacity = (int)(256.0*args[3].AsFloat(1.0));
   overlay = args[1].AsClip();

   overlayVi = overlay->GetVideoInfo();
   if (overlayVi.IsYV12()) {
     overlayConv = new Convert444FromYV12();
   } else {
     env->ThrowError("Overlay: Overlay image colorspace not supported.");
   }

   if (args[2].Defined()) {
     mask = args[2].AsClip();
     maskVi = mask->GetVideoInfo();
     if (maskVi.width!=overlayVi.width) {
       env->ThrowError("Overlay: Mask and overlay must have the same image size! (Width is not the same)");
     }
     if (maskVi.height!=overlayVi.height) {
       env->ThrowError("Overlay: Mask and overlay must have the same image size! (Height is not the same)");
     }
     if (maskVi.IsYV12()) {
       maskConv = new Convert444FromYV12();
     } else {
       env->ThrowError("Overlay: Mask image colorspace not supported.");
     }
     maskImg = new Image444(maskVi.width, maskVi.height);
   }
   
   if (vi.IsYV12()) {
     inputConv = new Convert444FromYV12();
     outputConv = new Convert444ToYV12();
   } else {
     env->ThrowError("Overlay: Colorspace not supported.");
   }
  img = new Image444(vi.width, vi.height);
  overlayImg = new Image444(overlayVi.width, overlayVi.height);

  func = new OL_AddImage();

}


Overlay::~Overlay() {
  if (mask)
    maskImg->free();
  overlayImg->free();
  img->free();
}

PVideoFrame __stdcall Overlay::GetFrame(int n, IScriptEnvironment *env) {
  // Fetch current frame and convert it.
  PVideoFrame frame = child->GetFrame(n, env);
  inputConv->ConvertImage(frame, img, env);
  // Fetch current overlay and convert it
  PVideoFrame Oframe = overlay->GetFrame(n, env);
  overlayConv->ConvertImage(Oframe, overlayImg, env);

  // Process the image
  func->setOpacity(opacity);
  if (!mask) {
    func->BlendImage(img, overlayImg);
  } else {
    func->BlendImageMask(img, overlayImg, maskImg);
  }

  // Convert output image back
  frame = outputConv->ConvertImage(img, frame, env);
  return frame;
}


AVSValue __cdecl Overlay::Create(AVSValue args, void*, IScriptEnvironment* env) {
   return new Overlay(args[0].AsClip(), args, env);
}