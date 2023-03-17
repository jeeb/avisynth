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

#include <avisynth.h>
#include <cmath>

// ------- Channels

// similarly to ffmpeg
#define AVS_CHANNEL_LAYOUT_MONO              (AvsChannelMask::MASK_SPEAKER_FRONT_CENTER)
#define AVS_CHANNEL_LAYOUT_STEREO            (AvsChannelMask::MASK_SPEAKER_FRONT_LEFT|AvsChannelMask::MASK_SPEAKER_FRONT_RIGHT)
#define AVS_CHANNEL_LAYOUT_2POINT1           (AVS_CHANNEL_LAYOUT_STEREO|AvsChannelMask::MASK_SPEAKER_LOW_FREQUENCY)
#define AVS_CHANNEL_LAYOUT_2_1               (AVS_CHANNEL_LAYOUT_STEREO|AvsChannelMask::MASK_SPEAKER_BACK_CENTER)
#define AVS_CHANNEL_LAYOUT_SURROUND          (AVS_CHANNEL_LAYOUT_STEREO|AvsChannelMask::MASK_SPEAKER_FRONT_CENTER)
#define AVS_CHANNEL_LAYOUT_3POINT1           (AVS_CHANNEL_LAYOUT_SURROUND|AvsChannelMask::MASK_SPEAKER_LOW_FREQUENCY)
#define AVS_CHANNEL_LAYOUT_4POINT0           (AVS_CHANNEL_LAYOUT_SURROUND|AvsChannelMask::MASK_SPEAKER_BACK_CENTER)
#define AVS_CHANNEL_LAYOUT_4POINT1           (AVS_CHANNEL_LAYOUT_4POINT0|AvsChannelMask::MASK_SPEAKER_LOW_FREQUENCY)
#define AVS_CHANNEL_LAYOUT_2_2               (AVS_CHANNEL_LAYOUT_STEREO|AvsChannelMask::MASK_SPEAKER_SIDE_LEFT|AvsChannelMask::MASK_SPEAKER_SIDE_RIGHT)
#define AVS_CHANNEL_LAYOUT_QUAD              (AVS_CHANNEL_LAYOUT_STEREO|AvsChannelMask::MASK_SPEAKER_BACK_LEFT|AvsChannelMask::MASK_SPEAKER_BACK_RIGHT)
#define AVS_CHANNEL_LAYOUT_5POINT0           (AVS_CHANNEL_LAYOUT_SURROUND|AvsChannelMask::MASK_SPEAKER_SIDE_LEFT|AvsChannelMask::MASK_SPEAKER_SIDE_RIGHT)
#define AVS_CHANNEL_LAYOUT_5POINT1           (AVS_CHANNEL_LAYOUT_5POINT0|AvsChannelMask::MASK_SPEAKER_LOW_FREQUENCY)
#define AVS_CHANNEL_LAYOUT_5POINT0_BACK      (AVS_CHANNEL_LAYOUT_SURROUND|AvsChannelMask::MASK_SPEAKER_BACK_LEFT|AvsChannelMask::MASK_SPEAKER_BACK_RIGHT)
#define AVS_CHANNEL_LAYOUT_5POINT1_BACK      (AVS_CHANNEL_LAYOUT_5POINT0_BACK|AvsChannelMask::MASK_SPEAKER_LOW_FREQUENCY)
#define AVS_CHANNEL_LAYOUT_6POINT0           (AVS_CHANNEL_LAYOUT_5POINT0|AvsChannelMask::MASK_SPEAKER_BACK_CENTER)
#define AVS_CHANNEL_LAYOUT_6POINT0_FRONT     (AVS_CHANNEL_LAYOUT_2_2|AvsChannelMask::MASK_SPEAKER_FRONT_LEFT_OF_CENTER|AvsChannelMask::MASK_SPEAKER_FRONT_RIGHT_OF_CENTER)
#define AVS_CHANNEL_LAYOUT_HEXAGONAL         (AVS_CHANNEL_LAYOUT_5POINT0_BACK|AvsChannelMask::MASK_SPEAKER_BACK_CENTER)
#define AVS_CHANNEL_LAYOUT_6POINT1           (AVS_CHANNEL_LAYOUT_5POINT1|AvsChannelMask::MASK_SPEAKER_BACK_CENTER)
#define AVS_CHANNEL_LAYOUT_6POINT1_BACK      (AVS_CHANNEL_LAYOUT_5POINT1_BACK|AvsChannelMask::MASK_SPEAKER_BACK_CENTER)
#define AVS_CHANNEL_LAYOUT_6POINT1_FRONT     (AVS_CHANNEL_LAYOUT_6POINT0_FRONT|AvsChannelMask::MASK_SPEAKER_LOW_FREQUENCY)
#define AVS_CHANNEL_LAYOUT_7POINT0           (AVS_CHANNEL_LAYOUT_5POINT0|AvsChannelMask::MASK_SPEAKER_BACK_LEFT|AvsChannelMask::MASK_SPEAKER_BACK_RIGHT)
#define AVS_CHANNEL_LAYOUT_7POINT0_FRONT     (AVS_CHANNEL_LAYOUT_5POINT0|AvsChannelMask::MASK_SPEAKER_FRONT_LEFT_OF_CENTER|AvsChannelMask::MASK_SPEAKER_FRONT_RIGHT_OF_CENTER)
#define AVS_CHANNEL_LAYOUT_7POINT1           (AVS_CHANNEL_LAYOUT_5POINT1|AvsChannelMask::MASK_SPEAKER_BACK_LEFT|AvsChannelMask::MASK_SPEAKER_BACK_RIGHT)
#define AVS_CHANNEL_LAYOUT_7POINT1_WIDE      (AVS_CHANNEL_LAYOUT_5POINT1|AvsChannelMask::MASK_SPEAKER_FRONT_LEFT_OF_CENTER|AvsChannelMask::MASK_SPEAKER_FRONT_RIGHT_OF_CENTER)
#define AVS_CHANNEL_LAYOUT_7POINT1_WIDE_BACK (AVS_CHANNEL_LAYOUT_5POINT1_BACK|AvsChannelMask::MASK_SPEAKER_FRONT_LEFT_OF_CENTER|AvsChannelMask::MASK_SPEAKER_FRONT_RIGHT_OF_CENTER)
#define AVS_CHANNEL_LAYOUT_7POINT1_TOP_BACK  (AVS_CHANNEL_LAYOUT_5POINT1_BACK|AvsChannelMask::MASK_SPEAKER_TOP_FRONT_LEFT|AvsChannelMask::MASK_SPEAKER_TOP_FRONT_RIGHT)
#define AVS_CHANNEL_LAYOUT_OCTAGONAL         (AVS_CHANNEL_LAYOUT_5POINT0|AvsChannelMask::MASK_SPEAKER_BACK_LEFT|AvsChannelMask::MASK_SPEAKER_BACK_CENTER|AvsChannelMask::MASK_SPEAKER_BACK_RIGHT)
#define AVS_CHANNEL_LAYOUT_CUBE              (AVS_CHANNEL_LAYOUT_QUAD|AvsChannelMask::MASK_SPEAKER_TOP_FRONT_LEFT|AvsChannelMask::MASK_SPEAKER_TOP_FRONT_RIGHT|AvsChannelMask::MASK_SPEAKER_TOP_BACK_LEFT|AvsChannelMask::MASK_SPEAKER_TOP_BACK_RIGHT)
//#define AVS_CHANNEL_LAYOUT_HEXADECAGONAL     (AVS_CHANNEL_LAYOUT_OCTAGONAL|AvsChannelMask::MASK_SPEAKER_WIDE_LEFT|AvsChannelMask::MASK_SPEAKER_WIDE_RIGHT|AvsChannelMask::MASK_SPEAKER_TOP_BACK_LEFT|AvsChannelMask::MASK_SPEAKER_TOP_BACK_RIGHT|AvsChannelMask::MASK_SPEAKER_TOP_BACK_CENTER|AvsChannelMask::MASK_SPEAKER_TOP_FRONT_CENTER|AvsChannelMask::MASK_SPEAKER_TOP_FRONT_LEFT|AvsChannelMask::MASK_SPEAKER_TOP_FRONT_RIGHT)
//#define AVS_CHANNEL_LAYOUT_STEREO_DOWNMIX    (AvsChannelMask::MASK_SPEAKER_STEREO_LEFT|AvsChannelMask::MASK_SPEAKER_STEREO_RIGHT)

