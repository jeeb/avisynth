// Avisynth v1.0 beta3.  Copyright 2000 Ben Rudiak-Gould.
// http://www.math.berkeley.edu/~benrg/avisynth.html
//
// While ben is away, please send any good modification to me (edwinvaneggelen@softhome.net)
// so I can put them on my website
//
// If you run into problem compiling this source please download the Windows SDK
// for microsoft. The full version is around 600MB !!!
// On most environments, the DirectX 8.1 SDK is enough - other also require the Platform SDK.

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
 

#include "internal.h"
#include "convert.h"
#include "AudioSource.h"
#include "VD_Audio.h"

#include "AVIReadHandler.h"


class AVISource : public IClip {
  IAVIReadHandler *pfile;
  IAVIReadStream *pvideo;
  HIC hic;
  VideoInfo vi;
  BYTE* srcbuffer;
  int srcbuffer_size;
  BITMAPINFOHEADER* pbiSrc;
  BITMAPINFOHEADER biDst;
  bool ex;
  bool dropped_frame;
  PVideoFrame last_frame;
  int last_frame_no;
  AudioSource* aSrc;
  AudioStreamSource* audioStreamSource;
  int audio_stream_pos;

  LRESULT DecompressBegin(LPBITMAPINFOHEADER lpbiSrc, LPBITMAPINFOHEADER lpbiDst);
  LRESULT DecompressFrame(int n, bool preroll, BYTE* buf);

  void CheckHresult(HRESULT hr, const char* msg, IScriptEnvironment* env);
  bool AttemptCodecNegotiation(DWORD fccHandler, BITMAPINFOHEADER* bmih);
  void LocateVideoCodec(IScriptEnvironment* env);

public:

  AVISource::AVISource(const char filename[], bool fAudio, const char pixel_type[], int mode, IScriptEnvironment* env);  // mode: 0=detect, 1=avifile, 2=opendml, 3=avifile (audio only)
  ~AVISource();
  const VideoInfo& __stdcall GetVideoInfo();
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
  void __stdcall GetAudio(void* buf, int start, int count, IScriptEnvironment* env);
  bool __stdcall GetParity(int n);

  static AVSValue __cdecl Create(AVSValue args, void* user_data, IScriptEnvironment* env) {
    const int mode = int(user_data);
    const bool fAudio = (mode == 3) || args[1].AsBool(true);
    const char* pixel_type = (mode != 3) ? args[2].AsString("") : "";
    PClip result = new AVISource(args[0][0].AsString(), fAudio, pixel_type, mode, env);
    for (int i=1; i<args[0].ArraySize(); ++i)
      result = new_Splice(result, new AVISource(args[0][i].AsString(), fAudio, pixel_type, mode, env), false, env);
    return result;
  }
};


LRESULT AVISource::DecompressBegin(LPBITMAPINFOHEADER lpbiSrc, LPBITMAPINFOHEADER lpbiDst) {
  if (!ex) {
    LRESULT result = ICDecompressBegin(hic, lpbiSrc, lpbiDst);
    if (result != ICERR_UNSUPPORTED)
      return result;
    else
      ex = true;
      // and fall thru
  }
  return ICDecompressExBegin(hic, 0,
    lpbiSrc, 0, 0, 0, lpbiSrc->biWidth, lpbiSrc->biHeight,
    lpbiDst, 0, 0, 0, lpbiDst->biWidth, lpbiDst->biHeight);
}

LRESULT AVISource::DecompressFrame(int n, bool preroll, BYTE* buf) {
  _RPT2(0,"AVISource: Decompressing frame %d%s\n", n, preroll ? " (preroll)" : "");
  if (!hic) {
    pvideo->Read(n, 1, buf, vi.BMPSize(), NULL, NULL);
    return ICERR_OK;
  }
  long bytes_read = srcbuffer_size;
  LRESULT err = pvideo->Read(n, 1, srcbuffer, srcbuffer_size, &bytes_read, NULL);
  while (err == AVIERR_BUFFERTOOSMALL || (err == 0 && !srcbuffer)) {
    delete[] srcbuffer;
    pvideo->Read(n, 1, 0, srcbuffer_size, &bytes_read, NULL);
    srcbuffer = new BYTE[srcbuffer_size = bytes_read];
    err = pvideo->Read(n, 1, srcbuffer, srcbuffer_size, &bytes_read, NULL);
  }
  dropped_frame = !bytes_read;
  int flags = preroll ? ICDECOMPRESS_PREROLL : 0;
  flags |= dropped_frame ? ICDECOMPRESS_NULLFRAME : 0;
  flags |= !pvideo->IsKeyFrame(n) ? ICDECOMPRESS_NOTKEYFRAME : 0;
  pbiSrc->biSizeImage = bytes_read;
  return !ex ? ICDecompress(hic, flags, pbiSrc, srcbuffer, &biDst, buf)
    : ICDecompressEx(hic, flags, pbiSrc, srcbuffer, 0, 0, vi.width, vi.height, &biDst, buf, 0, 0, vi.width, vi.height);
}


void AVISource::CheckHresult(HRESULT hr, const char* msg, IScriptEnvironment* env) {
  if (SUCCEEDED(hr)) return;
  char buf[1024] = {0};
  if (!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, hr, 0, buf, 1024, NULL))
    wsprintf(buf, "error code 0x%x", hr);
  env->ThrowError("AVISource: %s:\n%s", msg, buf);
}


// taken from VirtualDub
bool AVISource::AttemptCodecNegotiation(DWORD fccHandler, BITMAPINFOHEADER* bmih) {

    // Try the handler specified in the file first.  In some cases, it'll
  // be wrong or missing.

  if (fccHandler)
    hic = ICOpen(ICTYPE_VIDEO, fccHandler, ICMODE_DECOMPRESS);

  if (!hic || ICERR_OK!=ICDecompressQuery(hic, bmih, NULL)) {
    if (hic)
      ICClose(hic);

    // Pick a handler based on the biCompression field instead.

    hic = ICOpen(ICTYPE_VIDEO, bmih->biCompression, ICMODE_DECOMPRESS);

    if (!hic || ICERR_OK!=ICDecompressQuery(hic, bmih, NULL)) {
      if (hic)
        ICClose(hic);

      // This never seems to work...

      hic = ICLocate(ICTYPE_VIDEO, NULL, bmih, NULL, ICMODE_DECOMPRESS);
    }
  }

    return !!hic;
}


