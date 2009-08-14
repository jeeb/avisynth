// Avisynth v2.5.  Copyright 2009 Ben Rudiak-Gould et al.
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

#include "stdafx.h"

#include "avisynth.h"


/**********************************************************************/

// struct VideoInfo

// useful functions of the above
bool VideoInfo::HasVideo() const { return (width!=0); }
bool VideoInfo::HasAudio() const { return (audio_samples_per_second!=0); }
bool VideoInfo::IsRGB() const { return !!(pixel_type&CS_BGR); }
bool VideoInfo::IsRGB24() const { return (pixel_type&CS_BGR24)==CS_BGR24; } // Clear out additional properties
bool VideoInfo::IsRGB32() const { return (pixel_type & CS_BGR32) == CS_BGR32 ; }
bool VideoInfo::IsYUV() const { return !!(pixel_type&CS_YUV ); }
bool VideoInfo::IsYUY2() const { return (pixel_type & CS_YUY2) == CS_YUY2; }
bool VideoInfo::IsYV12() const { return ((pixel_type & CS_YV12) == CS_YV12)||((pixel_type & CS_I420) == CS_I420); }
bool VideoInfo::IsColorSpace(int c_space) const { return ((pixel_type & c_space) == c_space); }
bool VideoInfo::Is(int property) const { return ((pixel_type & property)==property ); }
bool VideoInfo::IsPlanar() const { return !!(pixel_type & CS_PLANAR); }
bool VideoInfo::IsFieldBased() const { return !!(image_type & IT_FIELDBASED); }
bool VideoInfo::IsParityKnown() const { return ((image_type & IT_FIELDBASED)&&(image_type & (IT_BFF|IT_TFF))); }
bool VideoInfo::IsBFF() const { return !!(image_type & IT_BFF); }
bool VideoInfo::IsTFF() const { return !!(image_type & IT_TFF); }

bool VideoInfo::IsVPlaneFirst() const {return ((pixel_type & CS_YV12) == CS_YV12); }  // Don't use this
int VideoInfo::BytesFromPixels(int pixels) const { return pixels * (BitsPerPixel()>>3); }   // Will not work on planar images, but will return only luma planes
int VideoInfo::RowSize() const { return BytesFromPixels(width); }  // Also only returns first plane on planar images
int VideoInfo::BMPSize() const { if (IsPlanar()) {int p = height * ((RowSize()+3) & ~3); p+=p>>1; return p;  } return height * ((RowSize()+3) & ~3); }
__int64 VideoInfo::AudioSamplesFromFrames(__int64 frames) const { return (fps_numerator && HasVideo()) ? ((__int64)(frames) * audio_samples_per_second * fps_denominator / fps_numerator) : 0; }
int VideoInfo::FramesFromAudioSamples(__int64 samples) const { return (fps_denominator && HasAudio()) ? (int)((samples * (__int64)fps_numerator)/((__int64)fps_denominator * (__int64)audio_samples_per_second)) : 0; }
__int64 VideoInfo::AudioSamplesFromBytes(__int64 bytes) const { return HasAudio() ? bytes / BytesPerAudioSample() : 0; }
__int64 VideoInfo::BytesFromAudioSamples(__int64 samples) const { return samples * BytesPerAudioSample(); }
int VideoInfo::AudioChannels() const { return HasAudio() ? nchannels : 0; }
int VideoInfo::SampleType() const{ return sample_type;}
bool VideoInfo::IsSampleType(int testtype) const{ return !!(sample_type&testtype);}
int VideoInfo::SamplesPerSecond() const { return audio_samples_per_second; }
int VideoInfo::BytesPerAudioSample() const { return nchannels*BytesPerChannelSample();}
void VideoInfo::SetFieldBased(bool isfieldbased)  { if (isfieldbased) image_type|=IT_FIELDBASED; else  image_type&=~IT_FIELDBASED; }
void VideoInfo::Set(int property)  { image_type|=property; }
void VideoInfo::Clear(int property)  { image_type&=~property; }