typedef struct ChannelLayoutDescriptor_t {
  int nb_channels;
  unsigned int mask;
} ChannelLayoutDescriptor_t;

#define AVS_CHANNEL_LAYOUT_MASK(nb, m) \
     { (nb), (m) }

// Common pre-defined channel layouts
#define AVS_CHANNEL_LAYOUT_MASK_MONO              AVS_CHANNEL_LAYOUT_MASK(1,  AVS_CHANNEL_LAYOUT_MONO)
#define AVS_CHANNEL_LAYOUT_MASK_STEREO            AVS_CHANNEL_LAYOUT_MASK(2,  AVS_CHANNEL_LAYOUT_STEREO)
#define AVS_CHANNEL_LAYOUT_MASK_2POINT1           AVS_CHANNEL_LAYOUT_MASK(3,  AVS_CHANNEL_LAYOUT_2POINT1)
#define AVS_CHANNEL_LAYOUT_MASK_2_1               AVS_CHANNEL_LAYOUT_MASK(3,  AVS_CHANNEL_LAYOUT_2_1)
#define AVS_CHANNEL_LAYOUT_MASK_SURROUND          AVS_CHANNEL_LAYOUT_MASK(3,  AVS_CHANNEL_LAYOUT_SURROUND)
#define AVS_CHANNEL_LAYOUT_MASK_3POINT1           AVS_CHANNEL_LAYOUT_MASK(4,  AVS_CHANNEL_LAYOUT_3POINT1)
#define AVS_CHANNEL_LAYOUT_MASK_4POINT0           AVS_CHANNEL_LAYOUT_MASK(4,  AVS_CHANNEL_LAYOUT_4POINT0)
#define AVS_CHANNEL_LAYOUT_MASK_4POINT1           AVS_CHANNEL_LAYOUT_MASK(5,  AVS_CHANNEL_LAYOUT_4POINT1)
#define AVS_CHANNEL_LAYOUT_MASK_2_2               AVS_CHANNEL_LAYOUT_MASK(4,  AVS_CHANNEL_LAYOUT_2_2)
#define AVS_CHANNEL_LAYOUT_MASK_QUAD              AVS_CHANNEL_LAYOUT_MASK(4,  AVS_CHANNEL_LAYOUT_QUAD)
#define AVS_CHANNEL_LAYOUT_MASK_5POINT0           AVS_CHANNEL_LAYOUT_MASK(5,  AVS_CHANNEL_LAYOUT_5POINT0)
#define AVS_CHANNEL_LAYOUT_MASK_5POINT1           AVS_CHANNEL_LAYOUT_MASK(6,  AVS_CHANNEL_LAYOUT_5POINT1)
#define AVS_CHANNEL_LAYOUT_MASK_5POINT0_BACK      AVS_CHANNEL_LAYOUT_MASK(5,  AVS_CHANNEL_LAYOUT_5POINT0_BACK)
#define AVS_CHANNEL_LAYOUT_MASK_5POINT1_BACK      AVS_CHANNEL_LAYOUT_MASK(6,  AVS_CHANNEL_LAYOUT_5POINT1_BACK)
#define AVS_CHANNEL_LAYOUT_MASK_6POINT0           AVS_CHANNEL_LAYOUT_MASK(6,  AVS_CHANNEL_LAYOUT_6POINT0)
#define AVS_CHANNEL_LAYOUT_MASK_6POINT0_FRONT     AVS_CHANNEL_LAYOUT_MASK(6,  AVS_CHANNEL_LAYOUT_6POINT0_FRONT)
#define AVS_CHANNEL_LAYOUT_MASK_HEXAGONAL         AVS_CHANNEL_LAYOUT_MASK(6,  AVS_CHANNEL_LAYOUT_HEXAGONAL)
#define AVS_CHANNEL_LAYOUT_MASK_6POINT1           AVS_CHANNEL_LAYOUT_MASK(7,  AVS_CHANNEL_LAYOUT_6POINT1)
#define AVS_CHANNEL_LAYOUT_MASK_6POINT1_BACK      AVS_CHANNEL_LAYOUT_MASK(7,  AVS_CHANNEL_LAYOUT_6POINT1_BACK)
#define AVS_CHANNEL_LAYOUT_MASK_6POINT1_FRONT     AVS_CHANNEL_LAYOUT_MASK(7,  AVS_CHANNEL_LAYOUT_6POINT1_FRONT)
#define AVS_CHANNEL_LAYOUT_MASK_7POINT0           AVS_CHANNEL_LAYOUT_MASK(7,  AVS_CHANNEL_LAYOUT_7POINT0)
#define AVS_CHANNEL_LAYOUT_MASK_7POINT0_FRONT     AVS_CHANNEL_LAYOUT_MASK(7,  AVS_CHANNEL_LAYOUT_7POINT0_FRONT)
#define AVS_CHANNEL_LAYOUT_MASK_7POINT1           AVS_CHANNEL_LAYOUT_MASK(8,  AVS_CHANNEL_LAYOUT_7POINT1)
#define AVS_CHANNEL_LAYOUT_MASK_7POINT1_WIDE      AVS_CHANNEL_LAYOUT_MASK(8,  AVS_CHANNEL_LAYOUT_7POINT1_WIDE)
#define AVS_CHANNEL_LAYOUT_MASK_7POINT1_WIDE_BACK AVS_CHANNEL_LAYOUT_MASK(8,  AVS_CHANNEL_LAYOUT_7POINT1_WIDE_BACK)
#define AVS_CHANNEL_LAYOUT_MASK_7POINT1_TOP_BACK  AVS_CHANNEL_LAYOUT_MASK(8,  AVS_CHANNEL_LAYOUT_7POINT1_TOP_BACK)
#define AVS_CHANNEL_LAYOUT_MASK_OCTAGONAL         AVS_CHANNEL_LAYOUT_MASK(8,  AVS_CHANNEL_LAYOUT_OCTAGONAL)
#define AVS_CHANNEL_LAYOUT_MASK_CUBE              AVS_CHANNEL_LAYOUT_MASK(8,  AVS_CHANNEL_LAYOUT_CUBE)
//#define AVS_CHANNEL_LAYOUT_MASK_HEXADECAGONAL     AVS_CHANNEL_LAYOUT_MASK(16, AVS_CHANNEL_LAYOUT_HEXADECAGONAL)

