// Avisynth v2.5.  Copyright 2002, 2005 Ben Rudiak-Gould et al.
// Avisynth v2.6.  Copyright 2006 Klaus Post.
// Avisynth v2.6.  Copyright 2009 Ian Brabham.
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


/*
Please NOTE! This version of avisynth.h DOES NOT have any special exemption!

         While this version is under development you are fully
       constrained by the terms of the GNU General Public License.

 Any derivative software you may publish MUST include the full source code.

    Normal licence conditions will be reapplied in a future version.
*/




#ifndef __AVISYNTH_H__
#define __AVISYNTH_H__

enum { AVISYNTH_INTERFACE_VERSION = 5 };


/* Define all types necessary for interfacing with avisynth.dll
   Moved from internal.h */

// Win32 API macros, notably the types BYTE, DWORD, ULONG, etc.
#include <windef.h>


// Raster types used by VirtualDub & Avisynth
#define in64 (__int64)(unsigned short)
typedef unsigned long	Pixel;    // this will break on 64-bit machines!
typedef unsigned long	Pixel32;
typedef unsigned char	Pixel8;
typedef long			PixCoord;
typedef long			PixDim;
typedef long			PixOffset;


/* Compiler-specific crap */

// Tell MSVC to stop precompiling here
#ifdef _MSC_VER
  #pragma hdrstop
#endif

// Set up debugging macros for MS compilers; for others, step down to the
// standard <assert.h> interface
#ifdef _MSC_VER
  #include <crtdbg.h>
#else
  #define _RPT0(a,b) ((void)0)
  #define _RPT1(a,b,c) ((void)0)
  #define _RPT2(a,b,c,d) ((void)0)
  #define _RPT3(a,b,c,d,e) ((void)0)
  #define _RPT4(a,b,c,d,e,f) ((void)0)

  #define _ASSERTE(x) assert(x)
  #include <assert.h>
#endif



// I had problems with Premiere wanting 1-byte alignment for its structures,
// so I now set the Avisynth struct alignment explicitly here.
#pragma pack(push,8)

#define FRAME_ALIGN 16
// Default frame alignment is 16 bytes, to help P4, when using SSE2

// The VideoInfo struct holds global information about a clip (i.e.
// information that does not depend on the frame number).  The GetVideoInfo
// method in IClip returns this struct.

// Audio Sample information
typedef float SFLOAT;

enum {SAMPLE_INT8  = 1<<0,
      SAMPLE_INT16 = 1<<1,
      SAMPLE_INT24 = 1<<2,    // Int24 is a very stupid thing to code, but it's supported by some hardware.
      SAMPLE_INT32 = 1<<3,
      SAMPLE_FLOAT = 1<<4};

enum {
   PLANAR_Y=1<<0,
   PLANAR_U=1<<1,
   PLANAR_V=1<<2,
   PLANAR_ALIGNED=1<<3,
   PLANAR_Y_ALIGNED=PLANAR_Y|PLANAR_ALIGNED,
   PLANAR_U_ALIGNED=PLANAR_U|PLANAR_ALIGNED,
   PLANAR_V_ALIGNED=PLANAR_V|PLANAR_ALIGNED,
   PLANAR_A=1<<4,
   PLANAR_R=1<<5,
   PLANAR_G=1<<6,
   PLANAR_B=1<<7,
   PLANAR_A_ALIGNED=PLANAR_A|PLANAR_ALIGNED,
   PLANAR_R_ALIGNED=PLANAR_R|PLANAR_ALIGNED,
   PLANAR_G_ALIGNED=PLANAR_G|PLANAR_ALIGNED,
   PLANAR_B_ALIGNED=PLANAR_B|PLANAR_ALIGNED,
  };

class AvisynthError /* exception */ {
public:
  const char* const msg;
  AvisynthError(const char* _msg) : msg(_msg) {}
}; // endclass AvisynthError

struct VideoInfo {
  int width, height;    // width=0 means no video
  unsigned fps_numerator, fps_denominator;
  int num_frames;
  // This is more extensible than previous versions. More properties can be added seeminglesly.