void AVISource::LocateVideoCodec(IScriptEnvironment* env) {
  AVISTREAMINFO asi;
  long size = sizeof(BITMAPINFOHEADER);
  CheckHresult(pvideo->Info(&asi, sizeof(asi)), "couldn't get video info", env);
  CheckHresult(pvideo->ReadFormat(0, 0, &size), "couldn't get video format size", env);
  pbiSrc = (LPBITMAPINFOHEADER)malloc(size);
  CheckHresult(pvideo->ReadFormat(0, pbiSrc, &size), "couldn't get video format", env);

  vi.width = pbiSrc->biWidth;
  vi.height = pbiSrc->biHeight;
  vi.SetFPS(asi.dwRate, asi.dwScale);
  vi.num_frames = asi.dwLength;
  vi.field_based = false;

  // see if we can handle the video format directly
  if (pbiSrc->biCompression == '2YUY') {
    vi.pixel_type = VideoInfo::YUY2;
  } else if (pbiSrc->biCompression == BI_RGB && pbiSrc->biBitCount == 32) {
    vi.pixel_type = VideoInfo::BGR32;
  } else if (pbiSrc->biCompression == BI_RGB && pbiSrc->biBitCount == 24) {
    vi.pixel_type = VideoInfo::BGR24;

  // otherwise, find someone who will decompress it
  } else {
    switch(pbiSrc->biCompression) {
    case '34PM':    // Microsoft MPEG-4 V3
    case '3VID':    // "DivX Low-Motion" (4.10.0.3917)
    case '4VID':    // "DivX Fast-Motion" (4.10.0.3920)
    case '14PA':    // "AngelPotion Definitive" (4.0.00.3688)
      if (AttemptCodecNegotiation(asi.fccHandler, pbiSrc)) return;
      pbiSrc->biCompression = '34PM';
      if (AttemptCodecNegotiation(asi.fccHandler, pbiSrc)) return;
      pbiSrc->biCompression = '3VID';
      if (AttemptCodecNegotiation(asi.fccHandler, pbiSrc)) return;
      pbiSrc->biCompression = '4VID';
      if (AttemptCodecNegotiation(asi.fccHandler, pbiSrc)) return;
      pbiSrc->biCompression = '14PA';
    default:
      if (AttemptCodecNegotiation(asi.fccHandler, pbiSrc)) return;
    }
    env->ThrowError("AVISource: couldn't locate a decompressor for fourcc %c%c%c%c",
      asi.fccHandler, asi.fccHandler>>8, asi.fccHandler>>16, asi.fccHandler>>24);
  }
}


AVISource::AVISource(const char filename[], bool fAudio, const char pixel_type[], int mode, IScriptEnvironment* env) {
  srcbuffer = 0; srcbuffer_size = 0;
  memset(&vi, 0, sizeof(vi));
  ex = false;
  last_frame_no = -1;
  pbiSrc = 0;
  aSrc = 0;
  audioStreamSource = 0;
  pvideo=0;
  pfile=0;

  AVIFileInit();

  if (mode == 0) {
    // if it looks like an AVI file, open in OpenDML mode; otherwise AVIFile mode
    HANDLE h = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) {
      char foo[50] = "AVISource autodetect: couldn't open file";
      strcat(foo, "\nError code: ");
      char bar[10];
      strcat(foo, itoa(GetLastError(), bar, 10));
      env->ThrowError(foo);
    }
    unsigned buf[3];
    DWORD bytes_read;
    if (ReadFile(h, buf, 12, &bytes_read, NULL) && bytes_read == 12 && buf[0] == 'FFIR' && buf[2] == ' IVA')
      mode = 2;
    else
      mode = 1;
    CloseHandle(h);
  }

  if (mode == 1 || mode == 3) {    // AVIFile mode
    PAVIFILE paf;
    if (FAILED(AVIFileOpen(&paf, filename, OF_READ, 0)))
      env->ThrowError("AVIFileSource: couldn't open file");
    pfile = CreateAVIReadHandler(paf);
  } else {              // OpenDML mode
    pfile = CreateAVIReadHandler(filename);
  }

  if (mode != 3) { // check for video stream
    hic = 0;
    pvideo = pfile->GetStream(streamtypeVIDEO, 0);
    if (pvideo) {
      LocateVideoCodec(env);
      if (hic) {
        bool fYUY2  = lstrcmpi(pixel_type, "YUY2" ) == 0 || pixel_type[0] == 0;
        bool fRGB32 = lstrcmpi(pixel_type, "RGB32") == 0 || pixel_type[0] == 0;
        bool fRGB24 = lstrcmpi(pixel_type, "RGB24") == 0 || pixel_type[0] == 0;       
        if (!(fYUY2 || fRGB32 || fRGB24))
          env->ThrowError("AVISource: requested format should be YUY2, RGB32 or RGB24");

        // try to decompress to YUY2, RGB32, and RGB24 in turn
        memset(&biDst, 0, sizeof(BITMAPINFOHEADER));
        biDst.biSize = sizeof(BITMAPINFOHEADER);
        biDst.biWidth = vi.width;
        biDst.biHeight = vi.height;
        biDst.biCompression = '2YUY';
        biDst.biBitCount = 16;
        biDst.biPlanes = 1;
        biDst.biSizeImage = ((vi.width*2+3)&~3) * vi.height;
        if (fYUY2 && ICERR_OK == ICDecompressQuery(hic, pbiSrc, &biDst)) {
          vi.pixel_type = VideoInfo::YUY2;
        } else {
          biDst.biCompression = BI_RGB;
          biDst.biBitCount = 32;
          biDst.biSizeImage = vi.width*vi.height*4;
          if (fRGB32 && ICERR_OK == ICDecompressQuery(hic, pbiSrc, &biDst)) {
            vi.pixel_type = VideoInfo::BGR32;
          } else {
            biDst.biBitCount = 24;
            biDst.biSizeImage = ((vi.width*3+3)&~3) * vi.height;
            if (fRGB24 && ICERR_OK == ICDecompressQuery(hic, pbiSrc, &biDst)) {
              vi.pixel_type = VideoInfo::BGR24;
            } else {
              if (fYUY2 && (fRGB32 || fRGB24))
                env->ThrowError("AVISource: the video decompressor couldn't produce YUY2 or RGB output");
              else if (fYUY2)
                env->ThrowError("AVISource: the video decompressor couldn't produce YUY2 output");
              else if (fRGB32)
                env->ThrowError("AVISource: the video decompressor couldn't produce RGB32 output");
              else if (fRGB24)
                env->ThrowError("AVISource: the video decompressor couldn't produce RGB24 output");
              else
                env->ThrowError("AVISource: internal error");
            }
          }
        }
        DecompressBegin(pbiSrc, &biDst);
      }
    }
  }

  // check for audio stream
  if (fAudio && pfile->GetStream(streamtypeAUDIO, 0)) {
    aSrc = new AudioSourceAVI(pfile, true);
    aSrc->init();
    audioStreamSource = new AudioStreamSource(aSrc,
                                              aSrc->lSampleFirst, 
                                              aSrc->lSampleLast - aSrc->lSampleFirst,
                                              true);
    WAVEFORMATEX* pwfx; 
    pwfx = audioStreamSource->GetFormat();
    vi.audio_samples_per_second = pwfx->nSamplesPerSec;
    vi.stereo = (pwfx->nChannels == 2);
    vi.sixteen_bit = (pwfx->wBitsPerSample == 16);
    vi.num_audio_samples = audioStreamSource->GetLength();

    audio_stream_pos = 0;
  }
  // try to decompress frame 0 if not audio only.
  if (mode!=3) {
    int keyframe = pvideo->NearestKeyFrame(0);
    PVideoFrame frame = env->NewVideoFrame(vi, 4);
    LRESULT error = DecompressFrame(keyframe, true, frame->GetWritePtr());
    if (error != ICERR_OK || (!frame)||(dropped_frame)) {   // shutdown, if init not succesful.
      if (hic) {
        !ex ? ICDecompressEnd(hic) : ICDecompressExEnd(hic);
        ICClose(hic);
      }
      if (pvideo) delete pvideo;
      if (aSrc) delete aSrc;
      if (audioStreamSource) delete audioStreamSource;
      if (pfile)
        pfile->Release();
      AVIFileExit();
      if (pbiSrc)
        free(pbiSrc);
      env->ThrowError("AviSource: Could not decompress frame 0");
      
    }
    last_frame_no=0;
    last_frame=frame;
  }
}