unsigned int av_get_channel_layout(const char* name);
unsigned int GetDefaultChannelLayout(int nChannels);


/******* Helper stuff *******/

#define MAX_SHORT (32767)
#define MIN_SHORT (-32768)


/* Conversion constants */    // We are using fixed point binary arithmetic here
#define Nhc       8           // Number of bits for filter points
#define Na        7           // Number of bits for interpolating between filter points
#define Np       (Nhc+Na)     // Total number of bits to represent fractional phase
#define Npc      (1<<Nhc)     // Sampling period binary point scaling factor
#define Amask    ((1<<Na)-1)  // Interpolation mask
#define Pmask    ((1<<Np)-1)  // Phase mask
#define Nh       16           // Number of bits to represent coefficients
#define Nb       16           // Unused
#define Nhxn     14           // Number of non-guard bits
#define Nhg      (Nh-Nhxn)    // Number of guard bits
#define NLpScl   13           // Number of overflow bits


#define IzeroEPSILON 1E-21               /* Max error acceptable in Izero */

static const double PI = 3.14159265358979323846;

static __inline short IntToShort(int v, const int scl)
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
static int makeFilter(short   Imp[], double &dLpScl, unsigned short Nwing, double Froll, double Beta);
static int makeFilter(SFLOAT fImp[], double  dLpScl, unsigned short Nwing, double Froll, double Beta);


