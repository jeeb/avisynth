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
 

#include "../stdafx.h"

#include "../internal.h"
#include "../convert/convert.h"
#include "../filters/transform.h"
#include <streams.h>

// For some reason KSDATAFORMAT_SUBTYPE_IEEE_FLOAT and KSDATAFORMAT_SUBTYPE_PCM doesn't work - we construct the GUID manually!
const GUID SUBTYPE_IEEE_AVSPCM  = {0x00000001, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
const GUID SUBTYPE_IEEE_AVSFLOAT  = {0x00000003, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};


extern void ApplyMessage(PVideoFrame* frame, const VideoInfo& vi,
  const char* message, int size, int textcolor, int halocolor, int bgcolor,
  IScriptEnvironment* env);


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
  IUnknown *m_pPos;  // Pointer to the CPosPassThru object.
  VideoInfo vi;

  HANDLE evtDoneWithSample, evtNewSampleReady;
  PVideoFrame pvf;

  __int64 sample_start_time, sample_end_time;
  IScriptEnvironment* const env;
  bool load_audio;
  bool load_video;

public:
  int a_sample_bytes;
  int a_allocated_buffer;
  BYTE* a_buffer;         // Killed on StopGraph

  GetSample(IScriptEnvironment* _env, bool _load_audio, bool _load_video);
  ~GetSample();

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

  void StartGraph();
  void StopGraph();
  void PauseGraph();
  HRESULT SeekTo(__int64 pos);
  void NextSample();
  ULONG __stdcall AddRef();
  ULONG __stdcall Release();
  HRESULT __stdcall QueryInterface(REFIID iid, void** ppv);
  HRESULT __stdcall GetClassID(CLSID* pClassID);
  HRESULT __stdcall Stop();
  HRESULT __stdcall Pause();
  HRESULT __stdcall Run(REFERENCE_TIME tStart); 
  HRESULT __stdcall GetState(DWORD dwMilliSecsTimeout, FILTER_STATE* State);
  HRESULT __stdcall SetSyncSource(IReferenceClock* pClock);
  HRESULT __stdcall GetSyncSource(IReferenceClock** ppClock);
  HRESULT __stdcall EnumPins(IEnumPins** ppEnum);
  HRESULT __stdcall FindPin(LPCWSTR Id, IPin** ppPin);
  HRESULT __stdcall QueryFilterInfo(FILTER_INFO* pInfo);
  HRESULT __stdcall JoinFilterGraph(IFilterGraph* pGraph, LPCWSTR pName);
  HRESULT __stdcall QueryVendorInfo(LPWSTR* pVendorInfo);

  HRESULT __stdcall Connect(IPin* pReceivePin, const AM_MEDIA_TYPE* pmt);
  HRESULT __stdcall ReceiveConnection(IPin* pConnector, const AM_MEDIA_TYPE* pmt);
  HRESULT __stdcall Disconnect();
  HRESULT __stdcall ConnectedTo(IPin** ppPin);
  HRESULT __stdcall ConnectionMediaType(AM_MEDIA_TYPE* pmt);
  HRESULT __stdcall QueryPinInfo(PIN_INFO* pInfo);
  HRESULT __stdcall QueryDirection(PIN_DIRECTION* pPinDir);
  HRESULT __stdcall QueryId(LPWSTR* Id);
  HRESULT __stdcall QueryAccept(const AM_MEDIA_TYPE* pmt);
  HRESULT __stdcall EnumMediaTypes(IEnumMediaTypes** ppEnum);
  HRESULT __stdcall QueryInternalConnections(IPin** apPin, ULONG* nPin);
  HRESULT __stdcall EndOfStream();
  HRESULT __stdcall BeginFlush();
  HRESULT __stdcall EndFlush();
  HRESULT __stdcall NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);

  // IMemInputPin

  HRESULT __stdcall GetAllocator(IMemAllocator** ppAllocator);
  HRESULT __stdcall NotifyAllocator(IMemAllocator* pAllocator, BOOL bReadOnly);
  HRESULT __stdcall GetAllocatorRequirements(ALLOCATOR_PROPERTIES* pProps);

  HRESULT __stdcall Receive(IMediaSample* pSamples);
  HRESULT __stdcall ReceiveMultiple(IMediaSample** ppSamples, long nSamples, long* nSamplesProcessed);
  HRESULT __stdcall ReceiveCanBlock();
};


GetSampleEnumPins::GetSampleEnumPins(GetSample* _parent, int _pos);
HRESULT __stdcall GetSampleEnumPins::Next(ULONG cPins, IPin** ppPins, ULONG* pcFetched);

static bool HasNoConnectedOutputPins(IBaseFilter* bf);

static void DisconnectAllPinsAndRemoveFilter(IGraphBuilder* gb, IBaseFilter* bf);
static void RemoveUselessFilters(IGraphBuilder* gb, IBaseFilter* not_this_one, IBaseFilter* nor_this_one);
static HRESULT AttemptConnectFilters(IGraphBuilder* gb, IBaseFilter* connect_filter);
static void SetMicrosoftDVtoFullResolution(IGraphBuilder* gb);



class DirectShowSource : public IClip {

  GetSample get_sample;
  IGraphBuilder* gb;
  __int64 next_sample;

  VideoInfo vi;
  __int64 duration;
  bool frame_units, known_framerate;
  int avg_time_per_frame;
  __int64 base_sample_time;
  int cur_frame;
  bool no_search;
  int audio_bytes_read;
  IScriptEnvironment* const env;
  void CheckHresult(HRESULT hr, const char* msg, const char* msg2 = "");
  HRESULT LoadGraphFile(IGraphBuilder *pGraph, const WCHAR* wszName);


public:

  DirectShowSource(const char* filename, int _avg_time_per_frame, bool _seek, bool _enable_audio, bool _enable_video, IScriptEnvironment* _env);
  ~DirectShowSource();
  const VideoInfo& __stdcall GetVideoInfo() { return vi; }
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);


  bool __stdcall GetParity(int n) { return false; }
  void __stdcall SetCacheHints(int cachehints,int frame_range) { };

  void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env);

private:


};


AVSValue __cdecl Create_DirectShowSource(AVSValue args, void*, IScriptEnvironment* env);
