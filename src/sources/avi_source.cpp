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

/*************************************************************
 *                                                           *
 *  WARNING !! This module has been excluded from the build  *
 *  ========== because it is #include'd in source/source.cpp *
 *                                                           *
 *************************************************************/

#include "../stdafx.h"

#include "../internal.h"
#include "../convert/convert.h"
#include "../filters/transform.h"
#include "./avi/AudioSource.h"
#include "./avi/VD_Audio.h"
#include "./avi/AVIReadHandler.h"


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
  bool bIsType1;

  PVideoFrame last_frame;
  int last_frame_no;
  AudioSource* aSrc;
  AudioStreamSource* audioStreamSource;
  __int64 audio_stream_pos;

  LRESULT DecompressBegin(LPBITMAPINFOHEADER lpbiSrc, LPBITMAPINFOHEADER lpbiDst);
  LRESULT DecompressFrame(int n, bool preroll, BYTE* buf);

  void CheckHresult(HRESULT hr, const char* msg, IScriptEnvironment* env);
  bool AttemptCodecNegotiation(DWORD fccHandler, BITMAPINFOHEADER* bmih);
  void LocateVideoCodec(const char fourCC[], IScriptEnvironment* env);

public:

  enum {
    MODE_NORMAL = 0,
    MODE_AVIFILE,
    MODE_OPENDML,
    MODE_WAV
  };

  AVISource(const char filename[], bool fAudio, const char pixel_type[],
            const char fourCC[], int mode, IScriptEnvironment* env);  // mode: 0=detect, 1=avifile, 2=opendml, 3=avifile (audio only)
  ~AVISource();
  void CleanUp(); // Tritical - Jan 2006
  const VideoInfo& __stdcall GetVideoInfo();
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
  void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) ;
  bool __stdcall GetParity(int n);
  void __stdcall SetCacheHints(int cachehints,int frame_range) { };

  static AVSValue __cdecl Create(AVSValue args, void* user_data, IScriptEnvironment* env) {
    const int mode = int(user_data);
    const bool fAudio = (mode == MODE_WAV) || args[1].AsBool(true);
    const char* pixel_type = (mode != MODE_WAV) ? args[2].AsString("") : "";
    const char* fourCC = (mode != MODE_WAV) ? args[3].AsString("") : "";

    PClip result = new AVISource(args[0][0].AsString(), fAudio, pixel_type, fourCC, mode, env);
    for (int i=1; i<args[0].ArraySize(); ++i)
      result = new_Splice(result, new AVISource(args[0][i].AsString(), fAudio, pixel_type, fourCC, mode, env), false, env);
    return AlignPlanar::Create(result);
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
  long bytes_read;
  if (!hic) {
    bytes_read = vi.BMPSize();
    pvideo->Read(n, 1, buf, vi.BMPSize(), &bytes_read, NULL);
    dropped_frame = !bytes_read;
    return ICERR_OK;
  }
  bytes_read = srcbuffer_size;
  LRESULT err = pvideo->Read(n, 1, srcbuffer, srcbuffer_size, &bytes_read, NULL);
  while (err == AVIERR_BUFFERTOOSMALL || (err == 0 && !srcbuffer)) {
    delete[] srcbuffer;
    pvideo->Read(n, 1, 0, srcbuffer_size, &bytes_read, NULL);
    srcbuffer_size = bytes_read;
    srcbuffer = new BYTE[bytes_read + 16]; // Provide 16 hidden guard bytes for HuffYUV, Xvid, etc bug
    err = pvideo->Read(n, 1, srcbuffer, srcbuffer_size, &bytes_read, NULL);
  }
  dropped_frame = !bytes_read;
  if (dropped_frame) return ICERR_OK;  // If frame is 0 bytes (dropped), return instead of attempt decompressing as Vdub.

  // Fill guard bytes with 0xA5's for Xvid bug
  memset(srcbuffer + bytes_read, 0xA5, 16);
  // and a Null terminator for good measure
  srcbuffer[bytes_read + 15] = 0;

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


void AVISource::LocateVideoCodec(const char fourCC[], IScriptEnvironment* env) {
  AVISTREAMINFO asi;
  CheckHresult(pvideo->Info(&asi, sizeof(asi)), "couldn't get video info", env);
  long size = sizeof(BITMAPINFOHEADER);

  // Read video format.  If it's a
  // type-1 DV, we're going to have to fake it.

  if (bIsType1) {
    if (!(pbiSrc = (BITMAPINFOHEADER *)malloc(size))) env->ThrowError("AviSource: Could not allocate BITMAPINFOHEADER.");

    pbiSrc->biSize      = sizeof(BITMAPINFOHEADER);
    pbiSrc->biWidth     = 720;

    if (asi.dwRate > asi.dwScale*26i64)
      pbiSrc->biHeight      = 480;
    else
      pbiSrc->biHeight      = 576;

    pbiSrc->biPlanes      = 1;
    pbiSrc->biBitCount    = 24;
    pbiSrc->biCompression   = 'dsvd';
    pbiSrc->biSizeImage   = asi.dwSuggestedBufferSize;
    pbiSrc->biXPelsPerMeter = 0;
    pbiSrc->biYPelsPerMeter = 0;
    pbiSrc->biClrUsed     = 0;
    pbiSrc->biClrImportant  = 0;

  } else {
    CheckHresult(pvideo->ReadFormat(0, 0, &size), "couldn't get video format size", env);
    pbiSrc = (LPBITMAPINFOHEADER)malloc(size);
    CheckHresult(pvideo->ReadFormat(0, pbiSrc, &size), "couldn't get video format", env);
  }

  vi.width = pbiSrc->biWidth;
  vi.height = pbiSrc->biHeight;
  vi.SetFPS(asi.dwRate, asi.dwScale);
  vi.num_frames = asi.dwLength;

  // try the requested decoder, if specified
  if (fourCC != NULL && strlen(fourCC) == 4) {
    DWORD fcc = fourCC[0] | (fourCC[1] << 8) | (fourCC[2] << 16) | (fourCC[3] << 24);
    asi.fccHandler = pbiSrc->biCompression = fcc;
  }

  // see if we can handle the video format directly
  if (pbiSrc->biCompression == '2YUY') {
    vi.pixel_type = VideoInfo::CS_YUY2;
  } else if (pbiSrc->biCompression == '21VY') {
    vi.pixel_type = VideoInfo::CS_YV12;
  } else if (pbiSrc->biCompression == '024I') {
    vi.pixel_type = VideoInfo::CS_I420;
  } else if (pbiSrc->biCompression == BI_RGB && pbiSrc->biBitCount == 32) {
    vi.pixel_type = VideoInfo::CS_BGR32;
  } else if (pbiSrc->biCompression == BI_RGB && pbiSrc->biBitCount == 24) {
    vi.pixel_type = VideoInfo::CS_BGR24;
/* For 2.6
  } else if (pbiSrc->biCompression == '008Y') {
    vi.pixel_type = VideoInfo::CS_Y8;
  } else if (pbiSrc->biCompression == '42VY') {
    vi.pixel_type = VideoInfo::CS_YV24;
  } else if (pbiSrc->biCompression == '61VY') {
    vi.pixel_type = VideoInfo::CS_YV16;
  } else if (pbiSrc->biCompression == 'B14Y') {
    vi.pixel_type = VideoInfo::CS_YV411;
*/

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


AVISource::AVISource(const char filename[], bool fAudio, const char pixel_type[], const char fourCC[], int mode, IScriptEnvironment* env) {
  srcbuffer = 0; srcbuffer_size = 0;
  memset(&vi, 0, sizeof(vi));
  ex = false;
  last_frame_no = -1;
  pbiSrc = 0;
  aSrc = 0;
  audioStreamSource = 0;
  pvideo=0;
  pfile=0;
  bIsType1 = false;
  hic = 0;

  AVIFileInit();
  try {

    if (mode == MODE_NORMAL) {
      // if it looks like an AVI file, open in OpenDML mode; otherwise AVIFile mode
      HANDLE h = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
      if (h == INVALID_HANDLE_VALUE) {
        env->ThrowError("AVISource autodetect: couldn't open file '%s'\nError code: %d", filename, GetLastError());
      }
      unsigned int buf[3];
      DWORD bytes_read;
      if (ReadFile(h, buf, 12, &bytes_read, NULL) && bytes_read == 12 && buf[0] == 'FFIR' && buf[2] == ' IVA')
        mode = MODE_OPENDML;
      else
        mode = MODE_AVIFILE;
      CloseHandle(h);
    }

    if (mode == MODE_AVIFILE || mode == MODE_WAV) {    // AVIFile mode
      PAVIFILE paf;
      try { // The damn .WAV clsid handler has only a 48 byte buffer to parse the filename and GPF's
		if (FAILED(AVIFileOpen(&paf, filename, OF_READ, 0)))
		  env->ThrowError("AVIFileSource: couldn't open file '%s'", filename);
      }
	  catch (AvisynthError) {
		throw;
	  }
	  catch (...) {
		env->ThrowError("AVIFileSource: VFW failure, AVIFileOpen(%s), length of filename part must be < 48", filename);
	  }
	  pfile = CreateAVIReadHandler(paf);
    } else {              // OpenDML mode
      pfile = CreateAVIReadHandler(filename);
    }

    if (mode != MODE_WAV) { // check for video stream
      pvideo = pfile->GetStream(streamtypeVIDEO, 0);

      if (!pvideo) { // Attempt DV type 1 video.
        pvideo = pfile->GetStream('svai', 0);
        bIsType1 = true;
      }

      if (pvideo) {
        LocateVideoCodec(fourCC, env);
        if (hic) {
          bool forcedType = !(pixel_type[0] == 0);

/* For 2.6
          bool fY8    = lstrcmpi(pixel_type, "Y8"   ) == 0 || pixel_type[0] == 0;
*/
          bool fYV12  = lstrcmpi(pixel_type, "YV12" ) == 0 || pixel_type[0] == 0;
/* For 2.6
          bool fYV16  = lstrcmpi(pixel_type, "YV16" ) == 0 || pixel_type[0] == 0;
          bool fYV24  = lstrcmpi(pixel_type, "YV24" ) == 0 || pixel_type[0] == 0;
          bool fYV411 = lstrcmpi(pixel_type, "YV411") == 0 || pixel_type[0] == 0;
*/
          bool fYUY2  = lstrcmpi(pixel_type, "YUY2" ) == 0 || pixel_type[0] == 0;
          bool fRGB32 = lstrcmpi(pixel_type, "RGB32") == 0 || pixel_type[0] == 0;
          bool fRGB24 = lstrcmpi(pixel_type, "RGB24") == 0 || pixel_type[0] == 0;
          if (!(fYV12 || fYUY2 || fRGB32 || fRGB24))
            env->ThrowError("AVISource: requested format must be YV12, YUY2, RGB32 or RGB24");

/* For 2.6
          if (!(fY8 || fYV12 || fYV16 || fYV24 || fYV411 || fYUY2 || fRGB32 || fRGB24))
            env->ThrowError("AVISource: requested format must be one of YV24, YV16, YV12, YV411, YUY2, Y8, RGB32 or RGB24");
*/

          // try to decompress to YV12, YV411, YV16, YV24, YUY2, Y8, RGB32, and RGB24 in turn
          memset(&biDst, 0, sizeof(BITMAPINFOHEADER));
          biDst.biSize = sizeof(BITMAPINFOHEADER);
          biDst.biWidth = vi.width;
          biDst.biHeight = vi.height;
          biDst.biPlanes = 1;
          bool bOpen = true;

/* For 2.6
          // YV24
          if (fYV24 && bOpen) {
            vi.pixel_type = VideoInfo::CS_YV24;
            biDst.biSizeImage = vi.BMPSize();
            biDst.biCompression = '42VY';
            biDst.biBitCount = 24;
            if (ICERR_OK == ICDecompressQuery(hic, pbiSrc, &biDst)) {
              _RPT0(0,"AVISource: Opening as YV24.\n");
              bOpen = false;  // Skip further attempts
            } else if (forcedType) {
               env->ThrowError("AVISource: the video decompressor couldn't produce YV24 output");
            }
          }

          // YV16
          if (fYV16 && bOpen) {
            vi.pixel_type = VideoInfo::CS_YV16;
            biDst.biSizeImage = vi.BMPSize();
            biDst.biCompression = '61VY';
            biDst.biBitCount = 16;
            if (ICERR_OK == ICDecompressQuery(hic, pbiSrc, &biDst)) {
              _RPT0(0,"AVISource: Opening as YV16.\n");
              bOpen = false;  // Skip further attempts
            } else if (forcedType) {
               env->ThrowError("AVISource: the video decompressor couldn't produce YV16 output");
            }
          }
*/

          // YV12
          if (fYV12 && bOpen) {
            vi.pixel_type = VideoInfo::CS_YV12;
            biDst.biSizeImage = vi.BMPSize();
            biDst.biCompression = '21VY';
            biDst.biBitCount = 12;
            if (ICERR_OK == ICDecompressQuery(hic, pbiSrc, &biDst)) {
              _RPT0(0,"AVISource: Opening as YV12.\n");
              bOpen = false;  // Skip further attempts
            } else if (forcedType) {
               env->ThrowError("AVISource: the video decompressor couldn't produce YV12 output");
            }
          }

/* For 2.6
// ::FIXME:: Is this the most appropriate order.  Not sure about YUY2 vrs YV411

          // YV411
          if (fYV411 && bOpen) {
            vi.pixel_type = VideoInfo::CS_YV411;
            biDst.biSizeImage = vi.BMPSize();
            biDst.biCompression = 'B14Y';
            biDst.biBitCount = 16;
            if (ICERR_OK == ICDecompressQuery(hic, pbiSrc, &biDst)) {
              _RPT0(0,"AVISource: Opening as YV411.\n");
              bOpen = false;  // Skip further attempts
            } else if (forcedType) {
               env->ThrowError("AVISource: the video decompressor couldn't produce YV411 output");
            }
          }
*/

          // YUY2
          if (fYUY2 && bOpen) {
            vi.pixel_type = VideoInfo::CS_YUY2;
            biDst.biSizeImage = vi.BMPSize();
            biDst.biCompression = '2YUY';
            biDst.biBitCount = 16;
            if (ICERR_OK == ICDecompressQuery(hic, pbiSrc, &biDst)) {
              _RPT0(0,"AVISource: Opening as YUY2.\n");
              bOpen = false;  // Skip further attempts
            } else if (forcedType) {
               env->ThrowError("AVISource: the video decompressor couldn't produce YUY2 output");
            }
          }

          // RGB32
          if (fRGB32 && bOpen) {
            vi.pixel_type = VideoInfo::CS_BGR32;
            biDst.biSizeImage = vi.BMPSize();
            biDst.biCompression = BI_RGB;
            biDst.biBitCount = 32;
            if (ICERR_OK == ICDecompressQuery(hic, pbiSrc, &biDst)) {
              _RPT0(0,"AVISource: Opening as RGB32.\n");
              bOpen = false;  // Skip further attempts
            } else if (forcedType) {
               env->ThrowError("AVISource: the video decompressor couldn't produce RGB32 output");
            }
          }

          // RGB24
          if (fRGB24 && bOpen) {
            vi.pixel_type = VideoInfo::CS_BGR24;
            biDst.biSizeImage = vi.BMPSize();
            biDst.biCompression = BI_RGB;
            biDst.biBitCount = 24;
            if (ICERR_OK == ICDecompressQuery(hic, pbiSrc, &biDst)) {
              _RPT0(0,"AVISource: Opening as RGB24.\n");
              bOpen = false;  // Skip further attempts
            } else if (forcedType) {
               env->ThrowError("AVISource: the video decompressor couldn't produce RGB24 output");
            }
          }

/* For 2.6
          // Y8
          if (fY8 && bOpen) {
            vi.pixel_type = VideoInfo::CS_Y8;
            biDst.biSizeImage = vi.BMPSize();
            biDst.biCompression = '008Y';
            biDst.biBitCount = 8;
            if (ICERR_OK == ICDecompressQuery(hic, pbiSrc, &biDst)) {
              _RPT0(0,"AVISource: Opening as Y8.\n");
              bOpen = false;  // Skip further attempts
            } else if (forcedType) {
               env->ThrowError("AVISource: the video decompressor couldn't produce Y8 output");
            }
          }
*/

          // No takers!
          if (bOpen)
            env->ThrowError("AviSource: Could not open video stream in any supported format.");

          DecompressBegin(pbiSrc, &biDst);
        }
      } else {
        env->ThrowError("AviSource: Could not locate video stream.");
      }
    }

    // check for audio stream
    if (fAudio) /*  && pfile->GetStream(streamtypeAUDIO, 0)) */ {
      aSrc = new AudioSourceAVI(pfile, true);
      if (aSrc->init()) {
          audioStreamSource = new AudioStreamSource(aSrc,
                                                    aSrc->lSampleFirst,
                                                    aSrc->lSampleLast - aSrc->lSampleFirst,
                                                    true);
          WAVEFORMATEX* pwfx;
          pwfx = audioStreamSource->GetFormat();
          vi.audio_samples_per_second = pwfx->nSamplesPerSec;
          vi.nchannels = pwfx->nChannels;
          if (pwfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) {
            vi.sample_type = SAMPLE_FLOAT;
          } else if (pwfx->wBitsPerSample == 16) {
            vi.sample_type = SAMPLE_INT16;
          } else if (pwfx->wBitsPerSample == 8) {
            vi.sample_type = SAMPLE_INT8;
          } else if (pwfx->wBitsPerSample == 24) {
            vi.sample_type = SAMPLE_INT24;
          } else if (pwfx->wBitsPerSample == 32) {
            vi.sample_type = SAMPLE_INT32;
          }
          vi.num_audio_samples = audioStreamSource->GetLength();

          audio_stream_pos = 0;
        }
    }

    // try to decompress frame 0 if not audio only.

    dropped_frame=false;

    if (mode != MODE_WAV) {
      int keyframe = pvideo->NearestKeyFrame(0);
      PVideoFrame frame = env->NewVideoFrame(vi, -4);
      BYTE *ptr = frame->GetWritePtr();
      LRESULT error = DecompressFrame(keyframe, false, ptr);
      if (error != ICERR_OK || (!frame))   // shutdown, if init not succesful.
        env->ThrowError("AviSource: Could not decompress frame 0");

      // Cope with dud AVI files that start with drop
      // frames, just return the first key frame
      if (dropped_frame) {
        keyframe = pvideo->NextKeyFrame(0);
        error = DecompressFrame(keyframe, false, ptr);
        if (error != ICERR_OK)   // shutdown, if init not succesful.
          env->ThrowError("AviSource: Could not decompress first keyframe %d", keyframe);
      }
      last_frame_no=0;
      last_frame=frame;
    }
  }
  catch (...) {
    AVISource::CleanUp();
    throw;
  }
}

AVISource::~AVISource() {
  AVISource::CleanUp();
}

void AVISource::CleanUp() { // Tritical - Jan 2006
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
  if (srcbuffer)  // Tritical May 2005
    delete[] srcbuffer;
}

const VideoInfo& AVISource::GetVideoInfo() { return vi; }

PVideoFrame AVISource::GetFrame(int n, IScriptEnvironment* env) {

  n = min(max(n, 0), vi.num_frames-1);
  dropped_frame=false;
  if (n != last_frame_no) {
    // find the last keyframe
    int keyframe = pvideo->NearestKeyFrame(n);
    // maybe we don't need to go back that far
    if (last_frame_no < n && last_frame_no >= keyframe)
      keyframe = last_frame_no+1;
    if (keyframe < 0) keyframe = 0;
    bool not_found_yet=false;

    PVideoFrame frame = env->NewVideoFrame(vi, -4);
    BYTE* frameWritePtr = frame->GetWritePtr();

    do {
      for (int i = keyframe; i <= n; ++i) {
        LRESULT error = DecompressFrame(i, i != n, frameWritePtr);
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

void AVISource::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) {
  __int64 bytes_read=0, samples_read=0;

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
    samples_read = audioStreamSource->Read(buf, count, (long *)&bytes_read);
    audio_stream_pos = start + samples_read;
  }

  if (samples_read < count)
    memset((char*)buf + bytes_read, 0, vi.BytesFromAudioSamples(count) - bytes_read);
}

bool AVISource::GetParity(int n) { return false; }