  // Colorspace properties.
/*
7<<0  Planar Width Subsampling bits
      Use (X+1) & 3 for GetPlaneWidthSubsampling
        000 => 1        YV12, YV16
        001 => 2        YV411, YUV9
        010 => reserved
        011 => 0        YV24
        1xx => reserved

1<<3  VPlaneFirst YV12, YV16, YV24, YV411, YUV9
1<<4  UPlaneFirst I420

7<<8  Planar Height Subsampling bits
      Use ((X>>8)+1) & 3 for GetPlaneHeightSubsampling
        000 => 1        YV12
        001 => 2        YUV9
        010 => reserved
        011 => 0        YV16, YV24, YV411
        1xx => reserved

7<<16 Sample resolution bits
        000 => 8
        001 => 16
        010 => 32
        011 => reserved
        1xx => reserved

Planar match mask  1111.0000.0000.0111.0000.0111.0000.0111
Planar signature   10xx.0000.0000.00xx.0000.00xx.00xx.00xx
Planar filter mask 1111.1111.1111.1111.1111.1111.1100.1111
*/
  enum {
    CS_BGR = 1<<28,
    CS_YUV = 1<<29,
    CS_INTERLEAVED = 1<<30,
    CS_PLANAR = 1<<31,

    CS_Shift_Sub_Width   =  0,
    CS_Shift_Sub_Height  =  8,
    CS_Shift_Sample_Bits = 16,

    CS_Sub_Width_Mask    = 7 << CS_Shift_Sub_Width,
    CS_Sub_Width_1       = 3 << CS_Shift_Sub_Width, // YV24
    CS_Sub_Width_2       = 0 << CS_Shift_Sub_Width, // YV12, I420, YV16
    CS_Sub_Width_4       = 1 << CS_Shift_Sub_Width, // YUV9, YV411

    CS_VPlaneFirst       = 1 << 3, // YV12, YV16, YV24, YV411, YUV9
    CS_UPlaneFirst       = 1 << 4, // I420

    CS_Sub_Height_Mask   = 7 << CS_Shift_Sub_Height,
    CS_Sub_Height_1      = 3 << CS_Shift_Sub_Height, // YV16, YV24, YV411
    CS_Sub_Height_2      = 0 << CS_Shift_Sub_Height, // YV12, I420
    CS_Sub_Height_4      = 1 << CS_Shift_Sub_Height, // YUV9

    CS_Sample_Bits_Mask  = 7 << CS_Shift_Sample_Bits,
    CS_Sample_Bits_8     = 0 << CS_Shift_Sample_Bits,
    CS_Sample_Bits_16    = 1 << CS_Shift_Sample_Bits,
    CS_Sample_Bits_32    = 2 << CS_Shift_Sample_Bits,

    CS_PLANAR_MASK       = CS_PLANAR | CS_INTERLEAVED | CS_YUV | CS_BGR | CS_Sample_Bits_Mask | CS_Sub_Height_Mask | CS_Sub_Width_Mask,
    CS_PLANAR_FILTER     = ~( CS_VPlaneFirst | CS_UPlaneFirst ),

  // Specific colorformats
    CS_UNKNOWN = 0,
    CS_BGR24 = 1<<0 | CS_BGR | CS_INTERLEAVED,
    CS_BGR32 = 1<<1 | CS_BGR | CS_INTERLEAVED,
    CS_YUY2  = 1<<2 | CS_YUV | CS_INTERLEAVED,
//  CS_YV12  = 1<<3  Reserved
//  CS_I420  = 1<<4  Reserved
    CS_RAW32 = 1<<5 | CS_INTERLEAVED,

//  YV12 must be 0xA000008 2.5 Baked API will see all new planar as YV12
//  I420 must be 0xA000010