int VideoInfo::BitsPerPixel() const {
  switch (pixel_type) {
    case CS_BGR24:
      return 24;
    case CS_BGR32:
      return 32;
    case CS_YUY2:
      return 16;
    case CS_YV12:
    case CS_I420:
      return 12;
    default:
      return 0;
  }
}

int VideoInfo::BytesPerChannelSample() const {
  switch (sample_type) {
  case SAMPLE_INT8:
    return sizeof(unsigned char);
  case SAMPLE_INT16:
    return sizeof(signed short);
  case SAMPLE_INT24:
    return 3;
  case SAMPLE_INT32:
    return sizeof(signed int);
  case SAMPLE_FLOAT:
    return sizeof(SFLOAT);
  default:
    _ASSERTE("Sample type not recognized!");
    return 0;
  }
}

// useful mutator
void VideoInfo::SetFPS(unsigned numerator, unsigned denominator) {
  if ((numerator == 0) || (denominator == 0)) {
    fps_numerator = 0;
    fps_denominator = 1;
  }
  else {
    unsigned x=numerator, y=denominator;
    while (y) {   // find gcd
      unsigned t = x%y; x = y; y = t;
    }
    fps_numerator = numerator/x;
    fps_denominator = denominator/x;
  }
}

// Range protected multiply-divide of FPS
void VideoInfo::MulDivFPS(unsigned multiplier, unsigned divisor) {
  unsigned __int64 numerator   = UInt32x32To64(fps_numerator,   multiplier);
  unsigned __int64 denominator = UInt32x32To64(fps_denominator, divisor);

  unsigned __int64 x=numerator, y=denominator;
  while (y) {   // find gcd
    unsigned __int64 t = x%y; x = y; y = t;
  }
  numerator   /= x; // normalize
  denominator /= x;

  unsigned __int64 temp = numerator | denominator; // Just looking top bit
  unsigned u = 0;
  while (temp & 0xffffffff80000000) { // or perhaps > 16777216*2
    temp = Int64ShrlMod32(temp, 1);
    u++;
  }
  if (u) { // Scale to fit
    const unsigned round = 1 << (u-1);
    SetFPS( (unsigned)Int64ShrlMod32(numerator   + round, u),
            (unsigned)Int64ShrlMod32(denominator + round, u) );
  }
  else {
    fps_numerator   = (unsigned)numerator;
    fps_denominator = (unsigned)denominator;
  }
}

// Test for same colorspace
bool VideoInfo::IsSameColorspace(const VideoInfo& vi) const {
  if (vi.pixel_type == pixel_type) return TRUE;
  if (IsYV12() && vi.IsYV12()) return TRUE;
  return FALSE;
}

// end struct VideoInfo

/**********************************************************************/

// class VideoFrameBuffer

const BYTE* VideoFrameBuffer::GetReadPtr() const { return data; }
BYTE* VideoFrameBuffer::GetWritePtr() { ++sequence_number; return data; }
int VideoFrameBuffer::GetDataSize() { return data_size; }
int VideoFrameBuffer::GetSequenceNumber() { return sequence_number; }
int VideoFrameBuffer::GetRefcount() { return refcount; }

// end class VideoFrameBuffer

/**********************************************************************/

// class VideoFrame

void VideoFrame::AddRef() { InterlockedIncrement((long *)&refcount); }
void VideoFrame::Release() { if (refcount==1) InterlockedDecrement(&vfb->refcount); InterlockedDecrement((long *)&refcount); }

int VideoFrame::GetPitch() const { return pitch; }
int VideoFrame::GetPitch(int plane) const { switch (plane) {case PLANAR_U: case PLANAR_V: return pitchUV;} return pitch; }
int VideoFrame::GetRowSize() const { return row_size; }

int VideoFrame::GetRowSize(int plane) const {
  switch (plane) {
  case PLANAR_U: case PLANAR_V: if (pitchUV) return row_size>>1; else return 0;
  case PLANAR_U_ALIGNED: case PLANAR_V_ALIGNED:
    if (pitchUV) {
      int r = ((row_size+FRAME_ALIGN-1)&(~(FRAME_ALIGN-1)) )>>1; // Aligned rowsize
      if (r<=pitchUV)
        return r;
      return row_size>>1;
    } else return 0;
  case PLANAR_Y_ALIGNED:
    int r = (row_size+FRAME_ALIGN-1)&(~(FRAME_ALIGN-1)); // Aligned rowsize
    if (r<=pitch)
      return r;
    return row_size;
  }
  return row_size; }

