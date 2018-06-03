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

#include <avisynth.h>
#include "../../convert/convert.h"
#include "../../filters/transform.h"
#include "../../core/alignplanar.h"
#include "AudioSource.h"
#include "VD_Audio.h"
#include "AVIReadHandler.h"
#include "avi_source.h"
#include <vfw.h>
#include <avs/minmax.h>
#include <emmintrin.h>
#include <tmmintrin.h>

#include "../../core/AviHelper.h"

static void __cdecl free_buffer(void* buff, IScriptEnvironment* env)
{
  if (buff)
    static_cast<IScriptEnvironment2*>(env)->Free(buff);
}

TemporalBuffer::TemporalBuffer(const VideoInfo& vi, bool bMediaPad,
  bool b64a, bool b48r, bool v210,
  bool P010, bool P016, bool P210, bool P216, bool v410, bool Y416,
  bool r210, bool R10k,
  IScriptEnvironment* env)
{
  int heightY = vi.height;
  int heightUV = (vi.pixel_type & VideoInfo::CS_INTERLEAVED) ? 0 : heightY >> vi.GetPlaneHeightSubsampling(PLANAR_U);

  if (Y416)
  { // Y416 packed 4444 U,Y,V,A
    // image_size = vi->width * vi->height * 4 * sizeof(uint16_t);
    pitchY = vi.width * 4 * sizeof(uint16_t); // n/a
    pitchUV = 0;
  }
  else if (v410)
  { // v410 packed 444 U,Y,V
    // image_size = vi->width * vi->height * 4 (32 bit holds 3x10 bits);
    pitchY = vi.width * 4; // n/a
    pitchUV = 0;
  }
  else if (v210) {
    pitchY = ((16 * ((vi.width + 5) / 6) + 127) & ~127);
    pitchUV = 0;
  }
  else if (r210) {
    pitchY = ((vi.width + 63) / 64) * 256;
    pitchUV = 0;
  }
  else if (R10k) {
    pitchY = vi.width * 4;
    pitchUV = 0;
  }
  else if (vi.IsPlanarRGB() || vi.IsPlanarRGBA()) {
    pitchY = vi.RowSize();
  }
  else {
    if (!bMediaPad) { // do not padding at all.
      pitchY = vi.RowSize();
      pitchUV = vi.RowSize(PLANAR_U);
    }
    else { // align 4bytes with minimum padding.
      pitchY = (vi.RowSize() + 3) & ~3;
      pitchUV = (vi.RowSize(PLANAR_U) + 3) & ~3;
    }
  }

  size_t sizeY = pitchY * heightY;
  size_t sizeUV = pitchUV * heightUV;
  if(r210 || R10k || Y416 || v410 || v210)
    size = sizeY;
  else if (vi.IsPlanarRGB())
    size = sizeY * 3;
  else if (vi.IsPlanarRGBA())
    size = sizeY * 4;
  else
    size = sizeY + 2 * sizeUV;

  auto env2 = static_cast<IScriptEnvironment2*>(env);
  // maybe memcpy become fast by aligned start address.
  orig = env2->Allocate(size, FRAME_ALIGN, AVS_POOLED_ALLOC);
  if (!orig)
    env->ThrowError("AVISource: couldn't allocate temporal buffer.");
  env->AtExit(free_buffer, orig);

  pY = reinterpret_cast<uint8_t*>(orig);
  pA = nullptr;
  if (vi.IsPlanarRGB() || vi.IsPlanarRGBA())
  {
    // pY: G
    pU = pY + sizeY; // B
    pV = pU + sizeY; // R
    if (vi.IsPlanarRGBA())
      pA = pV + sizeY; // A
  } else if (vi.pixel_type & VideoInfo::CS_UPlaneFirst) {
    pU = pY + sizeY;
    pV = pU + sizeUV;
  } else {
    pV = pY + sizeY;
    pU = pV + sizeUV;
  }
}