/********************************************************************
********************************************************************/

class AssumeRate : public NonCachedGenericVideoFilter
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

  void __stdcall GetAudio(void* buf, int64_t start, int64_t count, IScriptEnvironment* env);
  int __stdcall SetCacheHints(int cachehints, int frame_range);
  static PClip Create(PClip clip);
  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment*);

private:
  char *tempbuffer;
  int tempbuffer_size;
  int channels;
};

class EnsureVBRMP3Sync : public GenericVideoFilter
/**
  * Ensure VBR mp3 sync, by always reading audio sequencially.
 **/
{
public:
  EnsureVBRMP3Sync(PClip _clip);

  void __stdcall GetAudio(void* buf, int64_t start, int64_t count, IScriptEnvironment* env);
  int __stdcall SetCacheHints(int cachehints, int frame_range);
  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);

private:
  int64_t last_end;
};

class MergeChannels : public GenericVideoFilter
/**
  * Class to convert two mono sources to stereo
 **/
{
public:
  MergeChannels(PClip _clip, int _num_children, PClip* _child_array, IScriptEnvironment* env);
  ~MergeChannels();

  void __stdcall GetAudio(void* buf, int64_t start, int64_t count, IScriptEnvironment* env);
  int __stdcall SetCacheHints(int cachehints, int frame_range);
  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment*);

