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

#ifndef __Focus_H__
#define __Focus_H__

#include <avisynth.h>


class AdjustFocusV : public GenericVideoFilter 
/**
  * Class to adjust focus in the vertical direction, helper for sharpen/blue
 **/
{
public:
  AdjustFocusV(double _amount, PClip _child);
  virtual ~AdjustFocusV(void);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

private:
  const int amount;
  BYTE* line;

};

/* Helpers for AdjustFocusV */
void AFV_C(BYTE* l, BYTE* p, const int height, const int pitch, const int row_size, const int amount);
void AFV_MMX(const BYTE* l, const BYTE* p, const int height, const int pitch, const int row_size, const int amount);


class AdjustFocusH : public GenericVideoFilter 
/**
  * Class to adjust focus in the horizontal direction, helper for sharpen/blue
 **/
{
public:
  AdjustFocusH(double _amount, PClip _child);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

private:
  const int amount;

};

/* Helpers for AdjustFocusH */
void AFH_YUY2_C(BYTE* p, int height, const int pitch, const int width, const int amount);
void AFH_YUY2_MMX(const BYTE* p, const int height, const int pitch, const int width, const int amount);
void AFH_RGB32_C(BYTE* p, int height, const int pitch, const int width, const int amount);
void AFH_RGB32_MMX(const BYTE* p, const int height, const int pitch, const int width, const int amount);
void AFH_YV12_C(BYTE* p, int height, const int pitch, const int row_size, const int amount);
void AFH_YV12_MMX(BYTE* p, int height, const int pitch, const int row_size, const int amount);

void AFH_RGB24_C(BYTE* p, int height, const int pitch, const int width, const int amount);
// no mmx version. to be honest, who would intensively use this ?

/*** Sharpen/Blur Factory methods ***/

AVSValue __cdecl Create_Sharpen(AVSValue args, void*, IScriptEnvironment* env);
AVSValue __cdecl Create_Blur(AVSValue args, void*, IScriptEnvironment* env);




/*** Soften classes ***/

class TemporalSoften : public GenericVideoFilter 
/**
  * Class to soften the focus on the temporal axis
 **/
{
public:
  TemporalSoften( PClip _child, unsigned radius, unsigned luma_thresh, unsigned chroma_thresh,int _scenechange, IScriptEnvironment* env );
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
  virtual ~TemporalSoften(void);
  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);

private:
// YV12:
    int* planes;
    const BYTE** planeP;
    const BYTE** planeP2;
    int* planePitch;
    int* planePitch2;
    bool* planeDisabled;
    int scenechange;
    PVideoFrame *frames;

// YUY2:
  const unsigned luma_threshold, chroma_threshold;
  const int kernel;


  enum { MAX_RADIUS=7 };

};


class SpatialSoften : public GenericVideoFilter 
/**
  * Class to soften the focus on the spatial axis
 **/
{
public:
  SpatialSoften(PClip _child, int _radius, unsigned _luma_threshold, unsigned _chroma_threshold, IScriptEnvironment* env);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);

private:  
  const unsigned luma_threshold, chroma_threshold;
  const int diameter;

};



#endif  // __Focus_H__