// todo:
// // 10-bit+ planar RGB
/*
mmioFOURCC('G', '3', 00, 10); // ffmpeg GBRP10LE
mmioFOURCC('G', '3', 00, 12); // ffmpeg GBRP12LE
mmioFOURCC('G', '3', 00, 14); // ffmpeg GBRP14LE
mmioFOURCC('G', '3', 00, 16); // ffmpeg GBRP16LE
mmioFOURCC('G', '4', 00, 10); // ffmpeg GBRAP10LE
mmioFOURCC('G', '4', 00, 12); // ffmpeg GBRAP12LE
mmioFOURCC('G', '4', 00, 14); // ffmpeg GBRAP14LE
mmioFOURCC('G', '4', 00, 16); // ffmpeg GBRAP16LE
*/
static PVideoFrame AdjustFrameAlignment(TemporalBuffer* frame, const VideoInfo& vi, bool bInvertFrames, 
  bool b64a, bool b48r, bool v210, 
  bool P010, bool P016, bool P210, bool P216, bool v410, bool Y416,
  bool r210, bool R10k,
  IScriptEnvironment* env)
{
    auto result = env->NewVideoFrame(vi);
    BYTE* dstp = result->GetWritePtr();
    int pitch = result->GetPitch();
    int height = result->GetHeight();
    if (bInvertFrames) { // write from bottom to top
      dstp += pitch * (height - 1);
      pitch = -pitch;
    }

    if (v210) {
      v210_to_yuv422p10(dstp, pitch, result->GetWritePtr(PLANAR_U), result->GetWritePtr(PLANAR_V), result->GetPitch(PLANAR_U),
        frame->GetPtr(), vi.width, vi.height);
    }
    else if (P210 || P216 || P010 || P016) {
      Px10_16_to_yuv42xp10_16(dstp, pitch, result->GetWritePtr(PLANAR_U), result->GetWritePtr(PLANAR_V), result->GetPitch(PLANAR_U),
        frame->GetPtr(), frame->GetPitch(), vi.width, vi.height, result->GetHeight(PLANAR_U),
        P016 || P216, env);
    }
    else if (Y416) {
      FromY416_c<false>(dstp, pitch, result->GetWritePtr(PLANAR_U), result->GetWritePtr(PLANAR_V), result->GetPitch(PLANAR_U),
        result->GetWritePtr(PLANAR_A), result->GetPitch(PLANAR_A),
        frame->GetPtr(), frame->GetPitch(), vi.width, vi.height);
    }
    else if (r210) {
      From_r210_c(result->GetWritePtr(PLANAR_R), result->GetWritePtr(PLANAR_G), result->GetWritePtr(PLANAR_B), result->GetPitch(PLANAR_G),
        frame->GetPtr(), frame->GetPitch(), vi.width, vi.height);
    }
    else if (R10k) {
      From_R10k_c(result->GetWritePtr(PLANAR_R), result->GetWritePtr(PLANAR_G), result->GetWritePtr(PLANAR_B), result->GetPitch(PLANAR_G),
        frame->GetPtr(), frame->GetPitch(), vi.width, vi.height);
    }
    else if (v410) {
      v410_to_yuv444p10(dstp, pitch, result->GetWritePtr(PLANAR_U), result->GetWritePtr(PLANAR_V), result->GetPitch(PLANAR_U),
        frame->GetPtr(), vi.width, vi.height);
    }
    else {
      if (b64a) {
        // BGRA <-> big endian ARGB with byte swap
        uint8_t* pdst = dstp;

        int srcpitch = frame->GetPitch();
        const BYTE *src = frame->GetPtr();
        const bool ssse3 = (env->GetCPUFlags() & CPUF_SSSE3) != 0;
        const bool sse2 = (env->GetCPUFlags() & CPUF_SSE2) != 0; 
        if (ssse3)
          bgra_to_argbBE_sse<true>(pdst, pitch, src, srcpitch, vi.width, vi.height);
        else if (sse2)
          bgra_to_argbBE_sse<false>(pdst, pitch, src, srcpitch, vi.width, vi.height);
        else
          bgra_to_argbBE_c(pdst, pitch, src, srcpitch, vi.width, vi.height);
      }
      else if (b48r) {
        // BGR <-> 16 bits per component big-endian Red, Green and Blue with byte swap
        uint8_t* pdst = dstp;

        int srcpitch = frame->GetPitch();
        const BYTE *src = frame->GetPtr();
        // c only
        bgr_to_rgbBE_c(pdst, pitch, src, srcpitch, vi.width, vi.height);
      }
      else {
        if (vi.IsPlanarRGB() || vi.IsPlanarRGBA()) {
          env->BitBlt(result->GetWritePtr(PLANAR_G), result->GetPitch(PLANAR_G), frame->GetPtr(PLANAR_G), frame->GetPitch(PLANAR_G), result->GetRowSize(PLANAR_G), result->GetHeight(PLANAR_G));
          env->BitBlt(result->GetWritePtr(PLANAR_B), result->GetPitch(PLANAR_B), frame->GetPtr(PLANAR_B), frame->GetPitch(PLANAR_B), result->GetRowSize(PLANAR_B), result->GetHeight(PLANAR_B));
          env->BitBlt(result->GetWritePtr(PLANAR_R), result->GetPitch(PLANAR_R), frame->GetPtr(PLANAR_R), frame->GetPitch(PLANAR_R), result->GetRowSize(PLANAR_R), result->GetHeight(PLANAR_R));
          if(vi.IsPlanarRGBA())
            env->BitBlt(result->GetWritePtr(PLANAR_A), result->GetPitch(PLANAR_A), frame->GetPtr(PLANAR_A), frame->GetPitch(PLANAR_A), result->GetRowSize(PLANAR_A), result->GetHeight(PLANAR_A));
        }
        else {
          env->BitBlt(dstp, pitch, frame->GetPtr(), frame->GetPitch(), result->GetRowSize(), result->GetHeight());
          env->BitBlt(result->GetWritePtr(PLANAR_V), result->GetPitch(PLANAR_V), frame->GetPtr(PLANAR_V), frame->GetPitch(PLANAR_V), result->GetRowSize(PLANAR_V), result->GetHeight(PLANAR_V));
          env->BitBlt(result->GetWritePtr(PLANAR_U), result->GetPitch(PLANAR_U), frame->GetPtr(PLANAR_U), frame->GetPitch(PLANAR_U), result->GetRowSize(PLANAR_U), result->GetHeight(PLANAR_U));
        }
      }
    }
    return result;
}

#ifndef MSVC
static __inline LRESULT
ICDecompressEx(HIC hic,DWORD dwFlags,LPBITMAPINFOHEADER lpbiSrc,LPVOID lpSrc,INT xSrc,INT ySrc,INT dxSrc,INT dySrc,LPBITMAPINFOHEADER lpbiDst,LPVOID lpDst,INT xDst,INT yDst,INT dxDst,INT dyDst)
{
	ICDECOMPRESSEX ic;
	ic.dwFlags = dwFlags;
	ic.lpbiSrc = lpbiSrc;
	ic.lpSrc = lpSrc;
	ic.xSrc = xSrc;
	ic.ySrc = ySrc;
	ic.dxSrc = dxSrc;
	ic.dySrc = dySrc;
	ic.lpbiDst = lpbiDst;
	ic.lpDst = lpDst;
	ic.xDst = xDst;
	ic.yDst = yDst;
	ic.dxDst = dxDst;
	ic.dyDst = dyDst;
	return ICSendMessage(hic,ICM_DECOMPRESSEX,(DWORD_PTR)&ic,sizeof(ic));
}

static __inline LRESULT
ICDecompressExBegin(HIC hic,DWORD dwFlags,LPBITMAPINFOHEADER lpbiSrc,LPVOID lpSrc,INT xSrc,INT ySrc,INT dxSrc,INT dySrc,LPBITMAPINFOHEADER lpbiDst,LPVOID lpDst,INT xDst,INT yDst,INT dxDst,INT dyDst)
{
	ICDECOMPRESSEX ic;
	ic.dwFlags = dwFlags;
	ic.lpbiSrc = lpbiSrc;
	ic.lpSrc = lpSrc;
	ic.xSrc = xSrc;
	ic.ySrc = ySrc;
	ic.dxSrc = dxSrc;
	ic.dySrc = dySrc;
	ic.lpbiDst = lpbiDst;
	ic.lpDst = lpDst;
	ic.xDst = xDst;
	ic.yDst = yDst;
	ic.dxDst = dxDst;
	ic.dyDst = dyDst;
	return ICSendMessage(hic,ICM_DECOMPRESSEX_BEGIN,(DWORD_PTR)&ic,sizeof(ic));
}
#endif // MSVC

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

