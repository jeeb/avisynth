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

static inline short IntToShort(int v, int scl)
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
  {delete[] tempbuffer;tempbuffer_size=0;}

  void __stdcall GetAudio(void* buf, int start, int count, IScriptEnvironment* env);
  static PClip Create(PClip clip);
  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment*);

private:
  signed short *tempbuffer;
  int tempbuffer_size;
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

  static inline short Saturate(int n) {
    if (n <= -32768) return -32768;
    if (n >= 32767) return 32767;
    return (short)n;
  }

  static inline double dBtoScaleFactor(double dB) 
    { return pow(10.0, dB/10.0); }
};

class Normalize : public GenericVideoFilter 
/**
  * Normalize a clip's audio track
 **/
{
public:
  Normalize(PClip _child, double _left_factor, double _right_factor);
  void __stdcall GetAudio(void* buf, int start, int count, IScriptEnvironment* env);

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);
  static AVSValue __cdecl Create_dB(AVSValue args, void*, IScriptEnvironment* env);


private:
  int left_factor, right_factor;
  int max_volume;

  static inline short Saturate(int n) {
    if (n <= -32768) return -32768;
    if (n >= 32767) return 32767;
    return (short)n;
  }

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

