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


#include "stdafx.h"

#include "../internal.h"
#include "../convert/convert.h"
#include "../filters/transform.h"
#include "avi_source.cpp"

#define PI 3.1415926535897932384626433832795
#include <ctime>

/********************************************************************
********************************************************************/

enum {
    COLOR_MODE_RGB = 0,
    COLOR_MODE_YUV
};

class StaticImage : public IClip {
  const VideoInfo vi;
  const PVideoFrame frame;

public:
  StaticImage(const VideoInfo& _vi, const PVideoFrame& _frame)
    : vi(_vi), frame(_frame) {}
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env) { return frame; }
  void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) {
    memset(buf, 0, vi.BytesFromAudioSamples(count));
  }
  const VideoInfo& __stdcall GetVideoInfo() { return vi; }
  bool __stdcall GetParity(int n) { return vi.IsFieldBased() ? (n&1) : false; }
  void __stdcall SetCacheHints(int cachehints,int frame_range) { };
};


static PVideoFrame CreateBlankFrame(const VideoInfo& vi, int color, int mode, IScriptEnvironment* env) {

  if (!vi.HasVideo()) return 0;

  PVideoFrame frame = env->NewVideoFrame(vi);
  BYTE* p = frame->GetWritePtr();
  int size = frame->GetPitch() * frame->GetHeight();

  if (vi.IsYV12()) {
    int color_yuv =(mode == COLOR_MODE_YUV) ? color : RGB2YUV(color);
    int Cval = (color_yuv>>16)&0xff;
    Cval |= (Cval<<8)|(Cval<<16)|(Cval<<24);
    for (int i=0; i<size; i+=4)
      *(unsigned*)(p+i) = Cval;
    p = frame->GetWritePtr(PLANAR_U);
    size = frame->GetPitch(PLANAR_U) * frame->GetHeight(PLANAR_U);
    Cval = (color_yuv>>8)&0xff;
    Cval |= (Cval<<8)|(Cval<<16)|(Cval<<24);
    for (i=0; i<size; i+=4)
      *(unsigned*)(p+i) = Cval;
    size = frame->GetPitch(PLANAR_V) * frame->GetHeight(PLANAR_V);
    p = frame->GetWritePtr(PLANAR_V);
    Cval = (color_yuv)&0xff;
    Cval |= (Cval<<8)|(Cval<<16)|(Cval<<24);
    for (i=0; i<size; i+=4)
      *(unsigned*)(p+i) = Cval;
  } else if (vi.IsYUY2()) {
    int color_yuv =(mode == COLOR_MODE_YUV) ? color : RGB2YUV(color);
    unsigned d = ((color_yuv>>16)&255) * 0x010001 + ((color_yuv>>8)&255) * 0x0100 + (color_yuv&255) * 0x01000000;
    for (int i=0; i<size; i+=4)
      *(unsigned*)(p+i) = d;
  } else if (vi.IsRGB24()) {
    const unsigned char clr0 = (color & 0xFF);
    const unsigned short clr1 = (color >> 8);
    const int gr = frame->GetRowSize();
    const int gp = frame->GetPitch();
    for (int y=frame->GetHeight();y>0;y--) {
      for (int i=0; i<gr; i+=3) {
        p[i] = clr0; *(unsigned __int16*)(p+i+1) = clr1;
      }
      p+=gp;
    }
  } else if (vi.IsRGB32()) {
    for (int i=0; i<size; i+=4)
      *(unsigned*)(p+i) = color;
  }
  return frame;
}