    CS_YV24  = CS_PLANAR | CS_YUV | CS_Sample_Bits_8 | CS_VPlaneFirst | CS_Sub_Height_1 | CS_Sub_Width_1,  // YUV 4:4:4 planar
    CS_YV16  = CS_PLANAR | CS_YUV | CS_Sample_Bits_8 | CS_VPlaneFirst | CS_Sub_Height_1 | CS_Sub_Width_2,  // YUV 4:2:2 planar
    CS_YV12  = CS_PLANAR | CS_YUV | CS_Sample_Bits_8 | CS_VPlaneFirst | CS_Sub_Height_2 | CS_Sub_Width_2,  // y-v-u, 4:2:0 planar
    CS_I420  = CS_PLANAR | CS_YUV | CS_Sample_Bits_8 | CS_UPlaneFirst | CS_Sub_Height_2 | CS_Sub_Width_2,  // y-u-v, 4:2:0 planar
    CS_IYUV  = CS_I420,
    CS_YUV9  = CS_PLANAR | CS_YUV | CS_Sample_Bits_8 | CS_VPlaneFirst | CS_Sub_Height_4 | CS_Sub_Width_4,  // YUV 4:1:0 planar
    CS_YV411 = CS_PLANAR | CS_YUV | CS_Sample_Bits_8 | CS_VPlaneFirst | CS_Sub_Height_1 | CS_Sub_Width_4,  // YUV 4:1:1 planar

    CS_Y8    = CS_PLANAR | CS_INTERLEAVED | CS_YUV | CS_Sample_Bits_8,                                     // Y   4:0:0 planar
/*
    CS_YV48  = CS_PLANAR | CS_YUV | CS_Sample_Bits_16 | CS_VPlaneFirst | CS_Sub_Height_1 | CS_Sub_Width_1, // YUV 4:4:4 16bit samples
    CS_Y16   = CS_PLANAR | CS_INTERLEAVED | CS_YUV | CS_Sample_Bits_16,                                    // Y   4:0:0 16bit samples

    CS_YV96  = CS_PLANAR | CS_YUV | CS_Sample_Bits_32 | CS_VPlaneFirst | CS_Sub_Height_1 | CS_Sub_Width_1, // YUV 4:4:4 32bit samples
    CS_Y32   = CS_PLANAR | CS_INTERLEAVED | CS_YUV | CS_Sample_Bits_32,                                    // Y   4:0:0 32bit samples

    CS_PRGB  = CS_PLANAR | CS_RGB | CS_Sample_Bits_8,                                                      // Planar RGB
    CS_RGB48 = CS_PLANAR | CS_RGB | CS_Sample_Bits_16,                                                     // Planar RGB 16bit samples
    CS_RGB96 = CS_PLANAR | CS_RGB | CS_Sample_Bits_32,                                                     // Planar RGB 32bit samples
*/
  };

  int pixel_type;                // changed to int as of 2.5


  int audio_samples_per_second;   // 0 means no audio
  int sample_type;                // as of 2.5
  __int64 num_audio_samples;      // changed as of 2.5
  int nchannels;                  // as of 2.5

  // Imagetype properties

  int image_type;

  enum {
    IT_BFF = 1<<0,
    IT_TFF = 1<<1,
    IT_FIELDBASED = 1<<2
  };

  // Chroma placement bits 20 -> 23  ::FIXME:: Really want a Class to support this
  enum {
    CS_UNKNOWN_CHROMA_PLACEMENT = 0 << 20,
    CS_MPEG1_CHROMA_PLACEMENT   = 1 << 20,
    CS_MPEG2_CHROMA_PLACEMENT   = 2 << 20,
    CS_YUY2_CHROMA_PLACEMENT    = 3 << 20,
    CS_TOPLEFT_CHROMA_PLACEMENT = 4 << 20
  };

  // useful functions of the above
  bool HasVideo() const;
  bool HasAudio() const;
  bool IsRGB() const;
  bool IsRGB24() const;
  bool IsRGB32() const;
  bool IsYUV() const;
  bool IsYUY2() const;

  bool IsYV24()  const;
  bool IsYV16()  const;
  bool IsYV12()  const;
  bool IsYV411() const;
//bool IsYUV9()  const;
  bool IsY8()    const;