int VideoFrame::GetHeight() const { return height; }
int VideoFrame::GetHeight(int plane) const {  switch (plane) {case PLANAR_U: case PLANAR_V: if (pitchUV) return height>>1; return 0;} return height; }

// generally you shouldn't use these three
VideoFrameBuffer* VideoFrame::GetFrameBuffer() const { return vfb; }
int VideoFrame::GetOffset() const { return offset; }
int VideoFrame::GetOffset(int plane) const { switch (plane) {case PLANAR_U: return offsetU;case PLANAR_V: return offsetV;default: return offset;}; }

const BYTE* VideoFrame::GetReadPtr() const { return vfb->GetReadPtr() + offset; }
const BYTE* VideoFrame::GetReadPtr(int plane) const { return vfb->GetReadPtr() + GetOffset(plane); }

bool VideoFrame::IsWritable() const { return (refcount == 1 && vfb->refcount == 1); }

BYTE* VideoFrame::GetWritePtr() const {
  if (vfb->GetRefcount()>1) {
    _ASSERT(FALSE);
    //throw AvisynthError("Internal Error - refcount was more than one!");
  }
  return IsWritable() ? (vfb->GetWritePtr() + offset) : 0;
}

BYTE* VideoFrame::GetWritePtr(int plane) const {
  if (plane==PLANAR_Y) {
    if (vfb->GetRefcount()>1) {
      _ASSERT(FALSE);
//        throw AvisynthError("Internal Error - refcount was more than one!");
    }
    return IsWritable() ? vfb->GetWritePtr() + GetOffset(plane) : 0;
  }
  return vfb->data + GetOffset(plane);
}

VideoFrame::~VideoFrame() { InterlockedDecrement(&vfb->refcount); }

// end class VideoFrame

/**********************************************************************/

// class IClip

void IClip::AddRef() { InterlockedIncrement((long *)&refcnt); }
void IClip::Release() { InterlockedDecrement((long *)&refcnt); if (!refcnt) delete this; }

IClip::IClip() : refcnt(0) {}

int __stdcall IClip::GetVersion() { return AVISYNTH_INTERFACE_VERSION; }

__stdcall IClip::~IClip() {}

// end class IClip

/**********************************************************************/

// class PClip

IClip* PClip::GetPointerWithAddRef() const { if (p) p->AddRef(); return p; }

void PClip::Init(IClip* x) {
  if (x) x->AddRef();
  p=x;
}

void PClip::Set(IClip* x) {
  if (x) x->AddRef();
  if (p) p->Release();
  p=x;
}

PClip::PClip() { p = 0; }
PClip::PClip(const PClip& x) { Init(x.p); }
PClip::PClip(IClip* x) { Init(x); }
void PClip::operator=(IClip* x) { Set(x); }
void PClip::operator=(const PClip& x) { Set(x.p); }

IClip* PClip::operator->() const { return p; }

// useful in conditional expressions
PClip::operator void*() const { return p; }
bool PClip::operator!() const { return !p; }

PClip::~PClip() { if (p) p->Release(); }

// end class PClip

/**********************************************************************/

// class PVideoFrame

void PVideoFrame::Init(VideoFrame* x) {
  if (x) x->AddRef();
  p=x;
}

void PVideoFrame::Set(VideoFrame* x) {
  if (x) x->AddRef();
  if (p) p->Release();
  p=x;
}

PVideoFrame::PVideoFrame() { p = 0; }
PVideoFrame::PVideoFrame(const PVideoFrame& x) { Init(x.p); }
PVideoFrame::PVideoFrame(VideoFrame* x) { Init(x); }

void PVideoFrame::operator=(VideoFrame* x) { Set(x); }
void PVideoFrame::operator=(const PVideoFrame& x) { Set(x.p); }