private:
  int* clip_channels;
  signed char** clip_offset;
  signed char *tempbuffer;
  PClip* child_array;
  const int num_children;
  int tempbuffer_size;
};


class GetChannel : public GenericVideoFilter
/**
  * Class to get left or right channel from stereo source
 **/
{
public:
  GetChannel(PClip _clip, int* _channel, int numchannels);
  virtual ~GetChannel()
  {
    if (tempbuffer_size) {delete[] tempbuffer;tempbuffer_size=0;}
    if (channel)         {delete[] channel;   channel=0;        }
  }

  void __stdcall GetAudio(void* buf, int64_t start, int64_t count, IScriptEnvironment* env);
  int __stdcall SetCacheHints(int cachehints, int frame_range);
  static PClip Create_left(PClip clip);
  static PClip Create_right(PClip clip);
  static PClip Create_n(PClip clip, int* n, int numchannels);
  static AVSValue __cdecl Create_left(AVSValue args, void*, IScriptEnvironment*);
  static AVSValue __cdecl Create_right(AVSValue args, void*, IScriptEnvironment*);
  static AVSValue __cdecl Create_n(AVSValue args, void*, IScriptEnvironment* env);

private:
  char *tempbuffer;
  int* channel;
  int tempbuffer_size;
  int numchannels;
  int cbps;
  int src_bps;
  int dst_bps;
};

class KillVideo : public GenericVideoFilter
/**
  * Removes audio from clip
 **/
{
public:
  KillVideo(PClip _clip);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment*) { AVS_UNUSED(n); return NULL; };
  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment*);
};