AVISource::~AVISource() {
  if (hic) {
    !ex ? ICDecompressEnd(hic) : ICDecompressExEnd(hic);
    ICClose(hic);
  }
  if (pvideo) delete pvideo;
  if (aSrc) delete aSrc;
  if (audioStreamSource) delete audioStreamSource;
  if (pfile)
    pfile->Release();
  AVIFileExit();
  if (pbiSrc)
    free(pbiSrc);
}

const VideoInfo& AVISource::GetVideoInfo() { return vi; }

PVideoFrame AVISource::GetFrame(int n, IScriptEnvironment* env) {
  
  n = min(max(n, 0), vi.num_frames-1);
  
  if (n != last_frame_no) {
    // find the last keyframe
    int keyframe = pvideo->NearestKeyFrame(n);
    // maybe we don't need to go back that far
    if (last_frame_no < n && last_frame_no >= keyframe)
      keyframe = last_frame_no+1;
    if (keyframe < 0) keyframe = 0;
    bool not_found_yet=false;
    do {
      for (int i = keyframe; i <= n; ++i) {
        PVideoFrame frame = env->NewVideoFrame(vi, 4);
        LRESULT error = DecompressFrame(i, i != n, frame->GetWritePtr());
        // we don't want dropped frames to throw an error
        // Experiment to remove ALL error reporting, so it will always return last valid frame.
        if (error != ICERR_OK && !dropped_frame) {
          //        env->ThrowError("AVISource: failed to decompress frame %d (error %d)", i, error);
        }
        last_frame_no = i;
        if ((!dropped_frame) && frame && (error == ICERR_OK)) last_frame = frame;   // Better safe than sorry
      }
      if (!last_frame) {  // Last keyframe was not valid.
        not_found_yet=true;
        int key_pre=keyframe;
        keyframe = pvideo->NearestKeyFrame(keyframe-1);
        if (keyframe == key_pre) {
          env->ThrowError("AVISource: could not find valid keyframe for frame %d.", n);
        }
      }
      
    } while(not_found_yet);    
  }
  return last_frame;
}

void AVISource::GetAudio(void* buf, int start, int count, IScriptEnvironment* env) {
  LONG bytes_read=0, samples_read=0;

  if (start < 0) {
    int bytes = vi.BytesFromAudioSamples(min(-start, count));
    memset(buf, 0, bytes);
    buf = (char*)buf + bytes;
    count -= vi.AudioSamplesFromBytes(bytes);
    start += vi.AudioSamplesFromBytes(bytes);
  }

  if (audioStreamSource) {
    if (start != audio_stream_pos)
        audioStreamSource->Seek(start);
    samples_read = audioStreamSource->Read(buf, count, &bytes_read);
    audio_stream_pos = start + samples_read;
  }

  if (samples_read < count)
    memset((char*)buf + bytes_read, 0, vi.BytesFromAudioSamples(count) - bytes_read);
}

bool AVISource::GetParity(int n) { return false; }



/********************************************************************
********************************************************************/


class StaticImage : public IClip {
  const VideoInfo vi;
  const PVideoFrame frame;

public:
  StaticImage(const VideoInfo& _vi, const PVideoFrame& _frame)
    : vi(_vi), frame(_frame) {}
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env) { return frame; }
  void __stdcall GetAudio(void* buf, int start, int count, IScriptEnvironment* env) {
    memset(buf, 0, vi.BytesFromAudioSamples(count));
  }
  const VideoInfo& __stdcall GetVideoInfo() { return vi; }
  bool __stdcall GetParity(int n) { return vi.field_based ? (n&1) : false; }
};

 
static PVideoFrame CreateBlankFrame(const VideoInfo& vi, int color, IScriptEnvironment* env) {
  if (!vi.HasVideo()) return 0;
  PVideoFrame frame = env->NewVideoFrame(vi);
  BYTE* p = frame->GetWritePtr();
  const int size = frame->GetPitch() * frame->GetHeight();
  if (vi.IsYUY2()) {
    int color_yuv = RGB2YUV(color);
    unsigned d = (color_yuv>>16) * 0x010001 + ((color_yuv>>8)&255) * 0x0100 + (color_yuv&255) * 0x01000000;
    for (int i=0; i<size; i+=4)
      *(unsigned*)(p+i) = d;
  } else if (vi.IsRGB24()) {
    for (int y=frame->GetHeight();y>0;y--) {
      for (int i=0; i<frame->GetRowSize(); i+=3) {
        p[i] = color&0xff; p[i+1] = (color>>8)&0xff; p[i+2] = (color>>16)&0xff;
      }
      p+=frame->GetPitch();
    }
  } else if (vi.IsRGB32()) {
    for (int i=0; i<size; i+=4)
      *(unsigned*)(p+i) = color;
  }
  return frame;
}