  bool IsColorSpace(int c_space) const;

  bool Is(int property) const;
  bool IsPlanar() const;
  bool IsFieldBased() const;
  bool IsParityKnown() const;
  bool IsBFF() const;
  bool IsTFF() const;

  bool IsVPlaneFirst() const;  // Don't use this
  int BytesFromPixels(int pixels) const;   // Will not work on planar images, but will return only luma planes
  int RowSize(int plane=0) const;
  int BMPSize() const;

  __int64 AudioSamplesFromFrames(int frames) const;
  int FramesFromAudioSamples(__int64 samples) const;
  __int64 AudioSamplesFromBytes(__int64 bytes) const;
  __int64 BytesFromAudioSamples(__int64 samples) const;
  int AudioChannels() const;
  int SampleType() const;
  bool IsSampleType(int testtype) const;
  int SamplesPerSecond() const;
  int BytesPerAudioSample() const;
  void SetFieldBased(bool isfieldbased);
  void Set(int property);
  void Clear(int property);

  int GetPlaneWidthSubsampling(int plane) const;   // Subsampling in bitshifts!
  int GetPlaneHeightSubsampling(int plane) const;   // Subsampling in bitshifts!
  int BitsPerPixel() const;

  int BytesPerChannelSample() const;

  // useful mutator
  void SetFPS(unsigned numerator, unsigned denominator);

  // Range protected multiply-divide of FPS
  void MulDivFPS(unsigned multiplier, unsigned divisor);

  // Test for same colorspace
  bool IsSameColorspace(const VideoInfo& vi) const;

}; // endstruct VideoInfo




// VideoFrameBuffer holds information about a memory block which is used
// for video data.  For efficiency, instances of this class are not deleted
// when the refcount reaches zero; instead they're stored in a linked list
// to be reused.  The instances are deleted when the corresponding AVS
// file is closed.

class VideoFrameBuffer {
  BYTE* const data;
  const int data_size;
  // sequence_number is incremented every time the buffer is changed, so
  // that stale views can tell they're no longer valid.
  long sequence_number;

  friend class VideoFrame;
  friend class Cache;
  friend class ScriptEnvironment;
  long refcount;

public:
  VideoFrameBuffer(int size);
  VideoFrameBuffer();
  ~VideoFrameBuffer();

  const BYTE* GetReadPtr() const;
  BYTE* GetWritePtr();
  int GetDataSize();
  int GetSequenceNumber();
  int GetRefcount();
}; // endclass VideoFrameBuffer


class IClip;
class PClip;
class PVideoFrame;
class IScriptEnvironment;
class AVSValue;


// VideoFrame holds a "window" into a VideoFrameBuffer.  Operator new
// is overloaded to recycle class instances.

class VideoFrame {
  long refcount;
  VideoFrameBuffer* const vfb;
  const int offset, pitch, row_size, height, offsetU, offsetV, pitchUV;  // U&V offsets are from top of picture.
  const int row_sizeUV, heightUV;

  friend class PVideoFrame;
  void AddRef();
  void Release();

  friend class ScriptEnvironment;
  friend class Cache;

  VideoFrame(VideoFrameBuffer* _vfb, int _offset, int _pitch, int _row_size, int _height);
  VideoFrame(VideoFrameBuffer* _vfb, int _offset, int _pitch, int _row_size, int _height, int _offsetU, int _offsetV, int _pitchUV, int _row_sizeUV, int _heightUV);

  void* operator new(unsigned size);
// TESTME: OFFSET U/V may be switched to what could be expected from AVI standard!
public:
  int GetPitch(int plane=0) const;
  int GetRowSize(int plane=0) const;
  int GetHeight(int plane=0) const;

  // generally you shouldn't use these three
  VideoFrameBuffer* GetFrameBuffer() const;
  int GetOffset(int plane=0) const;

  // in plugins use env->SubFrame() -- because implementation code is only available inside avisynth.dll. Doh!
  VideoFrame* Subframe(int rel_offset, int new_pitch, int new_row_size, int new_height) const;
  VideoFrame* Subframe(int rel_offset, int new_pitch, int new_row_size, int new_height, int rel_offsetU, int rel_offsetV, int pitchUV) const;