LRESULT AVISource::DecompressFrame(int n, bool preroll, IScriptEnvironment* env) {
  AVS_UNUSED(env);
  _RPT2(0,"AVISource: Decompressing frame %d%s\n", n, preroll ? " (preroll)" : "");
  BYTE* buf = frame->GetPtr();
  long bytes_read;

  if (!hic) {
    bytes_read = long(frame->GetSize());
    pvideo->Read(n, 1, buf, bytes_read, &bytes_read, NULL);
    dropped_frame = !bytes_read;
    if (dropped_frame) return ICERR_OK;  // If frame is 0 bytes (dropped), return instead of attempt decompressing as Vdub.
  }
  else {
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
    LRESULT result = !ex ? ICDecompress(hic, flags, pbiSrc, srcbuffer, &biDst, buf)
                         : ICDecompressEx(hic, flags, pbiSrc, srcbuffer,
                                          0, 0, vi.width, vi.height, &biDst, buf,
                                          0, 0, vi.width, vi.height);
    if (result != ICERR_OK) return result;
  }
  return ICERR_OK;
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

      hic = ICLocate(ICTYPE_VIDEO, 0, bmih, NULL, ICMODE_DECOMPRESS);
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
    pbiSrc = (BITMAPINFOHEADER *)malloc(size);
    if (!pbiSrc) env->ThrowError("AviSource: Could not allocate BITMAPINFOHEADER.");

    pbiSrc->biSize      = sizeof(BITMAPINFOHEADER);
    pbiSrc->biWidth     = 720;

    if (asi.dwRate > asi.dwScale*26LL)
      pbiSrc->biHeight      = 480;
    else
      pbiSrc->biHeight      = 576;

    pbiSrc->biPlanes      = 1;
    pbiSrc->biBitCount    = 24;
    pbiSrc->biCompression = MAKEFOURCC('d','v','s','d'); // 'dsvd';
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
  vi.height = pbiSrc->biHeight < 0 ? -pbiSrc->biHeight : pbiSrc->biHeight;
  vi.SetFPS(asi.dwRate, asi.dwScale);
  vi.num_frames = asi.dwLength;

  // try the requested decoder, if specified
  if (fourCC != NULL && strlen(fourCC) == 4) {
    DWORD fcc = fourCC[0] | (fourCC[1] << 8) | (fourCC[2] << 16) | (fourCC[3] << 24);
    asi.fccHandler = pbiSrc->biCompression = fcc;
  }

  // see if we can handle the video format directly
  if (pbiSrc->biCompression == MAKEFOURCC('Y', 'U', 'Y', '2')) { // :FIXME: Handle UYVY, etc
    vi.pixel_type = VideoInfo::CS_YUY2;
  } else if (pbiSrc->biCompression == MAKEFOURCC('Y', 'V', '1', '2')) {
    vi.pixel_type = VideoInfo::CS_YV12;
  } else if (pbiSrc->biCompression == MAKEFOURCC('I', '4', '2', '0')) {
    vi.pixel_type = VideoInfo::CS_I420;
  } else if (pbiSrc->biCompression == BI_RGB && pbiSrc->biBitCount == 32) {
    vi.pixel_type = VideoInfo::CS_BGR32;
  } else if (pbiSrc->biCompression == BI_RGB && pbiSrc->biBitCount == 24) {
    vi.pixel_type = VideoInfo::CS_BGR24;
  } else if (pbiSrc->biCompression == MAKEFOURCC('B','R','A',64)) { // BRA@ ie. BRA[64]
      vi.pixel_type = VideoInfo::CS_BGR64;
  } else if (pbiSrc->biCompression == MAKEFOURCC('B','G','R',48)) { // BGR0 ie. BGR[48]
      vi.pixel_type = VideoInfo::CS_BGR48;
  } else if (pbiSrc->biCompression == MAKEFOURCC('G', 'R', 'E', 'Y')) {
    vi.pixel_type = VideoInfo::CS_Y8;
  } else if (pbiSrc->biCompression == MAKEFOURCC('Y', '8', '0', '0')) {
    vi.pixel_type = VideoInfo::CS_Y8;
  } else if (pbiSrc->biCompression == MAKEFOURCC('Y', '8', ' ', ' ')) {
    vi.pixel_type = VideoInfo::CS_Y8;
  } else if (pbiSrc->biCompression == MAKEFOURCC('Y', 'V', '2', '4')) {
    vi.pixel_type = VideoInfo::CS_YV24;
  } else if (pbiSrc->biCompression == MAKEFOURCC('Y', 'V', '1', '6')) {
    vi.pixel_type = VideoInfo::CS_YV16;
  } else if (pbiSrc->biCompression == MAKEFOURCC('Y', '4', '1', 'B')) {
    vi.pixel_type = VideoInfo::CS_YV411;/*
  } else if (pbiSrc->biCompression == MAKEFOURCC('U', 'Q', 'R', 'A')) {
    vi.pixel_type = VideoInfo::CS_BGR64; // bitcount is 40!
  } else if (pbiSrc->biCompression == MAKEFOURCC('U', 'Q', 'R', 'G')) {
    vi.pixel_type = VideoInfo::CS_BGR48;
  } else if (pbiSrc->biCompression == MAKEFOURCC('U', 'Q', 'Y', '2')) {
    vi.pixel_type = VideoInfo::CS_YUV422P10;*/
  /* UT Video codec
    UQRA	UtVideo Pro RGBA 10bit   RGBA 4:4:4 10bit full-range b64a 
    UQRG	UtVideo Pro RGB 10bit    RGB  4:4:4 10bit full-range b48r (b64a)
    UQY2	UtVideo Pro YUV422 10bit YCbCr 4:2:2 10bit limited   v210
  */
  // otherwise, find someone who will decompress it
  } else {
    switch(pbiSrc->biCompression) {
    case MAKEFOURCC('M','P','4','3'):    // Microsoft MPEG-4 V3 '34PM'
    case MAKEFOURCC('D','I','V','3'):    // "DivX Low-Motion" (4.10.0.3917) '3VID'
    case MAKEFOURCC('D','I','V','4'):    // "DivX Fast-Motion" (4.10.0.3920) 4VID'
    case MAKEFOURCC('A','P','4','1'):    // "AngelPotion Definitive" (4.0.00.3688) '14PA'
      if (AttemptCodecNegotiation(asi.fccHandler, pbiSrc)) return;
      pbiSrc->biCompression = MAKEFOURCC('M', 'P', '4', '3');
      if (AttemptCodecNegotiation(asi.fccHandler, pbiSrc)) return;
      pbiSrc->biCompression = MAKEFOURCC('D', 'I', 'V', '3');
      if (AttemptCodecNegotiation(asi.fccHandler, pbiSrc)) return;
      pbiSrc->biCompression = MAKEFOURCC('D', 'I', 'V', '4');
      if (AttemptCodecNegotiation(asi.fccHandler, pbiSrc)) return;
      pbiSrc->biCompression = MAKEFOURCC('A', 'P', '4', '1');
    default:
      if (AttemptCodecNegotiation(asi.fccHandler, pbiSrc)) return;
    }
    env->ThrowError("AVISource: couldn't locate a decompressor for fourcc %c%c%c%c",
      asi.fccHandler, asi.fccHandler>>8, asi.fccHandler>>16, asi.fccHandler>>24);
  }
}