static AVSValue __cdecl Create_BlankClip(AVSValue args, void*, IScriptEnvironment* env) {
  VideoInfo vi_default = { 640, 480, 24, 1, 240, VideoInfo::BGR32, false, 44100, 0, true, true };
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
    if (!lstrcmpi(pixel_type_string, "YUY2"))
      vi.pixel_type = VideoInfo::YUY2;
    else if (!lstrcmpi(pixel_type_string, "RGB24"))
      vi.pixel_type = VideoInfo::BGR24;
    else if (!lstrcmpi(pixel_type_string, "RGB32"))
      vi.pixel_type = VideoInfo::BGR32;
    else
      env->ThrowError("BlankClip: pixel_type must be \"RGB32\", \"RGB24\", or \"YUY2\"");
  }
  double n = args[5].AsFloat(double(vi_default.fps_numerator));
  if (args[5].Defined() && !args[6].Defined()) {
    unsigned d = 1;
    while (n < 16777216 && d < 16777216) { n*=2; d*=2; }
    vi.SetFPS(int(n+0.5), d);
  } else {
    vi.SetFPS(int(n+0.5), args[6].AsInt(vi_default.fps_denominator));
  }
  vi.field_based = vi_default.field_based;
  vi.audio_samples_per_second = args[7].AsInt(vi_default.audio_samples_per_second);
  vi.stereo = args[8].AsBool(vi_default.stereo);
  vi.sixteen_bit = args[9].AsBool(vi_default.sixteen_bit);
  vi.num_audio_samples = vi.AudioSamplesFromFrames(vi.num_frames);
  int color = args[10].AsInt(0);
  return new StaticImage(vi, CreateBlankFrame(vi, color, env));
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
  vi.field_based = false;

  PVideoFrame frame = CreateBlankFrame(vi, bgcolor, env);
  ApplyMessage(&frame, vi, message, size, textcolor, halocolor, bgcolor, env);
  return new StaticImage(vi, frame);
};


AVSValue __cdecl Create_MessageClip(AVSValue args, void*, IScriptEnvironment* env) {
  return Create_MessageClip(args[0].AsString(), args[1].AsInt(-1),
      args[2].AsInt(-1), VideoInfo::BGR24, args[3].AsBool(false),
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
    vi.pixel_type = VideoInfo::BGR32;
    vi.field_based = false;
    vi.sixteen_bit = true;
    vi.stereo = true;
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

  void __stdcall GetAudio(void* buf, int start, int count, IScriptEnvironment*) {
    double Hz=440;
    double add_per_sample=Hz/(double)vi.audio_samples_per_second;
    double second_offset=(double)start*add_per_sample;
    int d_mod=vi.audio_samples_per_second*2;
    short* samples = (short*)buf;
    for (int i=0;i<count;i++) {
        samples[i*2]=(short)(32767.0*sin(3.1415926535897932384626433832795*2.0*second_offset));
        if (((start+i)%d_mod)>vi.audio_samples_per_second) {
          samples[i*2+1]=samples[i*2];
        } else {
          samples[i*2+1]=0;
        } 
        second_offset+=add_per_sample;
    }
  }

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env) {
    return new ColorBars(args[0].AsInt(), args[1].AsInt(), env);
  }
};


/********************************************************************
********************************************************************/


#include <evcode.h>
#include <control.h>
#include <strmif.h>
#include <amvideo.h>
#include <dvdmedia.h>
#include <vfwmsgs.h>
#include <initguid.h>
#include <uuids.h>
#include <errors.h>


class GetSample;


class GetSampleEnumPins : public IEnumPins {
  long refcnt;
  GetSample* const parent;
  int pos;
public:
  GetSampleEnumPins(GetSample* _parent, int _pos=0);

  // IUnknown
  
  ULONG __stdcall AddRef() { InterlockedIncrement(&refcnt); return refcnt; }
  ULONG __stdcall Release() {
    if (!InterlockedDecrement(&refcnt)) {
      delete this;
      return 0;
    } else {
      return refcnt;
    }
  }
  HRESULT __stdcall QueryInterface(REFIID iid, void** ppv) {
    if (iid == IID_IUnknown)
    *ppv = static_cast<IUnknown*>(this);
    else if (iid == IID_IEnumPins)
    *ppv = static_cast<IEnumPins*>(this);
    else {
      *ppv = 0;
      return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
  }

  // IEnumPins

  HRESULT __stdcall Next(ULONG cPins, IPin** ppPins, ULONG* pcFetched);
  HRESULT __stdcall Skip(ULONG cPins) { return E_NOTIMPL; }
  HRESULT __stdcall Reset() { pos=0; return S_OK; }
  HRESULT __stdcall Clone(IEnumPins** ppEnum) { return E_NOTIMPL; }
};


class GetSample : public IBaseFilter, public IPin, public IMemInputPin {

  long refcnt;
  IPin* source_pin;  // not refcounted
  IFilterGraph* filter_graph;  // not refcounted
  IReferenceClock* pclock;  // not refcounted
  FILTER_STATE state;
  bool end_of_stream, flushing;

  VideoInfo vi;

  HANDLE evtDoneWithSample, evtNewSampleReady;

  PVideoFrame pvf;
  __int64 sample_start_time, sample_end_time;

  IScriptEnvironment* const env;

public:

  GetSample(IScriptEnvironment* _env) : env(_env) {
    refcnt = 1;
    source_pin = 0;
    filter_graph = 0;
    pclock = 0;
    state = State_Stopped;
    flushing = end_of_stream = false;
    memset(&vi, 0, sizeof(vi));
    sample_end_time = sample_start_time = 0;
    evtDoneWithSample = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    evtNewSampleReady = ::CreateEvent(NULL, FALSE, FALSE, NULL);
  }

  ~GetSample() {
    CloseHandle(evtDoneWithSample);
    CloseHandle(evtNewSampleReady);
  }

  // These are utility functions for use by DirectShowSource.  Note that
  // the other thread (the one used by the DirectShow filter graph side of
  // things) is always blocked when any of these functions is called, and
  // is always blocked when they return, though it may be temporarily
  // unblocked in between.

  bool IsConnected() { return !!source_pin; }

  bool IsEndOfStream() { return end_of_stream; }