  const BYTE* GetReadPtr(int plane=0) const;
  bool IsWritable() const;
  BYTE* GetWritePtr(int plane=0) const;

  ~VideoFrame();
}; // endclass VideoFrame

enum {
  CACHE_NOTHING=0,
  CACHE_RANGE=1,
  CACHE_ALL=2,
  CACHE_AUDIO=3,
  CACHE_AUDIO_NONE=4,
  CACHE_AUDIO_AUTO=5
 };

// Base class for all filters.
class IClip {
  friend class PClip;
  friend class AVSValue;
  long refcnt;
  void AddRef();
  void Release();
public:
  IClip();
  virtual int __stdcall GetVersion();
  virtual PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env) = 0;
  virtual bool __stdcall GetParity(int n) = 0;  // return field parity if field_based, else parity of first field in frame
  virtual void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) = 0;  // start and count are in samples
  virtual void __stdcall SetCacheHints(int cachehints,int frame_range) = 0 ;  // We do not pass cache requests upwards, only to the next filter.
  virtual const VideoInfo& __stdcall GetVideoInfo() = 0;
  virtual __stdcall ~IClip();
}; // endclass IClip


// smart pointer to IClip
class PClip {

  IClip* p;

  IClip* GetPointerWithAddRef() const;
  friend class AVSValue;
  friend class VideoFrame;

  void Init(IClip* x);
  void Set(IClip* x);

public:
  PClip();
  PClip(const PClip& x);
  PClip(IClip* x);
  void operator=(IClip* x);
  void operator=(const PClip& x);

  IClip* operator->() const;

  // useful in conditional expressions
  operator void*() const;
  bool operator!() const;

  ~PClip();
}; // endclass PClip


// smart pointer to VideoFrame
class PVideoFrame {

  VideoFrame* p;

  void Init(VideoFrame* x);
  void Set(VideoFrame* x);

public:
  PVideoFrame();
  PVideoFrame(const PVideoFrame& x);
  PVideoFrame(VideoFrame* x);
  void operator=(VideoFrame* x);
  void operator=(const PVideoFrame& x);

  VideoFrame* operator->() const;

  // for conditional expressions
  operator void*() const;
  bool operator!() const;

  ~PVideoFrame();
}; // endclass PVideoFrame


class AVSValue {
public:

  AVSValue();
  AVSValue(IClip* c);
  AVSValue(const PClip& c);
  AVSValue(bool b);
  AVSValue(int i);
//  AVSValue(__int64 l);
  AVSValue(float f);
  AVSValue(double f);
  AVSValue(const char* s);
  AVSValue(const AVSValue* a, int size);
  AVSValue(const AVSValue& v);

  ~AVSValue();
  AVSValue& operator=(const AVSValue& v);

  // Note that we transparently allow 'int' to be treated as 'float'.
  // There are no int<->bool conversions, though.

  bool Defined() const;
  bool IsClip() const;
  bool IsBool() const;
  bool IsInt() const;
//  bool IsLong() const;
  bool IsFloat() const;
  bool IsString() const;
  bool IsArray() const;

  PClip AsClip() const;
  bool AsBool() const;
  int AsInt() const;
//  int AsLong() const;
  const char* AsString() const;
  float AsFloat() const;

  bool AsBool(bool def) const;
  int AsInt(int def) const;
  double AsDblDef(double def) const; // Value is still a float
//float AsFloat(double def) const; // def demoted to a float
  float AsFloat(float def) const;
  const char* AsString(const char* def) const;

  int ArraySize() const;

  const AVSValue& operator[](int index) const;

private:

  short type;  // 'a'rray, 'c'lip, 'b'ool, 'i'nt, 'f'loat, 's'tring, 'v'oid, or 'l'ong
  short array_size;
  union {
    IClip* clip;
    bool boolean;
    int integer;
    float floating_pt;
    const char* string;
    const AVSValue* array;
//    __int64 longlong;
  };

