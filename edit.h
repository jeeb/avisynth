// Avisynth v1.0 beta.  Copyright 2000 Ben Rudiak-Gould.
// http://www.math.berkeley.edu/~benrg/avisynth.html

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

#ifndef __Edit_H__
#define __Edit_H__

#include "internal.h"


/********************************************************************
********************************************************************/

class Trim : public GenericVideoFilter 
/**
  * Class to select a range of frames from a longer clip
 **/
{
public:
  Trim(int _firstframe, int _lastframe, PClip _child);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
  void __stdcall GetAudio(void* buf, int start, int count, IScriptEnvironment* env);
  bool __stdcall GetParity(int n);

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);  

private:
  int firstframe;
  int audio_offset;
};




class FreezeFrame : public GenericVideoFilter 
/**
  * Class to display a single frame for the duration of several
 **/
{
public:
  FreezeFrame(int _first, int _last, int _source, PClip _child);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
  bool __stdcall GetParity(int n);

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env); 

private:
  const int first, last, source;
};




class DeleteFrame : public GenericVideoFilter 
/**
  * Class to delete a frame
 **/
{  
public:
  DeleteFrame(int _frame, PClip _child);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
  bool __stdcall GetParity(int n);

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);

private:
  const int frame;
};




class DuplicateFrame : public GenericVideoFilter 
/**
  * Class to duplicate a frame
 **/
{  
public:
  DuplicateFrame(int _frame, PClip _child);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
  bool __stdcall GetParity(int n);

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);

private:
  const int frame;
};




class Splice : public GenericVideoFilter 
/**
  * Class to splice together video clips
 **/
{
public:
  Splice(PClip _child1, PClip _child2, bool realign_sound, IScriptEnvironment* env);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
  void __stdcall GetAudio(void* buf, int start, int count, IScriptEnvironment* env);
  bool __stdcall GetParity(int n);

  static AVSValue __cdecl CreateUnaligned(AVSValue args, void*, IScriptEnvironment* env);
  static AVSValue __cdecl CreateAligned(AVSValue args, void*, IScriptEnvironment* env);

private:
  const PClip child2;
  int video_switchover_point;
  int audio_switchover_point;
};




class Dissolve : public GenericVideoFilter 
/**
  * Class to smoothly transition from one video clip to another
 **/
{
public:
  Dissolve(PClip _child1, PClip _child2, int _overlap, IScriptEnvironment* env);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
  void __stdcall GetAudio(void* buf, int start, int count, IScriptEnvironment* env);
  bool __stdcall GetParity(int n);

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);

private:
  const PClip child2;
  const int overlap;
  int video_fade_start, video_fade_end;
  int audio_fade_start, audio_fade_end;
  BYTE* audbuffer;
  int audbufsize;
  void EnsureBuffer(int minsize);
};




class AudioDub : public IClip {  
/**
  * Class to mux the audio track of one clip with the video of another
 **/
public:
  AudioDub(PClip child1, PClip child2, IScriptEnvironment* env);
  const VideoInfo& __stdcall GetVideoInfo();
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
  void __stdcall GetAudio(void* buf, int start, int count, IScriptEnvironment* env);
  bool __stdcall GetParity(int n);

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);

private:
  /*const*/ PClip vchild, achild;
  VideoInfo vi;
};




class Reverse : public GenericVideoFilter 
/**
  * Class to play a clip backwards
 **/
{
public:
  Reverse(PClip _child);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
  bool __stdcall GetParity(int n);
  void __stdcall GetAudio(void* buf, int start, int count, IScriptEnvironment* env);

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);
};




class Loop : public GenericVideoFilter {
/**
  * Class to loop over a range of frames
**/
public:
	Loop(PClip _child, int times, int _start, int _end);
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
	bool __stdcall GetParity(int n);

	static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);
private:
	int frames, start, end;
	int convert(int n);
};




/**** A few factory methods ****/

static AVSValue __cdecl Create_FadeOut(AVSValue args, void*, IScriptEnvironment* env);
static AVSValue __cdecl Create_FadeOut2(AVSValue args, void*, IScriptEnvironment* env);

PClip new_Splice(PClip _child1, PClip _child2, bool realign_sound, IScriptEnvironment* env);


#endif  // __Edit_H__