  const VideoInfo& GetVideoInfo() { return vi; }

  PVideoFrame GetCurrentFrame() { return pvf; }

  __int64 GetSampleStartTime() { return sample_start_time; }
  __int64 GetSampleEndTime() { return sample_end_time; }

  void StartGraph() {
    IMediaControl* mc;
    filter_graph->QueryInterface(&mc);
    mc->Run();
    mc->Release();
    _RPT0(0,"StartGraph() waiting for new sample...\n");
    WaitForSingleObject(evtNewSampleReady, 5000);    // MAX wait time = 5000ms!
    _RPT0(0,"...StartGraph() finished waiting for new sample\n");

  }

  void StopGraph() {
    IMediaControl* mc;
    filter_graph->QueryInterface(&mc);
    state = State_Paused;
    _RPT0(0,"StopGraph() indicating done with sample\n");
    PulseEvent(evtDoneWithSample);
    mc->Stop();
    mc->Release();

  }

  void PauseGraph() {
    IMediaControl* mc;
    filter_graph->QueryInterface(&mc);
    state = State_Paused;
    _RPT0(0,"PauseGraph() indicating done with sample\n");
    PulseEvent(evtDoneWithSample);
    mc->Pause();
    mc->Release();

  }

  HRESULT SeekTo(__int64 pos) {
    PauseGraph();
    HRESULT hr;
    IMediaSeeking* ms;
    filter_graph->QueryInterface(&ms);

    LONGLONG pStop=-1;
    LONGLONG pCurrent=-1;
    _RPT0(0,"SeekTo() seeking to new position\n");

    DWORD pCapabilities = AM_SEEKING_CanGetCurrentPos;
    HRESULT canDo = ms->CheckCapabilities(&pCapabilities);
    if (canDo) {
      HRESULT hr2 = ms->GetPositions(&pStop,&pCurrent);
    }

    pCapabilities = AM_SEEKING_CanSeekAbsolute;
    canDo = ms->CheckCapabilities(&pCapabilities);

    if (canDo) {
       hr = ms->SetPositions(&pos, AM_SEEKING_AbsolutePositioning, 0, 0);
    } else {
      pCapabilities = AM_SEEKING_CanSeekForwards;
      canDo = ms->CheckCapabilities(&pCapabilities);
      if (canDo && (pCurrent!=-1)) {
        pCurrent-=pos;
        hr = ms->SetPositions(&pCurrent, AM_SEEKING_RelativePositioning, 0, 0);
      } else {
        // No way of seeking
        ms->Release();
        StartGraph();
        return S_FALSE;
      }
    }
    ms->Release();
    StartGraph();

    return S_OK;  // Seek ok
  }

  void NextSample() {
    if (end_of_stream) return;
    _RPT0(0,"NextSample() indicating done with sample...\n");
    SetEvent(evtDoneWithSample);
    _RPT0(0,"...NextSample() waiting for new sample...\n");
    WaitForSingleObject(evtNewSampleReady, INFINITE);
    _RPT0(0,"...NextSample() done waiting for new sample\n");
  }

  // IUnknown