  void Assign(const AVSValue* src, bool init);
}; // endclass AVSValue


// instantiable null filter
class GenericVideoFilter : public IClip {
protected:
  PClip child;
  VideoInfo vi;
public:
  GenericVideoFilter(PClip _child) : child(_child) { vi = child->GetVideoInfo(); }
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env) { return child->GetFrame(n, env); }
  void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) { child->GetAudio(buf, start, count, env); }
  const VideoInfo& __stdcall GetVideoInfo() { return vi; }
  bool __stdcall GetParity(int n) { return child->GetParity(n); }
  void __stdcall SetCacheHints(int cachehints,int frame_range) { } ;  // We do not pass cache requests upwards, only to the next filter.
};





/* Helper classes useful to plugin authors */ // But we don't export the entry points, Doh!

class AlignPlanar : public GenericVideoFilter
{
public:
  AlignPlanar(PClip _clip);
  static PClip Create(PClip clip);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};



class FillBorder : public GenericVideoFilter
{
public:
  FillBorder(PClip _clip);
  static PClip Create(PClip clip);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};



class ConvertAudio : public GenericVideoFilter
/**
  * Helper class to convert audio to any format
 **/
{
public:
  ConvertAudio(PClip _clip, int prefered_format);
  void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env);
  void __stdcall SetCacheHints(int cachehints,int frame_range);  // We do pass cache requests upwards, to the cache!

  static PClip Create(PClip clip, int sample_type, int prefered_type);
  static AVSValue __cdecl Create_float(AVSValue args, void*, IScriptEnvironment*);
  static AVSValue __cdecl Create_32bit(AVSValue args, void*, IScriptEnvironment*);
  static AVSValue __cdecl Create_24bit(AVSValue args, void*, IScriptEnvironment*);
  static AVSValue __cdecl Create_16bit(AVSValue args, void*, IScriptEnvironment*);
  static AVSValue __cdecl Create_8bit (AVSValue args, void*, IScriptEnvironment*);
  static AVSValue __cdecl Create_Any  (AVSValue args, void*, IScriptEnvironment*);
  virtual ~ConvertAudio();

private:
  void convertToFloat(char* inbuf, float* outbuf, char sample_type, int count);
  void convertToFloat_3DN(char* inbuf, float* outbuf, char sample_type, int count);
  void convertToFloat_SSE(char* inbuf, float* outbuf, char sample_type, int count);
  void convertToFloat_SSE2(char* inbuf, float* outbuf, char sample_type, int count);
  void convertFromFloat(float* inbuf, void* outbuf, char sample_type, int count);
  void convertFromFloat_3DN(float* inbuf, void* outbuf, char sample_type, int count);
  void convertFromFloat_SSE(float* inbuf, void* outbuf, char sample_type, int count);
  void convertFromFloat_SSE2(float* inbuf, void* outbuf, char sample_type, int count);

  __inline int Saturate_int8(float n);
  __inline short Saturate_int16(float n);
  __inline int Saturate_int24(float n);
  __inline int Saturate_int32(float n);

  char src_format;
  char dst_format;
  int src_bps;
  char *tempbuffer;
  SFLOAT *floatbuffer;
  int tempbuffer_size;
};


// For GetCPUFlags.  These are backwards-compatible with those in VirtualDub.
enum {
                    /* slowest CPU to support extension */
  CPUF_FORCE        =  0x01,   //  N/A
  CPUF_FPU          =  0x02,   //  386/486DX
  CPUF_MMX          =  0x04,   //  P55C, K6, PII
  CPUF_INTEGER_SSE  =  0x08,   //  PIII, Athlon
  CPUF_SSE          =  0x10,   //  PIII, Athlon XP/MP
  CPUF_SSE2         =  0x20,   //  PIV, Hammer
  CPUF_3DNOW        =  0x40,   //  K6-2
  CPUF_3DNOW_EXT    =  0x80,   //  Athlon
  CPUF_X86_64       =  0xA0,   //  Hammer (note: equiv. to 3DNow + SSE2, which
                               //          only Hammer will have anyway)
  CPUF_SSE3         = 0x100,   //  PIV+, Hammer
  CPUF_SSSE3        = 0x200,   //  Core 2
  CPUF_SSE4			= 0x400,   //  Penryn, Wolfdale, Yorkfield
  CPUF_SSE4_1		= 0x400,
  CPUF_SSE4_2		= 0x800,   //  Nehalem
};
#define MAX_INT 0x7fffffff
#define MIN_INT -0x7fffffff  // ::FIXME:: research why this is not 0x80000000