VideoFrame* PVideoFrame::operator->() const { return p; }

// for conditional expressions
PVideoFrame::operator void*() const { return p; }
bool PVideoFrame::operator!() const { return !p; }

PVideoFrame::~PVideoFrame() { if (p) p->Release();}

// end class PVideoFrame

/**********************************************************************/

// class AVSValue

AVSValue::AVSValue() { type = 'v'; }
AVSValue::AVSValue(IClip* c) { type = 'c'; clip = c; if (c) c->AddRef(); }
AVSValue::AVSValue(const PClip& c) { type = 'c'; clip = c.GetPointerWithAddRef(); }
AVSValue::AVSValue(bool b) { type = 'b'; boolean = b; }
AVSValue::AVSValue(int i) { type = 'i'; integer = i; }
//  AVSValue(__int64 l) { type = 'l'; longlong = l; }
AVSValue::AVSValue(float f) { type = 'f'; floating_pt = f; }
AVSValue::AVSValue(double f) { type = 'f'; floating_pt = float(f); }
AVSValue::AVSValue(const char* s) { type = 's'; string = s; }
AVSValue::AVSValue(const AVSValue* a, int size) { type = 'a'; array = a; array_size = size; }
AVSValue::AVSValue(const AVSValue& v) { Assign(&v, true); }

AVSValue::~AVSValue() { if (IsClip() && clip) clip->Release(); }

AVSValue& AVSValue::operator=(const AVSValue& v) { Assign(&v, false); return *this; }

// Note that we transparently allow 'int' to be treated as 'float'.
// There are no int<->bool conversions, though.

bool AVSValue::Defined() const { return type != 'v'; }
bool AVSValue::IsClip() const { return type == 'c'; }
bool AVSValue::IsBool() const { return type == 'b'; }
bool AVSValue::IsInt() const { return type == 'i'; }
//  bool IsLong() const { return (type == 'l'|| type == 'i'); }
bool AVSValue::IsFloat() const { return type == 'f' || type == 'i'; }
bool AVSValue::IsString() const { return type == 's'; }
bool AVSValue::IsArray() const { return type == 'a'; }

PClip AVSValue::AsClip() const { _ASSERTE(IsClip()); return IsClip()?clip:0; }
bool AVSValue::AsBool() const { _ASSERTE(IsBool()); return boolean; }
int AVSValue::AsInt() const { _ASSERTE(IsInt()); return integer; }
//  int AsLong() const { _ASSERTE(IsLong()); return IsInt()?integer:longlong; }
const char* AVSValue::AsString() const { _ASSERTE(IsString()); return IsString()?string:0; }
double AVSValue::AsFloat() const { _ASSERTE(IsFloat()); return IsInt()?integer:floating_pt; }

bool AVSValue::AsBool(bool def) const { _ASSERTE(IsBool()||!Defined()); return IsBool() ? boolean : def; }
int AVSValue::AsInt(int def) const { _ASSERTE(IsInt()||!Defined()); return IsInt() ? integer : def; }
double AVSValue::AsFloat(double def) const { _ASSERTE(IsFloat()||!Defined()); return IsInt() ? integer : type=='f' ? floating_pt : def; }
const char* AVSValue::AsString(const char* def) const { _ASSERTE(IsString()||!Defined()); return IsString() ? string : def; }

int AVSValue::ArraySize() const { _ASSERTE(IsArray()); return IsArray()?array_size:1; }

const AVSValue& AVSValue::operator[](int index) const {
  _ASSERTE(IsArray() && index>=0 && index<array_size);
  return (IsArray() && index>=0 && index<array_size) ? array[index] : *this;
}

void AVSValue::Assign(const AVSValue* src, bool init) {
  if (src->IsClip() && src->clip)
    src->clip->AddRef();
  if (!init && IsClip() && clip)
    clip->Release();
  // make sure this copies the whole struct!
  ((__int32*)this)[0] = ((__int32*)src)[0];
  ((__int32*)this)[1] = ((__int32*)src)[1];
}

// end class AVSValue

/**********************************************************************/