  ULONG __stdcall AddRef() { InterlockedIncrement(&refcnt); _RPT1(0,"GetSample::AddRef() -> %d\n", refcnt); return refcnt; }
  ULONG __stdcall Release() { InterlockedDecrement(&refcnt); _RPT1(0,"GetSample::Release() -> %d\n", refcnt); return refcnt; }
  HRESULT __stdcall QueryInterface(REFIID iid, void** ppv) {
    if (iid == IID_IUnknown)
    *ppv = static_cast<IUnknown*>(static_cast<IBaseFilter*>(this));
    else if (iid == IID_IPersist)
    *ppv = static_cast<IPersist*>(this);
    else if (iid == IID_IMediaFilter)
    *ppv = static_cast<IMediaFilter*>(this);
    else if (iid == IID_IBaseFilter)
    *ppv = static_cast<IBaseFilter*>(this);
    else if (iid == IID_IPin)
    *ppv = static_cast<IPin*>(this);
    else if (iid == IID_IMemInputPin)
    *ppv = static_cast<IMemInputPin*>(this);
    else if (iid == IID_IMediaSeeking || iid == IID_IMediaPosition) {
      // We don't implement IMediaSeeking or IMediaPosition, but
      // expose the upstream filter's implementation.  This violates
      // the rule that QueryInterface should be an equivalence
      // relation, but it seems to work for this purpose.
      if (!source_pin)
        return E_NOINTERFACE;
      return source_pin->QueryInterface(iid, ppv);
    } else {
      *ppv = 0;
      return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
  }

  // IPersist

  HRESULT __stdcall GetClassID(CLSID* pClassID) { return E_NOTIMPL; }

  // IMediaFilter

  HRESULT __stdcall Stop() { _RPT0(0,"GetSample::Stop()\n"); state = State_Stopped; return S_OK; }
  HRESULT __stdcall Pause() { _RPT0(0,"GetSample::Pause()\n"); state = State_Paused; return S_OK; }
  HRESULT __stdcall Run(REFERENCE_TIME tStart) { _RPT0(0,"GetSample::Run()\n"); state = State_Running; return S_OK; }
  HRESULT __stdcall GetState(DWORD dwMilliSecsTimeout, FILTER_STATE* State) {
    if (!State) return E_POINTER;
    *State = state;
    return S_OK;
  }
  HRESULT __stdcall SetSyncSource(IReferenceClock* pClock) { pclock = pClock; return S_OK; }
  HRESULT __stdcall GetSyncSource(IReferenceClock** ppClock) {
    if (!ppClock) return E_POINTER;
    *ppClock = pclock;
    if (pclock) pclock->AddRef();
    return S_OK;
  }

  // IBaseFilter

  HRESULT __stdcall EnumPins(IEnumPins** ppEnum) {
    if (!ppEnum) return E_POINTER;
    *ppEnum = new GetSampleEnumPins(this);
    return S_OK;
  }
  HRESULT __stdcall FindPin(LPCWSTR Id, IPin** ppPin) { return E_NOTIMPL; }
  HRESULT __stdcall QueryFilterInfo(FILTER_INFO* pInfo) {
    if (!pInfo) return E_POINTER;
    lstrcpyW(pInfo->achName, L"GetSample");
    pInfo->pGraph = filter_graph;
    if (filter_graph) filter_graph->AddRef();
    return S_OK;
  }
  HRESULT __stdcall JoinFilterGraph(IFilterGraph* pGraph, LPCWSTR pName) {
    filter_graph = pGraph;
    return S_OK;
  }
  HRESULT __stdcall QueryVendorInfo(LPWSTR* pVendorInfo) { return E_NOTIMPL; }

  // IPin

  HRESULT __stdcall Connect(IPin* pReceivePin, const AM_MEDIA_TYPE* pmt) {
    return E_UNEXPECTED;
  }
  HRESULT __stdcall ReceiveConnection(IPin* pConnector, const AM_MEDIA_TYPE* pmt) {
    if (!pConnector || !pmt) return E_POINTER;
    if (GetSample::QueryAccept(pmt) != S_OK) return E_INVALIDARG;
    source_pin = pConnector;
    return S_OK;
  }
  HRESULT __stdcall Disconnect() {
    source_pin = 0;
    return S_OK;
  }
  HRESULT __stdcall ConnectedTo(IPin** ppPin) {
    if (!ppPin) return E_POINTER;
    if (source_pin) source_pin->AddRef();
    *ppPin = source_pin;
    return source_pin ? S_OK : VFW_E_NOT_CONNECTED;
  }
  HRESULT __stdcall ConnectionMediaType(AM_MEDIA_TYPE* pmt) {
    return E_NOTIMPL;
  }
  HRESULT __stdcall QueryPinInfo(PIN_INFO* pInfo) {
    if (!pInfo) return E_POINTER;
    pInfo->pFilter = static_cast<IBaseFilter*>(this);
    AddRef();
    pInfo->dir = PINDIR_INPUT;
    lstrcpyW(pInfo->achName, L"GetSample");
    return S_OK;
  }
  HRESULT __stdcall QueryDirection(PIN_DIRECTION* pPinDir) {
    if (!pPinDir) return E_POINTER;
    *pPinDir = PINDIR_INPUT;
    return S_OK;
  }
  HRESULT __stdcall QueryId(LPWSTR* Id) {
    return E_NOTIMPL;
  }

  HRESULT __stdcall QueryAccept(const AM_MEDIA_TYPE* pmt) {
    if (!pmt) return E_POINTER;

    if (pmt->majortype != MEDIATYPE_Video) {
      if (pmt->majortype == MEDIATYPE_Audio) {
        _RPT0(0, "*** Found majortype Audio\n");
      }
      _RPT0(0, "*** majortype was not video\n");
      return S_FALSE;
    }

    if (pmt->subtype == MEDIASUBTYPE_YUY2) {
      vi.pixel_type = VideoInfo::YUY2;
    } else if (pmt->subtype == MEDIASUBTYPE_RGB24) {
      vi.pixel_type = VideoInfo::BGR24;
    } else if (pmt->subtype == MEDIASUBTYPE_RGB32) {
      vi.pixel_type = VideoInfo::BGR32;
    } else {
      _RPT0(0, "*** subtype rejected\n");
      return S_FALSE;
    }

    BITMAPINFOHEADER* pbi;
    unsigned avg_time_per_frame;
    if (pmt->formattype == FORMAT_VideoInfo) {
      VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)pmt->pbFormat;
      avg_time_per_frame = unsigned(vih->AvgTimePerFrame);
      pbi = &vih->bmiHeader;
      vi.field_based = false;
    } else if (pmt->formattype == FORMAT_VideoInfo2) {
      VIDEOINFOHEADER2* vih = (VIDEOINFOHEADER2*)pmt->pbFormat;
      avg_time_per_frame = unsigned(vih->AvgTimePerFrame);
      pbi = &vih->bmiHeader;
      vi.field_based = !!(vih->dwInterlaceFlags & AMINTERLACE_1FieldPerSample);
    } else {
      _RPT0(0, "*** format rejected\n");
      return S_FALSE;
    }

    vi.width = pbi->biWidth;
    vi.height = pbi->biHeight;
    if (avg_time_per_frame) {
      vi.SetFPS(10000000, avg_time_per_frame);
    } else {
      vi.fps_numerator = 1;
      vi.fps_denominator = 0;
    }
    _RPT4(0, "*** format accepted: %dx%d, pixel_type %d, framerate %d\n",
      vi.width, vi.height, vi.pixel_type, avg_time_per_frame);
    return S_OK;
  }

  HRESULT __stdcall EnumMediaTypes(IEnumMediaTypes** ppEnum) {
    return E_NOTIMPL;
  }
  HRESULT __stdcall QueryInternalConnections(IPin** apPin, ULONG* nPin) {
    return E_NOTIMPL;
  }
  HRESULT __stdcall EndOfStream() {
    _RPT0(0,"GetSample::EndOfStream()\n");
    end_of_stream = true;
    if (state == State_Running) {
      if (filter_graph) {
        IMediaEventSink* mes;
        if (SUCCEEDED(filter_graph->QueryInterface(&mes))) {
          mes->Notify(EC_COMPLETE, (long)S_OK, (long)static_cast<IBaseFilter*>(this));
          mes->Release();
        }
      }
    }
    _RPT0(0,"EndOfStream() indicating new sample ready\n");
    SetEvent(evtNewSampleReady);
    return S_OK;
  }
  HRESULT __stdcall BeginFlush() {
    _RPT0(0,"GetSample::BeginFlush()\n");
    flushing = true;
    end_of_stream = false;
    return S_OK;
  }
  HRESULT __stdcall EndFlush() {
    _RPT0(0,"GetSample::EndFlush()\n");
    flushing = false;
    return S_OK;
  }
  HRESULT __stdcall NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate) {
    return S_OK;
  }

  // IMemInputPin

  HRESULT __stdcall GetAllocator(IMemAllocator** ppAllocator) { return E_NOTIMPL; }
  HRESULT __stdcall NotifyAllocator(IMemAllocator* pAllocator, BOOL bReadOnly) { return S_OK; }
  HRESULT __stdcall GetAllocatorRequirements(ALLOCATOR_PROPERTIES* pProps) { return E_NOTIMPL; }

