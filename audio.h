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

#ifndef __Audio_H__
#define __Audio_H__

#include "internal.h"
#include "text-overlay.h"




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


class ConvertAudioTo16bit : public GenericVideoFilter 
/**
  * Class to convert audio to 16-bit
 **/
{
public:
  ConvertAudioTo16bit(PClip _clip);
  void __stdcall GetAudio(void* buf, int start, int count, IScriptEnvironment* env);

  static PClip Create(PClip clip);
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

  void __stdcall GetAudio(void* buf, int start, int count, IScriptEnvironment* env);
  static PClip Create(PClip clip);
  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment*);

private:
  signed short *tempbuffer;
  int tempbuffer_size;
};

class EnsureVBRMP3Sync : public GenericVideoFilter 
/**
  * Class to convert audio to mono
 **/
{
public:
  EnsureVBRMP3Sync(PClip _clip);

  void __stdcall GetAudio(void* buf, int start, int count, IScriptEnvironment* env);
  static PClip Create(PClip clip);
  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment*);

private:
  int last_end;
};

class MonoToStereo : public GenericVideoFilter 
/**
  * Class to convert two mono sources to stereo
 **/
{
public:
  MonoToStereo(PClip _child,PClip _clip, IScriptEnvironment* env);
  virtual ~MonoToStereo()
  {if (tempbuffer_size) {delete[] tempbuffer;tempbuffer_size=0;}}

  void __stdcall GetAudio(void* buf, int start, int count, IScriptEnvironment* env);
  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment*);

private:
	PClip right;
  signed short *tempbuffer;
  int tempbuffer_size;
	bool left_stereo,right_stereo;
};


class GetChannel : public GenericVideoFilter 
/**
  * Class to get left or right channel from stereo source
 **/
{
public:
  GetChannel(PClip _clip, bool _left);
  virtual ~GetChannel()
   {if (tempbuffer_size) {delete[] tempbuffer;tempbuffer_size=0;}}

  void __stdcall GetAudio(void* buf, int start, int count, IScriptEnvironment* env);
  static PClip Create_left(PClip clip);
  static PClip Create_right(PClip clip);
  static AVSValue __cdecl Create_left(AVSValue args, void*, IScriptEnvironment*);
  static AVSValue __cdecl Create_right(AVSValue args, void*, IScriptEnvironment*);

private:
  signed short *tempbuffer;
  int tempbuffer_size;
	bool left;
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

class AssumeRate : public GenericVideoFilter 
/**
  * Changes the sample rate of a clip
 **/
{
public:
  AssumeRate(PClip _clip, int _rate);
  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment*);
};

class DelayAudio : public GenericVideoFilter 
/**
  * Class to delay audio stream
 **/
{  
public:
  DelayAudio(double delay, PClip _child);
  virtual void __stdcall GetAudio(void* buf, int start, int count, IScriptEnvironment* env);

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
  Amplify(PClip _child, double _left_factor, double _right_factor);
  void __stdcall GetAudio(void* buf, int start, int count, IScriptEnvironment* env);

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);
  static AVSValue __cdecl Create_dB(AVSValue args, void*, IScriptEnvironment* env);


private:
  const int left_factor, right_factor;

  static __inline short Saturate(int n) {
    if (n <= -32768) return -32768;
    if (n >= 32767) return 32767;
    return (short)n;
  }

 static __inline double dBtoScaleFactor(double dB)
 { return pow(10.0, dB/20.0); };
};


class FilterAudio : public GenericVideoFilter 
/**
  * FilterAudio a clip's audio track
 **/
{
public:
  FilterAudio(PClip _child, int _cutoff, float _rez, int lowpass);
  void __stdcall GetAudio(void* buf, int start, int count, IScriptEnvironment* env);
  virtual ~FilterAudio()
  {if (tempbuffer_size) {delete[] tempbuffer;tempbuffer_size=0;}}

  static AVSValue __cdecl Create_LowPass(AVSValue args, void*, IScriptEnvironment* env);
  static AVSValue __cdecl Create_HighPass(AVSValue args, void*, IScriptEnvironment* env);
  static AVSValue __cdecl Create_LowPassALT(AVSValue args, void*, IScriptEnvironment* env);


private:
  signed short *tempbuffer;
  int tempbuffer_size;
  int cutoff;
  float rez;
  int lowpass;
//algo 1:
  int lastsample;
  signed short last_4; 
  signed short last_3; 
  signed short last_2; 
  signed short last_1; 
//algo 2:
  float l_vibrapos;
  float l_vibraspeed; 
  float r_vibrapos;
  float r_vibraspeed; 

  static __inline short Saturate(int n) {
    if (n <= -32768) return -32768;
    if (n >= 32767) return 32767;
    return (short)n;
  }

};


class Normalize : public GenericVideoFilter 
/**
  * Normalize a clip's audio track
 **/
{
public:
  Normalize(PClip _child, double _left_factor, double _right_factor, bool showvalues);
  void __stdcall GetAudio(void* buf, int start, int count, IScriptEnvironment* env);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);
  static AVSValue __cdecl Create_dB(AVSValue args, void*, IScriptEnvironment* env);
  

private:
  int left_factor, right_factor;
  int max_volume;
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
  void __stdcall GetAudio(void* buf, int start, int count, IScriptEnvironment* env);

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);
  virtual ~MixAudio() {delete[] tempbuffer;tempbuffer_size=0;}


private:
  const int track1_factor, track2_factor;
	int tempbuffer_size;
  signed short *tempbuffer;
	PClip clip;

  static __inline short Saturate(int n) {
    if (n <= -32768) return -32768;
    if (n >= 32767) return 32767;
    return (short)n;
  }

 static __inline double dBtoScaleFactor(double dB)
 { return pow(10.0, dB/20.0); }

};

class ResampleAudio : public GenericVideoFilter 
/**
  * Class to resample the audio stream
 **/
{
public:
  ResampleAudio(PClip _child, int _target_rate, IScriptEnvironment* env);
  virtual ~ResampleAudio() 
    { delete[] srcbuffer; }
  void __stdcall GetAudio(void* buf, int start, int count, IScriptEnvironment* env);

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);

private:
  int FilterUD(short *Xp, short Ph, short Inc);

  enum { Nwing = 8192, Nmult = 65 };
  short Imp[Nwing+1];
  const int target_rate;
  double factor;
  int Xoff, dtb, dhb;
  int LpScl;
  short* srcbuffer;
  int srcbuffer_size;
  int skip_conversion;
};


#endif  // __Audio_H__

