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
#ifndef __Audio_H__
#define __Audio_H__

#include "../internal.h"
#include "../filters/text-overlay.h"




/******* Helper stuff *******/

#define MAX_SHORT (32767)
#define MIN_SHORT (-32768)


/* Conversion constants */
#define Nhc       8
#define Na        7
#define Np       (Nhc+Na)
#define Npc      (1<<Nhc)
#define Amask    ((1<<Na)-1)
#define Pmask    ((1<<Np)-1)
#define Nh       16
#define Nb       16
#define Nhxn     14
#define Nhg      (Nh-Nhxn)
#define NLpScl   13


#define IzeroEPSILON 1E-21               /* Max error acceptable in Izero */

static const long double PI = 3.14159265358979323846;

static __inline short IntToShort(int v, int scl)
{
  v += (1<<(scl-1));  /* round */
  v >>= scl;
  if (v>MAX_SHORT)
    return MAX_SHORT;
  else if (v < MIN_SHORT)
    return MIN_SHORT;
  else
    return (short)v;
}

static double Izero(double x);
static void LpFilter(double c[], int N, double frq, double Beta, int Num);
static int makeFilter(short Imp[], int *LpScl, unsigned short Nwing, double Froll, double Beta);


/********************************************************************
********************************************************************/

class AssumeRate : public GenericVideoFilter 
/**
  * Changes the sample rate of a clip
 **/
{
public:
  AssumeRate(PClip _clip, int _rate);
  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment*);
};



class ConvertToMono : public GenericVideoFilter 
/**
  * Class to convert audio to mono
 **/
{
public:
  ConvertToMono(PClip _clip);
  virtual ~ConvertToMono()
  {if (tempbuffer_size) {delete[] tempbuffer;tempbuffer_size=0;}}

  void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env);
  static PClip Create(PClip clip);
  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment*);

private:
  char *tempbuffer;
  int tempbuffer_size;
  int channels;
};

class EnsureVBRMP3Sync : public GenericVideoFilter 
/**
  * Class to convert audio to mono
 **/
{
public:
  EnsureVBRMP3Sync(PClip _clip);

  void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env);
  static PClip Create(PClip clip);
  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment*);

private:
  __int64 last_end;
};

class MergeChannels : public GenericVideoFilter 
/**
  * Class to convert two mono sources to stereo
 **/
{
public:
  MergeChannels(PClip _clip, int _num_children, PClip* _child_array, IScriptEnvironment* env);
  virtual ~MergeChannels()
   {if (tempbuffer_size) {delete[] tempbuffer;tempbuffer_size=0;}}

  void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env);
  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment*);

private:
  int* clip_channels;
  signed char** clip_offset;
  signed char *tempbuffer;
  int tempbuffer_size;
	int clip1_channels;

  const int num_children;
  PClip* child_array;
  PClip tclip;

  VideoInfo vi2;
};


class GetChannel : public GenericVideoFilter 
/**
  * Class to get left or right channel from stereo source
 **/
{
public:
  GetChannel(PClip _clip, int* _channel, int numchannels);
  virtual ~GetChannel()
   {if (tempbuffer_size) {delete[] tempbuffer;tempbuffer_size=0;}}

  void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env);
  static PClip Create_left(PClip clip);
  static PClip Create_right(PClip clip);
  static PClip Create_n(PClip clip, int* n, int numchannels);
  static AVSValue __cdecl Create_left(AVSValue args, void*, IScriptEnvironment*);
  static AVSValue __cdecl Create_right(AVSValue args, void*, IScriptEnvironment*);
  static AVSValue __cdecl Create_n(AVSValue args, void*, IScriptEnvironment* env);

private:
  char *tempbuffer;
  int tempbuffer_size;
	int* channel;
  int numchannels;
  int src_bps;
  int src_cbps;
  int dst_bps;
  int dst_cbps;
};

class KillAudio : public GenericVideoFilter 
/**
  * Removes audio from clip
 **/
{
public:
  KillAudio(PClip _clip);
  void __stdcall GetAudio(void* buf, int start, int count, IScriptEnvironment* env) {};
  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment*);
};


class DelayAudio : public GenericVideoFilter 
/**
  * Class to delay audio stream
 **/
{  
public:
  DelayAudio(double delay, PClip _child);
  void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env);

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);

private:
  const int delay_samples;
};



class Amplify : public GenericVideoFilter 
/**
  * Amplify a clip's audio track
 **/
{
public:
  Amplify(PClip _child, float* _volumes);
  void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env);

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);
  static AVSValue __cdecl Create_dB(AVSValue args, void*, IScriptEnvironment* env);


private:
  const float* volumes;

  static __inline short Saturate(int n) {
    if (n <= -32768) return -32768;
    if (n >= 32767) return 32767;
    return (short)n;
  }

  static __inline int Saturate_int32(__int64 n) {
    if (n <= MIN_INT) return MIN_INT;  
    if (n >= MAX_INT) return MAX_INT;
    return (int)n;
  }

 static __inline double dBtoScaleFactor(double dB)
 { return pow(10.0, dB/20.0);};
};





class Normalize : public GenericVideoFilter 
/**
  * Normalize a clip's audio track
 **/
{
public:
  Normalize(PClip _child, double _max_factor, bool _showvalues);
  void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);
  

private:
  float max_factor;
  float max_volume;
  bool showvalues;

  static __inline short Saturate(int n) {
    if (n <= -32768) return -32768;
    if (n >= 32767) return 32767;
    return (short)n;
  }

};

class MixAudio : public GenericVideoFilter 
/**
  * Mix audio from one clip into another.
 **/
{
public:
  MixAudio(PClip _child, PClip _clip, double _track1_factor, double _track2_factor, IScriptEnvironment* env);
  void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env);

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);
  virtual ~MixAudio() {if (tempbuffer_size) delete[] tempbuffer;tempbuffer_size=0;}


private:
  const int track1_factor, track2_factor;
	int tempbuffer_size;
  signed char *tempbuffer;
	PClip tclip,clip;

  static __inline short Saturate(int n) {
    if (n <= -32768) return -32768;
    if (n >= 32767) return 32767;
    return (short)n;
  }

 static __inline double dBtoScaleFactor(double dB)
 { return pow(10.0, dB/20.0);};
};


class ResampleAudio : public GenericVideoFilter 
/**
  * Class to resample the audio stream
 **/
{
public:
  ResampleAudio(PClip _child, int _target_rate, IScriptEnvironment* env);
  virtual ~ResampleAudio() 
    { if (srcbuffer) delete[] srcbuffer; }
  void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env);

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);

private:
  int FilterUD(short *Xp, short Ph, short Inc);

  enum { Nwing = 8192, Nmult = 65 };
  short Imp[Nwing+1];
  SFLOAT fImp[Nwing+1];
  const int target_rate;
  double factor;
  int Xoff, dtb, dhb;
  int LpScl;
  short* srcbuffer;
  int srcbuffer_size;
  bool skip_conversion;
};


#endif  // __Audio_H__

