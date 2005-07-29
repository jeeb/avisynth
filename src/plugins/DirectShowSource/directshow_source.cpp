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
 

#include "directshow_source.h"


/************************************
 *             GetSample            *
 ************************************/


GetSample::GetSample(bool _load_audio, bool _load_video)
  : load_audio(_load_audio), load_video(_load_video) {
    refcnt = 1;
    source_pin = 0;
    filter_graph = 0;
    pclock = 0;
    m_pPos =0;
    state = State_Stopped;
	a_sample_bytes = 0;
    av_buffer = 0;
    flushing = end_of_stream = false;
    memset(&vi, 0, sizeof(vi));
    sample_end_time = sample_start_time = 0;
    evtDoneWithSample = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    evtNewSampleReady = ::CreateEvent(NULL, FALSE, FALSE, NULL);
  }

  GetSample::~GetSample() {
    PulseEvent(evtDoneWithSample);
    CloseHandle(evtDoneWithSample);
    PulseEvent(evtNewSampleReady);
    CloseHandle(evtNewSampleReady);
  }


  void GetSample::WaitForStart() {

    // Give the graph every opportunity to start before we return empty data

    if (startGraphTimeout)
      startGraphTimeout = (WaitForSingleObject(evtNewSampleReady, 120000) == WAIT_TIMEOUT);    // MAX wait time = 2 minutes!
  }


  PVideoFrame GetSample::GetCurrentFrame(IScriptEnvironment* env) {

    PVideoFrame pvf = env->NewVideoFrame(vi);;

    WaitForStart();   

    if (av_buffer) {

      // Put any knowledge of video packing and alignment here and
      // keep it independant of any AviSynth packing and alignment.

      PBYTE buf = av_buffer;

      if (!vi.IsPlanar()) { // Packed formats have rows 32bit aligned

	    const int rowsize = pvf->GetRowSize();
        env->BitBlt(pvf->GetWritePtr(), pvf->GetPitch(), buf, (rowsize+3)&~3, rowsize, pvf->GetHeight());
      }
      else {

	    const int rowsize = pvf->GetRowSize(PLANAR_Y);
		const int height  = pvf->GetHeight(PLANAR_Y);

        // All planar formats have Y rows 32bit aligned
        env->BitBlt(pvf->GetWritePtr(PLANAR_Y), pvf->GetPitch(PLANAR_Y), buf, (rowsize+3)&~3, rowsize, height);

	    const int UVrowsize = pvf->GetRowSize(PLANAR_V);
		const int UVheight  = pvf->GetHeight(PLANAR_V);

        // YV12 format has UV rows 16bit aligned with
        // V plane first, after aligned end of Y plane
		buf += ((rowsize+3)&~3) * height;
        env->BitBlt(pvf->GetWritePtr(PLANAR_V), pvf->GetPitch(PLANAR_V), buf, (UVrowsize+1)&~1, UVrowsize, UVheight);

        // And U plane last, after aligned end of V plane
		buf += ((UVrowsize+1)&~1) * UVheight;
        env->BitBlt(pvf->GetWritePtr(PLANAR_U), pvf->GetPitch(PLANAR_U), buf, (UVrowsize+1)&~1, UVrowsize, UVheight);
      }
    }
    else {
      // If the graph still hasn't started yet we won't have a current frame
      // so dummy up a grey frame so at least things won't be fatal.

      memset(pvf->GetWritePtr(), 128, pvf->GetPitch()*(pvf->GetHeight()+pvf->GetHeight(PLANAR_U)));
    }
    return pvf;
  }


  void GetSample::StartGraph() {
    _RPT0(0,"DSS StartGraph() waiting for new sample...\n");
    ResetEvent(evtDoneWithSample);  // Nuke any unused SetEvents
    ResetEvent(evtNewSampleReady);  // Nuke any unused SetEvents
    IMediaControl* mc;
    filter_graph->QueryInterface(&mc);
    end_of_stream = false;
    mc->Run();
    mc->Release();
    startGraphTimeout = (WaitForSingleObject(evtNewSampleReady, 5000) == WAIT_TIMEOUT);    // MAX wait time = 5000ms!
    _RPT1(0,"DSS ...StartGraph() %s waiting for new sample\n", startGraphTimeout ? "*** TIMED OUT ***" : "finished");
  }

  void GetSample::StopGraph() {
    _RPT1(0,"DSS StopGraph() indicating done with sample - state:%d\n",state);
    IMediaControl* mc;
    filter_graph->QueryInterface(&mc);
    PulseEvent(evtDoneWithSample); // Free task if waiting
    SetEvent(evtDoneWithSample);   // Add 1 spare notify just in case
    mc->Stop();
    mc->Release();
    if (m_pPos)
      m_pPos->Release();
    m_pPos = 0;
  }

  void GetSample::PauseGraph() {
    _RPT0(0,"DSS PauseGraph() indicating done with sample\n");
    IMediaControl* mc;
    filter_graph->QueryInterface(&mc);
    mc->Pause();
    mc->Release();
  }

  HRESULT GetSample::SeekTo(__int64 pos) {
    _RPT1(0,"DSS SeekTo() seeking to new position %I64d\n", pos);

    IMediaControl* mc;
    filter_graph->QueryInterface(&mc);
    mc->Stop();
    mc->Release();

    IMediaSeeking* ms;
    filter_graph->QueryInterface(&ms);

    DWORD dwCaps = 0;
    ms->GetCapabilities(&dwCaps);
    
    ms->SetTimeFormat(&TIME_FORMAT_MEDIA_TIME); // Setting can give E_NOTIMPL

    HRESULT hr = S_FALSE;
    LONGLONG pCurrent = -1, pStop;
    GUID time_f;

    ms->GetTimeFormat(&time_f);                 // so check what it currently is
    if (time_f != TIME_FORMAT_MEDIA_TIME) {
      // Probably should implement code for all the time formats  :: FIXME
      _RPT0(0,"DSS Could not set time format to media time!\n");
      goto SeekExit;
    }
    if (dwCaps & AM_SEEKING_CanSeekAbsolute) {
      if (SUCCEEDED(hr = ms->SetPositions(&pos, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning)))
          goto SeekExit;
      _RPT0(0,"DSS Absolute seek failed!\n"); // and now trying relative seek
    }
    if ( !(dwCaps & AM_SEEKING_CanGetCurrentPos) ) {
      _RPT0(0,"DSS GetPositions unsupported!\n");
      goto SeekExit;
    }
    if (FAILED(hr = ms->GetPositions(&pCurrent, &pStop)) || (pCurrent == -1)) {
      _RPT0(0,"DSS GetPositions failed!\n");
      goto SeekExit;
    }
    pCurrent = pos - pCurrent;
    if ( ((dwCaps & AM_SEEKING_CanSeekForwards)  && (pCurrent > 0))
      || ((dwCaps & AM_SEEKING_CanSeekBackwards) && (pCurrent < 0)) ) {
      if (FAILED(hr = ms->SetPositions(&pCurrent, AM_SEEKING_RelativePositioning, NULL, AM_SEEKING_NoPositioning))) {
          _RPT0(0,"DSS Relative seek failed!\n"); }
    }
    else { // No way of seeking
      _RPT0(0,"DSS Could not perform any seek!\n");
    }
SeekExit:
    ms->Release();
    StartGraph(); // includes a "WaitForSingleObject(evtNewSampleReady, ..."

    return hr;
  }


  bool GetSample::NextSample() {
    const char * const streamName = load_audio ? "audio" : "video";

    if (end_of_stream) return false;

    // If the graph didn't start, check if it has yet. We absolutly have to keep in lock
    // step with the evtNewSampleReady and evtDoneWithSample synchronizaton objects. This
    // is a bit kludgy because when it happens blank frames or silence are returned but
    // it is better than returning totally corrupt data.

    WaitForStart();   

    if (startGraphTimeout) return false;

    _RPT1(0,"DSS NextSample() indicating done with sample...(%s)\n", streamName);

    SetEvent(evtDoneWithSample);  // We indicate that Receive can run again. We have now finished using the frame.

    if (state == State_Stopped) {
      _RPT1(0,"DSS NextSample() state == State_Stopped (%s)\n", streamName); // Opps should never happen!
      return false;
    }

    HRESULT wait_result;
    do {
      _RPT1(0,"DSS ...NextSample() waiting for new sample...(%s)\n", streamName);
      wait_result = WaitForSingleObject(evtNewSampleReady, 5000);
    } while (wait_result == WAIT_TIMEOUT);

    _RPT1(0,"DSS ...NextSample() done waiting for new sample (%s)\n", streamName);
	return !end_of_stream;
  }

  // IUnknown

  ULONG __stdcall GetSample::AddRef() { 
    InterlockedIncrement(&refcnt); 
    _RPT1(0,"DSS GetSample::AddRef() -> %d\n", refcnt); 
    return refcnt; 
  }

  ULONG __stdcall GetSample::Release() { 
    InterlockedDecrement(&refcnt); 
    _RPT1(0,"DSS GetSample::Release() -> %d\n", refcnt); 
    return refcnt; 
  }

  

  HRESULT __stdcall GetSample::QueryInterface(REFIID iid, void** ppv) {
    if      (iid == IID_IUnknown)     *ppv = static_cast<IUnknown*>(static_cast<IBaseFilter*>(this));
    else if (iid == IID_IPersist)     *ppv = static_cast<IPersist*>(this);
    else if (iid == IID_IMediaFilter) *ppv = static_cast<IMediaFilter*>(this);
    else if (iid == IID_IBaseFilter)  *ppv = static_cast<IBaseFilter*>(this);
    else if (iid == IID_IPin)         *ppv = static_cast<IPin*>(this);
    else if (iid == IID_IMemInputPin) *ppv = static_cast<IMemInputPin*>(this);
    else if (iid == IID_IMediaSeeking || iid == IID_IMediaPosition) {
      if (!source_pin)
        return E_NOINTERFACE;

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

  HRESULT __stdcall GetSample::Stop() {
    _RPT0(0,"DSS GetSample::Stop()\n");
    state = State_Stopped;
    PulseEvent(evtDoneWithSample);
    return S_OK;
  }

  HRESULT __stdcall GetSample::Pause() { _RPT0(0,"DSS GetSample::Pause()\n"); state = State_Paused; return S_OK; }

  HRESULT __stdcall GetSample::Run(REFERENCE_TIME tStart) {
    _RPT1(0,"DSS GetSample::Run(%I64d)\n", tStart);
    state = State_Running;
    return S_OK;
  }

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
//      if (pmt->subtype != MEDIASUBTYPE_PCM  || pmt->subtype != MEDIASUBTYPE_IEEE_FLOAT )
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

      _RPT3(0, "*** Audio Accepted!  - Channels:%d.  Samples/sec:%d.  Bits/sample:%d.\n",
            wex->nChannels, wex->nSamplesPerSec, wex->wBitsPerSample);      
      return S_OK;
    }

// Handle video:

    if        (pmt->subtype == MEDIASUBTYPE_YV12) {  
      vi.pixel_type = VideoInfo::CS_YV12;
    } else if (pmt->subtype == MEDIASUBTYPE_YUY2) {
      vi.pixel_type = VideoInfo::CS_YUY2;
    } else if (pmt->subtype == MEDIASUBTYPE_RGB24) {
      vi.pixel_type = VideoInfo::CS_BGR24;
    } else if (pmt->subtype == MEDIASUBTYPE_RGB32) {
      vi.pixel_type = VideoInfo::CS_BGR32;
    } else if (pmt->subtype == MEDIASUBTYPE_ARGB32) {
      vi.pixel_type = VideoInfo::CS_BGR32;
    } else {
      _RPT0(0, "*** subtype rejected\n");
      return S_FALSE;
    }

    BITMAPINFOHEADER* pbi;
    unsigned _avg_time_per_frame;

    if (pmt->formattype == FORMAT_VideoInfo) {
      VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)pmt->pbFormat;
      _avg_time_per_frame = unsigned(vih->AvgTimePerFrame);
      pbi = &vih->bmiHeader;
    } else if (pmt->formattype == FORMAT_VideoInfo2) {
      VIDEOINFOHEADER2* vih = (VIDEOINFOHEADER2*)pmt->pbFormat;
      _avg_time_per_frame = unsigned(vih->AvgTimePerFrame);
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

    if (_avg_time_per_frame) {
      vi.SetFPS(10000000, _avg_time_per_frame);
    } else {
      vi.fps_numerator = 1;
      vi.fps_denominator = 0;
    }

    _RPT4(0, "*** format accepted: %dx%d, pixel_type %d, framerate %d\n",
      vi.width, vi.height, vi.pixel_type, _avg_time_per_frame);
    return S_OK;
  }

  HRESULT __stdcall GetSample::EnumMediaTypes(IEnumMediaTypes** ppEnum) {
    return E_NOTIMPL;
  }
  HRESULT __stdcall GetSample::QueryInternalConnections(IPin** apPin, ULONG* nPin) {
    return E_NOTIMPL;
  }
  HRESULT __stdcall GetSample::EndOfStream() {
    _RPT0(0,"DSS GetSample::EndOfStream()\n");
    end_of_stream = true;
    if (filter_graph) {
      IMediaEventSink* mes;
      if (SUCCEEDED(filter_graph->QueryInterface(&mes))) {
        mes->Notify(EC_COMPLETE, (long)S_OK, (long)static_cast<IBaseFilter*>(this));
        mes->Release();
      }
    }
    _RPT0(0,"DSS EndOfStream() indicating new sample ready\n");
    SetEvent(evtNewSampleReady);
    return S_OK;
  }

  HRESULT __stdcall GetSample::BeginFlush() {
    _RPT0(0,"DSS GetSample::BeginFlush()\n");
    flushing = true;
    end_of_stream = false;
    return S_OK;
  }

  HRESULT __stdcall GetSample::EndFlush() {
    _RPT0(0,"DSS GetSample::EndFlush()\n");
    flushing = false;
    return S_OK;
  }

  HRESULT __stdcall GetSample::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate) {
    return S_OK;
  }

  // IMemInputPin

  HRESULT __stdcall GetSample::GetAllocator(IMemAllocator** ppAllocator) { return VFW_E_NO_ALLOCATOR; }
  HRESULT __stdcall GetSample::NotifyAllocator(IMemAllocator* pAllocator, BOOL bReadOnly) { return S_OK; }
  HRESULT __stdcall GetSample::GetAllocatorRequirements(ALLOCATOR_PROPERTIES* pProps) { return E_NOTIMPL; }

  HRESULT __stdcall GetSample::Receive(IMediaSample* pSamples) {
    if (state == State_Stopped) {
      _RPT0(0,"DSS discarding sample (State_Stopped)\n");
      return VFW_E_WRONG_STATE;
    }
    if (flushing) {
      _RPT0(0,"DSS discarding sample (flushing)\n");
      return S_FALSE;
    }
    if (S_OK == pSamples->IsPreroll()) {
      _RPT0(0,"DSS discarding sample (preroll)\n");
      return S_OK;
    }

    if (load_audio)
      _RPT0(0,"DSS ...Receive() running. (audio)\n");
    else 
      _RPT0(0,"DSS ...Receive() running. (video)\n");


    if (FAILED(pSamples->GetTime(&sample_start_time, &sample_end_time))) {
      _RPT0(0,"DSS GetTimefailed!\n");
    } else {
      _RPT3(0,"DSS %I64d - %I64d (%d)\n", sample_start_time, sample_end_time,
                                          DWORD(sample_end_time - sample_start_time));
    }

    pSamples->GetPointer(&av_buffer);

    if (load_audio) {  // audio
      a_sample_bytes = pSamples->GetActualDataLength();
      _RPT1(0,"DSS Receive: Got %d bytes of audio data.\n",a_sample_bytes);
    }

    HRESULT wait_result;
    SetEvent(evtNewSampleReady);  // New sample is finished - wait releasing it until it has been fetched (DoneWithSample).

    do {
      if (load_audio)
        _RPT0(0,"DSS ...Receive() waiting for DoneWithSample. (audio)\n");
      else 
        _RPT0(0,"DSS ...Receive() waiting for DoneWithSample. (video)\n");

      wait_result = WaitForSingleObject(evtDoneWithSample, 5000);
    } while (wait_result == WAIT_TIMEOUT);

    av_buffer = 0;
    a_sample_bytes = 0;

    if (load_audio)
      _RPT0(0,"DSS Receive() - returning. (audio)\n");
    else 
      _RPT0(0,"DSS Receive() - returning. (video)\n");

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



/***********************************************
 *             GetSampleEnumPins               *
 ***********************************************/



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


/***********************************************
 *    DirectShowSource Helper Functions.       *
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

// The following constant is from "wmcodecconst.h" in the 
// "Windows Media Audio and Video Codec Interfaces download package"
// available for download from MSDN.
static const WCHAR *g_wszWMVCDecoderDeinterlacing = L"_DECODERDEINTERLACING";

static void DisableDeinterlacing(IFilterGraph *pGraph)
{
    IEnumFilters *pEnum = NULL;
    IBaseFilter *pFilter;
    ULONG cFetched;
 
    HRESULT hr = pGraph->EnumFilters(&pEnum);
    if (FAILED(hr))
        return;

    while(pEnum->Next(1, &pFilter, &cFetched) == S_OK) {
        FILTER_INFO FilterInfo;
        hr = pFilter->QueryFilterInfo(&FilterInfo);
        if (FAILED(hr))
            continue;  // Maybe the next one will work.

        if (wcscmp(FilterInfo.achName, L"WMVideo Decoder DMO") == 0) {
            IPropertyBag *pPropertyBag = NULL;
            hr = pFilter->QueryInterface(IID_IPropertyBag, (void**)&pPropertyBag);
            if(SUCCEEDED(hr)) {
            VARIANT myVar;
            VariantInit(&myVar);
            // Disable decoder deinterlacing
            myVar.vt   = VT_BOOL;
            myVar.lVal = FALSE;
            pPropertyBag->Write(g_wszWMVCDecoderDeinterlacing, &myVar);
            }
        }

        // The FILTER_INFO structure holds a pointer to the Filter Graph
        // Manager, with a reference count that must be released.
        if (FilterInfo.pGraph != NULL)
            FilterInfo.pGraph->Release();
        pFilter->Release();
    }

    pEnum->Release();
}


/************************************************
 *               DirectShowSource               *
 ***********************************************/


DirectShowSource::DirectShowSource(const char* filename, int _avg_time_per_frame, bool _seek, bool _enable_audio,
                                   bool _enable_video, bool _convert_fps, IScriptEnvironment* env)
  : get_sample(_enable_audio, _enable_video), no_search(!_seek), convert_fps(_convert_fps), gb(NULL) {

    CheckHresult(env, CoCreateInstance(CLSID_FilterGraphNoThread, 0, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void**)&gb),
                 "couldn't create filter graph");

    WCHAR filenameW[MAX_PATH];
    MultiByteToWideChar(CP_ACP, 0, filename, -1, filenameW, MAX_PATH);

    CheckHresult(env, gb->AddFilter(static_cast<IBaseFilter*>(&get_sample), L"GetSample"), "couldn't add GetSample filter");

    bool load_grf = !strcmpi(filename+strlen(filename)-3,"grf");  // Detect ".GRF" extension and load as graph if so.

    if (load_grf) {
      CheckHresult(env, LoadGraphFile(gb, filenameW),"Couldn't open GRF file.",filename);
      // Try connecting to any open pins.
      AttemptConnectFilters(gb, &get_sample);
    } else {
      CheckHresult(env, gb->RenderFile(filenameW, NULL), "couldn't open file ", filename);
    }

    if (!get_sample.IsConnected()) {
      cleanUp();
      env->ThrowError("DirectShowSource: the filter graph manager won't talk to me");
    }

    RemoveUselessFilters(gb, &get_sample, &get_sample);

    SetMicrosoftDVtoFullResolution(gb);
    DisableDeinterlacing(gb);

    // Prevent the graph from trying to run in "real time"
    // ... Disabled because it breaks ASF.  Now I know why
    // Avery swears so much.
    IMediaFilter* mf;
    CheckHresult(env, gb->QueryInterface(&mf), "couldn't get IMediaFilter interface");
    CheckHresult(env, mf->SetSyncSource(NULL), "couldn't set null sync source");
    mf->Release();

    IMediaSeeking* ms=0;
    CheckHresult(env, gb->QueryInterface(&ms), "couldn't get IMediaSeeking interface");

    vi = get_sample.GetVideoInfo();

    if (vi.HasVideo()) {
      bool fc_failed = true, mt_failed = true;
      __int64 frame_count = 0, duration = 0;
      GUID time_fmt = {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}};

      // SetTimeFormat can give E_NOTIMPL
      ms->SetTimeFormat(&TIME_FORMAT_MEDIA_TIME);
      // so check what it currently is
      ms->GetTimeFormat(&time_fmt);

      if (time_fmt == TIME_FORMAT_MEDIA_TIME)
        mt_failed = (FAILED(ms->GetDuration(&duration)) || duration == 0);

      // run in frame mode if we can set time format to frame
      frame_units = SUCCEEDED(ms->SetTimeFormat(&TIME_FORMAT_FRAME));

      if (frame_units)
        fc_failed = (FAILED(ms->GetDuration(&frame_count)) || frame_count == 0);

      if (convert_fps || fc_failed) frame_units = false;

      if (mt_failed && !frame_units) {
        ms->Release();
        cleanUp();
        env->ThrowError("DirectShowSource: unable to determine the duration of the video.");
      }

      if (_avg_time_per_frame) {
        avg_time_per_frame = _avg_time_per_frame;
        vi.SetFPS(10000000, avg_time_per_frame);
        vi.num_frames = int(frame_units ? frame_count : duration / avg_time_per_frame);
      }
      else {
        // this is exact (no rounding needed) because of the way the fps is set in GetSample
        avg_time_per_frame = 10000000 / vi.fps_numerator * vi.fps_denominator;
        if (avg_time_per_frame != 0) {
          vi.num_frames = int(frame_units ? frame_count : duration / avg_time_per_frame);
        }
        else {
          // Try duration divided by frame count
          if (fc_failed || mt_failed) {
            ms->Release();
            cleanUp();
            env->ThrowError("DirectShowSource: I can't determine the frame rate\n"
                            "of the video, you must use the \"fps\" parameter."); // Note must match message below
          }
          else {
            avg_time_per_frame = int((duration + (frame_count>>1)) / frame_count);
            vi.num_frames = int(frame_count);

            unsigned __int64 numerator   = 10000000 * frame_count;
            unsigned __int64 denominator = duration;

            unsigned __int64 x=numerator, y=denominator;
            while (y) {   // find gcd
              unsigned __int64 t = x%y; x = y; y = t;
            }
            numerator   /= x; // normalize
            denominator /= x;

            unsigned __int64 temp = numerator | denominator; // Just looking top bit
            unsigned u = 0;
            while (temp & 0xffffffff80000000) {
              temp = Int64ShrlMod32(temp, 1);
              u++;
            }
            if (u) { // Scale to fit
              const unsigned round = 1 << (u-1);
              vi.SetFPS( (unsigned)Int64ShrlMod32(numerator   + round, u),
                         (unsigned)Int64ShrlMod32(denominator + round, u) );
            }
            else {
              vi.fps_numerator   = (unsigned)numerator;
              vi.fps_denominator = (unsigned)denominator;
            }
          }
        }
      }
    }

    if (vi.HasAudio()) {
      GUID time_fmt = {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}};
      __int64 audio_dur = 0;

      // SetTimeFormat can give E_NOTIMPL
      ms->SetTimeFormat(&TIME_FORMAT_MEDIA_TIME);
      // so check what it currently is
      ms->GetTimeFormat(&time_fmt);

      if ( (time_fmt != TIME_FORMAT_MEDIA_TIME)
        || FAILED(ms->GetDuration(&audio_dur))
        || (audio_dur == 0) ) {
          ms->Release();
          cleanUp();
          env->ThrowError("DirectShowSource: unable to determine the duration of the audio.");
      }

      vi.num_audio_samples = (__int64)((double)audio_dur * (double)vi.audio_samples_per_second / 10000000.0);
    }

    ms->Release();
    get_sample.StartGraph();

    cur_frame = 0;
    base_sample_time = 0;
    audio_bytes_read = 0;
    next_sample = 0;
  }


  DirectShowSource::~DirectShowSource() {
    cleanUp();
  }

  void DirectShowSource::cleanUp() {
    if (gb) {
      IMediaControl* mc;
      if (SUCCEEDED(gb->QueryInterface(&mc))) {
        OAFilterState st;
        mc->GetState(1000, &st);
        if (st != State_Stopped) mc->Stop();
        mc->Release();
      }
      get_sample.StopGraph();
      SAFE_RELEASE(gb);
    }
  }

  PVideoFrame __stdcall DirectShowSource::GetFrame(int n, IScriptEnvironment* env) {
    n = max(min(n, vi.num_frames-1), 0); 
    // Ask for the frame whose [start_time ->T<- end_time] span sample_time
    const __int64 sample_time = Int32x32To64(n, avg_time_per_frame) + (avg_time_per_frame>>1);
    if (frame_units) {
      if (n < cur_frame || n > cur_frame+10) {
        if ( no_search || FAILED(get_sample.SeekTo(sample_time)) ) {
          while (cur_frame < n) {
            if (!get_sample.NextSample()) break;
            cur_frame++;
          } // end while (cur_frame<n  fail, if n is behind cur_frame and no seek.
        }
        else { // seek ok!
          cur_frame = n;
        }
      }
      else {
        while (cur_frame < n) {
          if (!get_sample.NextSample()) break;
          cur_frame++;
        }
      }
    }
    else {
      if (n < cur_frame || n > cur_frame+10) {
        if (no_search || FAILED(get_sample.SeekTo(sample_time))) {
          if (cur_frame<n) {  // seek manually
            while (get_sample.GetSampleEndTime() <= sample_time) {
              if (!get_sample.NextSample()) break;
            }
            cur_frame = n;
          } // end if curframe<n  fail, if n is behind cur_frame and no seek.
        }
        else { // seek ok!
          // Stupid DirectShow bases its idea of the time of the frame
          // after a seek as somewhere between +/-(avg_time_per_frame/2)
          // i.e. It approximatly starts from zero.
          base_sample_time = sample_time - (avg_time_per_frame>>1) - get_sample.GetSampleStartTime();
          cur_frame = n;
        }
      }
      else if (convert_fps) {
        if (cur_frame<n) {  // automatic fps conversion: trust only sample time
          while (get_sample.GetSampleEndTime()+base_sample_time <= sample_time) {
            if(!get_sample.NextSample()) break;
          }
          cur_frame = n;
        }
      }
      else {
        while (cur_frame < n) {
          if (!get_sample.NextSample()) break;
          cur_frame++;
        }
      }
    }
    return get_sample.GetCurrentFrame(env);
  }


  void __stdcall DirectShowSource::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) {
    int bytes_filled = 0;
 
    if (next_sample != start) {  // We have been searching!  Skip until sync!

      _RPT2(0,"DSS GetAudio: Seeking %I64d previous %I64d samples.\n", start, next_sample);

      // Backup to begining of current buffer
      next_sample -= vi.AudioSamplesFromBytes(audio_bytes_read);
      audio_bytes_read = 0;

      // Avoid a seek if target is within the current buffer
      const __int64 avail_samples = vi.AudioSamplesFromBytes(get_sample.a_sample_bytes);
      if ((start < next_sample) || (start >= next_sample+avail_samples)) {

        const __int64 seekTo = (start*10000000 + (vi.audio_samples_per_second>>1)) / vi.audio_samples_per_second;
        _RPT1(0,"DSS GetAudio: SeekTo %I64d media time.\n", seekTo);

        if ((!no_search) && SUCCEEDED(get_sample.SeekTo(seekTo))) {
          // Seek succeeded!
          next_sample = start;
          audio_bytes_read = 0;
        } 
      } 

      if (start < next_sample) { // We are behind sync - pad with 0
        const int fill_nsamples  = (int)min(next_sample - start, count);
        _RPT1(0,"DSS GetAudio: Padding %d samples.\n", fill_nsamples);

        // We cannot seek.
        if (vi.sample_type == SAMPLE_FLOAT) {
          float* samps = (float*)buf;
          for (int i = 0; i < fill_nsamples; i++)
            samps[i] = 0.0f;
        } else {
          memset(buf,0, (unsigned int)vi.BytesFromAudioSamples(fill_nsamples));
        }

        if (fill_nsamples == count)  // Buffer is filled - return
          return;
        start += fill_nsamples;
        count -= fill_nsamples;
        bytes_filled += (int)vi.BytesFromAudioSamples(fill_nsamples);
      }

      if (start > next_sample) {  // Skip forward (decode)
        // Should we search?
        int skip_left = (int)vi.BytesFromAudioSamples(start - next_sample);
        _RPT1(0,"DSS GetAudio: Skipping %d bytes.\n", skip_left);

        get_sample.WaitForStart();
        while (skip_left > 0) {

          if (get_sample.a_sample_bytes-audio_bytes_read >= skip_left) {
            audio_bytes_read += skip_left;
            break;
          }
          skip_left -= get_sample.a_sample_bytes-audio_bytes_read;
          audio_bytes_read = get_sample.a_sample_bytes;
          
          if (get_sample.NextSample())
            audio_bytes_read = 0;
          else
             break;  // EndOfStream?
        } // end while
        next_sample = start;
      }
    }

    BYTE* samples = (BYTE*)buf;
    int bytes_left = (int)vi.BytesFromAudioSamples(count);
    _RPT2(0,"DSS GetAudio: Reading %I64d samples, %d bytes.\n", count, bytes_left);

    get_sample.WaitForStart();
    while (bytes_left) {
      // Can we read from the Directshow filters buffer?
      if (get_sample.a_sample_bytes - audio_bytes_read > 0) { // Copy as many bytes as needed.

        // This many bytes can be safely read.
        const int available_bytes = min(bytes_left, get_sample.a_sample_bytes - audio_bytes_read);
        _RPT2(0,"DSS GetAudio: Memcpy %d offset, %d bytes.\n", bytes_filled, available_bytes);

        memcpy(&samples[bytes_filled], &get_sample.av_buffer[audio_bytes_read], available_bytes);

        bytes_left -= available_bytes;
        bytes_filled += available_bytes;
        audio_bytes_read += available_bytes;

      }
      else { // Read more samples
        if (get_sample.NextSample()) {
          audio_bytes_read = 0;
        } else { // Pad with 0
          _RPT2(0,"DSS GetAudio: Memset %d offset, %d bytes.\n", bytes_filled, bytes_left);
          if (vi.sample_type == SAMPLE_FLOAT) {
            float* samps = (float*)((int)(&samples[bytes_filled]) & ~3); // Aligned just to be sure
            const int samples_left = (bytes_left+sizeof(float)-1)/sizeof(float);
            for (int i = 0; i < samples_left; i++)
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



void DirectShowSource::CheckHresult(IScriptEnvironment* env, HRESULT hr, const char* msg, const char* msg2) {
  if (SUCCEEDED(hr)) return;
//  char buf[1024] = {0};
//  if (!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, hr, 0, buf, 1024, NULL))
  char buf[MAX_ERROR_TEXT_LEN] = {0};
  if (!AMGetErrorText(hr, buf, MAX_ERROR_TEXT_LEN))
    wsprintf(buf, "error code 0x%x", hr);
  cleanUp();
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


/* As this is currently implemented we use two separate instance of DSS, one for video and
 * one for audio. This means we create two (2) filter graphs. An alternate implementation
 * would be to have a video GetSample object and a separate audio GetSample object in the
 * one filter graph. Possible problems with this idea could be related to independant 
 * positioning of the Video and Audio streams within the one filter graph. */


AVSValue __cdecl Create_DirectShowSource(AVSValue args, void*, IScriptEnvironment* env) {
  const char* filename = args[0][0].AsString();
  int _avg_time_per_frame = args[1].Defined() ? int(10000000 / args[1].AsFloat() + 0.5) : 0;
  
  bool audio = args[3].AsBool(true);
  bool video = args[4].AsBool(true);

  if (!(audio || video)) 
    env->ThrowError("DirectShowSource: Both video and audio was disabled!");

  if (!(audio && video)) { // Hey - simple!!
    if (audio) {
      return new DirectShowSource(filename, _avg_time_per_frame, args[2].AsBool(true), true , false, args[5].AsBool(false), env);
    } else {
      return new DirectShowSource(filename, _avg_time_per_frame, args[2].AsBool(true), false , true, args[5].AsBool(false), env);
    }
  }

  PClip DS_audio;
  PClip DS_video;

  bool audio_success = true;
  bool video_success = true;

  const char *a_e_msg;
  const char *v_e_msg;

  try {
    DS_audio = new DirectShowSource(filename, _avg_time_per_frame, args[2].AsBool(true), true , false, args[5].AsBool(false), env);
  } catch (AvisynthError e) {
    a_e_msg = e.msg;
    audio_success = false;
  }

  try {
    DS_video = new DirectShowSource(filename, _avg_time_per_frame, args[2].AsBool(true), false, true, args[5].AsBool(false), env);
  } catch (AvisynthError e) {
    if (!lstrcmpi(e.msg, "DirectShowSource: I can't determine the frame rate\n"
                         "of the video, you must use the \"fps\" parameter.") ) // Note must match message above
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
    return DS_video;

  if (!video_success)
    return DS_audio;

  AVSValue inv_args[2] = { DS_video, DS_audio }; 
  PClip ds_all =  env->Invoke("AudioDub",AVSValue(inv_args,2)).AsClip();

  return ds_all;
}



extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit2(IScriptEnvironment* env)
{
    env->AddFunction("DirectShowSource", "s+[fps]f[seek]b[audio]b[video]b[convertfps]b", Create_DirectShowSource, 0);
    return "DirectShowSource";
}

