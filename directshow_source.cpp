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

#include "directshow_source.h"



GetSample::GetSample(IScriptEnvironment* _env, bool _load_audio, bool _load_video) : env(_env), load_audio(_load_audio), load_video(_load_video) {
    refcnt = 1;
    source_pin = 0;
    filter_graph = 0;
    pclock = 0;
    a_allocated_buffer = 0;
    m_pPos =0;
    state = State_Stopped;
    a_buffer = 0;
    flushing = end_of_stream = false;
    memset(&vi, 0, sizeof(vi));
    sample_end_time = sample_start_time = 0;
//    evtDoneWithSample = ::CreateEvent(NULL, FALSE, FALSE, (load_audio) ? "AVS_DoneWithSample_audio" : "AVS_DoneWithSample_video");
//    evtNewSampleReady = ::CreateEvent(NULL, FALSE, FALSE, (load_audio) ? "AVS_NewSampleReady_audio" : "AVS_NewSampleReady_video");
    evtDoneWithSample = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    evtNewSampleReady = ::CreateEvent(NULL, FALSE, FALSE, NULL);
  }

  GetSample::~GetSample() {
    CloseHandle(evtDoneWithSample);
    CloseHandle(evtNewSampleReady);
  }


  void GetSample::StartGraph() {
    IMediaControl* mc;
    filter_graph->QueryInterface(&mc);
    mc->Run();
    mc->Release();
    _RPT0(0,"StartGraph() waiting for new sample...\n");
    WaitForSingleObject(evtNewSampleReady, 5000);    // MAX wait time = 5000ms!
    _RPT0(0,"...StartGraph() finished waiting for new sample\n");

  }

  void GetSample::StopGraph() {
    IMediaControl* mc;
    filter_graph->QueryInterface(&mc);
    state = State_Paused;
    _RPT1(0,"StopGraph() indicating done with sample - state:%d\n",state);
    PulseEvent(evtDoneWithSample);
    mc->Stop();
    mc->Release();
    if (m_pPos)
      m_pPos->Release();
    m_pPos = 0;
    if (a_buffer)
      delete[] a_buffer;
    a_buffer = 0;
  }

  void GetSample::PauseGraph() {
    IMediaControl* mc;
    filter_graph->QueryInterface(&mc);
    state = State_Paused;
    _RPT0(0,"PauseGraph() indicating done with sample\n");
    PulseEvent(evtDoneWithSample);
    mc->Pause();
    mc->Release();

  }

  HRESULT GetSample::SeekTo(__int64 pos) {
//    PauseGraph();
    HRESULT hr;

    IMediaControl* mc;
    filter_graph->QueryInterface(&mc);
    hr = mc->Stop();

    IMediaSeeking* ms;
    filter_graph->QueryInterface(&ms);

    LONGLONG pStop=-1;
    LONGLONG pCurrent=-1;
    _RPT0(0,"SeekTo() seeking to new position\n");

    DWORD dwCaps = 0;
    ms->GetCapabilities(&dwCaps);
    
    GUID pref_f;

    ms->QueryPreferredFormat(&pref_f);

    if (pref_f == TIME_FORMAT_FRAME) {
      _RPT0(0,"Prefered format: frames!\n");
    }
    if (pref_f == TIME_FORMAT_SAMPLE) {
      _RPT0(0,"Prefered format: samples!\n");
    }
    if (pref_f == TIME_FORMAT_MEDIA_TIME) {
      _RPT0(0,"Prefered format: media time!\n");
    }

    hr = ms->SetTimeFormat(&TIME_FORMAT_MEDIA_TIME);

    if (!SUCCEEDED(hr)) {
      _RPT0(0,"Could not seek to media time!\n");
      mc->Release();
      ms->Release();
      StartGraph();
      return hr;
    }

    if (dwCaps & AM_SEEKING_CanGetCurrentPos) {
      hr = ms->GetPositions(&pCurrent,&pStop);
    }

    if (dwCaps & AM_SEEKING_CanSeekAbsolute) {
       hr = ms->SetPositions(&pos, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
       if (FAILED(hr)) {
         _RPT0(0,"Absolute seek failed!\n");
         mc->Release();
         ms->Release();
         StartGraph();
         return hr;
       }
    } else {
      if ((dwCaps & AM_SEEKING_CanSeekForwards) && (pCurrent!=-1)) {
        pCurrent = pos - pCurrent;
        pStop = pos - pStop;
        hr = ms->SetPositions(&pCurrent, AM_SEEKING_RelativePositioning, &pStop, AM_SEEKING_NoPositioning);
        if (FAILED(hr)) {
           _RPT0(0,"Relative seek failed!\n");
        }
      } else {
        // No way of seeking
         _RPT0(0,"Could not perform any seek!\n");
        mc->Release();
        ms->Release();
        StartGraph();
        return S_FALSE;
      }
    }
    ms->Release();
    mc->Release();
    StartGraph();

    return hr;  // Seek ok
  }

  void GetSample::NextSample() {
    if (end_of_stream) return;

    if (load_audio) 
      _RPT0(0,"NextSample() indicating done with sample...(audio)\n");
    else 
      _RPT0(0,"NextSample() indicating done with sample...(video)\n");


    HRESULT wait_result;
    SetEvent(evtDoneWithSample);  // We indicate that Recieve can run again. We have now finished using the frame.

    do {
      if (load_audio) {
        _RPT0(0,"...NextSample() waiting for new sample...(audio)\n");
      } else {
        _RPT0(0,"...NextSample() waiting for new sample... (video)\n");
      }
      wait_result = WaitForSingleObject(evtNewSampleReady, 1000);
    } while (wait_result == WAIT_TIMEOUT);
//    } while (wait_result == WAIT_TIMEOUT && state == State_Running);
//    WaitForSingleObject(evtNewSampleReady, INFINITE);   // Max wait for new sample 10 secs.

    if (load_audio) {
      _RPT0(0,"...NextSample() done waiting for new sample (audio)\n");
    } else {
      _RPT0(0,"...NextSample() done waiting for new sample (video)\n");
    }
  }

  // IUnknown

  ULONG __stdcall GetSample::AddRef() { 
    InterlockedIncrement(&refcnt); 
    _RPT1(0,"GetSample::AddRef() -> %d\n", refcnt); 
    return refcnt; 
  }

  ULONG __stdcall GetSample::Release() { 
    InterlockedDecrement(&refcnt); 
    _RPT1(0,"GetSample::Release() -> %d\n", refcnt); 
    return refcnt; 
  }

  

  HRESULT __stdcall GetSample::QueryInterface(REFIID iid, void** ppv) {
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
      if (!source_pin)
        return E_NOINTERFACE;

//      return source_pin->QueryInterface(iid, ppv);

      if (m_pPos == NULL)  {
          // We have not created the CPosPassThru object yet. Do so now.
          HRESULT hr = S_OK;
          hr = CreatePosPassThru(NULL , FALSE, static_cast<IPin*>(this), &m_pPos);

          if (FAILED(hr)) return hr;
        }
        return m_pPos->QueryInterface(iid, ppv);
    } else {
      *ppv = 0;
      return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
  }

  // IPersist

  HRESULT __stdcall GetSample::GetClassID(CLSID* pClassID) { return E_NOTIMPL; }

  // IMediaFilter

  HRESULT __stdcall GetSample::Stop() { _RPT0(0,"GetSample::Stop()\n"); state = State_Stopped; return S_OK; }
  HRESULT __stdcall GetSample::Pause() { _RPT0(0,"GetSample::Pause()\n"); state = State_Paused; return S_OK; }
  HRESULT __stdcall GetSample::Run(REFERENCE_TIME tStart) { _RPT0(0,"GetSample::Run()\n"); state = State_Running; return S_OK; }
  HRESULT __stdcall GetSample::GetState(DWORD dwMilliSecsTimeout, FILTER_STATE* State) {
    if (!State) return E_POINTER;
    *State = state;
    return S_OK;
  }
  HRESULT __stdcall GetSample::SetSyncSource(IReferenceClock* pClock) { pclock = pClock; return S_OK; }
  HRESULT __stdcall GetSample::GetSyncSource(IReferenceClock** ppClock) {
    if (!ppClock) return E_POINTER;
    *ppClock = pclock;
    if (pclock) pclock->AddRef();
    return S_OK;
  }

  // IBaseFilter

  HRESULT __stdcall GetSample::EnumPins(IEnumPins** ppEnum) {
    if (!ppEnum) return E_POINTER;
    *ppEnum = new GetSampleEnumPins(this);
    return S_OK;
  }
  HRESULT __stdcall GetSample::FindPin(LPCWSTR Id, IPin** ppPin) { return E_NOTIMPL; }
  HRESULT __stdcall GetSample::QueryFilterInfo(FILTER_INFO* pInfo) {
    if (!pInfo) return E_POINTER;
    lstrcpyW(pInfo->achName, L"GetSample");
    pInfo->pGraph = filter_graph;
    if (filter_graph) filter_graph->AddRef();
    return S_OK;
  }
  HRESULT __stdcall GetSample::JoinFilterGraph(IFilterGraph* pGraph, LPCWSTR pName) {
    filter_graph = pGraph;
    return S_OK;
  }
  HRESULT __stdcall GetSample::QueryVendorInfo(LPWSTR* pVendorInfo) { return E_NOTIMPL; }

  // IPin

  HRESULT __stdcall GetSample::Connect(IPin* pReceivePin, const AM_MEDIA_TYPE* pmt) {
    return E_UNEXPECTED;
  }
  HRESULT __stdcall GetSample::ReceiveConnection(IPin* pConnector, const AM_MEDIA_TYPE* pmt) {
    if (!pConnector || !pmt) return E_POINTER;
    if (GetSample::QueryAccept(pmt) != S_OK) return E_INVALIDARG;
    source_pin = pConnector;
    return S_OK;
  }
  HRESULT __stdcall GetSample::Disconnect() {
    if (a_allocated_buffer) { 
      a_allocated_buffer = 0;
      delete[] a_buffer;
    }
    source_pin = 0;
    return S_OK;
  }
  HRESULT __stdcall GetSample::ConnectedTo(IPin** ppPin) {
    if (!ppPin) return E_POINTER;
    if (source_pin) source_pin->AddRef();
    *ppPin = source_pin;
    return source_pin ? S_OK : VFW_E_NOT_CONNECTED;
  }
  HRESULT __stdcall GetSample::ConnectionMediaType(AM_MEDIA_TYPE* pmt) {
    return E_NOTIMPL;
  }
  HRESULT __stdcall GetSample::QueryPinInfo(PIN_INFO* pInfo) {
    if (!pInfo) return E_POINTER;
    pInfo->pFilter = static_cast<IBaseFilter*>(this);
    AddRef();
    pInfo->dir = PINDIR_INPUT;
    lstrcpyW(pInfo->achName, L"GetSample");
    return S_OK;
  }

  HRESULT __stdcall GetSample::QueryDirection(PIN_DIRECTION* pPinDir) {
    if (!pPinDir) return E_POINTER;
    *pPinDir = PINDIR_INPUT;
    return S_OK;
  }

  HRESULT __stdcall GetSample::QueryId(LPWSTR* Id) {
    return E_NOTIMPL;
  }



  HRESULT __stdcall GetSample::QueryAccept(const AM_MEDIA_TYPE* pmt) {
    if (!pmt) return E_POINTER;

    if (load_audio) {
      if (pmt->majortype != MEDIATYPE_Audio) {
        return S_FALSE;
      }
    } else if (load_video) {
      if (pmt->majortype != MEDIATYPE_Video) {
        return S_FALSE;
      }
    }

    if (!load_video && pmt->majortype == MEDIATYPE_Video) {
      return S_FALSE;
    }

    if (!load_audio && pmt->majortype == MEDIATYPE_Audio) {
      return S_FALSE;
    }

// Handle audio:
    if (pmt->majortype == MEDIATYPE_Audio) {
//      if (pmt->subtype != MEDIASUBTYPE_PCM  || pmt->subtype != MEDIASUBTYPE_IEEE_FLOAT ) {
      if (pmt->subtype != MEDIASUBTYPE_PCM ) {
        _RPT0(0, "*** In majortype Audio - Subtype rejected\n");
        return S_FALSE;
      }

      WAVEFORMATEX* wex = (WAVEFORMATEX*)pmt->pbFormat;

      if ((wex->wFormatTag != WAVE_FORMAT_PCM) && (wex->wFormatTag != WAVE_FORMAT_EXTENSIBLE)) {
        _RPT0(0, "*** Audio: Secondary check rejected - Not PCM after all???\n");
        return S_FALSE;
      }

      vi.nchannels = wex->nChannels;
      switch (wex->wBitsPerSample) {
        case 8:
          vi.sample_type = SAMPLE_INT8;
          break;
        case 16:
          vi.sample_type = SAMPLE_INT16;
          break;
        case 24:
          vi.sample_type = SAMPLE_INT24;
          break;
        case 32:
          vi.sample_type = SAMPLE_INT32;
          break;
        default:
        _RPT0(0, "*** Audio: Unsupported number of bits per \n");
        return S_FALSE;

      }

      if (wex->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
        if (wex->cbSize < 22) {
          _RPT0(0, "*** Audio: Extended wave format structure does not have the correct size!\n");
          return S_FALSE;
        }
        // Override settings with extended data (float or >2 ch).
        WAVEFORMATEXTENSIBLE* _wext =  (WAVEFORMATEXTENSIBLE*)pmt->pbFormat;
        WAVEFORMATEXTENSIBLE wext =  *_wext;
        
        
        if (wext.Samples.wValidBitsPerSample != wext.Format.wBitsPerSample) {  // FIXME:  Allow bit padding!
          _RPT0(0, "*** Audio: Cannot accept sound, if ValidBitsPerSample != BitsPerSample!\n");
          return S_FALSE;
        }

        if (wext.SubFormat == SUBTYPE_IEEE_AVSFLOAT) {  // We have float audio.
          vi.sample_type = SAMPLE_FLOAT;
        } else if (wext.SubFormat != SUBTYPE_IEEE_AVSPCM) {
          _RPT0(0, "*** Audio: Extended WAVE format must be float or PCM.\n");
          return S_FALSE;
        }

      }

      vi.audio_samples_per_second = wex->nSamplesPerSec;

      _RPT3(0, "*** Audio Accepted!  - Channels:%d.  Samples/sec:%d.  Bits/sample:%d.\n",wex->nChannels, wex->nSamplesPerSec, wex->wBitsPerSample);      
      return S_OK;
    }

// Handle video:

    if (pmt->subtype == MEDIASUBTYPE_YV12) {  
      vi.pixel_type = VideoInfo::CS_YV12;
    } else if (pmt->subtype == MEDIASUBTYPE_YUY2) {
      vi.pixel_type = VideoInfo::CS_YUY2;
    } else if (pmt->subtype == MEDIASUBTYPE_RGB24) {
      vi.pixel_type = VideoInfo::CS_BGR24;
    } else if (pmt->subtype == MEDIASUBTYPE_RGB32) {
      vi.pixel_type = VideoInfo::CS_BGR32;
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
    } else if (pmt->formattype == FORMAT_VideoInfo2) {
      VIDEOINFOHEADER2* vih = (VIDEOINFOHEADER2*)pmt->pbFormat;
      avg_time_per_frame = unsigned(vih->AvgTimePerFrame);
      pbi = &vih->bmiHeader;
//      if (vih->dwInterlaceFlags & AMINTERLACE_1FieldPerSample) {
//        vi.SetFieldBased(true);
//      }
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

  HRESULT __stdcall GetSample::EnumMediaTypes(IEnumMediaTypes** ppEnum) {
    return E_NOTIMPL;
  }
  HRESULT __stdcall GetSample::QueryInternalConnections(IPin** apPin, ULONG* nPin) {
    return E_NOTIMPL;
  }
  HRESULT __stdcall GetSample::EndOfStream() {
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

  HRESULT __stdcall GetSample::BeginFlush() {
    _RPT0(0,"GetSample::BeginFlush()\n");
    flushing = true;
    end_of_stream = false;
    return S_OK;
  }

  HRESULT __stdcall GetSample::EndFlush() {
    _RPT0(0,"GetSample::EndFlush()\n");
    flushing = false;
    return S_OK;
  }

  HRESULT __stdcall GetSample::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate) {
    return S_OK;
  }

  // IMemInputPin

  HRESULT __stdcall GetSample::GetAllocator(IMemAllocator** ppAllocator) { return E_NOTIMPL; }
  HRESULT __stdcall GetSample::NotifyAllocator(IMemAllocator* pAllocator, BOOL bReadOnly) { return S_OK; }
  HRESULT __stdcall GetSample::GetAllocatorRequirements(ALLOCATOR_PROPERTIES* pProps) { return E_NOTIMPL; }

  HRESULT __stdcall GetSample::Receive(IMediaSample* pSamples) {
    if (end_of_stream || flushing) {
      _RPT0(0,"discarding sample (end of stream or flushing)\n");
      return S_OK;
    }
    if (S_OK == pSamples->IsPreroll()) {
      _RPT0(0,"discarding sample (preroll)\n");
      return S_OK;
    }

    if (load_audio)
      _RPT0(0,"...Recieve() running. (audio)\n");
    else 
      _RPT0(0,"...Recieve() running. (video)\n");


    if (FAILED(pSamples->GetTime(&sample_start_time, &sample_end_time))) {
      _RPT0(0,"failed!\n");
    } else {
      _RPT4(0,"%x%08x - %x%08x",
        DWORD(sample_start_time>>32), DWORD(sample_start_time),
        DWORD(sample_end_time>>32), DWORD(sample_end_time));
      _RPT1(0," (%d)\n", DWORD(sample_end_time - sample_start_time));
    }
    if (vi.HasVideo() && (!load_audio)) {
      pvf = env->NewVideoFrame(vi,-4);
      PBYTE buf;
      pSamples->GetPointer(&buf);
      if (!vi.IsPlanar()) {
        env->BitBlt(pvf->GetWritePtr(), pvf->GetPitch(), buf,
          pvf->GetPitch(), pvf->GetRowSize(), pvf->GetHeight());
      } else {
        env->BitBlt(pvf->GetWritePtr(PLANAR_Y), pvf->GetPitch(PLANAR_Y), buf,
          pvf->GetPitch(PLANAR_Y), pvf->GetRowSize(PLANAR_Y), pvf->GetHeight(PLANAR_Y));
        env->BitBlt(pvf->GetWritePtr(PLANAR_U), pvf->GetPitch(PLANAR_U), buf + pvf->GetOffset(PLANAR_U) - pvf->GetOffset(PLANAR_Y),
          pvf->GetPitch(PLANAR_U), pvf->GetRowSize(PLANAR_U), pvf->GetHeight(PLANAR_U));
        env->BitBlt(pvf->GetWritePtr(PLANAR_V), pvf->GetPitch(PLANAR_V), buf+ pvf->GetOffset(PLANAR_V) - pvf->GetOffset(PLANAR_Y),
          pvf->GetPitch(PLANAR_V), pvf->GetRowSize(PLANAR_V), pvf->GetHeight(PLANAR_V));
      }
    } else if (load_audio) {  // audio

      if (!a_allocated_buffer) {  // Allocate new buffer based on data length
        a_buffer = new BYTE[pSamples->GetActualDataLength()];
        a_allocated_buffer = pSamples->GetActualDataLength();
      }

      if (a_allocated_buffer < pSamples->GetActualDataLength()) { // Buffer too small  -- delete + reallocate
        delete[] a_buffer;
        a_buffer = new BYTE[pSamples->GetActualDataLength()];
        a_allocated_buffer = pSamples->GetActualDataLength();
      }

      PBYTE buf;
      pSamples->GetPointer(&buf);

      memcpy(a_buffer, buf, pSamples->GetActualDataLength());
      a_sample_bytes = pSamples->GetActualDataLength();

      _RPT1(0,"Recieve: Got %d bytes of audio data.\n",pSamples->GetActualDataLength());

    }

    HRESULT wait_result;
    SetEvent(evtNewSampleReady);  // New sample is finished - wait releasing it until it has been fetched (DoneWithSample).

    if (state == State_Running) {
      do {
        if (load_audio)
          _RPT0(0,"...Recieve() waiting for DoneWithSample. (audio)\n");
        else 
          _RPT0(0,"...Recieve() waiting for DoneWithSample. (video)\n");

        wait_result = WaitForSingleObject(evtDoneWithSample, 1000);
      } while (wait_result == WAIT_TIMEOUT && state ==State_Running);
    }

    if (load_audio)
      _RPT0(0,"Recieve() - returning. (audio)\n");
    else 
      _RPT0(0,"Recieve() - returning. (video)\n");

    return S_OK;
  }

  HRESULT __stdcall GetSample::ReceiveMultiple(IMediaSample** ppSamples, long nSamples, long* nSamplesProcessed) {
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

  HRESULT __stdcall GetSample::ReceiveCanBlock() { return S_OK; }



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


/************************************************
 *    DirectShowSource Helper Funcctions.       *
 ***********************************************/



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


static void RemoveUselessFilters(IGraphBuilder* gb, IBaseFilter* not_this_one, IBaseFilter* nor_this_one) {
  IEnumFilters* ef;
  if (FAILED(gb->EnumFilters(&ef)))
    return;
  ULONG fetched=1;
  IBaseFilter* bf;
  while (S_OK == ef->Next(1, &bf, &fetched)) {
    if (bf != not_this_one && bf != nor_this_one) {
      if (HasNoConnectedOutputPins(bf)) {
        DisconnectAllPinsAndRemoveFilter(gb, bf);
        ef->Reset();
      }
    }
    bf->Release();
  }
  ef->Release();
}

static HRESULT AttemptConnectFilters(IGraphBuilder* gb, IBaseFilter* connect_filter) {
  IEnumFilters* ef;

  if (FAILED(gb->EnumFilters(&ef)))
    return E_UNEXPECTED;

  HRESULT hr;
  ULONG fetched=1;
  IBaseFilter* bf;
  IEnumPins* ep_conn;

  connect_filter->EnumPins(&ep_conn);
  IPin* p_conn;

  if (FAILED(ep_conn->Next(1, &p_conn, &fetched))) 
    return E_UNEXPECTED;

  while (S_OK ==(ef->Next(1, &bf, &fetched))) {
    if (bf != connect_filter) {
      IEnumPins* ep;

      bf->EnumPins(&ep);
      IPin* pPin;
      while (S_OK == (ep->Next(1, &pPin, &fetched)))  {

        PIN_DIRECTION PinDirThis;
        pPin->QueryDirection(&PinDirThis);

        if (PinDirThis == PINDIR_OUTPUT) {
          hr = gb->ConnectDirect(pPin, p_conn, NULL);
          if (SUCCEEDED(hr)) {
            pPin->Release();
            ep->Release();
            bf->Release();
            ep_conn->Release();
            ef->Release();
            return S_OK;
          }
        }
      }
      pPin->Release();
      ep->Release();
    }
    bf->Release();
  }
  ep_conn->Release();
  ef->Release();
  return S_OK;
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

/************************************************
 *               DirectShowSource               *
 ***********************************************/


DirectShowSource::DirectShowSource(const char* filename, int _avg_time_per_frame, bool _seek, bool _enable_audio, bool _enable_video, IScriptEnvironment* _env) : env(_env), get_sample(_env, _enable_audio, _enable_video), no_search(!_seek) {

    CheckHresult(CoCreateInstance(CLSID_FilterGraphNoThread, 0, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void**)&gb), "couldn't create filter graph");


    WCHAR filenameW[MAX_PATH];
    MultiByteToWideChar(CP_ACP, 0, filename, -1, filenameW, MAX_PATH);

    CheckHresult(gb->AddFilter(static_cast<IBaseFilter*>(&get_sample), L"GetSample"), "couldn't add Video GetSample filter");

    bool load_grf = !strcmpi(filename+strlen(filename)-3,"grf");  // Detect ".GRF" extension and load as graph if so.

    if (load_grf) {
      CheckHresult(LoadGraphFile(gb, filenameW),"Couldn't open GRF file.",filename);
      // Try connecting to any open pins.
      AttemptConnectFilters(gb, &get_sample);
    } else {
      CheckHresult(gb->RenderFile(filenameW, NULL), "couldn't open file ", filename);
    }

    if (!get_sample.IsConnected()) {
      env->ThrowError("DirectShowSource: the filter graph manager won't talk to me");
    }

    RemoveUselessFilters(gb, &get_sample, &get_sample);

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

    bool audio_time = SUCCEEDED(ms->SetTimeFormat(&TIME_FORMAT_MEDIA_TIME));
    __int64 audio_dur;

    if (FAILED(ms->GetDuration(&audio_dur)) || audio_dur == 0) {
      env->ThrowError("DirectShowSource: unable to determine the duration of the audio");
    }

    ms->Release();

    vi = get_sample.GetVideoInfo();

    if (vi.HasVideo()) {
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
    }

    if (vi.HasAudio()) {
      vi.num_audio_samples = (__int64)((double)audio_dur * (double)vi.audio_samples_per_second / 10000000.0);
    }

    get_sample.StartGraph();

    cur_frame = 0;
    base_sample_time = 0;
    audio_bytes_read = 0;
    next_sample = 0;
  }


  DirectShowSource::~DirectShowSource() {
    IMediaControl* mc;
    if (SUCCEEDED(gb->QueryInterface(&mc))) {
      OAFilterState st;
      mc->GetState(1000, &st);
      if (st == State_Running) {
				mc->Stop();
			}
			mc->Release();
		}
    get_sample.StopGraph();
  }


  PVideoFrame __stdcall DirectShowSource::GetFrame(int n, IScriptEnvironment* env) {
    n = max(min(n, vi.num_frames-1), 0); 
    if (frame_units) {
      if (n < cur_frame || n > cur_frame+10) {
        if ( no_search || FAILED(get_sample.SeekTo(__int64(n) * avg_time_per_frame + (avg_time_per_frame>>1))) ) {
          no_search=true;  // Do not attempt further searches.
          if (cur_frame < n) {
            while (cur_frame < n) {
              get_sample.NextSample();
              cur_frame++;
            } // end while
          } // end if curframe<n  fail, if n is behind cur_frame and no seek.
        } else { // seek ok!
          next_sample = (__int64(n+1) * avg_time_per_frame + (avg_time_per_frame>>1)) * vi.audio_samples_per_second / 10000000;
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
      if (n < cur_frame || n > cur_frame+10) {
        if (no_search || FAILED(get_sample.SeekTo(sample_time))) {
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
          next_sample = sample_time * vi.audio_samples_per_second / 10000000;
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


  void __stdcall DirectShowSource::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) {

    int bytes_left = vi.BytesFromAudioSamples(count);

 
    if (next_sample != start) {  // We have been searching!  Skip until sync!

      __int64 seekTo = start*(__int64)10000000/(__int64)vi.audio_samples_per_second;

      if ((!no_search) && SUCCEEDED(get_sample.SeekTo(seekTo))) {
        // Seek succeeded!
        next_sample = start;

      } else if (start < next_sample) { // We are behind sync - pad with 0
        if (no_search || vi.HasVideo() || FAILED(get_sample.SeekTo(seekTo))) {
          // We cannot seek.
          if (vi.sample_type == SAMPLE_FLOAT) {
            float* samps = (float*)buf;
            for (int i = 0; i < bytes_left/sizeof(float); i++)
              samps[i] = 0.0f;
          }
          memset(buf,0, bytes_left);
          return;
        }
        // We skipped successfully
        next_sample = start;
      } else {  // Skip forward (decode)
        // Should we search?
        int skip_left = start - next_sample;
        bool cont = !get_sample.IsEndOfStream();
        while (cont) {
          if (vi.AudioSamplesFromBytes(get_sample.a_sample_bytes) > skip_left) {
            audio_bytes_read = vi.BytesFromAudioSamples(skip_left);
            cont = false;
          }
          
          if (get_sample.IsEndOfStream())
            cont= false;
          
          if (cont) {  // Read on
            get_sample.NextSample();
            skip_left -= vi.AudioSamplesFromBytes(get_sample.a_sample_bytes);
            audio_bytes_read = 0;
          }
        } // end while
        next_sample = start;
      }
    }

    int bytes_filled = 0;
    BYTE* samples = (BYTE*)buf;

    while (bytes_left) {
      // Can we read from the Directshow filter?
      if (get_sample.a_sample_bytes - audio_bytes_read > 0) { // Copy as many bytes as needed.

        int ds_offset = audio_bytes_read;  // First byte we can read.
        int available_bytes = min(bytes_left, get_sample.a_sample_bytes - ds_offset);  // This many bytes can be safely read.

        memcpy(&samples[bytes_filled], &get_sample.a_buffer[ds_offset], available_bytes);

        bytes_left -= available_bytes;
        bytes_filled += available_bytes;
        audio_bytes_read += available_bytes;

      } else { // Read more samples
        if (!get_sample.IsEndOfStream()) {
          get_sample.NextSample();
          audio_bytes_read = 0;
        } else { // Pad with 0
          if (vi.sample_type == SAMPLE_FLOAT) {
            float* samps = (float*)buf;
            for (int i = 0; i < bytes_left/sizeof(float); i++)
              samps[i] = 0.0f;
          } else {
            memset(&samples[bytes_filled],0,bytes_left);
          }
          bytes_left = 0;
        }
      }
    }
    next_sample +=count;
  }



void DirectShowSource::CheckHresult(HRESULT hr, const char* msg, const char* msg2) {
  if (SUCCEEDED(hr)) return;
//  char buf[1024] = {0};
//  if (!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, hr, 0, buf, 1024, NULL))
  char buf[MAX_ERROR_TEXT_LEN] = {0};
  if (!AMGetErrorText(hr, buf, MAX_ERROR_TEXT_LEN))
    wsprintf(buf, "error code 0x%x", hr);
  env->ThrowError("DirectShowSource: %s%s:\n%s", msg, msg2, buf);
}

HRESULT DirectShowSource::LoadGraphFile(IGraphBuilder *pGraph, const WCHAR* wszName)
{
    IStorage *pStorage = 0;
    if (S_OK != StgIsStorageFile(wszName))
    {
        return E_FAIL;
    }
    HRESULT hr = StgOpenStorage(wszName, 0, 
        STGM_TRANSACTED | STGM_READ | STGM_SHARE_DENY_WRITE, 
        0, 0, &pStorage);
    if (FAILED(hr))
    {
        return hr;
    }
    IPersistStream *pPersistStream = 0;
    hr = pGraph->QueryInterface(IID_IPersistStream,
             reinterpret_cast<void**>(&pPersistStream));
    if (SUCCEEDED(hr))
    {
        IStream *pStream = 0;
        hr = pStorage->OpenStream(L"ActiveMovieGraph", 0, 
            STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &pStream);
        if(SUCCEEDED(hr))
        {
            hr = pPersistStream->Load(pStream);
            pStream->Release();
        }
        pPersistStream->Release();
    }
    pStorage->Release();
    return hr;
}


AVSValue __cdecl Create_DirectShowSource(AVSValue args, void*, IScriptEnvironment* env) {
  const char* filename = args[0][0].AsString();
  int avg_time_per_frame = args[1].Defined() ? int(10000000 / args[1].AsFloat() + 0.5) : 0;
  
  bool audio = args[3].AsBool(true);
  bool video = args[4].AsBool(true);

  if (!(audio || video)) 
    env->ThrowError("DirectShowSource: Both video and audio was disabled!");

  if (!(audio && video)) { // Hey - simple!!
    if (audio) {
      return AlignPlanar::Create(new DirectShowSource(filename, avg_time_per_frame, args[2].AsBool(true), true , false, env));
    } else {
      return AlignPlanar::Create(new DirectShowSource(filename, avg_time_per_frame, args[2].AsBool(true), false , true, env));
    }
  }

  PClip DS_audio;
  PClip DS_video;

  bool audio_success = true;
  bool video_success = true;

  const char *a_e_msg;
  const char *v_e_msg;

  try {
    DS_audio = new DirectShowSource(filename, avg_time_per_frame, args[2].AsBool(true), audio , false, env);
  } catch (AvisynthError e) {
    a_e_msg = e.msg;
    audio_success = false;
  }

  try {
    DS_video = new DirectShowSource(filename, avg_time_per_frame, args[2].AsBool(true), false, video, env);
  } catch (AvisynthError e) {
    if (!lstrcmpi(e.msg, "DirectShowSource: I can't determine the frame rate of\nthe video; you must use the \"fps\" parameter"))
      env->ThrowError(e.msg);
    v_e_msg = e.msg;
    video_success = false;
  }

  if (!(audio_success || video_success)) {
    char err[1024] = "DirectShowSource: Could not open as video or audio.\r\n\r\nVideo returned:  \"";
    strcat(err,v_e_msg);
    strcat(err,"\"\r\n\r\nAudio returned:  \"");
    strcat(err,a_e_msg);
    strcat(err,"\"\r\n");
    env->ThrowError(err);
  }

  if (!audio_success)
    return AlignPlanar::Create(DS_video);

  if (!video_success)
    return DS_audio;

  AVSValue inv_args[2] = { DS_video, DS_audio }; 
  PClip ds_all =  env->Invoke("AudioDub",AVSValue(inv_args,2)).AsClip();

  return AlignPlanar::Create(ds_all);
}