static AVSValue __cdecl Create_BlankClip(AVSValue args, void*, IScriptEnvironment* env) {
  VideoInfo vi_default;
  memset(&vi_default, 0, sizeof(VideoInfo));
  vi_default.fps_denominator=1; vi_default.fps_numerator=24; vi_default.height=480; vi_default.pixel_type=VideoInfo::CS_BGR32; vi_default.num_frames=240; vi_default.width=640;
  vi_default.audio_samples_per_second=44100; vi_default.nchannels=1; vi_default.num_audio_samples=44100*10; vi_default.sample_type=SAMPLE_INT16;

  VideoInfo vi;

  if (args[0].Defined()) {
    vi_default = args[0].AsClip()->GetVideoInfo();
  }

  vi.num_frames = args[1].AsInt(vi_default.num_frames);
  vi.width = args[2].AsInt(vi_default.width);
  vi.height = args[3].AsInt(vi_default.height);
  vi.pixel_type = vi_default.pixel_type;

  if (args[4].Defined()) {
    const char* pixel_type_string = args[4].AsString();
    if (!lstrcmpi(pixel_type_string, "YUY2")) {
      vi.pixel_type = VideoInfo::CS_YUY2;
    } else if (!lstrcmpi(pixel_type_string, "YV12")) {
      vi.pixel_type = VideoInfo::CS_YV12;
    } else if (!lstrcmpi(pixel_type_string, "RGB24")) {
      vi.pixel_type = VideoInfo::CS_BGR24;
    } else if (!lstrcmpi(pixel_type_string, "RGB32")) {
      vi.pixel_type = VideoInfo::CS_BGR32;
    } else {
      env->ThrowError("BlankClip: pixel_type must be \"RGB32\", \"RGB24\", \"YV12\" or \"YUY2\"");
    }
  }

  double n = args[5].AsFloat(double(vi_default.fps_numerator));

  if (args[5].Defined() && !args[6].Defined()) {
    unsigned d = 1;
    while (n < 16777216 && d < 16777216) { n*=2; d*=2; }
    vi.SetFPS(int(n+0.5), d);
  } else {
    vi.SetFPS(int(n+0.5), args[6].AsInt(vi_default.fps_denominator));
  }

  if (!vi.pixel_type)
    vi.pixel_type = vi_default.pixel_type;

  vi.SetFieldBased(false);
  vi.audio_samples_per_second = args[7].AsInt(vi_default.audio_samples_per_second);

  if (args[8].Defined())
    vi.nchannels = args[8].AsBool() ? 2 : 1;
  else
    vi.nchannels = vi_default.nchannels;

  vi.sample_type = args[9].AsInt(vi_default.sample_type);
  vi.num_audio_samples = vi.AudioSamplesFromFrames(vi.num_frames);

  int color = args[10].AsInt(0);
  int mode = COLOR_MODE_RGB;
  if (args[11].Defined()) {
	if (color != 0) // Not quite 100% test
	  env->ThrowError("BlankClip: color and color_yuv are mutually exclusive");
	if (!vi.IsYUV())
	  env->ThrowError("BlankClip: color_yuv only valid for YUV color spaces");
    color = args[11].AsInt();
    mode=COLOR_MODE_YUV;
	if ((unsigned)color > 0xffffff)
	  env->ThrowError("BlankClip: color_yuv must be between 0 and %d($ffffff)", 0xffffff);
  }

  return new StaticImage(vi, CreateBlankFrame(vi, color, mode, env));
}


/********************************************************************
********************************************************************/

// in text-overlay.cpp
extern void ApplyMessage(PVideoFrame* frame, const VideoInfo& vi,
  const char* message, int size, int textcolor, int halocolor, int bgcolor,
  IScriptEnvironment* env);

extern bool GetTextBoundingBox(const char* text, const char* fontname,
  int size, bool bold, bool italic, int align, int* width, int* height);


PClip Create_MessageClip(const char* message, int width, int height, int pixel_type, bool shrink, int textcolor, int halocolor, int bgcolor, IScriptEnvironment* env) {
  int size;
  for (size = 24*8; /*size>=9*8*/; size-=4) {
    int text_width, text_height;
    GetTextBoundingBox(message, "Arial", size, true, false, TA_TOP | TA_CENTER, &text_width, &text_height);
    text_width = ((text_width>>3)+8+3) & -4;
    text_height = ((text_height>>3)+8+1) & -2;
    if (size<=9*8 || ((width<=0 || text_width<=width) && (height<=0 || text_height<=height))) {
      if (width <= 0 || (shrink && width>text_width))
        width = text_width;
      if (height <= 0 || (shrink && height>text_height))
        height = text_height;
      break;
    }
  }

  VideoInfo vi;
  memset(&vi, 0, sizeof(vi));
  vi.width = width;
  vi.height = height;
  vi.pixel_type = pixel_type;
  vi.fps_numerator = 24;
  vi.fps_denominator = 1;
  vi.num_frames = 240;

  PVideoFrame frame = CreateBlankFrame(vi, bgcolor, COLOR_MODE_RGB, env);
  ApplyMessage(&frame, vi, message, size, textcolor, halocolor, bgcolor, env);
  return new StaticImage(vi, frame);
};


