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

#include "../../core/alignplanar.h"
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
  bool bIsType1;
  bool bInvertFrames;
  bool bMediaPad;

  PVideoFrame last_frame;
  int last_frame_no;
  AudioSource* aSrc;
  AudioStreamSource* audioStreamSource;
  __int64 audio_stream_pos;

  LRESULT DecompressBegin(LPBITMAPINFOHEADER lpbiSrc, LPBITMAPINFOHEADER lpbiDst);
  LRESULT DecompressFrame(int n, bool preroll, PVideoFrame &frame, IScriptEnvironment* env);

  void CheckHresult(HRESULT hr, const char* msg, IScriptEnvironment* env);
  bool AttemptCodecNegotiation(DWORD fccHandler, BITMAPINFOHEADER* bmih);
  void LocateVideoCodec(const char fourCC[], IScriptEnvironment* env);

public:

  typedef enum {
    MODE_NORMAL = 0,
    MODE_AVIFILE,
    MODE_OPENDML,
    MODE_WAV
  } avi_mode_e;

  AVISource(const char filename[], bool fAudio, const char pixel_type[],
            const char fourCC[], int vtrack, int atrack, avi_mode_e mode, IScriptEnvironment* env);  // mode: 0=detect, 1=avifile, 2=opendml, 3=avifile (audio only)
  ~AVISource();
  void CleanUp(); // Tritical - Jan 2006
  const VideoInfo& __stdcall GetVideoInfo();
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
  void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) ;
  bool __stdcall GetParity(int n);
  int __stdcall SetCacheHints(int cachehints,int frame_range);

  static AVSValue __cdecl Create(AVSValue args, void* user_data, IScriptEnvironment* env) {
    const avi_mode_e mode = (avi_mode_e)size_t(user_data);
    const bool fAudio = (mode == MODE_WAV) || args[1].AsBool(true);
    const char* pixel_type = (mode != MODE_WAV) ? args[2].AsString("") : "";
    const char* fourCC = (mode != MODE_WAV) ? args[3].AsString("") : "";
    const int vtrack = args[4].AsInt(0);
    const int atrack = args[5].AsInt(0);

    PClip result = new AVISource(args[0][0].AsString(), fAudio, pixel_type, fourCC, vtrack, atrack, mode, env);
    for (int i=1; i<args[0].ArraySize(); ++i)
      result = new_Splice(result, new AVISource(args[0][i].AsString(), fAudio, pixel_type, fourCC, vtrack, atrack, mode, env), false, env);
    return AlignPlanar::Create(result);
  }
};