  HRESULT __stdcall Receive(IMediaSample* pSamples) {
    if (end_of_stream || flushing) {
      _RPT0(0,"discarding sample (end of stream or flushing)\n");
      return E_UNEXPECTED;
    }
    if (S_OK == pSamples->IsPreroll()) {
      _RPT0(0,"discarding sample (preroll)\n");
      return S_OK;
    }
    if (FAILED(pSamples->GetTime(&sample_start_time, &sample_end_time))) {
      _RPT0(0,"failed!\n");
    } else {
      _RPT4(0,"%x%08x - %x%08x",
        DWORD(sample_start_time>>32), DWORD(sample_start_time),
        DWORD(sample_end_time>>32), DWORD(sample_end_time));
      _RPT1(0," (%d)\n", DWORD(sample_end_time - sample_start_time));
    }
    pvf = env->NewVideoFrame(vi, 4);
    PBYTE buf;
    pSamples->GetPointer(&buf);
    env->BitBlt(pvf->GetWritePtr(), pvf->GetPitch(), buf,
      pvf->GetPitch(), pvf->GetRowSize(), pvf->GetHeight());
    if (state == State_Running) {
      SetEvent(evtNewSampleReady);
      WaitForSingleObject(evtDoneWithSample, INFINITE);
    }
    return S_OK;
  }

  HRESULT __stdcall ReceiveMultiple(IMediaSample** ppSamples, long nSamples, long* nSamplesProcessed) {
    for (int i=0; i<nSamples; ++i) {
      HRESULT hr = Receive(ppSamples[i]);
      if (FAILED(hr)) {
        *nSamplesProcessed = i;
        return hr;
      }
    }
    *nSamplesProcessed = nSamples;
    return S_OK;
  }

  HRESULT __stdcall ReceiveCanBlock() { return S_OK; }
};


GetSampleEnumPins::GetSampleEnumPins(GetSample* _parent, int _pos) : parent(_parent) { pos=_pos; refcnt = 1; }

HRESULT __stdcall GetSampleEnumPins::Next(ULONG cPins, IPin** ppPins, ULONG* pcFetched) {
  if (!ppPins || !pcFetched) return E_POINTER;
  int copy = *pcFetched = min(int(cPins), 1-pos);
  if (copy>0) {
    *ppPins = static_cast<IPin*>(parent);
    parent->AddRef();
  }
  pos += copy;
  return int(cPins) > copy ? S_FALSE : S_OK;
}


static bool HasNoConnectedOutputPins(IBaseFilter* bf) {
  IEnumPins* ep;
  if (FAILED(bf->EnumPins(&ep)))
    return true;
  ULONG fetched=1;
  IPin* pin;
  while (S_OK == ep->Next(1, &pin, &fetched)) {
    PIN_DIRECTION dir;
    pin->QueryDirection(&dir);
    if (dir == PINDIR_OUTPUT) {
      IPin* other;
      pin->ConnectedTo(&other);
      if (other) {
        other->Release();
        pin->Release();
        ep->Release();
        return false;
      }
    }
    pin->Release();
  }
  ep->Release();
  return true;
}


static void DisconnectAllPinsAndRemoveFilter(IGraphBuilder* gb, IBaseFilter* bf) {
  IEnumPins* ep;
  if (SUCCEEDED(bf->EnumPins(&ep))) {
    ULONG fetched=1;
    IPin* pin;
    while (S_OK == ep->Next(1, &pin, &fetched)) {
      IPin* other;
      pin->ConnectedTo(&other);
      if (other) {
        gb->Disconnect(other);
        gb->Disconnect(pin);
        other->Release();
      }
      pin->Release();
    }
    ep->Release();
  }
  gb->RemoveFilter(bf);
}


static void RemoveUselessFilters(IGraphBuilder* gb, IBaseFilter* not_this_one) {
  IEnumFilters* ef;
  if (FAILED(gb->EnumFilters(&ef)))
    return;
  ULONG fetched=1;
  IBaseFilter* bf;
  while (S_OK == ef->Next(1, &bf, &fetched)) {
    if (bf != not_this_one) {
      if (HasNoConnectedOutputPins(bf)) {
        DisconnectAllPinsAndRemoveFilter(gb, bf);
        ef->Reset();
      }
    }
    bf->Release();
  }
  ef->Release();
}


static void SetMicrosoftDVtoFullResolution(IGraphBuilder* gb) {
  // Microsoft's DV codec defaults to half-resolution, to everyone's
  // great annoyance.  This will set it to full res if possible.
  // Note that IIPDVDec is not declared in older versions of
  // strmif.h; you may need the Win2000 platform SDK.
  IEnumFilters* ef;
  if (FAILED(gb->EnumFilters(&ef)))
    return;
  ULONG fetched=1;
  IBaseFilter* bf;
  while (S_OK == ef->Next(1, &bf, &fetched)) {
    IIPDVDec* pDVDec;
    if (SUCCEEDED(bf->QueryInterface(&pDVDec))) {
      pDVDec->put_IPDisplay(DVDECODERRESOLUTION_720x480);   // yes, this includes 720x576
      pDVDec->Release();
    }
    bf->Release();
  }
  ef->Release();
  
}


class DirectShowSource : public IClip {

  GetSample get_sample;
  IGraphBuilder* gb;

  VideoInfo vi;
  __int64 duration;
  bool frame_units, known_framerate;
  int avg_time_per_frame;
  __int64 base_sample_time;
  int cur_frame;
  bool no_search;
  IScriptEnvironment* const env;

  void CheckHresult(HRESULT hr, const char* msg, const char* msg2 = "");

public:

  DirectShowSource(const char* filename, int _avg_time_per_frame, bool _seek,IScriptEnvironment* _env) : env(_env), get_sample(_env) {
    CheckHresult(CoCreateInstance(CLSID_FilterGraphNoThread, 0, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void**)&gb), "couldn't create filter graph");
    no_search=!_seek;
    WCHAR filenameW[MAX_PATH];
    MultiByteToWideChar(CP_ACP, 0, filename, -1, filenameW, MAX_PATH);
    CheckHresult(gb->AddFilter(static_cast<IBaseFilter*>(&get_sample), L"GetSample"), "couldn't add GetSample filter");

    CheckHresult(gb->RenderFile(filenameW, NULL), "couldn't open file ", filename);

    if (!get_sample.IsConnected()) {
      env->ThrowError("DirectShowSource: the filter graph manager won't talk to me");
    }

    RemoveUselessFilters(gb, &get_sample);

    SetMicrosoftDVtoFullResolution(gb);

    // Prevent the graph from trying to run in "real time"
    // ... Disabled because it breaks ASF.  Now I know why
    // Avery swears so much.
    IMediaFilter* mf;
    CheckHresult(gb->QueryInterface(&mf), "couldn't get IMediaFilter interface");
    CheckHresult(mf->SetSyncSource(NULL), "couldn't set null sync source");
    mf->Release();