AVSValue __cdecl Create_MessageClip(AVSValue args, void*, IScriptEnvironment* env) {
  return Create_MessageClip(args[0].AsString(), args[1].AsInt(-1),
      args[2].AsInt(-1), VideoInfo::CS_BGR32, args[3].AsBool(false),
      args[4].AsInt(0xFFFFFF), args[5].AsInt(0), args[6].AsInt(0), env);
}


/********************************************************************
********************************************************************/




class ColorBars : public IClip {
  VideoInfo vi;
  PVideoFrame frame;

public:

  ColorBars(int w, int h, IScriptEnvironment* env) {
    memset(&vi, 0, sizeof(VideoInfo));
    vi.width = w;
    vi.height = h;
    vi.fps_numerator = 2997;
    vi.fps_denominator = 100;
    vi.num_frames = 107892;   // 1 hour
    vi.pixel_type = VideoInfo::CS_BGR32;
    vi.sample_type = SAMPLE_FLOAT;
    vi.nchannels = 2;
    vi.audio_samples_per_second = 48000;
    vi.num_audio_samples=(60*60)*vi.audio_samples_per_second;

    frame = env->NewVideoFrame(vi);
    unsigned* p = (unsigned*)frame->GetWritePtr();
    const int pitch = frame->GetPitch()/4;

    int y = 0;

    // these values are taken from http://www.greatdv.com/video/smptebars3.htm
    // note we go bottom->top
    static const int bottom_quarter[] = { 0x001d42, 0xebebeb, 0x2c005c, 0x101010,  0x070707, 0x101010, 0x181818,  0x101010 };
    for (; y < h/4; ++y) {
      int x = 0;
      for (int i=0; i<4; ++i) {
        for (; x < (w*(i+1)*5+14)/28; ++x)
          p[x] = bottom_quarter[i];
      }
      for (int j=4; j<7; ++j) {
        for (; x < (w*(j+12)+10)/21; ++x)
          p[x] = bottom_quarter[j];
      }
      for (; x < w; ++x)
        p[x] = bottom_quarter[7];
      p += pitch;
    }

    static const int two_thirds_to_three_quarters[] = { 0x0f0fb4, 0x101010, 0xb410b4, 0x101010, 0x0db4b4, 0x101010, 0xb4b4b4 };
    for (; y < h/3; ++y) {
      int x = 0;
      for (int i=0; i<7; ++i) {
        for (; x < (w*(i+1)+3)/7; ++x)
          p[x] = two_thirds_to_three_quarters[i];
      }
      p += pitch;
    }

    static const int top_two_thirds[] = { 0xb4b4b4, 0xb4b40c, 0x0db4b4, 0x0db40c, 0xb410b4, 0xb40f0e, 0x0f0fb4 };
    for (; y < h; ++y) {
      int x = 0;
      for (int i=0; i<7; ++i) {
        for (; x < (w*(i+1)+3)/7; ++x)
          p[x] = top_two_thirds[i];
      }
      p += pitch;
    }
  }

  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env) { return frame; }
  bool __stdcall GetParity(int n) { return false; }
  const VideoInfo& __stdcall GetVideoInfo() { return vi; }
  void __stdcall SetCacheHints(int cachehints,int frame_range) { };

  void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) {
    __int64 Hz=440;
    // Calculate what start equates in cycles.
    // This is the number of cycles (rounded down) that has already been taken.
    __int64 startcycle = (start*Hz) /  vi.audio_samples_per_second;  
    
    // Move offset down - this is to avoid float rounding errors
    int start_offset = (int)(start - ((startcycle * vi.audio_samples_per_second) / Hz));

    double add_per_sample=Hz/(double)vi.audio_samples_per_second;
    double second_offset=((double)start_offset*add_per_sample);
    int d_mod=vi.audio_samples_per_second*2;
    float* samples = (float*)buf;

    for (int i=0;i<count;i++) {
        samples[i*2]=sinf(3.1415926535897932384626433832795f*2.0f*(float)second_offset);
        if (((start+i)%d_mod)>vi.audio_samples_per_second) {
          samples[i*2+1]=samples[i*2];
        } else {
          samples[i*2+1]=0;
        }
        second_offset+=add_per_sample;
    }
  }

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env) {
    return new ColorBars(args[0].AsInt(640), args[1].AsInt(480), env);
  }
};