AVISource::AVISource(const char filename[], bool fAudio, const char pixel_type[], const char fourCC[], int vtrack, int atrack, avi_mode_e mode, IScriptEnvironment* env) {
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
  bInvertFrames = false;
  // todo: too many vars, move them for enum
  P010 = P016 = false;
  P210 = v210 = false;
  P216 = false;
  v410 = false;
  Y416 = false;
  b64a = false;
  b48r = false;
  r210 = false;
  R10k = false;
  bMediaPad = false;
  frame = 0;

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
      if (ReadFile(h, buf, 12, &bytes_read, NULL) && bytes_read == 12 && buf[0] == MAKEFOURCC('R','I','F','F') && buf[2] == MAKEFOURCC('A','V','I',' '))
        mode = MODE_OPENDML;
      else
        mode = MODE_AVIFILE;
      CloseHandle(h);
    }

    if (mode == MODE_AVIFILE || mode == MODE_WAV) {    // AVIFile mode
      PAVIFILE paf = NULL;
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
      pvideo = pfile->GetStream(streamtypeVIDEO, vtrack);

      if (!pvideo) { // Attempt DV type 1 video.
        pvideo = pfile->GetStream(MAKEFOURCC('i','a','v','s'), vtrack);
        bIsType1 = true;
      }

      if (pvideo) {
        LocateVideoCodec(fourCC, env);
        if (hic) {
          if (pixel_type[0] == '+') {
            pixel_type += 1;
            bMediaPad = true;
          }
          bool forcedType = !(pixel_type[0] == 0);

          bool fY8    = pixel_type[0] == 0 || lstrcmpi(pixel_type, "Y8"   ) == 0;
          bool fYV12  = pixel_type[0] == 0 || lstrcmpi(pixel_type, "YV12" ) == 0;
          bool fYV16  = pixel_type[0] == 0 || lstrcmpi(pixel_type, "YV16" ) == 0;
          bool fYV24  = pixel_type[0] == 0 || lstrcmpi(pixel_type, "YV24" ) == 0;
          bool fYV411 = pixel_type[0] == 0 || lstrcmpi(pixel_type, "YV411") == 0;
          bool fYUY2  = pixel_type[0] == 0 || lstrcmpi(pixel_type, "YUY2" ) == 0;
          bool fRGB32 = pixel_type[0] == 0 || lstrcmpi(pixel_type, "RGB32") == 0;
          bool fRGB24 = pixel_type[0] == 0 || lstrcmpi(pixel_type, "RGB24") == 0;
          bool fRGB48 = pixel_type[0] == 0 || lstrcmpi(pixel_type, "RGB48") == 0;
          bool fRGB64 = pixel_type[0] == 0 || lstrcmpi(pixel_type, "RGB64") == 0;

          bool fYUV420P10 = pixel_type[0] == 0 || lstrcmpi(pixel_type, "YUV420P10") == 0;
          bool fP010 = pixel_type[0] == 0 || lstrcmpi(pixel_type, "P010") == 0;

          bool fYUV420P16 = pixel_type[0] == 0 || lstrcmpi(pixel_type, "YUV420P16") == 0;
          bool fP016 = pixel_type[0] == 0 || lstrcmpi(pixel_type, "P016") == 0;

          bool fYUV422P10 = pixel_type[0] == 0 || lstrcmpi(pixel_type, "YUV422P10") == 0;
          // we cannot know it beforehand, don't set it true if not specified, only generic YUV422P10 is valid
          bool fv210 = lstrcmpi(pixel_type, "v210") == 0;
          bool fP210 = lstrcmpi(pixel_type, "P210") == 0;

          bool fYUV422P16 = pixel_type[0] == 0 || lstrcmpi(pixel_type, "YUV422P16") == 0;
          bool fP216 = pixel_type[0] == 0 || lstrcmpi(pixel_type, "P216") == 0;

          bool fYUV444P10 = pixel_type[0] == 0 || lstrcmpi(pixel_type, "YUV444P10") == 0;
          bool fv410 = pixel_type[0] == 0 || lstrcmpi(pixel_type, "v410") == 0;

          bool fYUV444P16 = pixel_type[0] == 0 || lstrcmpi(pixel_type, "YUV444P16") == 0;
          bool fY416 = pixel_type[0] == 0 || lstrcmpi(pixel_type, "Y416") == 0;

          bool fRGBP = pixel_type[0] == 0 || lstrcmpi(pixel_type, "RGBP") == 0; // RGBP means planar RGB 10-16
          bool fGrayscale = pixel_type[0] == 0 || lstrcmpi(pixel_type, "Y") == 0; // Y8-16

          // special 10 bit RGB
          bool fRGBP10 = pixel_type[0] == 0 || lstrcmpi(pixel_type, "RGBP10") == 0;
          bool fr210 = pixel_type[0] == 0 || lstrcmpi(pixel_type, "r210") == 0;
          bool fR10k = pixel_type[0] == 0 || lstrcmpi(pixel_type, "R10k") == 0;

          // we don't set specifically v210, P010, P016, P210, P216, Y410, Y416 for auto.
          // These are set only when negotiated
          if (lstrcmpi(pixel_type, "AUTO") == 0) {
            fY8 = fYV12 = fYUY2 = fRGB32 = fRGB24 = fRGB48 = fRGB64 = true;
            fYUV420P10 = true;
            fYUV420P16 = true;
            fYUV422P10 = true; 
            fYUV422P16 = true;
            fYUV444P10 = true;
            fYUV444P16 = true;
            fRGBP = true;
            fRGBP10 = true;
            fr210 = true;
            fR10k = true;
            fGrayscale = true;
            forcedType = false;
          }
          else if (lstrcmpi(pixel_type, "FULL") == 0) {
            fY8 = fYV12 = fYV16 = fYV24 = fYV411 = fYUY2 = fRGB32 = fRGB24 = fRGB48 = fRGB64 = true;
            fYUV420P10 = true;
            fYUV420P16 = true;
            fYUV422P10 = true;
            fYUV422P16 = true;
            fYUV444P10 = true;
            fYUV444P16 = true;
            fRGBP = true;
            fRGBP10 = true;
            fr210 = true;
            fR10k = true;
            fGrayscale = true;
            forcedType = false;
          }

          if (!(fY8 || fYV12 || fYV16 || fYV24 || fYV411 || fYUY2 || fRGB32 || fRGB24 || fRGB48 || fRGB64
            || fYUV420P10 || fP010
            || fYUV420P16 || fP016
            || fYUV422P10 || fP210 || fv210
            || fYUV422P16 || fP216
            || fYUV444P10 || fv410
            || fYUV444P16 || fY416
            || fRGBP
            || fRGBP10
            || fr210 || fR10k
            || fGrayscale
            ))
            env->ThrowError("AVISource: requested format must be one of YV12/16/24, YV411, YUY2, Y8, Y, RGBP, RGBP10, r210, R10k, RGB24/32/48/64, YUV420P10/16, YUV422P10/16, YUV444P10/16, v210, P010/16, P210/16, v410, Y416, AUTO or FULL");

          // try to decompress to YV12, YV411, YV16, YV24, YUY2, Y8, RGB32, and RGB24, RGB48, RGB64, YUV422P10 in turn
          memset(&biDst, 0, sizeof(BITMAPINFOHEADER));
          biDst.biSize = sizeof(BITMAPINFOHEADER);
          biDst.biWidth = vi.width;
          biDst.biHeight = vi.height;
          biDst.biPlanes = 1;
          bool bOpen = true;

          // YV24
          if (fYV24 && bOpen) {
            vi.pixel_type = VideoInfo::CS_YV24;
            biDst.biSizeImage = vi.BMPSize();
            biDst.biCompression = MAKEFOURCC('Y', 'V', '2', '4');
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
            biDst.biCompression = MAKEFOURCC('Y', 'V', '1', '6');
            biDst.biBitCount = 16;
            if (ICERR_OK == ICDecompressQuery(hic, pbiSrc, &biDst)) {
              _RPT0(0,"AVISource: Opening as YV16.\n");
              bOpen = false;  // Skip further attempts
            } else if (forcedType) {
               env->ThrowError("AVISource: the video decompressor couldn't produce YV16 output");
            }
          }

          // YV12
          if (fYV12 && bOpen) {
            vi.pixel_type = VideoInfo::CS_YV12;
            biDst.biSizeImage = vi.BMPSize();
            biDst.biCompression = MAKEFOURCC('Y', 'V', '1', '2');
            biDst.biBitCount = 12;
            if (ICERR_OK == ICDecompressQuery(hic, pbiSrc, &biDst)) {
              _RPT0(0,"AVISource: Opening as YV12.\n");
              bOpen = false;  // Skip further attempts
            } else if (forcedType) {
               env->ThrowError("AVISource: the video decompressor couldn't produce YV12 output");
            }
          }

// ::FIXME:: Is this the most appropriate order.  Not sure about YUY2 vrs YV411

          // YV411
          if (fYV411 && bOpen) {
            vi.pixel_type = VideoInfo::CS_YV411;
            biDst.biSizeImage = vi.BMPSize();
            biDst.biCompression = MAKEFOURCC('Y', '4', '1', 'B');
            biDst.biBitCount = 16;
            if (ICERR_OK == ICDecompressQuery(hic, pbiSrc, &biDst)) {
              _RPT0(0,"AVISource: Opening as YV411.\n");
              bOpen = false;  // Skip further attempts
            } else if (forcedType) {
               env->ThrowError("AVISource: the video decompressor couldn't produce YV411 output");
            }
          }

          // YUY2
          if (fYUY2 && bOpen) {
            vi.pixel_type = VideoInfo::CS_YUY2;
            biDst.biSizeImage = vi.BMPSize();
            biDst.biCompression = MAKEFOURCC('Y', 'U', 'Y', '2'); // :FIXME: Handle UYVY, etc
            biDst.biBitCount = 16;
            if (ICERR_OK == ICDecompressQuery(hic, pbiSrc, &biDst)) {
              _RPT0(0,"AVISource: Opening as YUY2.\n");
              bOpen = false;  // Skip further attempts
            } else if (forcedType) {
               env->ThrowError("AVISource: the video decompressor couldn't produce YUY2 output");
            }
          }

          // RGBP 10 bit special
          if ((fRGBP10 || fr210 || fR10k) && bOpen) {
            if (fRGBP10 && bOpen) {
              vi.pixel_type = VideoInfo::CS_RGBP10;
              biDst.biSizeImage = vi.BMPSize();
              biDst.biCompression = MAKEFOURCC('G', '3', 0, 10); // ffmpeg GBRP10LE
              biDst.biBitCount = 30;
              if (ICERR_OK == ICDecompressQuery(hic, pbiSrc, &biDst)) {
                _RPT0(0, "AVISource: Opening as G3[0][10].\n");
                bOpen = false;  // Skip further attempts
              }
            }
            if ((fRGBP10 || fr210) && bOpen) {
              vi.pixel_type = VideoInfo::CS_RGBP10;
              biDst.biSizeImage = AviHelper_ImageSize(&vi, false, false, false, true, false);
              biDst.biCompression = MAKEFOURCC('r', '2', '1', '0');
              biDst.biBitCount = 30;
              if (ICERR_OK == ICDecompressQuery(hic, pbiSrc, &biDst)) {
                _RPT0(0, "AVISource: Opening as r210.\n");
                r210 = true;
                bOpen = false;  // Skip further attempts
              }
            }
            if ((fRGBP10 || fR10k) && bOpen) {
              vi.pixel_type = VideoInfo::CS_RGBP10;
              biDst.biSizeImage = AviHelper_ImageSize(&vi, false, false, false, false, true);
              biDst.biCompression = MAKEFOURCC('R', '1', '0', 'k');
              biDst.biBitCount = 30;
              if (ICERR_OK == ICDecompressQuery(hic, pbiSrc, &biDst)) {
                _RPT0(0, "AVISource: Opening as R10k.\n");
                R10k = true;
                bOpen = false;  // Skip further attempts
              }
            }
            if (bOpen && forcedType) {
              env->ThrowError("AVISource: the video decompressor couldn't produce Planar RGB 10 output");
            }
          }

          // RGBP: planar RGB, no specific bit depth
          if (fRGBP && bOpen) {
            vi.pixel_type = VideoInfo::CS_RGBP10;
            biDst.biSizeImage = vi.BMPSize();
            biDst.biCompression = MAKEFOURCC('G', '3', 0, 10); // ffmpeg GBRP10LE
            biDst.biBitCount = 30;
            if (ICERR_OK == ICDecompressQuery(hic, pbiSrc, &biDst)) {
              _RPT0(0, "AVISource: Opening as G3[0][10].\n");
              bOpen = false;  // Skip further attempts
            }
            else {
              vi.pixel_type = VideoInfo::CS_RGBP10;
              biDst.biSizeImage = AviHelper_ImageSize(&vi, false, false, false, true, false);
              biDst.biCompression = MAKEFOURCC('r', '2', '1', '0');
              biDst.biBitCount = 30;
              if (ICERR_OK == ICDecompressQuery(hic, pbiSrc, &biDst)) {
                _RPT0(0, "AVISource: Opening as r210.\n");
                r210 = true;
                bOpen = false;  // Skip further attempts
              }
              else {
                vi.pixel_type = VideoInfo::CS_RGBP10;
                biDst.biSizeImage = AviHelper_ImageSize(&vi, false, false, false, false, true);
                biDst.biCompression = MAKEFOURCC('R', '1', '0', 'k');
                biDst.biBitCount = 30;
                if (ICERR_OK == ICDecompressQuery(hic, pbiSrc, &biDst)) {
                  _RPT0(0, "AVISource: Opening as R10k.\n");
                  R10k = true;
                  bOpen = false;  // Skip further attempts
                }
                else {
                  vi.pixel_type = VideoInfo::CS_RGBP12;
                  biDst.biCompression = MAKEFOURCC('G', '3', 0, 12); // ffmpeg GBRP12LE
                  biDst.biBitCount = 36;
                  if (ICERR_OK == ICDecompressQuery(hic, pbiSrc, &biDst)) {
                    _RPT0(0, "AVISource: Opening as G3[0][12].\n");
                    bOpen = false;  // Skip further attempts
                  }
                  else {
                    vi.pixel_type = VideoInfo::CS_RGBP14;
                    biDst.biCompression = MAKEFOURCC('G', '3', 0, 14); // ffmpeg GBRP14LE
                    biDst.biBitCount = 42;
                    if (ICERR_OK == ICDecompressQuery(hic, pbiSrc, &biDst)) {
                      _RPT0(0, "AVISource: Opening as G3[0][1].\n");
                      bOpen = false;  // Skip further attempts
                    }
                    else {
                      vi.pixel_type = VideoInfo::CS_RGBP16;
                      biDst.biCompression = MAKEFOURCC('G', '3', 0, 16); // ffmpeg GBRP16LE
                      biDst.biBitCount = 48;
                      if (ICERR_OK == ICDecompressQuery(hic, pbiSrc, &biDst)) {
                        _RPT0(0, "AVISource: Opening as G3[0][16].\n");
                        bOpen = false;  // Skip further attempts
                      }
                      else {
                        vi.pixel_type = VideoInfo::CS_RGBAP10;
                        biDst.biSizeImage = vi.BMPSize();
                        biDst.biCompression = MAKEFOURCC('G', '4', 0, 10); // ffmpeg GBRAP10LE
                        biDst.biBitCount = 30;
                        if (ICERR_OK == ICDecompressQuery(hic, pbiSrc, &biDst)) {
                          _RPT0(0, "AVISource: Opening as G4[0][10].\n");
                          bOpen = false;  // Skip further attempts
                        }
                        else {
                          vi.pixel_type = VideoInfo::CS_RGBAP12;
                          biDst.biCompression = MAKEFOURCC('G', '4', 0, 12); // ffmpeg GBRAP12LE
                          biDst.biBitCount = 36;
                          if (ICERR_OK == ICDecompressQuery(hic, pbiSrc, &biDst)) {
                            _RPT0(0, "AVISource: Opening as G4[0][12].\n");
                            bOpen = false;  // Skip further attempts
                          }
                          else {
                            vi.pixel_type = VideoInfo::CS_RGBAP14;
                            biDst.biCompression = MAKEFOURCC('G', '4', 0, 14); // ffmpeg GBRAP14LE
                            biDst.biBitCount = 42;
                            if (ICERR_OK == ICDecompressQuery(hic, pbiSrc, &biDst)) {
                              _RPT0(0, "AVISource: Opening as G4[0][1].\n");
                              bOpen = false;  // Skip further attempts
                            }
                            else {
                              vi.pixel_type = VideoInfo::CS_RGBAP16;
                              biDst.biCompression = MAKEFOURCC('G', '4', 0, 16); // ffmpeg GBRAP16LE
                              biDst.biBitCount = 48;
                              if (ICERR_OK == ICDecompressQuery(hic, pbiSrc, &biDst)) {
                                _RPT0(0, "AVISource: Opening as G4[0][16].\n");
                                bOpen = false;  // Skip further attempts
                              }
                              else
                                if (forcedType) {
                                  env->ThrowError("AVISource: the video decompressor couldn't produce Planar RGB(A) output");
                                }
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
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

          // RGB48
          if (fRGB48 && bOpen) {
            vi.pixel_type = VideoInfo::CS_BGR48;
            biDst.biSizeImage = vi.BMPSize();
            biDst.biCompression = MAKEFOURCC('B', 'G', 'R', 48); // BGR0 ie. BGR[48]
            biDst.biBitCount = 48;
            if (ICERR_OK == ICDecompressQuery(hic, pbiSrc, &biDst)) {
              _RPT0(0, "AVISource: Opening as BGR0 (BGR[48]).\n");
              bOpen = false;  // Skip further attempts
            }
            else {
              biDst.biSizeImage = vi.BMPSize();
              biDst.biCompression = MAKEFOURCC('b', '4', '8', 'r'); // e.g. preferred by UT RGB 10 bits : UQRG
              biDst.biBitCount = 48;
              if (ICERR_OK == ICDecompressQuery(hic, pbiSrc, &biDst)) {
                _RPT0(0, "AVISource: Opening as b48r.\n");
                b48r = true;
                bOpen = false;  // Skip further attempts
              }
              else if (forcedType) {
                env->ThrowError("AVISource: the video decompressor couldn't produce RGB48 output");
              }
            }
          }

          // RGB64
          if (fRGB64 && bOpen) {
            vi.pixel_type = VideoInfo::CS_BGR64;
            biDst.biSizeImage = vi.BMPSize();
            biDst.biCompression = MAKEFOURCC('B', 'R', 'A', 64); // BRA@ ie. BRA[64]
            biDst.biBitCount = 64;
            if (ICERR_OK == ICDecompressQuery(hic, pbiSrc, &biDst)) {
              _RPT0(0, "AVISource: Opening as BRA@ (BRA[64]).\n");
              bOpen = false;  // Skip further attempts
            }
            else {
              biDst.biCompression = MAKEFOURCC('b', '6', '4', 'a'); // e.g. preferred by UT RGBA 10 bits : UQRA
              biDst.biBitCount = 64;
              if (ICERR_OK == ICDecompressQuery(hic, pbiSrc, &biDst)) {
                _RPT0(0, "AVISource: Opening as b64a.\n");
                bOpen = false;  // Skip further attempts
                b64a = true;
              }
              else
                if (forcedType) {
                  env->ThrowError("AVISource: the video decompressor couldn't produce RGB64 output");
                }
            }
          }

          // Y8
          if ((fY8 || fGrayscale) && bOpen) {
            vi.pixel_type = VideoInfo::CS_Y8;
            biDst.biSizeImage = vi.BMPSize();
            biDst.biCompression = MAKEFOURCC('Y', '8', '0', '0');
            biDst.biBitCount = 8;
            if (ICERR_OK == ICDecompressQuery(hic, pbiSrc, &biDst)) {
              _RPT0(0,"AVISource: Opening as Y800.\n");
              bOpen = false;  // Skip further attempts
            } else {
              biDst.biCompression = MAKEFOURCC('Y', '8', ' ', ' ');
              if (ICERR_OK == ICDecompressQuery(hic, pbiSrc, &biDst)) {
                _RPT0(0,"AVISource: Opening as Y8.\n");
                bOpen = false;  // Skip further attempts
              } else {
                biDst.biCompression = MAKEFOURCC('G', 'R', 'E', 'Y');
                if (ICERR_OK == ICDecompressQuery(hic, pbiSrc, &biDst)) {
                  _RPT0(0,"AVISource: Opening as GREY.\n");
                  bOpen = false;  // Skip further attempts
                } else if (forcedType) {
                   env->ThrowError("AVISource: the video decompressor couldn't produce Y8 output");
                }
              }
            }
          }

          // Greyscale 'Y' no specific bit depth
          if (fGrayscale && bOpen) {
            vi.pixel_type = VideoInfo::CS_Y10;
            biDst.biSizeImage = vi.BMPSize();
            biDst.biCompression = MAKEFOURCC('Y', '1', 0, 10);
            biDst.biBitCount = 10;
            if (ICERR_OK == ICDecompressQuery(hic, pbiSrc, &biDst)) {
              _RPT0(0, "AVISource: Opening as Y1[0][10].\n");
              bOpen = false;  // Skip further attempts
            }
            else {
              vi.pixel_type = VideoInfo::CS_Y12;
              biDst.biSizeImage = vi.BMPSize();
              biDst.biCompression = MAKEFOURCC('Y', '1', 0, 12);
              biDst.biBitCount = 12;
              if (ICERR_OK == ICDecompressQuery(hic, pbiSrc, &biDst)) {
                _RPT0(0, "AVISource: Opening as Y1[0][12].\n");
                bOpen = false;  // Skip further attempts
              }
              else {
                vi.pixel_type = VideoInfo::CS_Y14;
                biDst.biSizeImage = vi.BMPSize();
                biDst.biCompression = MAKEFOURCC('Y', '1', 0, 14);
                biDst.biBitCount = 14;
                if (ICERR_OK == ICDecompressQuery(hic, pbiSrc, &biDst)) {
                  _RPT0(0, "AVISource: Opening as Y1[0][14].\n");
                  bOpen = false;  // Skip further attempts
                }
                else {
                  vi.pixel_type = VideoInfo::CS_Y16;
                  biDst.biSizeImage = vi.BMPSize();
                  biDst.biCompression = MAKEFOURCC('Y', '1', 0, 16);
                  biDst.biBitCount = 16;
                  if (ICERR_OK == ICDecompressQuery(hic, pbiSrc, &biDst)) {
                    _RPT0(0, "AVISource: Opening as Y1[0][16].\n");
                    bOpen = false;  // Skip further attempts
                  }
                  else
                    if (forcedType) {
                      env->ThrowError("AVISource: the video decompressor couldn't produce grayscale output");
                    }
                }
              }
            }
          }

          // YUV422P10
          if ((fYUV422P10 || fv210 || fP210) && bOpen) {
            vi.pixel_type = VideoInfo::CS_YUV422P10;
            if ((fv210 || fYUV422P10) && bOpen) {
              biDst.biSizeImage = AviHelper_ImageSize(&vi, false, true, false, false, false);
              biDst.biCompression = MAKEFOURCC('v', '2', '1', '0'); // as v210
              biDst.biBitCount = 30; // should be 30, not 10+(10+10)/2
              if (ICERR_OK == ICDecompressQuery(hic, pbiSrc, &biDst)) {
                _RPT0(0, "AVISource: Opening as v210.\n");
                v210 = true;
                bOpen = false;  // Skip further attempts
              }
            }
            if ((fP210 || fYUV422P10) && bOpen) {
              biDst.biSizeImage = vi.BMPSize();
              biDst.biCompression = MAKEFOURCC('P', '2', '1', '0'); // as P210
              biDst.biBitCount = 30; // should be 30, not 10+(10+10)/2
              if (ICERR_OK == ICDecompressQuery(hic, pbiSrc, &biDst)) {
                _RPT0(0, "AVISource: Opening as P210.\n");
                P210 = true;
                bOpen = false;  // Skip further attempts
              }
            }
            else if (forcedType && bOpen) {
              env->ThrowError("AVISource: the video decompressor couldn't produce YUV422P10 output");
            }
          }

          // YUV422P16
          if ((fYUV422P16 || fP216) && bOpen) {
            vi.pixel_type = VideoInfo::CS_YUV422P16;
            biDst.biSizeImage = vi.BMPSize();
            biDst.biCompression = MAKEFOURCC('P', '2', '1', '6'); // as P216
            biDst.biBitCount = 32; // 16+(16+16)/2 or 3x16?
            if (ICERR_OK == ICDecompressQuery(hic, pbiSrc, &biDst)) {
              _RPT0(0, "AVISource: Opening as P216.\n");
              P216 = true;
              bOpen = false;  // Skip further attempts
            }
            else if (forcedType) {
              env->ThrowError("AVISource: the video decompressor couldn't produce YUV422P16 output");
            }
          }

          // YUV420P10
          if ((fYUV420P10 || fP010) && bOpen) {
            vi.pixel_type = VideoInfo::CS_YUV420P10;
            biDst.biSizeImage = vi.BMPSize();
            biDst.biCompression = MAKEFOURCC('P', '0', '1', '0'); // as P010
            biDst.biBitCount = 15; // 10+(10+10)/4 or 3x10?
            if (ICERR_OK == ICDecompressQuery(hic, pbiSrc, &biDst)) {
              _RPT0(0, "AVISource: Opening as P010.\n");
              P010 = true;
              bOpen = false;  // Skip further attempts
            }
            else if (forcedType) {
              env->ThrowError("AVISource: the video decompressor couldn't produce YUV420P10 output");
            }
          }
          // YUV420P16
          if ((fYUV420P16 || fP016) && bOpen) {
            vi.pixel_type = VideoInfo::CS_YUV420P16;
            biDst.biSizeImage = vi.BMPSize();
            biDst.biCompression = MAKEFOURCC('P', '0', '1', '6'); // as P016
            biDst.biBitCount = 24; // 16+(16+16)/4 or 3x16?
            if (ICERR_OK == ICDecompressQuery(hic, pbiSrc, &biDst)) {
              _RPT0(0, "AVISource: Opening as P016.\n");
              P016 = true;
              bOpen = false;  // Skip further attempts
            }
            else if (forcedType) {
              env->ThrowError("AVISource: the video decompressor couldn't produce YUV420P16 output");
            }
          }

          // YUV444P16
          if ((fYUV444P16 || fY416) && bOpen) {
            vi.pixel_type = VideoInfo::CS_YUV444P16;
            biDst.biSizeImage = vi.BMPSize();
            biDst.biCompression = MAKEFOURCC('Y', '4', '1', '6'); // as Y416
            biDst.biBitCount = 48; // 3x16?
            if (ICERR_OK == ICDecompressQuery(hic, pbiSrc, &biDst)) {
              _RPT0(0, "AVISource: Opening as Y416.\n");
              Y416 = true;
              bOpen = false;  // Skip further attempts
            }
            else if (forcedType) {
              env->ThrowError("AVISource: the video decompressor couldn't produce YUV444P16 output");
            }
          }

          // YUV444P10
          if ((fYUV444P10 || fv410) && bOpen) {
            vi.pixel_type = VideoInfo::CS_YUV444P10;
            biDst.biSizeImage = AviHelper_ImageSize(&vi, false, false, true, false, false);
            biDst.biCompression = MAKEFOURCC('v', '4', '1', '0'); // as v410
            biDst.biBitCount = 30; // 3x16?
            if (ICERR_OK == ICDecompressQuery(hic, pbiSrc, &biDst)) {
              _RPT0(0, "AVISource: Opening as v410.\n");
              v410 = true;
              bOpen = false;  // Skip further attempts
            }
            else if (forcedType) {
              env->ThrowError("AVISource: the video decompressor couldn't produce YUV444P10 output");
            }
          }

          // No takers!
          if (bOpen)
            env->ThrowError("AviSource: Could not open video stream in any supported format.");

          DecompressBegin(pbiSrc, &biDst);
        }
        // Flip DIB formats if negative height (FIXME: Y8 too?). Flip RGB48/64 always.
        if ((pbiSrc->biHeight < 0 && (vi.IsRGB24() || vi.IsRGB32() || vi.IsY8())) || vi.IsRGB48() || vi.IsRGB64())
          bInvertFrames = true;
      }
      else {
        env->ThrowError("AviSource: Could not locate video stream.");
      }
    }

    // check for audio stream
    if (fAudio) /*  && pfile->GetStream(streamtypeAUDIO, 0)) */ {
      aSrc = new AudioSourceAVI(pfile, true, atrack);
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
      bMediaPad = !(!bMediaPad && !vi.IsY8() && vi.IsPlanar());
      int keyframe = pvideo->NearestKeyFrame(0);
      frame = new TemporalBuffer(vi, bMediaPad, b64a, b48r, v210, P010, P016, P210, P216, v410, Y416, r210, R10k, env);

      LRESULT error = DecompressFrame(keyframe, false, env);
      if (error != ICERR_OK)   // shutdown, if init not succesful.
        env->ThrowError("AviSource: Could not decompress frame 0");

      // Cope with dud AVI files that start with drop
      // frames, just return the first key frame
      if (dropped_frame) {
        keyframe = pvideo->NextKeyFrame(0);
        error = DecompressFrame(keyframe, false, env);
        if (error != ICERR_OK)   // shutdown, if init not succesful.
          env->ThrowError("AviSource: Could not decompress first keyframe %d", keyframe);
      }
      last_frame_no=0;
      last_frame = AdjustFrameAlignment(frame, vi, bInvertFrames, b64a, b48r, v210, P010, P016, P210, P216, v410, Y416, r210, R10k, env);
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
  if (frame)
    delete frame;
}

const VideoInfo& AVISource::GetVideoInfo() { return vi; }

PVideoFrame AVISource::GetFrame(int n, IScriptEnvironment* env) {

  n = clamp(n, 0, vi.num_frames-1);
  dropped_frame=false;
  if (n != last_frame_no) {
    // find the last keyframe
    int keyframe = pvideo->NearestKeyFrame(n);
    // maybe we don't need to go back that far
    if (last_frame_no < n && last_frame_no >= keyframe)
      keyframe = last_frame_no+1;
    if (keyframe < 0) keyframe = 0;

    bool frameok = false;
    //PVideoFrame frame = env->NewVideoFrame(vi, -4);

    bool not_found_yet;
    do {
      not_found_yet=false;
      for (int i = keyframe; i <= n; ++i) {
        LRESULT error = DecompressFrame(i, i != n, env);
        if ((!dropped_frame) && (error == ICERR_OK)) frameok = true;   // Better safe than sorry
      }
      last_frame_no = n;

      if (!last_frame && !frameok) {  // Last keyframe was not valid.
        const int key_pre=keyframe;
        keyframe = pvideo->NearestKeyFrame(keyframe-1);
        if (keyframe < 0) keyframe = 0;
        if (keyframe == key_pre)
          env->ThrowError("AVISource: could not find valid keyframe for frame %d.", n);

        not_found_yet=true;
      }
    } while(not_found_yet);

    if (frameok) {
      last_frame = AdjustFrameAlignment(frame, vi, bInvertFrames, b64a, b48r, v210, P010, P016, P210, P216, v410, Y416, r210, R10k, env);
    }
  }
  return last_frame;
}

void AVISource::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) {
  AVS_UNUSED(env);
  long bytes_read=0;
  __int64 samples_read=0;

  if (start < 0) {
    int bytes = (int)vi.BytesFromAudioSamples(min(-start, count));
    memset(buf, 0, bytes);
    buf = (char*)buf + bytes;
    count -= vi.AudioSamplesFromBytes(bytes);
    start += vi.AudioSamplesFromBytes(bytes);
  }

  if (audioStreamSource) {
    if (start != audio_stream_pos)
        audioStreamSource->Seek((long)start);
    samples_read = audioStreamSource->Read(buf, (long)count, &bytes_read);
    audio_stream_pos = start + samples_read;
  }

  if (samples_read < count)
    memset((char*)buf + bytes_read, 0, (size_t)(vi.BytesFromAudioSamples(count) - bytes_read));
}

bool AVISource::GetParity(int n) {
  AVS_UNUSED(n);
  return false; 
}

int __stdcall AVISource::SetCacheHints(int cachehints,int frame_range)
{
  AVS_UNUSED(frame_range);
  switch(cachehints)
  {
  case CACHE_GET_MTMODE:
    return MT_SERIALIZED;
  default:
    return 0;
  }
}