class IScriptEnvironment {
public:
  virtual __stdcall ~IScriptEnvironment() {}

  virtual /*static*/ long __stdcall GetCPUFlags() = 0;

  virtual char* __stdcall SaveString(const char* s, int length = -1) = 0;
  virtual char* __stdcall Sprintf(const char* fmt, ...) = 0;
  // note: val is really a va_list; I hope everyone typedefs va_list to a pointer
  virtual char* __stdcall VSprintf(const char* fmt, void* val) = 0;

  __declspec(noreturn) virtual void __stdcall ThrowError(const char* fmt, ...) = 0;

  class NotFound /*exception*/ {};  // thrown by Invoke and GetVar

  typedef AVSValue (__cdecl *ApplyFunc)(AVSValue args, void* user_data, IScriptEnvironment* env);

  virtual void __stdcall AddFunction(const char* name, const char* params, ApplyFunc apply, void* user_data) = 0;
  virtual bool __stdcall FunctionExists(const char* name) = 0;
  virtual AVSValue __stdcall Invoke(const char* name, const AVSValue args, const char** arg_names=0) = 0;

  virtual AVSValue __stdcall GetVar(const char* name) = 0;
  virtual bool __stdcall SetVar(const char* name, const AVSValue& val) = 0;
  virtual bool __stdcall SetGlobalVar(const char* name, const AVSValue& val) = 0;

  virtual void __stdcall PushContext(int level=0) = 0;
  virtual void __stdcall PopContext() = 0;

  // align should be 4 or 8
  virtual PVideoFrame __stdcall NewVideoFrame(const VideoInfo& vi, int align=FRAME_ALIGN) = 0;

  virtual bool __stdcall MakeWritable(PVideoFrame* pvf) = 0;

  virtual /*static*/ void __stdcall BitBlt(BYTE* dstp, int dst_pitch, const BYTE* srcp, int src_pitch, int row_size, int height) = 0;

  typedef void (__cdecl *ShutdownFunc)(void* user_data, IScriptEnvironment* env);
  virtual void __stdcall AtExit(ShutdownFunc function, void* user_data) = 0;

  virtual void __stdcall CheckVersion(int version = AVISYNTH_INTERFACE_VERSION) = 0;

  virtual PVideoFrame __stdcall Subframe(PVideoFrame src, int rel_offset, int new_pitch, int new_row_size, int new_height) = 0;

  virtual int __stdcall SetMemoryMax(int mem) = 0;

  virtual int __stdcall SetWorkingDir(const char * newdir) = 0;

  virtual void* __stdcall ManageCache(int key, void* data) = 0;

  enum PlanarChromaAlignmentMode {
			PlanarChromaAlignmentOff,
			PlanarChromaAlignmentOn,
			PlanarChromaAlignmentTest };

  virtual bool __stdcall PlanarChromaAlignment(PlanarChromaAlignmentMode key) = 0;

  virtual PVideoFrame __stdcall SubframePlanar(PVideoFrame src, int rel_offset, int new_pitch, int new_row_size, int new_height, int rel_offsetU, int rel_offsetV, int new_pitchUV) = 0;
}; // endclass IScriptEnvironment


// avisynth.dll exports this; it's a way to use it as a library, without
// writing an AVS script or without going through AVIFile.
IScriptEnvironment* __stdcall CreateScriptEnvironment(int version = AVISYNTH_INTERFACE_VERSION);


#pragma pack(pop)

#endif //__AVISYNTH_H__