class KillAudio : public NonCachedGenericVideoFilter
/**
  * Removes audio from clip
 **/
{
public:
  KillAudio(PClip _clip);
  void __stdcall GetAudio(void* buf, int64_t start, int64_t count, IScriptEnvironment* env) { AVS_UNUSED(buf); AVS_UNUSED(start); AVS_UNUSED(count); AVS_UNUSED(env); };
  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment*);
};

class SetChannelMask : public NonCachedGenericVideoFilter
  /**
    * Sets/remove Audio Channel Mask from VideoInfo
   **/
{
public:
  SetChannelMask(PClip _clip, bool IsChannelMaskKnown, unsigned int dwChannelMask);
  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment*);
};

class DelayAudio : public GenericVideoFilter
/**
  * Class to delay audio stream
 **/
{
public:
  DelayAudio(double delay, PClip _child);
  void __stdcall GetAudio(void* buf, int64_t start, int64_t count, IScriptEnvironment* env);

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);

private:
  const int64_t delay_samples;
};



class Amplify : public GenericVideoFilter
/**
  * Amplify a clip's audio track
 **/
{
public:
  Amplify(PClip _child, float* _volumes, int* _i_v);
  ~Amplify();
  void __stdcall GetAudio(void* buf, int64_t start, int64_t count, IScriptEnvironment* env);

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);
  static AVSValue __cdecl Create_dB(AVSValue args, void*, IScriptEnvironment* env);


private:
  const float* volumes;
  const int* i_v;

 static __inline float dBtoScaleFactorf(float dB)
 { return powf(10.0f, dB/20.0f);};
};





class Normalize : public GenericVideoFilter
/**
  * Normalize a clip's audio track
 **/
{
public:
  Normalize(PClip _child, float _max_factor, bool _showvalues);
  void __stdcall GetAudio(void* buf, int64_t start, int64_t count, IScriptEnvironment* env);
  int __stdcall SetCacheHints(int cachehints, int frame_range);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);


private:
  float max_factor;
  float max_volume;
  int   frameno;
  bool showvalues;
};

class MixAudio : public GenericVideoFilter
/**
  * Mix audio from one clip into another.
 **/
{
public:
  MixAudio(PClip _child, PClip _clip, double _track1_factor, double _track2_factor, IScriptEnvironment* env);
  void __stdcall GetAudio(void* buf, int64_t start, int64_t count, IScriptEnvironment* env);
  int __stdcall SetCacheHints(int cachehints, int frame_range);
  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);
  virtual ~MixAudio() {if (tempbuffer_size) delete[] tempbuffer;tempbuffer_size=0;}


private:
  PClip clip;
  signed char *tempbuffer;
  const int track1_factor, track2_factor;
  const float t1factor, t2factor;
  int tempbuffer_size;
};


class ResampleAudio : public GenericVideoFilter
/**
  * Class to resample the audio stream
 **/
{
public:
  ResampleAudio(PClip _child, int _target_rate_n, int _target_rate_d, IScriptEnvironment* env);
  virtual ~ResampleAudio()
    { delete[]  srcbuffer;
      delete[] fsrcbuffer; }
  void __stdcall GetAudio(void* buf, int64_t start, int64_t count, IScriptEnvironment* env);
  int __stdcall SetCacheHints(int cachehints, int frame_range);
  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);

  enum { Nwing = 8192, Nmult = 65 };   // Number of filter points, (Nwing>>Nhc)*2+1

private:
  int64_t FilterUD(short  *Xp, short Ph, short Inc);
  SFLOAT  FilterUD(SFLOAT *Xp, short Ph, short Inc);

  const double factor;
  int Xoff, dtb, dhb;
  unsigned dtbe;

  int LpScl, mLpScl, mNhg;

  short*   srcbuffer;
  SFLOAT* fsrcbuffer;

  int srcbuffer_size;
  bool skip_conversion;

  int64_t last_start, last_samples;

  union { // Share storage
  SFLOAT fImp[Nwing+1];
  short Imp[Nwing+1];
  };
};


#endif  // __Audio_H__