    IMediaSeeking* ms=0;
    CheckHresult(gb->QueryInterface(&ms), "couldn't get IMediaSeeking interface");
    frame_units = SUCCEEDED(ms->SetTimeFormat(&TIME_FORMAT_FRAME));
    if (FAILED(ms->GetDuration(&duration)) || duration == 0) {
      env->ThrowError("DirectShowSource: unable to determine the duration of the video");
    }
    ms->Release();

    vi = get_sample.GetVideoInfo();

    if (_avg_time_per_frame) {
      avg_time_per_frame = _avg_time_per_frame;
      vi.SetFPS(10000000, avg_time_per_frame);
    } else {
      // this is exact (no rounding needed) because of the way the fps is set in GetSample
      avg_time_per_frame = 10000000 / vi.fps_numerator * vi.fps_denominator;
    }
    if (avg_time_per_frame == 0) {
      gb->Release();
      env->ThrowError("DirectShowSource: I can't determine the frame rate of\nthe video; you must use the \"fps\" parameter");
    }

    vi.num_frames = int(frame_units ? duration : duration / avg_time_per_frame);

    get_sample.StartGraph();

    cur_frame = 0;
    base_sample_time = 0;
  }

  ~DirectShowSource() { get_sample.StopGraph(); gb->Release(); }

  const VideoInfo& __stdcall GetVideoInfo() { return vi; }

  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env) {
    n = max(min(n, vi.num_frames-1), 0);
    if (frame_units) {
      if (n < cur_frame || n > cur_frame+10) {
        if ( no_search || get_sample.SeekTo(n)!=S_OK) {
          no_search=true;
          if (cur_frame < n) {
            while (cur_frame < n) {
              get_sample.NextSample();
              cur_frame++;
            } // end while
          } // end if curframe<n  fail, if n is behind cur_frame and no seek.
        } else { // seek ok!
          cur_frame = n;
        }
      } else {
        while (cur_frame < n) {
          get_sample.NextSample();
          cur_frame++;
        }
      }
    } else {
      __int64 sample_time = __int64(n) * avg_time_per_frame + (avg_time_per_frame>>1);
      if (n > cur_frame || n > cur_frame+10) {
        if (no_search || get_sample.SeekTo(sample_time)!=S_OK) {
          no_search=true;
          if (cur_frame<n) {  // seek manually
            while (get_sample.GetSampleEndTime()+base_sample_time <= sample_time) {
              get_sample.NextSample();
            }
            cur_frame = n;
          } // end if curframe<n  fail, if n is behind cur_frame and no seek.
        } else { // seek ok!
          base_sample_time = sample_time - (avg_time_per_frame>>1) - get_sample.GetSampleStartTime();
          cur_frame = n;
        }
      } else {
        while (cur_frame < n) {
          get_sample.NextSample();
          cur_frame++;
        }
      }
    }
    PVideoFrame v = get_sample.GetCurrentFrame();
    if ((cur_frame!=n) && (n%10>4)) {
      env->MakeWritable(&v);
      ApplyMessage(&v, vi, "Video Desync!",256,0xffffff,0,0,env);
    }
    return v;
  }
  bool __stdcall GetParity(int n) { return vi.field_based ? (n&1) : false; }

  void __stdcall GetAudio(void*, int, int, IScriptEnvironment*) {}
};


void DirectShowSource::CheckHresult(HRESULT hr, const char* msg, const char* msg2) {
  if (SUCCEEDED(hr)) return;
//  char buf[1024] = {0};
//  if (!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, hr, 0, buf, 1024, NULL))
  char buf[MAX_ERROR_TEXT_LEN] = {0};
  if (!AMGetErrorText(hr, buf, MAX_ERROR_TEXT_LEN))
    wsprintf(buf, "error code 0x%x", hr);
  env->ThrowError("DirectShowSource: %s%s:\n%s", msg, msg2, buf);
}


AVSValue __cdecl Create_DirectShowSource(AVSValue args, void*, IScriptEnvironment* env) {
  const char* filename = args[0][0].AsString();
  int avg_time_per_frame = args[1].Defined() ? int(10000000 / args[1].AsFloat() + 0.5) : 0;
  return new DirectShowSource(filename, avg_time_per_frame, args[2].AsBool(true), env);
}


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
  void GetAudio(void* buf, int start, int count, IScriptEnvironment* env) {}
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
  if (!use_directshow) pixel_type = args[2].AsString("");
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
        try {
          PClip clip = use_directshow ? (IClip*)(new DirectShowSource(filename, avg_time_per_frame, args[2].AsBool(true),env))
                                    : (IClip*)(new AVISource(filename, bAudio, pixel_type, 0, env));
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

AVSValue __cdecl Create_Version(AVSValue args, void*, IScriptEnvironment* env) {
  return Create_MessageClip(AVS_VERSTR
          "\n\xA9 2000-2002 Ben Rudiak-Gould, et al.\n"
          "http://www.avisynth.org",
  -1, -1, VideoInfo::BGR24, false, 0xECF2BF, 0, 0x404040, env);
}
 

AVSFunction Source_filters[] = {
  { "AVISource", "s+[audio]b[pixel_type]s", AVISource::Create, (void*)0 },
  { "AVIFileSource", "s+[audio]b[pixel_type]s", AVISource::Create, (void*)1 },
  { "WAVSource", "s+", AVISource::Create, (void*)3 },
  { "OpenDMLSource", "s+[audio]b[pixel_type]s", AVISource::Create, (void*)2 },
  { "DirectShowSource", "s+[fps]f[seek]b", Create_DirectShowSource },
  { "SegmentedAVISource", "s+[audio]b[pixel_type]s", Create_SegmentedSource, (void*)0 },
  { "SegmentedDirectShowSource", "s+[fps]f[seek]b", Create_SegmentedSource, (void*)1 },
//  { "QuickTimeSource", "s", QuickTimeSource::Create },
  { "BlankClip", "[clip]c[length]i[width]i[height]i[pixel_type]s[fps]f[fps_denominator]i[audio_rate]i[stereo]b[sixteen_bit]b[color]i", Create_BlankClip },
  { "Blackness", "[clip]c[length]i[width]i[height]i[pixel_type]s[fps]f[fps_denominator]i[audio_rate]i[stereo]b[sixteen_bit]b[color]i", Create_BlankClip },
  { "MessageClip", "s[width]i[height]i[shrink]b[text_color]i[halo_color]i[bg_color]i", Create_MessageClip },
  { "ColorBars", "ii", ColorBars::Create },

  { "Version", "", Create_Version },
  { 0,0,0 }
};