/********************************************************************
********************************************************************/



/********************************************************************
********************************************************************/

#if 0
class QuickTimeSource : public IClip {
public:
  QuickTimeSource() {
//    extern void foo();
//    foo();
  }
  PVideoFrame GetFrame(int n, IScriptEnvironment* env) {}
  void GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) {}
  const VideoInfo& GetVideoInfo() {}
  bool GetParity(int n) { return false; }

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env) {
    return new QuickTimeSource;
  }
};
#endif

/********************************************************************
********************************************************************/

AVSValue __cdecl Create_SegmentedSource(AVSValue args, void* use_directshow, IScriptEnvironment* env) {
  int avg_time_per_frame = (use_directshow && args[1].Defined()) ? int(10000000 / args[1].AsFloat() + 0.5) : 0;
  bool bAudio = !use_directshow && args[1].AsBool(true);
  const char* pixel_type;
  const char* fourCC;
  if (!use_directshow) {
    pixel_type = args[2].AsString("");
    fourCC = args[3].AsString("");
  }
  args = args[0];
  PClip result = 0;
  const char* error_msg=0;
  for (int i = 0; i < args.ArraySize(); ++i) {
    char basename[260];
    strcpy(basename, args[i].AsString());
    char* extension = strrchr(basename, '.');
    if (extension)
      *extension++ = 0;
    else
      extension = "";
    for (int j = 0; j < 100; ++j) {
      char filename[260];
      wsprintf(filename, "%s.%02d.%s", basename, j, extension);
      if (GetFileAttributes(filename) != (DWORD)-1) {   // check if file exists
          PClip clip;
        try {
          if (use_directshow) {
            AVSValue inv_args[5] = { filename, avg_time_per_frame, true, true, true};
            clip = env->Invoke("DirectShowSource",AVSValue(inv_args,5)).AsClip();
          } else {
            clip =  (IClip*)(new AVISource(filename, bAudio, pixel_type, fourCC, 0, env));
          }
          result = !result ? clip : new_Splice(result, clip, false, env);
        } catch (AvisynthError e) {
          error_msg=e.msg;
        }
      }
    }
  }
  if (!result) {
    if (!error_msg) {
      env->ThrowError("Segmented%sSource: no files found!", use_directshow ? "DirectShow" : "AVI");
    } else {
      env->ThrowError("Segmented%sSource: decompressor returned error:\n%s!", use_directshow ? "DirectShow" : "AVI",error_msg);
    }
  }
  return result;
}

/**********************************************************
 *                         TONE                           *
 **********************************************************/
class SampleGenerator {
public:
  SampleGenerator() {}
  virtual SFLOAT getValueAt(double where) {return 0.0f;}
};

class SineGenerator : public SampleGenerator {
public:
  SineGenerator() {}
  SFLOAT getValueAt(double where) {return sinf(PI * where* 2.0);}
};


class NoiseGenerator : public SampleGenerator {
public:
  NoiseGenerator() {
    srand( (unsigned)time( NULL ) );
  }

  SFLOAT getValueAt(double where) {return (float) rand()*(2.0f/RAND_MAX) -1.0f;}
};

class SquareGenerator : public SampleGenerator {
public:
  SquareGenerator() {}

  SFLOAT getValueAt(double where) {
    if (where<=0.5) {
      return 1.0f;
    } else {
      return -1.0f;
    }
  }
};

class TriangleGenerator : public SampleGenerator {
public:
  TriangleGenerator() {}

  SFLOAT getValueAt(double where) {
    if (where<=0.25) {
      return (where*4.0);
    } else if (where<=0.75) {
      return ((-4.0*(where-0.50)));
    } else {
      return ((4.0*(where-1.00)));
    }
  }
};

class SawtoothGenerator : public SampleGenerator {
public:
  SawtoothGenerator() {}

  SFLOAT getValueAt(double where) {
    return 2.0*(where-0.5);
  }
};


class Tone : public IClip {
  VideoInfo vi;
  SampleGenerator *s;
  double freq;
  int samplerate;
  int ch;

public:

  Tone(float _length, double _freq, int _samplerate, int _ch, const char* _type, IScriptEnvironment* env): freq(_freq), samplerate(_samplerate), ch(_ch) {
    memset(&vi, 0, sizeof(VideoInfo));
    vi.sample_type = SAMPLE_FLOAT;
    vi.nchannels = ch;
    vi.audio_samples_per_second = samplerate;
    vi.num_audio_samples=(__int64)(_length*(float)vi.audio_samples_per_second);
    if (!lstrcmpi(_type, "Sine"))
      s = new SineGenerator();
    else if (!lstrcmpi(_type, "Noise"))
      s = new NoiseGenerator();
    else if (!lstrcmpi(_type, "Square"))
      s = new SquareGenerator();
    else if (!lstrcmpi(_type, "Triangle"))
      s = new TriangleGenerator();
    else if (!lstrcmpi(_type, "Sawtooth"))
      s = new SawtoothGenerator();
    else if (!lstrcmpi(_type, "Silence"))
      s = new SampleGenerator();
    else
      env->ThrowError("Tone: Type was not recognized!");
  }

  void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) {
    // How much should we add per sample
    // In seconds (beware of inprecision!)
    double add_per_sample = freq / (double)vi.audio_samples_per_second;

    // Where do we start timewise
    // In seconds - precise (enough)
    double start_offset = ((double)start) / (double)samplerate;


    // Where in the cycle are we in?
    double cycle = (freq * start) / samplerate;

    // Which cycle are we in?
    double round_cycle = (double)(int)(cycle);

    double period_place = cycle-round_cycle;

    SFLOAT* samples = (SFLOAT*)buf;

    for (int i=0;i<count;i++) {
      SFLOAT v = s->getValueAt(max(0.0, min(1.0,period_place)));
      for (int o=0;o<ch;o++) {
        samples[o+i*ch] = v;
      }
      period_place += add_per_sample;
      while (period_place > 1.0) {
        period_place -= 1.0;
      }
    }
  }

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env) {
    return new Tone(args[0].AsFloat(10.0), args[1].AsFloat(440), args[2].AsInt(48000), args[3].AsInt(2), args[4].AsString("Sine"), env);
  }

  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env) { return NULL; }
  const VideoInfo& __stdcall GetVideoInfo() { return vi; }
  bool __stdcall GetParity(int n) { return false; }
  void __stdcall SetCacheHints(int cachehints,int frame_range) { };

};



AVSValue __cdecl Create_Version(AVSValue args, void*, IScriptEnvironment* env) {
  return Create_MessageClip(AVS_VERSTR
          "\n\xA9 2000-2004 Ben Rudiak-Gould, et al.\n"
          "http://www.avisynth.org",
  -1, -1, VideoInfo::CS_BGR24, false, 0xECF2BF, 0, 0x404040, env);
}


AVSFunction Source_filters[] = {
  { "AVISource", "s+[audio]b[pixel_type]s[fourCC]s", AVISource::Create, (void*) AVISource::MODE_NORMAL },
  { "AVIFileSource", "s+[audio]b[pixel_type]s[fourCC]s", AVISource::Create, (void*) AVISource::MODE_AVIFILE },
  { "WAVSource", "s+", AVISource::Create, (void*) AVISource::MODE_WAV },
  { "OpenDMLSource", "s+[audio]b[pixel_type]s[fourCC]s", AVISource::Create, (void*) AVISource::MODE_OPENDML },
  { "SegmentedAVISource", "s+[audio]b[pixel_type]s[fourCC]s", Create_SegmentedSource, (void*)0 },
  { "SegmentedDirectShowSource", "s+[fps]f", Create_SegmentedSource, (void*)1 },
  { "BlankClip", "[clip]c[length]i[width]i[height]i[pixel_type]s[fps]f[fps_denominator]i[audio_rate]i[stereo]b[sixteen_bit]b[color]i[color_yuv]i", Create_BlankClip },
  { "Blackness", "[clip]c[length]i[width]i[height]i[pixel_type]s[fps]f[fps_denominator]i[audio_rate]i[stereo]b[sixteen_bit]b[color]i[color_yuv]i", Create_BlankClip },
  { "MessageClip", "s[width]i[height]i[shrink]b[text_color]i[halo_color]i[bg_color]i", Create_MessageClip },
  { "ColorBars", "[width]i[height]i", ColorBars::Create },
  { "Tone", "[length]f[frequency]f[samplerate]i[channels]i[type]s", Tone::Create },

  { "Version", "", Create_Version },
  { 0,0,0 }
};

